/*-
 * Copyright (c) 1992, 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: bootparam.h,v 1.3 1994/01/05 18:12:01 karels Exp $
 */

/*
 * Structure for ISA/EISA disk parameters in BIOS.
 */
#ifndef LOCORE
struct biosgeom {
	u_short	ncylinders;		/* number of cylinders */
	u_char	ntracks;		/* number of heads */
	u_char	res0[2];
	u_char	wpcom[2];		/* start of write precompensation */
	u_char	res1;
	u_char	ctl;			/* "control" byte */
	u_char	res2[3];
	u_char	lzone[2];		/* landing zone */
	u_char	nsectors;		/* number of sectors/track */
	u_char	res3;
};
#else /* LOCORE */
#define	SZ_BIOSGEOM	16
#endif /* LOCORE */

#ifndef LOCORE
/*
 * Various information from the bios to pass along for later.
 */
struct biosinfo {
	int	basemem;		/* base memory */
	int	extmem;			/* "extended" memory */
	u_char	kbflags;		/* keyboard status, including NUMLOCK */
	int	spare[3];		/* expansion space */
};

/*
 * Much-shrunken version of disklabel, to be passed from bootxx
 * on to kernel so that we don't have to try bios/default geometry
 * (which screws up on certain controllers).
 * This is implemented, but not normally compiled in,
 * as it is only a partial solution to the problem.
 */
struct bsdgeom {
	int	unit;			/* unit number this is for */
	u_long	ncylinders;		/* number of cylinders */
	u_long	ntracks;		/* number of heads */
	u_long	nsectors;		/* number of sectors/track */
	daddr_t	bsd_startsec;		/* sector number of BSD label */
};
#endif /* LOCORE */

/*
 * The initial interrupt table contains "far" pointers to disk parameters
 * for drives 0 and 1, if present and known to the BIOS.
 */
#define	BIOSGEOM0	(0x41*4)	/* INT 41 */
#define	BIOSGEOM1	(0x46*4)	/* INT 46 */

/*
 * Location of the BIOS keyboard status byte in BIOS low memory.
 * (This should probably be elsewhere.)
 */
#define	BIOSKBFLAGP	((u_char *) 0x417)

#define	BIOS_KB_NUMLOCK	0x20		/* bit for numlock active in kbflags */

/*
 * Machine-dependent boot parameters
 */
#define	B_BIOSGEOM	B_MACHDEP(1)	/* biosgeom[2]: CMOS/BIOS disk params */
#define	B_BIOSINFO	B_MACHDEP(2)	/* biosinfo: sundry info */
#define	B_BSDGEOM	B_MACHDEP(3)	/* bsdgeom: distilled disklabel */

#define BOOT_MAXPARAMS	2048
