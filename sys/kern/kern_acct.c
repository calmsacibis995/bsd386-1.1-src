/*
 * Copyright (c) 1991 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	$Id: kern_acct.c,v 1.10 1994/02/06 20:10:56 karels Exp $
 */

/*
 *
 * Copyright (c) 1982, 1986, 1989 Regents of the University of California.
 * All rights reserved.
 *
 * (c) UNIX System Laboratories, Inc.  All or some portions of this file
 * are derived from material licensed to the University of California by
 * American Telephone and Telegraph Co. or UNIX System Laboratories, Inc.
 * and are reproduced herein with the permission of UNIX System Laboratories,
 * Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)kern_acct.c	7.18 (Berkeley) 5/11/91
 */

#include "param.h"
#include "systm.h"
#include "namei.h"
#include "resourcevar.h"
#include "proc.h"
#include "mount.h"
#include "vnode.h"
#include "kernel.h"
#include "file.h"
#include "filedesc.h"
#include "acct.h"
#include "ioctl.h"
#include "tty.h"
#include "syslog.h"

/*
 * Values associated with enabling and disabling accounting
 */
int	acctsuspend = 2;	/* stop accounting when < 2% free space left */
int	acctresume = 4;		/* resume when free space risen to > 4% */
struct	timeval acctinterval =
	{ 15, 0 };		/* frequency to check space for accounting */

/*
 * The first macro tests true if we have run out of space on the accounting
 * file's filesystem; the second tests true if we can turn it back on.
 */
#define	ACCT_FS_INSUFFICIENT_SPACE(sfp) \
	((sfp)->f_bavail <= acctsuspend * (sfp)->f_blocks / 100)
#define	ACCT_FS_ENOUGH_SPACE(sfp) \
	((sfp)->f_bavail > acctresume * (sfp)->f_blocks / 100)

static struct file *acctfp;
static int accounting_disabled;
static void check_acct_fs_space();
static comp_t tv_to_comp __P((struct timeval *));

extern struct fileops vnops;

/*
 * Enable or disable process accounting.
 *
 * If a non-null filename is given, that file is used to store accounting
 * records on process exit. If a null filename is given process accounting
 * is suspended. If accounting is enabled, the system checks the amount
 * of freespace on the filesystem at timeval intervals. If the amount of
 * freespace is below acctsuspend percent, accounting is suspended. If
 * accounting has been suspended, and freespace rises above acctresume,
 * accounting is resumed.
 */
int
sysacct(p, uap, retval)
	struct proc *p;
	struct { char *path; } *uap;
	int *retval;
{
	struct nameidata nd;
	register struct file *fp;
	register struct filedesc *fdp = p->p_fd;
	int mode;
	int error;
	int fd;
	int s;

	if (error = suser(p->p_cred->pc_ucred, (short *) 0))
		return error;

	if (acctfp) {
#if 0		/* XXX why should this be an error?? */
		if (uap->path)
			/*
			 * The man page says that this is an error,
			 * but doesn't state what the error is!
			 * Let's guess that it should be EBUSY.
			 */
			return EBUSY;
#endif
		(void) closef(acctfp, p);
		acctfp = 0;
	}
	if (uap->path == 0)
		return 0;

	/*
	 * Allocate a file struct by 'opening' the file.
	 * XXX What if the caller is out of file descriptors?
	 */
	mode = FFLAGS(O_WRONLY|O_APPEND);
	nd.ni_segflg = UIO_USERSPACE;
	nd.ni_dirp = uap->path;
	if ((error = vn_open(&nd, p, mode, 0)) ||
	    (error = falloc(p, &acctfp, &fd)))
		return error;
	fdp->fd_ofiles[fd] = 0;
	if (fdp->fd_lastfile == fd)
		--fdp->fd_lastfile;
	fdp->fd_freefile = fd;
	fp = acctfp;
	fp->f_flag = mode;
	fp->f_type = DTYPE_VNODE;
	fp->f_ops = &vnops;
	fp->f_data = (caddr_t) nd.ni_vp;
	VOP_UNLOCK(nd.ni_vp);

	check_acct_fs_space();

	return 0;
}

/*
 * Write an accounting record to the accounting file.
 * This code doesn't need a lock since crucial structures are on the stack.
 */
void
acct(p)
	register struct proc *p;
{
	struct acct a;
	struct uio uio;
	struct iovec iov;
	struct timeval tv;
	struct pstats *psp = p->p_stats;
	struct rusage *rup;
	unsigned int i;
	int s;

	check_acct_fs_space();

	if (acctfp == 0 || accounting_disabled)
		return;

	tv.tv_usec = 0;

	strncpy(a.ac_comm, p->p_comm, sizeof a.ac_comm);
	rup = &psp->p_ru;
	a.ac_utime = tv_to_comp(&p->p_utime);
	a.ac_stime = tv_to_comp(&p->p_stime);
	tv = time;
	timevalsub(&tv, &psp->p_start);
	a.ac_etime = tv_to_comp(&tv);
	a.ac_btime = psp->p_start.tv_sec;
	a.ac_uid = p->p_cred->p_ruid;
	a.ac_gid = p->p_cred->p_rgid;
	/* XXX should we bother to preserve low order time bits? */
	if (i = (tv.tv_sec * AHZ) + (tv.tv_usec * AHZ) / 1000000)
		a.ac_mem = ((rup->ru_ixrss + rup->ru_idrss + rup->ru_isrss)
			* AHZ) / i;
	else
		a.ac_mem = 0;
	/* the magic 64 comes from section 3.9 of the BSD book */
	tv.tv_sec = (rup->ru_inblock + rup->ru_oublock) * 64;
	tv.tv_usec = 0;
	a.ac_io = tv_to_comp(&tv);
	if (p->p_pgrp->pg_session->s_ttyp)
		a.ac_tty = p->p_pgrp->pg_session->s_ttyp->t_dev;
	else
		a.ac_tty = NODEV;
	a.ac_flag = p->p_acflag;

	iov.iov_base = (caddr_t) &a;
	iov.iov_len = sizeof a;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_offset = 0;
	uio.uio_resid = sizeof a;
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_rw = UIO_WRITE;
	uio.uio_procp = p;

	/*
	 * If the write fails, we assume we're out of space.
	 */
	if (acctfp && !accounting_disabled)
		if (vn_write(acctfp, &uio, acctfp->f_cred))
			accounting_disabled = 1;
}

/*
 * Check the space available to the accounting file.
 * Disable accounting if there's too little, re-enable if it comes back.
 */
static void
check_acct_fs_space()
{
	struct mount *mp;
	register struct statfs *sfp;
	static struct timeval lasttime;
	struct timeval nexttime;

	if (acctfp == 0)
		return;

	if (timerisset(&lasttime)) {
		nexttime = lasttime;
		timevaladd(&nexttime, &acctinterval);
		if (timercmp(&time, &nexttime, <))
			return;
	}
	lasttime = time;

	mp = ((struct vnode *) acctfp->f_data)->v_mount;
	sfp = &mp->mnt_stat;
	if (VFS_STATFS(mp, sfp, (struct proc *) 0)) {
		accounting_disabled = 1;
		return;
	}

	if (accounting_disabled == 0 && ACCT_FS_INSUFFICIENT_SPACE(sfp)) {
		accounting_disabled = 1;
		log(LOG_NOTICE, "Accounting suspended\n");
	} else if (accounting_disabled && ACCT_FS_ENOUGH_SPACE(sfp)) {
		accounting_disabled = 0;
		log(LOG_NOTICE, "Accounting resumed\n");
	}
}

/*
 * ASSUMES TWO'S COMPLEMENT ARITHMETIC.
 */
static comp_t
tv_to_comp(tvp)
	struct timeval *tvp;
{
	register int i;
	u_long raw = tvp->tv_sec * AHZ + (tvp->tv_usec * AHZ) / 1000000;

	for (i = 0; i < 8; ++i)
		if (raw < 1 << (i * 3 + 13))
			break;
	if (i == 8)
		return ~0;

	return raw >> (i * 3) | (i << 13);
}
