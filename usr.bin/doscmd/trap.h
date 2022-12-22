/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	Krystal $Id: trap.h,v 1.2 1994/01/15 21:19:36 polk Exp $ */
#define	CLI	0xfa
#define	STI	0xfb
#define	PUSHF	0x9c
#define	POPF	0x9d
#define	INTn	0xcd
#define	TRACETRAP	0xcc
#define	IRET	0xcf
#define	LOCK	0xf0
#define	REBOOT	0xff

#define	INd	0xe4
#define	INdX	0xe5
#define	OUTd	0xe6
#define	OUTdX	0xe7

#define	IN	0xec
#define	INX	0xed
#define	OUT	0xee
#define	OUTX	0xef

#define	INSB	0x6c
#define	INSW	0x6d
#define	OUTSB	0x6e
#define	OUTSW	0x6f

#define	IOFS	0x64
#define	IOGS	0x65

#define	TWOBYTE	0x0f
#define	 LAR	0x02

#define OKFLAGS (PSL_ALLCC | PSL_T | PSL_D | PSL_V)
#define PSL_86ONES (PSL_IOPL | PSL_NT)

#define	AC_P	0x8000			/* Present */
#define	AC_P0	0x0000			/* Priv Level 0 */
#define	AC_P1	0x2000			/* Priv Level 1 */
#define	AC_P2	0x4000			/* Priv Level 2 */
#define	AC_P3	0x6000			/* Priv Level 3 */
#define AC_S	0x1000			/* Memory Segment */
#define	AC_RO	0x0000			/* Read Only */
#define	AC_RW	0x0200			/* Read Write */
#define	AC_RWE	0x0600			/* Read Write Expand Down */
#define	AC_EX	0x0800			/* Execute Only */
#define	AC_EXR	0x0a00			/* Execute Readable */
#define	AC_EXC	0x0c00			/* Execute Only Conforming */
#define	AC_EXRC	0x0e00			/* Execute Readable Conforming */
#define	AC_A	0x0100			/* Accessed */

#define T_BPTFLT	3		/* breakpoint instruction */
