/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: coff_compat.h,v 1.1 1993/11/12 01:57:38 karels Exp $
 */

/*
 * Bare bones definitions necessary to support
 * foreign COFF binary files under BSD.
 */

/* iBCS2 p4-3 */
#define	COFF_MAGIC		0514

struct coff_filehdr {
	u_short	f_magic;
	u_short	f_nscns;
	long	_f_pad[3];
	u_short	f_opthdr;
	u_short	f_flags;
};

/* iBCS2 p4-4 */
#define	COFF_F_EXEC		2

/* iBCS2 p4-5 */
struct coff_aouthdr {
	short	magic;
	short	_pad1;
	long	_pad2[3];
	long	entry;
	long	_pad3[2];
};

/* iBCS2 p4-6 */
#define	COFF_OMAGIC		0407
#define	COFF_NMAGIC		0410
#define	COFF_ZMAGIC		0413
#define	COFF_LMAGIC		0443

/* iBCS2 p4-7 */
struct coff_scnhdr {
	char	_s_pad1[8];
	long	_s_pad2;
	long	s_vaddr;
	long	s_size;
	long	s_scnptr;
	long	_s_pad3[2];
	u_short	_s_pad4[2];
	long	s_flags;
};

/* iBCS2 p4-9 */
#define	COFF_STYP_DSECT		0x0001
#define	COFF_STYP_NOLOAD	0x0002
#define COFF_STYP_GROUP		0x0004
#define	COFF_STYP_PAD		0x0008
#define	COFF_STYP_TEXT		0x0020
#define	COFF_STYP_DATA		0x0040
#define	COFF_STYP_BSS		0x0080
#define	COFF_STYP_INFO		0x0200
#define	COFF_STYP_OVER		0x0400
#define	COFF_STYP_LIB		0x0800

/*
 * There seems to be a division of labor between STYP bits which
 * mark boolean features of the section, and STYP values which
 * designate orthogonal section types.
 * This mask is supposed to strip out the feature bits and
 * leave behind the section types.
 * The documentation on iBCS2 p4-9/4-10 is not very clear about this...
 */
#define	COFF_STYP_FEATURES \
	(COFF_STYP_DSECT|COFF_STYP_NOLOAD|COFF_STYP_GROUP|COFF_STYP_PAD)

/*
 * Shared library section contents, from iBCS2 p4-10.
 */
struct shlib_entry {
	long	entsz;
	long	pathndx;
};

/*
 * The default location for the COFF emulator.
 * We need to make this more flexible...
 */
#define	COFF_EMULATOR		"/sco/emulator"

/*
 * A hack: set the default emulator stack size.
 */
#define	COFF_STACKSIZE		(2*NBPG)
