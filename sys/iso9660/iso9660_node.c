/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI $Id: iso9660_node.c,v 1.3 1993/02/21 20:43:30 karels Exp $
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
 *	@(#)iso9660_inode.c
 */

#include "param.h"
#include "systm.h"
#include "mount.h"
#include "proc.h"
#include "file.h"
#include "buf.h"
#include "vnode.h"
#include "kernel.h"
#include "malloc.h"

#include "iso9660.h"
#include "iso9660_node.h"

#define	INOHSZ	512
#if	((INOHSZ&(INOHSZ-1)) == 0)
#define	INOHASH(dev,ino)	(((dev)+(ino))&(INOHSZ-1))
#else
#define	INOHASH(dev,ino)	(((unsigned)((dev)+(ino)))%INOHSZ)
#endif

union iso9660_ihead {
	union  iso9660_ihead *ih_head[2];
	struct iso9660_node *ih_chain[2];
} iso9660_ihead[INOHSZ];

int prtactive;	/* 1 => print out reclaim of active vnodes */

/*
 * Initialize hash links for inodes.
 */
int
iso9660_init()
{
	register int i;
	register union iso9660_ihead *ih = iso9660_ihead;

#ifndef lint
	if (VN_MAXPRIVATE < sizeof(struct iso9660_node))
		panic("iso9660_init: too small");
#endif /* not lint */
	for (i = INOHSZ; --i >= 0; ih++) {
		ih->ih_head[0] = ih;
		ih->ih_head[1] = ih;
	}

	return (0);
}

/*
 * Look up a ISO9660 dinode number to find its incore vnode.
 * If it is not in core, read it in from the specified device.
 * If it is in core, wait for the lock bit to clear, then
 * return the inode locked. Detection and handling of mount
 * points must be done by the calling routine.
 */
int
iso9660_iget(xp, fileid, ipp, dirp)
	struct iso9660_node *xp;
	iso9660_fileid_t fileid;
	struct iso9660_node **ipp;
	struct iso9660_dir_info *dirp;
{
	dev_t dev = xp->i_dev;
	struct mount *mntp = ITOV(xp)->v_mount;
	extern struct vnodeops iso9660_vnodeops;
	extern struct vnodeops spec_iso9660_vnodeops;
	register struct iso9660_node *ip, *iq;
	register struct vnode *vp;
	struct vnode *nvp;
	union iso9660_ihead *ih;
	int error;
	struct iso9660_mnt *imp;

	ih = &iso9660_ihead[INOHASH(dev, fileid)];
loop:
	for (ip = ih->ih_chain[0]; ip != (struct iso9660_node *)ih;
	    ip = ip->i_forw) {
		if (fileid != ip->fileid || dev != ip->i_dev)
			continue;
		if ((ip->i_flag&ILOCKED) != 0) {
			ip->i_flag |= IWANT;
			sleep((caddr_t)ip, PINOD);
			goto loop;
		}
		if (vget(ITOV(ip)))
			goto loop;
		*ipp = ip;
		return(0);
	}

	if (dirp == NULL) {
		*ipp = 0;
		return (ENOENT);
	}

	/*
	 * Allocate a new inode.
	 */
	if (error = getnewvnode(VT_ISO9660, mntp, &iso9660_vnodeops, &nvp)) {
		*ipp = 0;
		return (error);
	}
	ip = VTOI(nvp);
	bzero (ip, sizeof *ip);
	ip->i_vnode = nvp;
	ip->i_flag = 0;
	ip->i_devvp = 0;
	ip->dir_last_fileid = 0;
	/*
	 * Put it onto its hash chain and lock it so that other requests for
	 * this inode will block if they arrive while we are sleeping waiting
	 * for old data structures to be purged or for the contents of the
	 * disk portion of this inode to be read.
	 */
	ip->i_dev = dev;
	ip->fileid = fileid;
	insque(ip, ih);
	ISO9660_ILOCK(ip);

	ip->extent = dirp->extent;
	ip->size = dirp->size;
	ip->time_parsed = 0;
	bcopy(dirp->iso9660_time, ip->iso9660_time, sizeof ip->iso9660_time);

	ip->rr = dirp->rr;

	/*
	 * Initialize the associated vnode
	 */
	vp = ITOV(ip);

	if (dirp->rr.rr_flags & RR_PX) {
		switch (dirp->rr.rr_mode & 0170000) {
		case 0010000: vp->v_type = VFIFO; break;
		case 0020000: vp->v_type = VCHR; break;
		case 0040000: vp->v_type = VDIR; break;
		case 0060000: vp->v_type = VBLK; break;
		case 0100000: vp->v_type = VREG; break;
		case 0120000: vp->v_type = VLNK; break;
		case 0140000: vp->v_type = VSOCK; break;
		default: vp->v_type = VNON; break;
		}
	} else if (dirp->iso9660_flags & ISO9660_DIR_FLAG)
		vp->v_type = VDIR;
	else
		vp->v_type = VREG;

	if (vp->v_type == VLNK)
		ip->size = dirp->link_used;

	if (vp->v_type == VCHR || vp->v_type == VBLK) {
		vp->v_op = &spec_iso9660_vnodeops;
		if (nvp = checkalias(vp, ip->rr.rr_rdev, mntp)) {
			/*
			 * Reinitialize aliased inode.
			 */
			vp = nvp;
			iq = VTOI(vp);
			iq->i_vnode = vp;
			iq->i_flag = 0;
			ISO9660_ILOCK(iq);
			iq->rr = ip->rr;
			iq->i_dev = dev;
			iq->fileid = fileid;
			insque(iq, ih);
			/*
			 * Discard unneeded vnode
			 */
			ip->deleted = 1;
			iso9660_iput(ip);
			ip = iq;
		}
	}

	imp = VFSTOISO9660(mntp);

	if (fileid == imp->root_fileid)
		vp->v_flag |= VROOT;
	/*
	 * Finish inode initialization.
	 */
	ip->i_mnt = imp;
	ip->i_devvp = imp->im_devvp;
	VREF(ip->i_devvp);
	*ipp = ip;
	return (0);
}

/*
 * Unlock and decrement the reference count of an inode structure.
 */
void
iso9660_iput(ip)
	register struct iso9660_node *ip;
{

	if ((ip->i_flag & ILOCKED) == 0)
		panic("iso9660_iput");
	ISO9660_IUNLOCK(ip);
	vrele(ITOV(ip));
}

/*
 * Last reference to an inode, write the inode out and if necessary,
 * truncate and deallocate the file.
 */
int
iso9660_inactive(vp, p)
	struct vnode *vp;
	struct proc *p;
{
	register struct iso9660_node *ip = VTOI(vp);
	int error = 0;

	if (prtactive && vp->v_usecount != 0)
		vprint("iso9660_inactive: pushing active", vp);

	ip->i_flag = 0;

	/*
	 * If we are done with the inode, reclaim it
	 * so that it can be reused immediately.
	 */
	if (vp->v_usecount == 0 && ip->deleted)
		vgone(vp);

	return (error);
}

/*
 * Reclaim an inode so that it can be used for other purposes.
 */
int
iso9660_reclaim(vp)
	register struct vnode *vp;
{
	register struct iso9660_node *ip = VTOI(vp);

	if (prtactive && vp->v_usecount != 0)
		vprint("iso9660_reclaim: pushing active", vp);
	/*
	 * Remove the inode from its hash chain.
	 */
	remque(ip);
	ip->i_forw = ip;
	ip->i_back = ip;
	/*
	 * Purge old data structures associated with the inode.
	 */
	cache_purge(vp);
	if (ip->i_devvp) {
		vrele(ip->i_devvp);
		ip->i_devvp = 0;
	}
	ip->i_flag = 0;
	return (0);
}

/*
 * Lock an inode. If its already locked, set the WANT bit and sleep.
 */
void
iso9660_ilock(ip)
	register struct iso9660_node *ip;
{

	while (ip->i_flag & ILOCKED) {
		ip->i_flag |= IWANT;
		if (ip->i_spare0 == curproc->p_pid)
			panic("locking against myself");
		ip->i_spare1 = curproc->p_pid;
		(void) sleep((caddr_t)ip, PINOD);
	}
	ip->i_spare1 = 0;
	ip->i_spare0 = curproc->p_pid;
	ip->i_flag |= ILOCKED;
}

/*
 * Unlock an inode.  If WANT bit is on, wakeup.
 */
void
iso9660_iunlock(ip)
	register struct iso9660_node *ip;
{

	if ((ip->i_flag & ILOCKED) == 0)
		vprint("iso9660_iunlock: unlocked inode", ITOV(ip));
	ip->i_spare0 = 0;
	ip->i_flag &= ~ILOCKED;
	if (ip->i_flag&IWANT) {
		ip->i_flag &= ~IWANT;
		wakeup((caddr_t)ip);
	}
}
