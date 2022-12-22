/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI $Id: iso9660_node.h,v 1.3 1992/12/01 18:11:17 karels Exp $
 */

struct iso9660_node {
	struct	iso9660_node *i_chain[2]; /* hash chain, MUST be first */
	struct	vnode *i_vnode;	/* vnode associated with this inode */
	struct	vnode *i_devvp;	/* vnode for block I/O */
	u_long	i_flag;		/* see below */
	dev_t	i_dev;		/* device where inode resides */
	struct	iso9660_mnt *i_mnt; /* filesystem associated with this inode */
	long	i_spare0;
	long	i_spare1;

	int 	deleted;	/* used to abandon aliased special node */

	iso9660_fileid_t fileid;
	int	extent;
	int	size;

	u_char	iso9660_time[7];

	struct	iso9660_rr_info rr;

	u_char	time_parsed;
	time_t	mtime;
	time_t	atime;
	time_t	ctime;
	
	iso9660_fileid_t	dir_last_fileid; /* last successful lookup */
};

#define	i_forw		i_chain[0]
#define	i_back		i_chain[1]

/* flags */
#define	ILOCKED		0x0001		/* inode is locked */
#define	IWANT		0x0002		/* some process waiting on lock */

#define VTOI(vp) ((struct iso9660_node *)(vp)->v_data)
#define ITOV(ip) ((ip)->i_vnode)

#define ISO9660_ILOCK(ip)	iso9660_ilock(ip)
#define ISO9660_IUNLOCK(ip)	iso9660_iunlock(ip)

/*
 * Prototypes for ISO9660 vnode operations
 */
int iso9660_lookup __P((struct vnode *vp, struct nameidata *ndp, struct proc *p));
int iso9660_open __P((struct vnode *vp, int mode, struct ucred *cred,
	struct proc *p));
int iso9660_close __P((struct vnode *vp, int fflag, struct ucred *cred,
	struct proc *p));
int iso9660_access __P((struct vnode *vp, int mode, struct ucred *cred,
	struct proc *p));
int iso9660_getattr __P((struct vnode *vp, struct vattr *vap, struct ucred *cred,
	struct proc *p));
int iso9660_read __P((struct vnode *vp, struct uio *uio, int ioflag,
	struct ucred *cred));
int iso9660_ioctl __P((struct vnode *vp, int command, caddr_t data, int fflag,
	struct ucred *cred, struct proc *p));
int iso9660_select __P((struct vnode *vp, int which, int fflags, struct ucred *cred,
	struct proc *p));
int iso9660_mmap __P((struct vnode *vp, int fflags, struct ucred *cred,
	struct proc *p));
int iso9660_seek __P((struct vnode *vp, off_t oldoff, off_t newoff,
	struct ucred *cred));
int iso9660_readdir __P((struct vnode *vp, struct uio *uio, struct ucred *cred,
	int *eofflagp, u_int *cookies, int ncookies));
int iso9660_abortop __P((struct nameidata *ndp));
int iso9660_inactive __P((struct vnode *vp, struct proc *p));
int iso9660_reclaim __P((struct vnode *vp));
int iso9660_lock __P((struct vnode *vp));
int iso9660_unlock __P((struct vnode *vp));
int iso9660_strategy __P((struct buf *bp));
int iso9660_print __P((struct vnode *vp));
int iso9660_islocked __P((struct vnode *vp));

void iso9660_iunlock __P((struct iso9660_node *ip));
void iso9660_ilock __P((struct iso9660_node *ip));
void iso9660_iput __P((struct iso9660_node *ip));
int iso9660_iget __P((struct iso9660_node *xp, iso9660_fileid_t fileid,
	struct iso9660_node **ipp, struct iso9660_dir_info *dirp));
