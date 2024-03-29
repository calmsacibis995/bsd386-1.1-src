/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: nec765.h,v 1.4 1993/01/12 17:08:22 karels Exp $
 */

/*-
 * Copyright (c) 1991 The Regents of the University of California.
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
 *	@(#)nec765.h	7.1 (Berkeley) 5/9/91
 */

/*
 * Nec 765 floppy disc controller definitions
 */

/* Main status register */
#define NE7_DAB	0x01	/* Diskette drive A is seeking, thus busy */
#define NE7_DBB	0x02	/* Diskette drive B is seeking, thus busy */
#define NE7_CB	0x10	/* Diskette Controller Busy */
#define NE7_NDM	0x20	/* Diskette Controller in Non Dma Mode */
#define NE7_DIO	0x40	/* Diskette Controller Data register I/O */
#define NE7_RQM	0x80	/* Diskette Controller ReQuest for Master */

/* Status register ST0 */
#define NE7_ST0BITS	"\020\010invalid\007abnormal\006seek complete\005drive check\004drive_rdy\003top head\002unit_sel1\001unit_sel0"

/* Status register ST1 */
#define NE7_ST1BITS	"\020\010end_of_cyl\006bad_crc\005data_overrun\003sec_not_fnd\002write_protect\001no_am"

/* Status register ST2 */
#define NE7_ST2BITS	"\020\007control_mark\006bad_crc\005wrong_cyl\004scan_equal\003scan_not_found\002bad_cyl\001no_dam"

/* Status register ST3 */
#define NE7_ST3BITS	"\020\010fault\007write_protect\006drdy\005tk0\004two_side\003side_sel\002unit_sel1\0012unit_sel0"

/* Commands */
#define NE7CMD_SPECIFY	3	/*  specify drive parameters - requires unit
					parameters byte */
#define NE7CMD_SENSED	4	/*  sense drive - requires unit select byte */
#define NE7CMD_WRITE	5	/*  write - requires eight additional bytes */
#define NE7CMD_READ	6	/*  read - requires eight additional bytes */
#define NE7CMD_RECAL	7	/*  recalibrate drive - requires
					unit select byte */
#define NE7CMD_SENSEI	8	/*  sense controller interrupt status */
#define NE7CMD_READID	0xa	/*  read id */
#define NE7CMD_FORMAT	0xd	/*  format track */
#define NE7CMD_SEEK	0xf	/*  seek drive - requires unit select byte
					and new cyl byte */
					
/* bits modifying commands */
#define	NE7CMD_SK	0x20	/* skip sectors with deleted data bit */					
#define	NE7CMD_MFM	0x40	/* modified frequency modulation bit */					
#define	NE7CMD_MT 	0x80	/* multitrack bit */					

/* commands as used in driver */
#define NE7_WRITE	(NE7CMD_WRITE|NE7CMD_MFM|NE7CMD_MT)
#define NE7_READ	(NE7CMD_READ|NE7CMD_SK|NE7CMD_MFM|NE7CMD_MT)
#define NE7_FORMAT	(NE7CMD_FORMAT|NE7CMD_MFM)

/* some masks for status 0 register */
#define NE7_ST0_NOT_GOOD	0xF8
#define NE7_ST0_SEEK_COMPLETE	0x20
#define NE7_ST0_NOT_READY	0x08

/* status 1 register */
#define NE7_ST1_WRITE_PROTECTED 0x02
#define NE7_ST1_NO_DATA		0x04

/* status 2 register */
#define NE7_ST2_BADCYL		0x02
#define NE7_ST2_WRONGCYL	0x10

/* status 3 register */
#define NE7_ST3_TRK0		0x10

/* Sector size codes */
#define NE7_B128	0
#define NE7_B256	1
#define NE7_B512	2
#define NE7_B1024	3
#define NE7_B2048	4
#define NE7_B4098	5
