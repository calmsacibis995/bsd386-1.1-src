/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI $Id: iso9660_vnops.c,v 1.4 1993/12/17 04:07:42 karels Exp $
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
 * Started with
 *	ufs_vnops.c	7.64 (Berkeley) 5/16/91
 */
#include "param.h"
#include "systm.h"
#include "namei.h"
#include "resourcevar.h"
#include "kernel.h"
#include "file.h"
#include "stat.h"
#include "buf.h"
#include "proc.h"
#include "conf.h"
#include "mount.h"
#include "vnode.h"
#include "specdev.h"
#include "fifo.h"
#include "malloc.h"
#include "dir.h"
#include "ioctl.h"

#include "iso9660.h"
#include "iso9660_node.h"

/*
 * Open called.
 *
 * Nothing to do.
 */
/* ARGSUSED */
int
iso9660_open(vp, mode, cred, p)
	struct vnode *vp;
	int mode;
	struct ucred *cred;
	struct proc *p;
{

	return (0);
}

/*
 * Close called
 *
 * Update the times on the inode on writeable file systems.
 */
/* ARGSUSED */
int
iso9660_close(vp, fflag, cred, p)
	struct vnode *vp;
	int fflag;
	struct ucred *cred;
	struct proc *p;
{

	return (0);
}

/*
 * Check mode permission on inode pointer. Mode is READ, WRITE or EXEC.
 * The mode is shifted to select the owner/group/other fields. The
 * super user is granted all permissions.
 */
int
iso9660_access(vp, mode, cred, p)
	struct vnode *vp;
	register int mode;
	struct ucred *cred;
	struct proc *p;
{
	register struct iso9660_node *ip = VTOI(vp);
	register gid_t *gp;
	int i, error;

#ifdef DIAGNOSTIC
	if (!VOP_ISLOCKED(vp)) {
		vprint("ufs_access: not locked", vp);
		panic("ufs_access: not locked");
	}
#endif

	/* If no Rock Ridge info, you always get access. */
	if ((ip->rr.rr_flags & RR_PX) == 0)
		return (0);

	/*
	 * If you're the super-user, you always get access.
	 */
	if (cred->cr_uid == 0)
		return (0);
	/*
	 * Access check is based on only one of owner, group, public.
	 * If not owner, then check group. If not a member of the
	 * group, then check public access.
	 */
	if (cred->cr_uid != ip->rr.rr_uid) {
		mode >>= 3;
		gp = cred->cr_groups;
		for (i = 0; i < cred->cr_ngroups; i++, gp++)
			if (ip->rr.rr_gid == *gp)
				goto found;
		mode >>= 3;
found:
		;
	}
	if ((ip->rr.rr_mode & mode) != 0)
		return (0);
	return (EACCES);
}

static time_t
iso9660_parse_time(ip, ts)
	struct iso9660_node *ip;
	int ts;
{
	int year;
	unsigned int month, day, hour, minute, second;
	unsigned int days;
	int tz;
	time_t t;
	u_char *buf;
	int len;

	if ((len = ip->rr.rr_time_format[TS_INDEX(ts)]) == 0) {
		buf = ip->iso9660_time;
		len = RR_SHORT_TIMESTAMP_LEN;
	} else {
		buf = ip->rr.rr_time[TS_INDEX(ts)];
		len = ip->rr.rr_time_format[TS_INDEX(ts)];
	}

	if (len == RR_LONG_TIMESTAMP_LEN) {
		year = (buf[0] - '0') * 1000 + (buf[1] - '0') * 100 +
		    (buf[2] - '0') * 10 + (buf[3] - '0') - 1970;
		month =  (buf[4] - '0') * 10 + (buf[5] - '0');
		day =    (buf[6] - '0') * 10 + (buf[7] - '0');
		hour =   (buf[8] - '0') * 10 + (buf[9] - '0');
		minute = (buf[10] - '0') * 10 + (buf[11] - '0');
		second = (buf[12] - '0') * 10 + (buf[13] - '0');
		tz = iso9660num_712(buf + 16);
	} else {
		year = buf[0] - 70;
		month = buf[1];
		day = buf[2];
		hour = buf[3];
		minute = buf[4];
		second = buf[5];
		tz = iso9660num_712(buf + 6);
	}

	if (year < 0 || month > 12)
		t = 0;
	else {
		/* monlen[X] is number of days before month X */
		static int monlen[12] =
		    {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

		days = year * 365;
		/* 
		 * add a day for each leap year since 1970 (since
		 * 2000 is a leap year, this will work through 2100)
		 */
		if (year > 2)
			days += (year+2) / 4;
		days += monlen[month-1];
		/* 
		 * subtract a day if this is a leap year, 
		 * but we aren't past February yet
		 */
		if (((year+2) % 4) == 0 && month <= 2)
			days--;
		days += day - 1;
		t = ((((days * 24) + hour) * 60 + minute) * 60) + second;
		
		if (-48 <= tz && tz <= 52)
			t -= tz * 15 * 60;
	}

	return (t);
}

/* ARGSUSED */
int
iso9660_getattr(vp, vap, cred, p)
	struct vnode *vp;
	register struct vattr *vap;
	struct ucred *cred;
	struct proc *p;
{
	register struct iso9660_node *ip = VTOI(vp);

	if (!ip->time_parsed) {
		ip->time_parsed = 1;
		ip->mtime = iso9660_parse_time(ip, RR_MODIFY);
		ip->atime = iso9660_parse_time(ip, RR_ACCESS);
		ip->ctime = iso9660_parse_time(ip, RR_ATTRIBUTES);
	}

	vap->va_mtime.tv_sec = ip->mtime;
	vap->va_mtime.tv_usec = 0;

	vap->va_atime.tv_sec = ip->atime;
	vap->va_atime.tv_usec = 0;

	vap->va_ctime.tv_sec = ip->ctime;
	vap->va_ctime.tv_usec = 0;

	if (ip->rr.rr_flags & RR_PX) {
		vap->va_mode = ip->rr.rr_mode & 07777;
		vap->va_uid = ip->rr.rr_uid;
		vap->va_gid = ip->rr.rr_gid;
		vap->va_nlink = ip->rr.rr_nlink;
	} else {
		vap->va_mode = VREAD|VWRITE|VEXEC;
		vap->va_mode |= (vap->va_mode >> 3) | (vap->va_mode >> 6);
		vap->va_uid = 0;
		vap->va_gid = 0;
		vap->va_nlink = 1;
	}

	if (ip->rr.rr_flags & RR_PN)
		vap->va_rdev = ip->rr.rr_rdev;
	else
		vap->va_rdev = 0;

	vap->va_fsid = ip->i_dev;
	vap->va_fileid = ip->fileid;
	vap->va_size = ip->size;
	vap->va_size_rsv = 0;
	vap->va_flags = 0;
	vap->va_gen = 1;
	vap->va_blocksize = ip->i_mnt->logical_block_size;
	vap->va_bytes = roundup (ip->size, vap->va_blocksize);
	vap->va_bytes_rsv = 0;
	vap->va_type = vp->v_type;
	return (0);
}

/*
 * Set attribute vnode op. Only valid for O_TRUNC opens.
 */
iso9660_setattr(vp, vap, cred, p)
	register struct vnode *vp;
	register struct vattr *vap;
	register struct ucred *cred;
	struct proc *p;
{
	/*
	 * Check for unsetable attributes.
	 */
	if ((vap->va_type != VNON) || (vap->va_nlink != VNOVAL) ||
	    (vap->va_fsid != VNOVAL) || (vap->va_fileid != VNOVAL) ||
	    (vap->va_blocksize != VNOVAL) || (vap->va_rdev != VNOVAL) ||
	    ((int) vap->va_bytes != VNOVAL) || (vap->va_gen != VNOVAL)) {
		return (EINVAL);
	}

	if (vp->v_type != VCHR && vp->v_type != VBLK)
		return (EROFS);

	/*
	 * Go through the fields and update iff not VNOVAL.
	 */
	if (vap->va_uid != (u_short) VNOVAL || vap->va_gid != (u_short) VNOVAL)
		return (EROFS);

	if (vap->va_atime.tv_sec != VNOVAL || vap->va_mtime.tv_sec != VNOVAL)
		return (EROFS);

	if (vap->va_mode != (u_short) VNOVAL)
		return (EROFS);

	if (vap->va_flags != VNOVAL)
		return (EROFS);

	return (0);
}
/*
 * Vnode op for reading.
 */
/* ARGSUSED */
int
iso9660_read(vp, uio, ioflag, cred)
	struct vnode *vp;
	register struct uio *uio;
	int ioflag;
	struct ucred *cred;
{
	register struct iso9660_node *ip = VTOI(vp);
	register struct iso9660_mnt *imp;
	struct buf *bp;
	daddr_t rablock;
	int error = 0;
	int logical_block_size;
	int lblk_to_lsec_divisor;
	int togo;
	int lblknum_in_file;
	int lsecnum_on_disk;
	int lsecoff_on_disk;
	int dev_blknum;
	int off;
	int avail;
	int thistime;

	/*
	 * DEV_BSIZE <= logical_block_size <= logical_sector_size,
	 * all are powers of 2
	 */
	logical_block_size = ip->i_mnt->im_bsize;
	lblk_to_lsec_divisor = ISO9660_LSS / logical_block_size;

	if (uio->uio_resid == 0)
		return (0);
	if (uio->uio_offset < 0)
		return (EINVAL);
	imp = ip->i_mnt;
	do {
		togo = MIN (uio->uio_resid, ip->size - uio->uio_offset);
		if (togo <= 0)
			return (0);

		lblknum_in_file = iso9660_lblkno(imp, uio->uio_offset);

		lsecnum_on_disk = (ip->extent + lblknum_in_file)
			/ lblk_to_lsec_divisor;
		lsecoff_on_disk = (ip->extent + lblknum_in_file)
			% lblk_to_lsec_divisor;

		off = lsecoff_on_disk * logical_block_size
			+ uio->uio_offset % logical_block_size;
		avail = ISO9660_LSS - off;
		thistime = MIN (avail, togo);

		dev_blknum = lsecnum_on_disk
			* (ISO9660_LSS / DEV_BSIZE);

		if (vp->v_lastr + 1 == lsecnum_on_disk &&
		    uio->uio_offset + thistime < ip->size)
			error = breada (imp->im_devvp,
					dev_blknum,
					ISO9660_LSS,
					(lsecnum_on_disk + 1)
					  * (ISO9660_LSS / DEV_BSIZE),
					ISO9660_LSS,
					NOCRED,
					&bp);
		else
			error = bread (imp->im_devvp,
				       dev_blknum,
				       ISO9660_LSS,
				       NOCRED,
				       &bp);
		vp->v_lastr = lsecnum_on_disk;

		if (error || bp->b_resid != 0) {
			brelse(bp);
			return (error ? error : EIO);
		}
		
		error = uiomove(bp->b_un.b_addr + off, thistime, uio);

		if (thistime == avail || uio->uio_offset == ip->size)
			bp->b_flags |= B_AGE;

		brelse(bp);
	} while (error == 0);
	return (error);
}

/* ARGSUSED */
int
iso9660_ioctl(vp, com, data, fflag, cred, p)
	struct vnode *vp;
	int com;
	caddr_t data;
	int fflag;
	struct ucred *cred;
	struct proc *p;
{
	struct iso9660_getsusp *gp;
	struct nameidata *ndp;
	struct nameidata nd;
	extern struct vnodeops iso9660_vnodeops;
	extern struct vnodeops spec_iso9660_vnodeops;
	struct iovec iov;
	struct uio uio;
	int error;
	struct iso9660_node *ip;
	struct iso9660_mnt *imp;
	struct iso9660_dir_info *dirp;
	char *name;
	int namelen;

	switch (com) {
	case ISO9660GETSUSP:
		gp = (struct iso9660_getsusp *) data;

		ndp = &nd;
		ndp->ni_nameiop = LOOKUP | LOCKLEAF | NOFOLLOW;
		ndp->ni_segflg = UIO_USERSPACE;
		ndp->ni_dirp = gp->name;
		if (error = namei(ndp, p))
			return (error);

		vp = ndp->ni_vp;

		if (vp->v_op != &iso9660_vnodeops &&
		    vp->v_op != &spec_iso9660_vnodeops) {
			vput(vp);
			return (EINVAL);
		}

		ip = VTOI(vp);
		imp = ip->i_mnt;

		iov.iov_base = gp->buf;
		iov.iov_len = gp->buflen;
		uio.uio_iov = &iov;
		uio.uio_iovcnt = 1;
		uio.uio_offset = 0;
		uio.uio_rw = UIO_READ;
		uio.uio_segflg = UIO_USERSPACE;
		uio.uio_procp = p;
		uio.uio_resid = gp->buflen;

		if (error = iso9660_diropen(imp, ip->fileid, ISO9660_MAX_FILEID,
		    0, 0, &uio, &dirp)) {
			vput(ndp->ni_vp);
			return (error);
		}
					    
		error = iso9660_dirget(dirp);

		iso9660_dirclose(dirp);
		vput(ndp->ni_vp);

		gp->buflen = gp->buflen - uio.uio_resid;
		return (error);

	default:
		if (vp->v_type == VCHR || vp->v_type == VBLK)
			return (spec_ioctl(vp, com, data, fflag, cred, p));

		return (ENOTTY);
	}
}

/* ARGSUSED */
int
iso9660_select(vp, which, fflags, cred, p)
	struct vnode *vp;
	int which, fflags;
	struct ucred *cred;
	struct proc *p;
{

	/*
	 * We should really check to see if I/O is possible.
	 */
	return (1);
}

/*
 * Mmap a file
 *
 * NB Currently unsupported.
 */
/* ARGSUSED */
int
iso9660_mmap(vp, fflags, cred, p)
	struct vnode *vp;
	int fflags;
	struct ucred *cred;
	struct proc *p;
{

	return (EINVAL);
}

/*
 * Seek on a file
 *
 * Nothing to do, so just return.
 */
/* ARGSUSED */
int
iso9660_seek(vp, oldoff, newoff, cred)
	struct vnode *vp;
	off_t oldoff, newoff;
	struct ucred *cred;
{

	return (0);
}

/*
 * Vnode op for readdir
 */
int
iso9660_readdir(vp, uio, cred, eofflagp, cookies, ncookies)
	struct vnode *vp;
	register struct uio *uio;
	struct ucred *cred;
	int *eofflagp;
	u_int *cookies;
	int ncookies;
{
	int error = 0;
	struct iso9660_dir_info *dirp;
	struct dirent dirent;
	struct iso9660_node *ip;
	struct iso9660_mnt *imp;
	iso9660_fileid_t dir_first_fileid;
	iso9660_fileid_t dir_last_fileid;
	iso9660_fileid_t fileid;
	u_int *end_cookies;

	if (cookies)
		end_cookies = cookies + ncookies;

	ip = VTOI(vp);
	imp = ip->i_mnt;

	dir_first_fileid = iso9660_lblktosize(imp,
	    (iso9660_fileid_t) ip->extent);
	dir_last_fileid = dir_first_fileid + ip->size;

	fileid = uio->uio_offset;
	if (fileid == 0)
		fileid = dir_first_fileid;

	if (error = iso9660_diropen(imp, fileid, dir_last_fileid,
	    1, 0, NULL, &dirp))
		return (error);

	while (uio->uio_resid) {
		if (error = iso9660_dirget(dirp))
			break;

		dirent.d_fileno = dirp->translated_fileid;
		dirent.d_namlen = dirp->namelen;
		bcopy(dirp->name, dirent.d_name, dirent.d_namlen);
		dirent.d_name[dirent.d_namlen] = 0;
		dirent.d_reclen = DIRSIZ(&dirent);

		if (uio->uio_resid < dirent.d_reclen)
			break;
		
		if (error = uiomove(&dirent, dirent.d_reclen, uio))
			break;

		uio->uio_offset = dirp->next_fileid;
		if (cookies) {
			*cookies++ = uio->uio_offset;
			if (cookies == end_cookies)
				break;
		}
	}

	iso9660_dirclose(dirp);

	if (error == ENOENT) {
		error = 0;
		*eofflagp = 1;
	} else
		*eofflagp = 0;

	return (error);
}
	
/*
 * Return target name of a symbolic link
 */
int
iso9660_readlink(vp, uiop, cred)
        struct vnode *vp;
        struct uio *uiop;
        struct ucred *cred;
{
	struct iso9660_node *ip;
	int error = 0;
	struct iso9660_mnt *imp;
	struct iso9660_dir_info *dirp;

	ip = VTOI(vp);
	imp = ip->i_mnt;

	if (error = iso9660_diropen(imp, ip->fileid, ISO9660_MAX_FILEID,
	    0, 1, NULL, &dirp))
		return (error);

	if (error = iso9660_dirget(dirp))
		goto ret;

	if (dirp->link == NULL || *dirp->link == 0) {
		error = EINVAL;
		goto ret;
	}
		
	error = uiomove(dirp->link, dirp->link_used, uiop);

ret:
	iso9660_dirclose(dirp);
	return (error);
}


/*
 * Ufs abort op, called after namei() when a CREATE/DELETE isn't actually
 * done. If a buffer has been saved in anticipation of a CREATE, delete it.
 */
/* ARGSUSED */
int
iso9660_abortop(ndp)
	struct nameidata *ndp;
{

	if ((ndp->ni_nameiop & (HASBUF | SAVESTART)) == HASBUF)
		FREE(ndp->ni_pnbuf, M_NAMEI);
	return (0);
}

/*
 * Lock an inode.
 */
int
iso9660_lock(vp)
	struct vnode *vp;
{
	register struct iso9660_node *ip = VTOI(vp);

	ISO9660_ILOCK(ip);
	return (0);
}

/*
 * Unlock an inode.
 */
int
iso9660_unlock(vp)
	struct vnode *vp;
{
	register struct iso9660_node *ip = VTOI(vp);

	if (!(ip->i_flag & ILOCKED))
		panic("iso9660_unlock NOT LOCKED");
	ISO9660_IUNLOCK(ip);
	return (0);
}

/*
 * Check for a locked inode.
 */
int
iso9660_islocked(vp)
	struct vnode *vp;
{

	if (VTOI(vp)->i_flag & ILOCKED)
		return (1);
	return (0);
}

/*
 * Calculate the logical to physical mapping if not done already,
 * then call the device strategy routine.
 */
int
iso9660_strategy(bp)
	register struct buf *bp;
{
	printf ("obsolete call to iso9660_strategy\n");
	return (EIO);
}

/*
 * Print out the contents of an inode.
 */
iso9660_print(vp)
	struct vnode *vp;
{
	struct iso9660_node *ip;

	ip = VTOI(vp);
	
	printf("tag VT_ISO9660, fileid %d, on dev %d, %d", ip->fileid,
		major(ip->i_dev), minor(ip->i_dev));
	printf("%s\n", (ip->i_flag & ILOCKED) ? " (LOCKED)" : "");
	if (ip->i_spare0 == 0)
		return;
	printf("\towner pid %d", ip->i_spare0);
	if (ip->i_spare1)
		printf("waiting pid %d", ip->i_spare1);
	printf("\n");
}

extern int enodev();

/*
 * Global vfs data structures for iso9660
 */
struct vnodeops iso9660_vnodeops = {
	iso9660_lookup,		/* lookup */
	(void *)enodev,		/* create */
	(void *)enodev,		/* mknod */
	iso9660_open,		/* open */
	iso9660_close,		/* close */
	iso9660_access,		/* access */
	iso9660_getattr,	/* getattr */
	iso9660_setattr,	/* setattr */
	iso9660_read,		/* read */
	(void *)enodev,		/* write */
	iso9660_ioctl,		/* ioctl */
	iso9660_select,		/* select */
	iso9660_mmap,		/* mmap */
	(void *)nullop,		/* fsync */
	iso9660_seek,		/* seek */
	(void *)enodev,		/* remove */
	(void *)enodev,		/* link */
	(void *)enodev,		/* rename */
	(void *)enodev,		/* mkdir */
	(void *)enodev,		/* rmdir */
	(void *)enodev,		/* symlink */
	iso9660_readdir,	/* readdir */
	(void *)iso9660_readlink, /* readlink */
	iso9660_abortop,	/* abortop */
	iso9660_inactive,	/* inactive */
	iso9660_reclaim,	/* reclaim */
	iso9660_lock,		/* lock */
	iso9660_unlock,		/* unlock */
	(void *)enodev,		/* bmap */
	iso9660_strategy,	/* strategy */
	iso9660_print,		/* print */
	iso9660_islocked,	/* islocked */
	(void *)enodev,		/* advlock */
};

struct vnodeops spec_iso9660_vnodeops = {
	spec_lookup,		/* lookup */
	spec_create,		/* create */
	spec_mknod,		/* mknod */
	spec_open,		/* open */
	spec_close,		/* close */
	iso9660_access,		/* access */
	iso9660_getattr,	/* getattr */
	iso9660_setattr,	/* setattr */
	spec_read,		/* read */
	spec_write,		/* write */
	spec_ioctl,		/* ioctl */
	spec_select,		/* select */
	spec_mmap,		/* mmap */
	spec_fsync,		/* fsync */
	spec_seek,		/* seek */
	spec_remove,		/* remove */
	spec_link,		/* link */
	spec_rename,		/* rename */
	spec_mkdir,		/* mkdir */
	spec_rmdir,		/* rmdir */
	spec_symlink,		/* symlink */
	spec_readdir,		/* readdir */
	spec_readlink,		/* readlink */
	spec_abortop,		/* abortop */
	iso9660_inactive,	/* inactive */
	iso9660_reclaim,	/* reclaim */
	iso9660_lock,		/* lock */
	iso9660_unlock,		/* unlock */
	spec_bmap,		/* bmap */
	spec_strategy,		/* strategy */
	iso9660_print,		/* print */
	iso9660_islocked,	/* islocked */
	spec_advlock,		/* advlock */
};
