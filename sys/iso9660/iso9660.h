/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI $Id: iso9660.h,v 1.2 1993/12/17 04:06:33 karels Exp $
 */

/*
 * CD-ROM file system based on ISO-9660 : 1988 (E) with Rock Ridge Extensions
 *
 * When an on-disk data structure contains a number, it is encoded in
 * one of 8 ways, as described in sections 7.1.1 through 7.3.3 of
 * the ISO document.  In this file, a comment such as "711" which
 * follows a structure element tells how that particular number is
 * encoded.  See the file iso9660_util.c for more details.
 */

struct iso9660_volume_descriptor {
	u_char	type[1]; /* 711 */
	char	id[5];
	u_char	version[1];
	char	data[2041];
};

/* volume descriptor types */
#define	ISO9660_VD_PRIMARY 1
#define	ISO9660_VD_END 255

#define	ISO9660_STANDARD_ID "CD001"

struct iso9660_primary_descriptor {
	u_char	type			[1]; /* 711 */
	char	id			[5];
	u_char	version			[1]; /* 711 */
	char	unused1			[1];
	char	system_id		[32]; /* achars */
	char	volume_id		[32]; /* dchars */
	char	unused2			[8];
	u_char	volume_space_size	[8]; /* 733 */
	char	unused3			[32];
	u_char	volume_set_size		[4]; /* 723 */
	u_char	volume_sequence_number	[4]; /* 723 */
	u_char	logical_block_size	[4]; /* 723 */
	u_char	path_table_size		[8]; /* 733 */
	u_char	type_l_path_table	[4]; /* 731 */
	u_char	opt_type_l_path_table	[4]; /* 731 */
	u_char	type_m_path_table	[4]; /* 732 */
	u_char	opt_type_m_path_table	[4]; /* 732 */
	u_char	root_directory_record	[34]; /* 9.1 */
	char	volume_set_id		[128]; /* dchars */
	char	publisher_id		[128]; /* achars */
	char	preparer_id		[128]; /* achars */
	char	application_id		[128]; /* achars */
	char	copyright_file_id	[37]; /* 7.5 dchars */
	char	abstract_file_id	[37]; /* 7.5 dchars */
	char	bibliographic_file_id	[37]; /* 7.5 dchars */
	u_char	creation_date		[17]; /* 8.4.26.1 */
	u_char	modification_date	[17]; /* 8.4.26.1 */
	u_char	expiration_date		[17]; /* 8.4.26.1 */
	u_char	effective_date		[17]; /* 8.4.26.1 */
	u_char	file_structure_version	[1]; /* 711 */
	char	unused4			[1];
	char	application_data	[512];
	char	unused5			[653];
};

#define	ISO9660_DIR_NAME	33 /* offset to start of name field */

#define	ISO9660_DIR_FLAG	2 /* bit in flags field */

struct iso9660_directory_record {
	u_char	length			[1]; /* 711 */
	u_char	ext_attr_length		[1]; /* 711 */
	u_char	extent			[8]; /* 733 */
	u_char	size			[8]; /* 733 */
	u_char	date			[7]; /* 7 by 711 */
	u_char	flags			[1];
	u_char	file_unit_size		[1]; /* 711 */
	u_char	interleave		[1]; /* 711 */
	u_char	volume_sequence_number	[4]; /* 723 */
	u_char	name_len		[1]; /* 711 */
	/* name follows */
};
#define	iso9660_dir_name(dp)	((char *)dp + ISO9660_DIR_NAME)

/* must fit in uio.uio_offset and vattr.va_fileid */
typedef u_int iso9660_fileid_t;
#define	ISO9660_MAX_FILEID	UINT_MAX

struct iso9660_mnt {
	int	logical_block_size;
	int	volume_space_size;
	struct	vnode *im_devvp;
	char	im_fsmnt[50];
	
	struct	mount *im_mountp;
	dev_t	im_dev;

	int	im_bshift;
	int	im_bmask;
	int	im_bsize;

	u_char	root[34];
	iso9660_fileid_t root_fileid;
	int	root_size;

	int	im_raw; /* ignore rock ridge, don't translate iso9660 names */

	int	im_checked_for_rock_ridge;
	int	im_have_rock_ridge;
	int	im_rock_ridge_skip;
};

#define	VFSTOISO9660(mp)	((struct iso9660_mnt *)((mp)->mnt_data))

#define ISO9660_LSS 2048 /* logical sector size */

#define	iso9660_blkoff(imp, loc) ((loc) & ~(imp)->im_bmask)
#define	iso9660_lblkno(imp, loc) ((loc) >> (imp)->im_bshift)
#define	iso9660_blksize(imp, ip, lbn) ((imp)->im_bsize)
#define	iso9660_lblktosize(imp, blk) ((blk) << (imp)->im_bshift)

/* time stamp info - we only care about modify, access and creation */
#define	RR_TIMESTAMPS		7
#define	RR_LONG_FORM		0x80
#define	RR_LONG_TIMESTAMP_LEN	17
#define	RR_SHORT_TIMESTAMP_LEN	7

#define	RR_CREATION		0
#define	RR_MODIFY		1
#define	RR_ACCESS		2
#define	RR_ATTRIBUTES		3
#define	RR_BACKUP		4
#define	RR_EXPIRATION		5
#define	RR_EFFECTIVE		6

#define	TS_INDEX(val) ((val) - RR_MODIFY)

struct iso9660_rr_info {
	int	rr_flags;
	int	rr_mode;
	int	rr_nlink;
	int	rr_uid;
	int	rr_gid;
	int	rr_dev_high;
	int	rr_dev_low;

	int	rr_rdev;

	u_char	rr_have_rr;
	u_char	rr_rr;

	u_char	rr_time[3][RR_LONG_TIMESTAMP_LEN];
	u_char	rr_time_format[3];

};

struct iso9660_dir_info {
	struct	iso9660_mnt *imp;
	iso9660_fileid_t fileid;
	iso9660_fileid_t translated_fileid;
	iso9660_fileid_t next_fileid;
	iso9660_fileid_t max_fileid;
	int	extent;

	int	error;

	int	namelen;
	char	*name;
	struct	buf *bp;

	int	size;
	u_char	iso9660_time[7];
	int	iso9660_flags;

	struct	iso9660_rr_info rr;

	char	*link;
	int	link_used;
	int	link_separator;

	struct	uio *uio;	/* used to implement ISO9660GETSUSP ioctl */
};

/*
 * Each of these is a piece of Rock Ridge information that may be present
 * for a particular file.
 */
#define	RR_PX	0x01
#define	RR_PN	0x02
#define	RR_SL	0x04
#define	RR_NM	0x08
#define	RR_CL	0x10
#define	RR_PL	0x20
#define	RR_RE	0x40
#define	RR_TF	0x80


#define	SUSP_CODE(a,b) ((((a) & 0xff) << 8) | ((b) & 0xff))

#define	SUSP_MIN_SIZE	4

struct susp {
	u_char	code[2];
	u_char	length[1];
	u_char	version[1];
};

#define	SUSP_SP_LEN	7
struct susp_sp {
	u_char	code[2];
	u_char	length[1];
	u_char	version[1];
	u_char	check[2];
	u_char	len_skp[1];
};

#define	SUSP_PX_LEN	36
struct susp_px {
	u_char	code[2];
	u_char	length[1];
	u_char	version[1];
	u_char	mode[8];
	u_char	nlink[8];
	u_char	uid[8];
	u_char	gid[8];
};

#define	SUSP_PN_LEN	20
struct susp_pn {
	u_char	code[2];
	u_char	length[1];
	u_char	version[1];
	u_char	dev_high[8];
	u_char	dev_low[8];
};

#define	SUSP_RR_LEN	5
struct susp_rr {
	u_char	code[2];
	u_char	length[1];
	u_char	version[1];
	u_char	flags[1];
};

#define	SUSP_TF_LEN	5
struct susp_tf {
	u_char	code[2];
	u_char	length[1];
	u_char	version[1];
	u_char	flags[1];
};

#define	SUSP_NM_LEN	5
struct susp_nm {
	u_char	code[2];
	u_char	length[1];
	u_char	version[1];
	u_char	flags[1];
};
#define	SUSP_NM_NAME(sp) ((char *)sp + SUSP_NM_LEN)

#define	SUSP_CE_LEN	28
struct susp_ce {
	u_char	code[2];
	u_char	length[1];
	u_char	version[1];
	u_char	block[8];
	u_char	offset[8];
	u_char	ce_length[8];
};

#define	SUSP_SL_LEN	5
struct susp_sl {
	u_char	code[2];
	u_char	length[1];
	u_char	version[1];
	u_char	flags[1];
};

#define	SUSP_SLC_LEN	2
struct susp_slc {
	u_char	flags[1];
	u_char	length[1];
};
#define	SLC_NAME(slc) ((char *)slc + 2)

#define	SUSP_ER_LEN	8
struct susp_er {
	u_char	code[2];
	u_char	length[1];
	u_char	version[1];
	u_char	len_id[1];
	u_char	len_des[1];
	u_char	len_src[1];
	u_char	ext_ver[1];
};
#define	SUSP_ER_ID(sp)	((char *)sp + SUSP_ER_LEN)
#define	SUSP_ER_DES(sp)	(SUSP_ER_ID(sp) + iso9660num_711(sp->len_id))
#define	SUSP_ER_SRC(sp) (SUSP_ER_DES(sp) + iso9660num_711(sp->len_des))

#define	SUSP_CL_LEN 12
struct susp_cl {
	u_char	code[2];
	u_char	length[1];
	u_char	version[1];
	u_char	loc[8];
};

#define	SUSP_PL_LEN	12
struct susp_pl {
	u_char	code[2];
	u_char	length[1];
	u_char	version[1];
	u_char	loc[8];
};

/* name flags */
#define	RR_NAME_CONTINUE	0x01
#define	RR_NAME_CURRENT		0x02
#define	RR_NAME_PARENT		0x04
#define	RR_NAME_ROOT		0x08
#define	RR_NAME_VOLROOT		0x10
#define	RR_NAME_HOST		0x20

struct iso9660_getsusp {
	char	*name;
	char	*buf;
	int	buflen;
};
#define	ISO9660GETSUSP _IOWR('i', 213, struct iso9660_getsusp)

#ifdef KERNEL
int iso9660_mount __P((struct mount *mp, char *path, caddr_t data,
	struct nameidata *ndp, struct proc *p));
int iso9660_start __P((struct mount *mp, int flags, struct proc *p));
int iso9660_unmount __P((struct mount *mp, int mntflags, struct proc *p));
int iso9660_root __P((struct mount *mp, struct vnode **vpp));
int iso9660_statfs __P((struct mount *mp, struct statfs *sbp, struct proc *p));
int iso9660_sync __P((struct mount *mp, int waitfor));
int iso9660_fhtovp __P((struct mount *mp,struct fid *fhp,struct vnode **vpp));
int iso9660_vptofh __P((struct vnode *vp, struct fid *fhp));
int iso9660_init __P(());

int iso9660num_711 __P((u_char *p));
int iso9660num_712 __P((u_char *p));
int iso9660num_721 __P((u_char *p));
int iso9660num_722 __P((u_char *p));
int iso9660num_723 __P((u_char *p));
int iso9660num_731 __P((u_char *p));
int iso9660num_732 __P((u_char *p));
int iso9660num_733 __P((u_char *p));
	
int iso9660_dir_parse __P((struct iso9660_dir_info *dirp,
	   struct iso9660_directory_record *ep, int reclen));

int iso9660_diropen __P((struct iso9660_mnt *imp, iso9660_fileid_t fileid,
	 iso9660_fileid_t max_fileid, int want_name, int want_link,
	 struct uio *uio, struct iso9660_dir_info **dirpp));
int iso9660_dirget __P((struct iso9660_dir_info *dirp));
void iso9660_dirclose __P((struct iso9660_dir_info *));
#endif

