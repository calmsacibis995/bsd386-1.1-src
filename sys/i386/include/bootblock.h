/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: bootblock.h,v 1.3 1992/10/25 21:30:48 karels Exp $
 */

/* 
 * Master boot structure - as in block 0 of DOS hard disk
 * includes: some geometry info, primary boot code, partition table
 *
 * source: Adrian Alting-Mees, The Hard-Drive Encyclopedia, Annabooks
 *
 * NB! may be problems with alignment in compiler
 */

#ifndef LOCORE
struct mbpart {
	u_char	active;
		/* start of partition */
	u_char	shead;		/* start: head */
	u_char	ssec;		/* start: sec (and 2 high bits of cylinder) */
	u_char	scyl;		/* start: 8 low bits of cylinder */
	
	u_char	system;		/* system type indicator - see below */
		/* end of partition */
	u_char	ehead;		/* end: head */
	u_char	esec;		/* end: sec (and 2 high bits of cylinder) */
	u_char	ecyl;		/* end: 8 low bits of cylinder */

	long	start;		/* start sector of partition */
	long	size;		/* size of partitions (in secs) */
};
#endif
	
#define MB_MAXCYL	1024	/* max cylinder number in mbpart - 10 bits*/

#define MB_SECMASK	0x3f	/* to get sector from mbpart.[se]sec */
#define MB_CYLMASK	0xc0	/* high cylinder bits from mbpart.[se]sec */
#define	MB_CYLSHIFT	2	/* left shift for 2 high cylinder bits */

/* extract starting and ending sec/trk/cyl (sectors with origin 0) */
#define mbpssec(mbpp)	(((mbpp)->ssec & MB_SECMASK) - 1)
#define mbpesec(mbpp)	(((mbpp)->esec & MB_SECMASK) - 1)
#define mbpstrk(mbpp)	((mbpp)->shead)
#define mbpetrk(mbpp)	((mbpp)->ehead)
#define mbpscyl(mbpp)	((mbpp)->scyl + \
		(((int)((mbpp)->ssec) & MB_CYLMASK) << MB_CYLSHIFT))
#define mbpecyl(mbpp)	((mbpp)->ecyl + \
		(((int)((mbpp)->esec) & MB_CYLMASK) << MB_CYLSHIFT))

/* Active boot partition indicator values */
#define	MBA_NOTACTIVE	0
#define	MBA_ACTIVE	0x80

/*
 * some values for system indicator (don't need all that long list)
 */
#define	MBS_UNKNOWN	0
#define	MBS_DOS12	1	/* DOS with 12 bit FAT (not DOS 1.2) */
#define	MBS_DOS16	4	/* DOS with 16 bit FAT */
#define	MBS_DOS		5	/* DOS (3.30 or higher) */
#define	MBS_DOS4	6	/* DOS (4.0) */
#define MBS_DRDOS	0xdb	/* Concurrent DOS by DR */

#define	MBS_BSDI	0x9F	/* BSD system partition */

#define	MBS_BOOTANY	0x80	/* marked as bootable, not active (bootany) */

#define MBS_ISDOS(si)	((si) == MBS_DOS12 || (si) == MBS_DOS16 || \
			 (si) == MBS_DOS || (si) == MBS_DOS4 || \
			 (si) == MBS_DRDOS)
			 
#define	MB_MAXPARTS	4	/* # partitions in boot block table */
#define	MB_PARTOFF	446	/* offset of fdisk table in sector 0 */

#ifndef LOCORE
struct masterboot {
#ifdef MB_HAS_INFO
	/* should be this way, but too often not */
	char	jmp[3];		/* jmp to boot loading routine */
	char	oem[8];		/* OEM name and version */
	u_char	secsize[2];	/* short: sector size */
	u_char	clusecs;	/* sectors per cluster */
	u_char	secreserved[2];	/* short: # of reserved sectors */
	u_char	nfatcopies;	/* # of copies of fat */
	u_char	nrootslots[2];	/* max number of DOS root dir entires */
	u_char	volsecs[2];	/* total # of sectors in logical volume */
	u_char	media;		/* DOS media byte (0xf8 for hard disk) */
	short	fatsecs;	/* # of secs per FAT */
	short	nsectors;	/* # sectors per track */
	short	ntracks;	/* # of heads (tracks per cylinder) */
	short	nhidsecs;	/* # of hidden sectors */
	
	char	code[416];	/* boot code */
#else
	char	code[MB_PARTOFF];	/* boot code */
#endif
		/*
		 * master boot partitions table
		 * should be:
		 *
		 * struct mbpart mbparts[MB_MAXPARTS];
		 *
		 * but for compiler alignment reasons:
		 */
	char	_mbparts[sizeof(struct mbpart) * MB_MAXPARTS];

	u_short	signature;	/* should be MB_SIGNATURE(0xaa55) */
};
#endif

#define	mbparts(mbp)	((struct mbpart *)((mbp)->_mbparts))

#ifndef LOCORE
#define MB_SIGNATURE	((u_short)0xAA55)
#else
#define MB_SIGNATURE	0xaa55
#endif

#if defined(KERNEL) && !defined(LOCORE)
daddr_t	bsdlabelsector __P((dev_t, int (*strat)(), struct disklabel *));
#endif
