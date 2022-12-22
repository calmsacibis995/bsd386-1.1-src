/*	BSDI $Id: fp_reg.h,v 1.2 1992/09/04 21:08:00 karels Exp $	*/

/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */
/*
 * HISTORY
 * $Log: fp_reg.h,v $
 * Revision 1.2  1992/09/04  21:08:00  karels
 * clarify comments, move default mode here, use 53-bit mantissa by default
 *
 * Revision 1.1  1992/01/08  23:58:52  bil
 * Floating point emulator
 *
 * Revision 2.3  91/02/05  17:11:41  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:33:50  mrt]
 * 
 * Revision 2.2  90/05/03  15:25:14  dbg
 * 	Created.
 * 	[90/02/08            dbg]
 * 
 */

#ifndef	_I386_FP_SAVE_H_
#define	_I386_FP_SAVE_H_
/*
 *	Floating point registers and status, as saved
 *	and restored by FP save/restore instructions.
 */
struct i386_fp_save	{
	unsigned short	fp_control;	/* control */
	unsigned short	fp_unused_1;
	unsigned short	fp_status;	/* status */
	unsigned short	fp_unused_2;
	unsigned short	fp_tag;		/* register tags */
	unsigned short	fp_unused_3;
	unsigned int	fp_eip;		/* eip at failed instruction */
	unsigned short	fp_cs;		/* cs at failed instruction */
	unsigned short	fp_opcode;	/* opcode of failed instruction */
	unsigned int	fp_dp;		/* data address */
	unsigned short	fp_ds;		/* data segment */
	unsigned short	fp_unused_4;
};

struct i386_fp_regs {
	unsigned short	fp_reg_word[5][8];
					/* space for 8 80-bit FP registers */
};

/*
 * Control register
 */
#define	FPC_IE		0x0001		/* mask invalid operation exception */
#define	FPC_DE		0x0002		/* mask denormal operand exception */
#define	FPC_ZE		0x0004		/* mask zero-divide exception */
#define	FPC_OE		0x0008		/* mask overflow exception */
#define	FPC_UE		0x0010		/* mask underflow exception */
#define	FPC_PE		0x0020		/* mask precision exception */
#define	FPC_PC		0x0300		/* precision control: */
#define	FPC_PC_24	0x0000			/* 24 bits */
#define	FPC_PC_53	0x0200			/* 53 bits */
#define	FPC_PC_64	0x0300			/* 64 bits */
#define	FPC_RC		0x0c00		/* rounding control: */
#define	FPC_RC_RN	0x0000			/* round to nearest or even */
#define	FPC_RC_RD	0x0400			/* round down */
#define	FPC_RC_RU	0x0800			/* round up */
#define	FPC_RC_CHOP	0x0c00			/* chop */
/* remainder on 80287/8087 only */
#define	FPC_IC		0x1000		/* infinity control (obsolete) */
#define	FPC_IC_PROJ	0x0000			/* projective infinity */
#define	FPC_IC_AFF	0x1000			/* affine infinity (std) */

#ifndef OLD_FP
#define	FPC_DEFAULT	0x127f		/* set by fpinit: 53-bit, exc. off */
#else
#define	FPC_DEFAULT	0x037f		/* set by fpinit: 64-bit, exc. off */
#endif
#define	FPC_ABI		0x1272		/* specified by Intel ABI */

/*
 * Status register
 */
#define	FPS_IE		0x0001		/* invalid operation */
#define	FPS_DE		0x0002		/* denormalized operand */
#define	FPS_ZE		0x0004		/* divide by zero */
#define	FPS_OE		0x0008		/* overflow */
#define	FPS_UE		0x0010		/* underflow */
#define	FPS_PE		0x0020		/* precision */
#define	FPS_SF		0x0040		/* stack flag */
#define	FPS_ES		0x0080		/* error summary */
#define	FPS_C0		0x0100		/* condition code bit 0 */
#define	FPS_C1		0x0200		/* condition code bit 1 */
#define	FPS_C2		0x0400		/* condition code bit 2 */
#define	FPS_TOS		0x3800		/* top-of-stack pointer */
#define	FPS_TOS_SHIFT	11
#define	FPS_C3		0x4000		/* condition code bit 3 */
#define	FPS_BUSY	0x8000		/* FPU busy */

#endif	_I386_FP_SAVE_H_
