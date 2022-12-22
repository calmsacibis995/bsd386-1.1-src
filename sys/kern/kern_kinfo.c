/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: kern_kinfo.c,v 1.8 1993/08/23 02:14:49 karels Exp $
 */

/*
 * Copyright (c) 1982, 1986, 1989 Regents of the University of California.
 * All rights reserved.
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
 *	@(#)kern_kinfo.c	7.17 (Berkeley) 6/26/91
 */

#include "param.h"
#include "proc.h"
#include "ioctl.h"
#include "tty.h"
#include "buf.h"
#include "file.h"
#include "kernel.h"
#include "systm.h"

#include "vm/vm.h"
#include "vm/vm_param.h"
#include "vm/vm_page.h"
#include "vm/vm_statistics.h"
#include "vmmeter.h"
#include "malloc.h"
#include "namei.h"

#include "kinfo.h"
#include "kinfo_proc.h"
#include "cpustats.h"
#include "disk.h"
#include "sysinfo.h"

extern int kinfo_doproc();
extern int kinfo_rtable();
extern int kinfo_vnode();
extern int kinfo_file();
extern int kinfo_disk_stats(), kinfo_disk_names();
extern int kinfo_tty_stats(), kinfo_tty_names();
extern int kinfo_sysinfo();
extern int kinfo_vm();

extern struct cpustats cpustats;
extern struct ttytotals ttytotals;

struct kinfo_lock kinfo_lock;

/* ARGSUSED */
getkerninfo(p, uap, retval)
	struct proc *p;
	register struct args {
		int	op;
		char	*where;
		int	*size;
		int	arg;
	} *uap;
	int *retval;
{
	int bufsize;		/* max size of users buffer */
	int needed, locked, (*server)() = NULL, error = 0;
	void *t;

	/*
	 * For each supported type, we set either a server procedure
	 * to locate and copy out the data, or we set a pointer
	 * and length to the structure to be copied out.
	 * In the latter case, we don't need to lock down memory
	 * or serialize.
	 */
	switch (ki_type(uap->op)) {

	case KINFO_PROC:
		server = kinfo_doproc;
		break;

	case KINFO_RT:
		server = kinfo_rtable;
		break;

	case KINFO_VNODE:
		server = kinfo_vnode;
		break;

	case KINFO_FILE:
		server = kinfo_file;
		break;

	case KINFO_SYSINFO:
		server = kinfo_sysinfo;
		break;

	case KINFO_CPU:
		t = &cpustats;
		needed = sizeof(cpustats);
		break;

	case KINFO_VM:
		server = kinfo_vm;
		break;

	case KINFO_DISK_NAMES:
		server = kinfo_disk_names;
		break;

	case KINFO_DISK_STATS:
		server = kinfo_disk_stats;
		break;

	case KINFO_TTY_TOTALS:
		t = &ttytotals;
		needed = sizeof(ttytotals);
		break;

	case KINFO_TTY_NAMES_TMP:
		server = kinfo_tty_names;
		break;

	case KINFO_TTY_STATS_TMP:
		server = kinfo_tty_stats;
		break;

	default:
		error = EINVAL;
		goto done;
	}

	/*
	 * If either where or size parameters are null,
	 * this is a call to determine the required size
	 * without copying out anything.
	 */
	if (uap->where == NULL || uap->size == NULL) {
		if (server)
			error = (*server)(uap->op, NULL, NULL, uap->arg,
			    &needed);
		goto done;
	}
	if (error = copyin((caddr_t)uap->size, (caddr_t)&bufsize,
	    sizeof(bufsize)))
		goto done;

	if (server == NULL) {
		if (bufsize < needed)
			bufsize = 0;
		else {
			bufsize = needed;
			error = copyout(t, uap->where, needed);
		}
	} else {
		if (!useracc(uap->where, bufsize, B_WRITE)) {
			error = EFAULT;
			goto done;
		}
		while (kinfo_lock.kl_lock) {
			kinfo_lock.kl_want++;
			sleep(&kinfo_lock, PRIBIO+1);
			kinfo_lock.kl_want--;
			kinfo_lock.kl_locked++;
		}
		kinfo_lock.kl_lock++;

		if (server != kinfo_vnode)	/* XXX */
			vslock(uap->where, bufsize);
		locked = bufsize;
		error = (*server)(uap->op, uap->where, &bufsize, uap->arg,
		    &needed);
		if (server != kinfo_vnode)	/* XXX */
			vsunlock(uap->where, locked, B_WRITE);
		kinfo_lock.kl_lock--;
		if (kinfo_lock.kl_want)
			wakeup(&kinfo_lock);
	}
	if (error == 0)
		error = copyout((caddr_t)&bufsize, (caddr_t)uap->size,
		    sizeof(bufsize));
done:
	if (!error)
		*retval = needed;
	return (error);
}

/* 
 * try over estimating by 5 procs 
 */
#define KINFO_PROCSLOP	(5 * sizeof (struct kinfo_proc))

kinfo_doproc(op, where, acopysize, arg, aneeded)
	char *where;
	int *acopysize, *aneeded;
{
	register struct proc *p;
	register struct kinfo_proc *dp = (struct kinfo_proc *)where;
	register needed = 0;
	int buflen;
	int doingzomb;
	struct eproc eproc;
	int error = 0;

	if (where != NULL)
		buflen = *acopysize;

	p = allproc;
	doingzomb = 0;
again:
	for (; p != NULL; p = p->p_nxt) {
		/* 
		 * TODO - make more efficient (see notes below).
		 * do by session. 
		 */
		switch (ki_op(op)) {

		case KINFO_PROC_PID:
			/* could do this with just a lookup */
			if (p->p_pid != (pid_t)arg)
				continue;
			break;

		case KINFO_PROC_PGRP:
			/* could do this by traversing pgrp */
			if (p->p_pgrp->pg_id != (pid_t)arg)
				continue;
			break;

		case KINFO_PROC_TTY:
			if ((p->p_flag&SCTTY) == 0 || 
			    p->p_session->s_ttyp == NULL ||
			    p->p_session->s_ttyp->t_dev != (dev_t)arg)
				continue;
			break;

		case KINFO_PROC_UID:
			if (p->p_ucred->cr_uid != (uid_t)arg)
				continue;
			break;

		case KINFO_PROC_RUID:
			if (p->p_cred->p_ruid != (uid_t)arg)
				continue;
			break;
		}
		if (where != NULL && buflen >= sizeof (struct kinfo_proc)) {
			fill_eproc(p, &eproc);
			if (error = copyout((caddr_t)p, &dp->kp_proc, 
			    sizeof (struct proc)))
				return (error);
			if (error = copyout((caddr_t)&eproc, &dp->kp_eproc, 
			    sizeof (eproc)))
				return (error);
			dp++;
			buflen -= sizeof (struct kinfo_proc);
		}
		needed += sizeof (struct kinfo_proc);
	}
	if (doingzomb == 0) {
		p = zombproc;
		doingzomb++;
		goto again;
	}
	if (where != NULL)
		*acopysize = (caddr_t)dp - where;
	else
		needed += KINFO_PROCSLOP;
	*aneeded = needed;

	return (0);
}

/*
 * Fill in an eproc structure for the specified process.
 */
void
fill_eproc(p, ep)
	register struct proc *p;
	register struct eproc *ep;
{
	register struct tty *tp;

	ep->e_paddr = p;
	ep->e_sess = p->p_pgrp->pg_session;
	ep->e_pcred = *p->p_cred;
	ep->e_ucred = *p->p_ucred;
	ep->e_vm = *p->p_vmspace;
	if (p->p_pptr)
		ep->e_ppid = p->p_pptr->p_pid;
	else
		ep->e_ppid = 0;
	ep->e_pgid = p->p_pgrp->pg_id;
	ep->e_jobc = p->p_pgrp->pg_jobc;
	if ((p->p_flag&SCTTY) && 
	     (tp = ep->e_sess->s_ttyp)) {
		ep->e_tdev = tp->t_dev;
		ep->e_tpgid = tp->t_pgrp ? tp->t_pgrp->pg_id : NO_PID;
		ep->e_tsess = tp->t_session;
	} else
		ep->e_tdev = NODEV;
	ep->e_flag = ep->e_sess->s_ttyvp ? EPROC_CTTY : 0;
	if (SESS_LEADER(p))
		ep->e_flag |= EPROC_SLEADER;
	if (p->p_wchan && p->p_wmesg)
		strncpy(ep->e_wmesg, p->p_wmesg, WMESGLEN);
	else
		ep->e_wmesg[0] = '\0';
	strncpy(ep->e_login, p->p_session->s_login, sizeof(ep->e_login));
	ep->e_xsize = ep->e_xrssize = 0;
	ep->e_xccount = ep->e_xswrss = 0;
}

/*
 * Get file structures.
 */
kinfo_file(op, where, acopysize, arg, aneeded)
	register char *where;
	int *acopysize, *aneeded;
{
	int buflen, needed, error;
	struct file *fp;
	char *start = where;

	if (where == NULL) {
		/*
		 * overestimate by 10 files
		 */
		*aneeded = sizeof (filehead) + 
			(nfiles + 10) * sizeof (struct file);
		return (0);
	}
	buflen = *acopysize;
	needed = 0;

	/*
	 * first copyout filehead
	 */
	if (buflen > sizeof (filehead)) {
		if (error = copyout((caddr_t)&filehead, where,
		    sizeof (filehead)))
			return (error);
		buflen -= sizeof (filehead);
		where += sizeof (filehead);
	}
	needed += sizeof (filehead);

	/*
	 * followed by an array of file structures
	 */
	for (fp = filehead; fp != NULL; fp = fp->f_filef) {
		if (buflen > sizeof (struct file)) {
			if (error = copyout((caddr_t)fp, where,
			    sizeof (struct file)))
				return (error);
			buflen -= sizeof (struct file);
			where += sizeof (struct file);
		}
		needed += sizeof (struct file);
	}
	*acopysize = where - start;
	*aneeded = needed;

	return (0);
}

/*
 * Return a list of disk names, each null terminated,
 * each starting after the previous null, in the same order
 * as the disk statistics will be returned.
 */
kinfo_disk_names(op, where, acopysize, arg, aneeded)
	register char *where;
	int *acopysize, *aneeded;
{
	int error;
	int buflen = 0;
	char *start = where;
	register int needed = 0;
	register struct dkdevice *dk;
	int namelen;

	if (where != NULL)
		buflen = *acopysize;

	/*
	 * copyout the disk names
	 */
	for (dk = diskhead; dk; dk = dk->dk_next) {
		namelen = strlen(dk->dk_dev.dv_xname) + 1;

		needed += namelen;
		if (needed > buflen)
			continue;

		/* copyout name */
		if (error = copyout(dk->dk_dev.dv_xname, where, namelen))
			return (error);
		where += namelen;
	}
	*aneeded = needed;
	return (0);
}

/*
 * Get disk statistics.
 */
kinfo_disk_stats(op, where, acopysize, arg, aneeded)
	register char *where;
	int *acopysize, *aneeded;
{
	int error;
	int buflen = 0;
	char *start = where;
	register int needed = 0;
	register struct dkdevice *dk;

	if (where != NULL)
		buflen = *acopysize;
	/*
	 * copyout the kinfo_disk_stats's
	 */
	for (dk = diskhead; dk; dk = dk->dk_next) {
		needed += sizeof(dk->dk_stats);
		if (needed > buflen)
			continue;
		if (error = copyout(&dk->dk_stats, where, sizeof(dk->dk_stats)))
			return (error);
		where += sizeof(dk->dk_stats);
	}

	if (where != NULL)
		*acopysize = where - start;
	*aneeded = needed;
	return (0);
}

/*
 * XXX The sysinfo struct should be elsewhere.
 * It should be filled in at compile/boot/config time,
 * as these things are set.
 */
struct	sysinfo sysinfo;
extern	char sysinfostrings[];
extern	int sysinfostringlen;

kinfo_sysinfo(op, where, acopysize, arg, aneeded)
	register char *where;
	int *acopysize, *aneeded;
{
	int error;
	int needed = sizeof(sysinfo) + sysinfostringlen + hostnamelen;
	extern int cpu;
	int len;

	/*
	 * If there isn't sufficient room for the initial structure,
	 * don't bother with anything else.
	 */
	*aneeded = needed;
	if (where == NULL || *acopysize < sizeof(sysinfo)) {
		if (acopysize)
			*acopysize = 0;
		return (0);
	}

	/* Copy values into sysinfo that may have changed */

	/* machine info */
	sysinfo.sys_usermem = (vm_page_active_count +
	    vm_page_inactive_count + vm_page_free_count) * CLBYTES;

	/* local info */
	sysinfo.sys_boottime = boottime;

	if (error = copyout((caddr_t)&sysinfo, where, sizeof(sysinfo)))
		return (error);
	len = min(*acopysize - sizeof(struct sysinfo),
	    sysinfostringlen + hostnamelen);
	if (error = copyout(sysinfostrings, where + sizeof(sysinfo), len))
		return (error);
	
	*acopysize = sizeof(struct sysinfo) + len;
	return (0);
}

/*
 * XXX this should be made into a (more reasonable) structure.
 * This isn't all VM; some is cpu.
 * kinfo_vm() copies out:
 *	struct	vmtotal total;
 *	struct	vmmeter cnt;
 *	struct	vm_statistics vm_stat;
 *	struct	nchstats nchstats;
 *      int	num_kmemstats;
 *	struct	kmemstats kmemstats[];
 *	int	num_buckets;
 *	struct	kmembuckets kmembuckets[];
 */
extern	struct nchstats nchstats;
extern	struct kmemstats kmemstats[M_LAST];
extern	struct kmembuckets bucket[MINBUCKET + 16];

kinfo_vm(op, where, acopysize, arg, aneeded)
	register char *where;
	int *acopysize, *aneeded;
{
	int error;
	int buflen = 0;
	char *start = where;
	register int needed = 0;
	int num_kmemstats = M_LAST;
	int num_buckets = MINBUCKET + 16;

	if (where != NULL)
		buflen = *acopysize;

#define KI_CPOUT(what) \
	needed += sizeof(what); \
	if (where != NULL && needed <= buflen) { \
		if (error = copyout((caddr_t)&what, where, sizeof(what))) \
			return (error); \
		where += sizeof(what); \
	}

	vm_stat.pagesize = CLBYTES;
	vm_stat.free_count = vm_page_free_count;
	vm_stat.active_count = vm_page_active_count;
	vm_stat.inactive_count = vm_page_inactive_count;
	vm_stat.wire_count = vm_page_wire_count;

	KI_CPOUT(total);
	KI_CPOUT(cnt);
	KI_CPOUT(vm_stat);
	KI_CPOUT(nchstats);
	KI_CPOUT(num_kmemstats);
	KI_CPOUT(kmemstats);
	KI_CPOUT(num_buckets);
	KI_CPOUT(bucket);

#undef KI_CPOUT
	
	if (where != NULL)
		*acopysize = where - start;
	*aneeded = needed;
	return (0);
}

/*
 * Data for kinfo_tty_stats() and kinfo_tty_names().
 *
 * We maintain a linked list of ttydevice_tmp's.
 * The driver is responsible for maintaining the tty statistics.
 *
 * The driver must call ttyattach() when the tty is configured.
 * The next few bits belong in tty.c.
 */

struct	ttydevice_tmp *ttyhead;		/* null head of chain */
static	int nttydev, ntty;

/*
 * Attach a tty to the stat gathering functions
 */
void
tty_attach(td)
	struct ttydevice_tmp *td;
{
	static struct ttydevice_tmp *ttylast;

	if (ttylast)
		ttylast->tty_next = td;
	else
		ttyhead = td;
	ttylast = td;
	nttydev++;
	ntty += td->tty_count;
}

kinfo_tty_names(op, where, acopysize, arg, aneeded)
	register char *where;
	int *acopysize, *aneeded;
{
	struct ttydevice_tmp *td;
	int buflen, len, error;

	*aneeded = nttydev * sizeof(struct ttydevice_tmp);
	if (where != NULL) {
		buflen = *acopysize;
		len = 0;
		for (td = ttyhead; td && buflen >= sizeof(*td);
		     td = td->tty_next) {
		    	if (error = copyout(td, where, sizeof(*td)))
		    		return (error);
			buflen -= sizeof(*td);
			len += sizeof(*td);
			where += sizeof(*td);
		}
		*acopysize = len;
	}
	return (0);
}

kinfo_tty_stats(op, where, acopysize, arg, aneeded)
	register char *where;
	int *acopysize, *aneeded;
{
	struct ttydevice_tmp *td;
	struct tty *tp;
	struct pgrp *pgrp;
	int buflen, len, count, error;

	*aneeded = ntty * sizeof(struct tty);
	if (where != NULL) {
		buflen = *acopysize;
		len = 0;
		for (td = ttyhead; td; td = td->tty_next) {
		    tp = td->tty_ttys;
		    for (count = td->tty_count; count > 0; count--) {
		    	if (buflen < sizeof(*tp))
		    		break;
		    	if (error = copyout(tp, where, sizeof(*tp)))
		    		return (error);
			/* XXX copy out pgid into pgrp */
			if (tp->t_pgrp) {
				pgrp = (struct pgrp *)tp->t_pgrp->pg_id;
				copyout(&pgrp, &((struct tty *)where)->t_pgrp,
				    sizeof(pgrp));
			}
			buflen -= sizeof(*tp);
			len += sizeof(*tp);
			where += sizeof(*tp);
			tp++;
		    }
		}
		*acopysize = len;
	}
	return (0);
}
