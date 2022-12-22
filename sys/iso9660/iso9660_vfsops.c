/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI $Id: iso9660_vfsops.c,v 1.5 1993/12/17 04:07:17 karels Exp $
 */

/*
 * Copyright (c) 1989, 1991 The Regents of the University of California.
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
 *  Started from
 *	ufs_vfsops.c	7.56 (Berkeley) 6/28/91
 */

#include "param.h"
#include "systm.h"
#include "namei.h"
#include "proc.h"
#include "kernel.h"
#include "vnode.h"
#include "specdev.h"
#include "mount.h"
#include "buf.h"
#include "file.h"
#include "disklabel.h"
#include "ioctl.h"
#include "errno.h"
#include "malloc.h"

#include "iso9660.h"
#include "iso9660_node.h"

static int iso9660_mountedon __P((struct vnode *vp));
static int iso9660_mountfs __P((struct vnode *devvp, struct mount *mp,
		struct proc *p));

extern int enodev();

struct vfsops iso9660_vfsops = {
	iso9660_mount,
	iso9660_start,
	iso9660_unmount,
	iso9660_root,
	(void *)enodev, /* quotactl */
	iso9660_statfs,
	iso9660_sync,
	iso9660_fhtovp,
	iso9660_vptofh,
	iso9660_init
};

/*
 * Called by vfs_mountroot when ufs is going to be mounted as root.
 *
 * Name is updated by mount(8) after booting.
 */
#define ROOTNAME	"root_device"

int
iso9660_mountroot()
{
	register struct mount *mp;
	extern struct vnode *rootvp;
	struct proc *p = curproc;	/* XXX */
	struct iso9660_mnt *imp;
	u_int size;
	int error;

	mp = (struct mount *) malloc((u_long) sizeof(struct mount),
		M_MOUNT, M_WAITOK);
	bzero(mp, sizeof(*mp));
	mp->mnt_op = &iso9660_vfsops;
	mp->mnt_flag = MNT_RDONLY;
	mp->mnt_exroot = 0;
	mp->mnt_mounth = NULLVP;
	error = iso9660_mountfs(rootvp, mp, p);
	if (error) {
		free((caddr_t)mp, M_MOUNT);
		return (error);
	}
	if (error = vfs_lock(mp)) {
		(void)iso9660_unmount(mp, 0, p);
		free((caddr_t)mp, M_MOUNT);
		return (error);
	}
	rootfs = mp;
	mp->mnt_next = mp;
	mp->mnt_prev = mp;
	mp->mnt_vnodecovered = NULLVP;
	imp = VFSTOISO9660(mp);
	bzero(imp->im_fsmnt, sizeof(imp->im_fsmnt));
	imp->im_fsmnt[0] = '/';
	bcopy((caddr_t)imp->im_fsmnt, (caddr_t)mp->mnt_stat.f_mntonname,
	    MNAMELEN);
	(void) copystr(ROOTNAME, mp->mnt_stat.f_mntfromname, MNAMELEN - 1,
	    &size);
	bzero(mp->mnt_stat.f_mntfromname + size, MNAMELEN - size);
	(void) iso9660_statfs(mp, &mp->mnt_stat, p);
	vfs_unlock(mp);
	inittodr((time_t)0);
	return (0);
}

/*
 * Flag to allow forcible unmounting.
 */
int iso9660_doforce = 1;

/*
 * VFS Operations.
 *
 * mount system call
 */
int
iso9660_mount(mp, path, data, ndp, p)
	register struct mount *mp;
	char *path;
	caddr_t data;
	struct nameidata *ndp;
	struct proc *p;
{
	struct vnode *devvp;
	struct iso9660_args args;
	u_int size;
	int error;
	struct iso9660_mnt *imp;

	if (error = copyin(data, (caddr_t)&args, sizeof (struct iso9660_args)))
		return (error);

	if ((mp->mnt_flag & MNT_RDONLY) == 0)
		return (EROFS);

	/*
	 * Process export requests.
	 */
	if ((args.exflags & MNT_EXPORTED) || (mp->mnt_flag & MNT_EXPORTED)) {
		if (args.exflags & MNT_EXPORTED)
			mp->mnt_flag |= MNT_EXPORTED;
		else
			mp->mnt_flag &= ~MNT_EXPORTED;
		if (args.exflags & MNT_EXRDONLY)
			mp->mnt_flag |= MNT_EXRDONLY;
		else
			mp->mnt_flag &= ~MNT_EXRDONLY;
		mp->mnt_exroot = args.exroot;
	}
	/*
	 * If updating, check whether changing from read-only to
	 * read/write; if there is no device name, that's all we do.
	 */
	if (mp->mnt_flag & MNT_UPDATE) {
		imp = VFSTOISO9660(mp);
		if (args.fspec == 0)
			return (0);
	}
	/*
	 * Not an update, or updating the name: look up the name
	 * and verify that it refers to a sensible block device.
	 */
	ndp->ni_nameiop = LOOKUP | FOLLOW;
	ndp->ni_segflg = UIO_USERSPACE;
	ndp->ni_dirp = args.fspec;
	if (error = namei(ndp, p))
		return (error);
	devvp = ndp->ni_vp;
	if (devvp->v_type != VBLK) {
		vrele(devvp);
		return (ENOTBLK);
	}
	if (major(devvp->v_rdev) >= nblkdev) {
		vrele(devvp);
		return (ENXIO);
	}

	if ((mp->mnt_flag & MNT_UPDATE) == 0)
		error = iso9660_mountfs(devvp, mp, p);
	else {
		if (devvp != imp->im_devvp)
			error = EINVAL;	/* needs translation */
		else
			vrele(devvp);
	}
	if (error) {
		vrele(devvp);
		return (error);
	}
	imp = VFSTOISO9660(mp);

	imp->im_raw = args.raw;

	(void) copyinstr(path, imp->im_fsmnt, sizeof(imp->im_fsmnt)-1, &size);
	bzero(imp->im_fsmnt + size, sizeof(imp->im_fsmnt) - size);
	bcopy((caddr_t)imp->im_fsmnt, (caddr_t)mp->mnt_stat.f_mntonname,
	    MNAMELEN);
	(void) copyinstr(args.fspec, mp->mnt_stat.f_mntfromname, MNAMELEN - 1, 
	    &size);
	bzero(mp->mnt_stat.f_mntfromname + size, MNAMELEN - size);
	(void) iso9660_statfs(mp, &mp->mnt_stat, p);
	return (0);
}

/*
 * Common code for mount and mountroot
 */
static int
iso9660_mountfs(devvp, mp, p)
	register struct vnode *devvp;
	struct mount *mp;
	struct proc *p;
{
	register struct iso9660_mnt *imp = (struct iso9660_mnt *)0;
	struct buf *bp = NULL;
	dev_t dev = devvp->v_rdev;
	int error = EINVAL;
	int needclose = 0;
	extern struct vnode *rootvp;
	int iso9660_blknum;
	struct iso9660_volume_descriptor *vdp;
	struct iso9660_primary_descriptor *pri;
	struct iso9660_directory_record *rootp;
	int logical_block_size;
	int extent;

	if ((mp->mnt_flag & MNT_RDONLY) == 0)
		return (EROFS);

	/*
	 * Disallow multiple mounts of the same device.
	 * Disallow mounting of a device that is currently in use
	 * (except for root, which might share swap device for miniroot).
	 * Flush out any old buffers remaining from a previous use.
	 */
	if (error = iso9660_mountedon(devvp))
		return (error);
	if (vcount(devvp) > 1 && devvp != rootvp)
		return (EBUSY);
	vinvalbuf(devvp, 1);
	if (error = VOP_OPEN(devvp, FREAD, NOCRED, p))
		return (error);
	needclose = 1;

	for (iso9660_blknum = 16; iso9660_blknum < 100; iso9660_blknum++) {
		if (error = bread(devvp,
		    iso9660_blknum * (ISO9660_LSS / DEV_BSIZE),
		    ISO9660_LSS, NOCRED, &bp))
			goto out;

		vdp = (struct iso9660_volume_descriptor *)bp->b_un.b_addr;
		if (bcmp(vdp->id, ISO9660_STANDARD_ID, sizeof vdp->id) != 0) {
			error = EINVAL;
			goto out;
		}

		if (iso9660num_711(vdp->type) == ISO9660_VD_END) {
			error = EINVAL;
			goto out;
		}

		if (iso9660num_711(vdp->type) == ISO9660_VD_PRIMARY)
			break;
	}

	if (iso9660num_711(vdp->type) != ISO9660_VD_PRIMARY) {
		error = EINVAL;
		goto out;
	}
	
	pri = (struct iso9660_primary_descriptor *) vdp;

	logical_block_size = iso9660num_723(pri->logical_block_size);

	if (logical_block_size < DEV_BSIZE || logical_block_size >= MAXBSIZE ||
	    (logical_block_size & (logical_block_size - 1)) != 0) {
		error = EINVAL;
		goto out;
	}

	rootp = (struct iso9660_directory_record *)pri->root_directory_record;

	imp = (struct iso9660_mnt *) malloc(sizeof(*imp), M_UFSMNT, M_WAITOK);
	bzero(imp, sizeof(*imp));
	imp->logical_block_size = logical_block_size;
	imp->volume_space_size = iso9660num_733(pri->volume_space_size);
	bcopy(rootp, imp->root, sizeof imp->root);

	imp->im_bsize = logical_block_size;
	imp->im_bmask = ~(imp->im_bsize - 1);
	imp->im_bshift = 0;
	while ((1 << imp->im_bshift) < imp->im_bsize)
		imp->im_bshift++;

	extent = iso9660num_733(rootp->extent);
	imp->root_fileid = iso9660_lblktosize(imp, (iso9660_fileid_t)extent);
	imp->root_size = iso9660num_733(rootp->size);

	bp->b_flags |= B_INVAL;
	brelse(bp);
	bp = NULL;

	mp->mnt_data = (qaddr_t)imp;
	mp->mnt_stat.f_fsid.val[0] = (long)dev;
	mp->mnt_stat.f_fsid.val[1] = MOUNT_ISO9660;
	mp->mnt_flag |= MNT_LOCAL;
	imp->im_mountp = mp;
	imp->im_dev = dev;
	imp->im_devvp = devvp;

	devvp->v_specflags |= SI_MOUNTEDON;

	return (0);
out:
	if (bp)
		brelse(bp);
	if (needclose)
		(void)VOP_CLOSE(devvp, FREAD, NOCRED, p);
	if (imp) {
		free((caddr_t)imp, M_UFSMNT);
		mp->mnt_data = (qaddr_t)0;
	}
	return (error);
}

/*
 * Make a filesystem operational.
 * Nothing to do at the moment.
 */
/* ARGSUSED */
int
iso9660_start(mp, flags, p)
	struct mount *mp;
	int flags;
	struct proc *p;
{

	return (0);
}

/*
 * unmount system call
 */
int
iso9660_unmount(mp, mntflags, p)
	struct mount *mp;
	int mntflags;
	struct proc *p;
{
	register struct iso9660_mnt *imp;
	int error, flags = 0;

	if (mntflags & MNT_FORCE) {
		if (!iso9660_doforce || mp == rootfs)
			return (EINVAL);
		flags |= FORCECLOSE;
	}
	mntflushbuf(mp, 0);
	if (mntinvalbuf(mp))
		return (EBUSY);
	imp = VFSTOISO9660(mp);

	if (error = vflush(mp, NULLVP, flags))
		return (error);
	imp->im_devvp->v_specflags &= ~SI_MOUNTEDON;
	error = VOP_CLOSE(imp->im_devvp, FREAD, NOCRED, p);
	vrele(imp->im_devvp);
	free((caddr_t)imp, M_UFSMNT);
	mp->mnt_data = (qaddr_t)0;
	mp->mnt_flag &= ~MNT_LOCAL;
	return (error);
}

/*
 * Check to see if a filesystem is mounted on a block device.
 */
static int
iso9660_mountedon(vp)
	register struct vnode *vp;
{
	register struct vnode *vq;

	if (vp->v_specflags & SI_MOUNTEDON)
		return (EBUSY);
	if (vp->v_flag & VALIASED) {
		for (vq = *vp->v_hashchain; vq; vq = vq->v_specnext) {
			if (vq->v_rdev != vp->v_rdev ||
			    vq->v_type != vp->v_type)
				continue;
			if (vq->v_specflags & SI_MOUNTEDON)
				return (EBUSY);
		}
	}
	return (0);
}

struct ifid {
	u_short	len;	/* length of structure */
	u_short	pad;	/* force long alignment */
	iso9660_fileid_t fileid;
};

/*
 * Return root of a filesystem
 */
int
iso9660_root(mp, vpp)
	struct mount *mp;
	struct vnode **vpp;
{
	struct ifid ifid;
	struct iso9660_mnt *imp;
	int error;
	int suoffset;
	struct susp_sp *sp;
	struct iso9660_directory_record *ep;
	int reclen;
	struct uio uio;
	struct iovec iov;
	char dirbuf[256];

	imp = VFSTOISO9660(mp);

	ifid.len = sizeof ifid;
	ifid.fileid = imp->root_fileid;

	if (error = iso9660_fhtovp(mp, (struct fid *)&ifid, vpp))
		return (error);

	if (imp->im_raw || imp->im_checked_for_rock_ridge)
		return (0);

	imp->im_checked_for_rock_ridge = 1;

	iov.iov_base = dirbuf;
	iov.iov_len = sizeof dirbuf;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_offset = 0;
	uio.uio_resid = sizeof dirbuf;
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_rw = UIO_READ;
	uio.uio_procp = NULL;

	if (error = iso9660_read (*vpp, &uio, 0, NOCRED))
		goto done;

	if (sizeof dirbuf - uio.uio_resid < 
	    sizeof (struct iso9660_directory_record))
		goto done;

	ep = (struct iso9660_directory_record *) dirbuf;

	reclen = iso9660num_711(ep->length);

	suoffset = roundup(ISO9660_DIR_NAME + iso9660num_711(ep->name_len), 2);

	if (suoffset + SUSP_SP_LEN > reclen)
		goto done;

	sp = (struct susp_sp *)(dirbuf + suoffset);

	if (sp->code[0] == 'S' && sp->code[1] == 'P' &&
	    iso9660num_711 (sp->length) >= SUSP_SP_LEN &&
	    sp->check[0] == 0xbe && sp->check[1] == 0xef) {
		imp->im_have_rock_ridge = 1;
		imp->im_rock_ridge_skip = iso9660num_711(sp->len_skp);
	}

done:
	return (error);
}

/*
 * Get file system statistics.
 */
int
iso9660_statfs(mp, sbp, p)
	struct mount *mp;
	register struct statfs *sbp;
	struct proc *p;
{
	register struct iso9660_mnt *imp;
	int cdmax;
	int cdfree;

	imp = VFSTOISO9660(mp);

	/* 74 minutes * 60 sec/min * 75 blocks/sec */
	cdmax = 74 * 60 * 75;
	if (cdmax < imp->volume_space_size)
		cdmax = imp->volume_space_size;
	cdfree = cdmax - imp->volume_space_size;

	sbp->f_type = MOUNT_ISO9660;
	sbp->f_fsize = imp->logical_block_size;
	sbp->f_bsize = sbp->f_fsize;
	sbp->f_blocks = cdmax;
	sbp->f_bfree = cdfree; /* total free blocks */
	sbp->f_bavail = cdfree; /* blocks for non root */
	sbp->f_files =  0; /* total files */
	sbp->f_ffree = 0; /* free file nodes */
	if (sbp != &mp->mnt_stat) {
		bcopy((caddr_t)mp->mnt_stat.f_mntonname,
			(caddr_t) &sbp->f_mntonname[0], MNAMELEN);
		bcopy((caddr_t)mp->mnt_stat.f_mntfromname,
			(caddr_t) &sbp->f_mntfromname[0], MNAMELEN);
	}
	return (0);
}

int
iso9660_sync(mp, waitfor)
	struct mount *mp;
	int waitfor;
{
	return (0);
}

int
iso9660_fhtovp(mp, fhp, vpp)
	register struct mount *mp;
	struct fid *fhp; /* 16 bytes of private data */
	struct vnode **vpp;
{
	struct ifid *ifhp;
	struct iso9660_mnt *imp;
	struct vnode tvp;
	struct iso9660_node *ip, *nip;
	int error = 0;
	struct iso9660_dir_info *dirp = NULL;

	ifhp = (struct ifid *)fhp;
	imp = VFSTOISO9660(mp);

	if (ifhp->fileid >= iso9660_lblktosize(imp,
	    (iso9660_fileid_t) imp->volume_space_size))
		return (EINVAL);

	tvp.v_mount = mp;
	ip = VTOI(&tvp);
	ip->i_vnode = &tvp;
	ip->i_dev = imp->im_dev;

	/* this just does a lookup in the cache */
	if ((error = iso9660_iget(ip, ifhp->fileid, &nip, NULL)) == 0) {
		ip = nip;
		*vpp = ITOV(ip);
		return (0);
	}

	if (error != ENOENT)
		return (error);

	/* not in inode cache ... must get directory info */

	if (error = iso9660_diropen (imp, ifhp->fileid, ISO9660_MAX_FILEID,
	    0, 0, NULL, &dirp))
		return (error);

	if (error = iso9660_dirget (dirp))
		goto ret;

	if (dirp->translated_fileid != ifhp->fileid) {
		error = EINVAL;
		goto ret;
	}

	if (error = iso9660_iget(ip, ifhp->fileid, &nip, dirp))
		goto ret;

	ip = nip;
	*vpp = ITOV(ip);

ret:
	if (dirp)
		iso9660_dirclose(dirp);

	return (error);
}


/*
 * Vnode pointer to File handle
 */
/* ARGSUSED */
int
iso9660_vptofh(vp, fhp)
	struct vnode *vp;
	struct fid *fhp;
{
	register struct iso9660_node *ip = VTOI(vp);
	register struct ifid *ifhp;

	ifhp = (struct ifid *)fhp;
	ifhp->len = sizeof(struct ifid);
	ifhp->fileid = ip->fileid;
	return (0);
}
