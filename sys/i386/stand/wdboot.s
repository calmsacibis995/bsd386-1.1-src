/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: wdboot.s,v 1.10 1993/12/31 00:58:26 karels Exp $
 */

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)wdbootblk.c	7.1 (Berkeley) 4/28/91
 */

/*
 * wdboot.s:
 *	Initial block boot for AT/386 with typical Western Digital
 *	WD 1002-WA2 (or upward compatible) controller. Works either as
 *	first and sole partition bootstrap, or as loaded by a
 *	earlier BIOS boot when on an inner partition of the disk.
 *
 *	Goal is to read in sucessive 7.5Kbytes of bootstrap to
 *	execute.
 *
 *	No attempt is made to handle disk errors.
 */

#include "reboot.h"
#include "i386/include/bootblock.h"
#include "i386/isa/isa.h"
#include "i386/isa/wdreg.h"

#define	NOP	jmp 1f; nop; 1:
#define BIOSRELOC	0x7c00

#define	BOOTADDR	(SMRELOC)	/* where we load the 15-sector boot */
#define	BOOTSTART	(SMRELOC+0x200)	/* where we start the bootstrap */
#define	BOOTBLOCKS	15

#define	PROTENABLE	0x1	/* protection enable bit in cr0 */

#define	CODESEL		0x08	/* code segment selector */
#define	DATASEL		0x10	/* data segment selector */

#define	WD_MAJORDEV	0	/* from conf.c */

#ifdef WDDEBUG
#define CRT     (0xb8000+(24*80*2))	/* start of last line of display */
#define	PUT(c,col) \
	movw	$(0x1f<<8)+(c),CRT+2*(col)
#else
#define	PUT(c,col)	/* void */
#endif /* WDDEBUG */
	.text
start:

	/*
	 * For this first little bit, we execute in 8086 'real' mode.
	 * We have to be careful to use only 16-bit instructions
	 * until we enable 32-bit protected mode.
	 */

	/*
	 * C&G p604: 'Clearing IF is not really needed, since the 80386
	 * reset sequence clears IF also, but it is shown here for emphasis.'
	 */
	cli

	/*
	 * Load the Global Descriptor Table.
	 * Since the assembler won't generate a 16-bit LGDT,
	 * we just use the 32-bit address escape prefix.
	 * Using the CS segment prefix means we don't need to initialize DS.
	 */
	aword
	lgdt	%cs:GDTaddr

	/*
	 * Turn on protected mode.
	 */
	smsw	%ax
	orw	$PROTENABLE,%ax
	lmsw	%ax		/* we're in protected mode forever now */
	jmp	0f		/* flush the instruction queue */
0:

	/*
	 * Jump to 32-bit protected code.
	 * We use the 32-bit operand escape,
	 * since the assembler won't generate a 16-bit LJMP.
	 */
	word
	ljmp	$CODESEL,$start32	/* jump to 32-bit protected code */

start32:
	/*
	 * We have reached 32-bit mode.
	 * Set up selectors and get ready to do some real work.
	 */
	movw	$DATASEL,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%ss
	movl	$BOOTADDR,%esp	/* stack grows down from 15-sector boot */
	PUT(0x41,0)

	/* save first 8K of memory into video memory, for dos emulation */
	movl	$0,%esi
	movl	$0xb8000+4096, %edi
	movl	$8192, %ecx
	rep
	movsb

	/* check for fdisk label in sector 0 */
	movl	$BOOTADDR,%edi
	movl	$BOOTADDR+512,%esi
	movl	$IO_WD1+wd_seccnt,%edx
	movb	$1,%al
	outb	%al,%dx
	incl	%edx		/* wd_sector = 1 */
	outb	%al,%dx
	incl	%edx		/* wd_cyl_lo = 0 */
	movb	$0,%al
	outb	%al,%dx
	incl	%edx		/* wd_cyl_hi = 0 */
	outb	%al,%dx
	/* assume that sdh unit is correct; get it and save for later */
	movl	$IO_WD1+wd_sdh,%edx
	xorl	%eax,%eax
	inb	%dx,%al		/* get sdh */
	andb	$0xf0,%al	/* keep drive/format, clear head */
	outb	%al,%dx		/* set sdh */
	andb	$0x10,%al	/* isolate unit # bit */
#if B_UNITSHIFT != 4
#if B_UNITSHIFT > 4
	shll	$B_UNITSHIFT-4,%al
#else
	shrl	$4-B_UNITSHIFT,%al
#endif
#endif
	orl	$MAKEBOOTDEV(WD_MAJORDEV, 0, 0, 0, 0),%eax
	pushl	%eax		/* save bootdev with unit number */
	PUT(0x42,1)
	call	doread

	xorl	%ebx,%ebx		/* default cylinder, 0 */

	/* %esi points at end of sector 0 */
	cmpw	$MB_SIGNATURE,-2(%esi)	/* signature present? */
	jnz	doit
	/*
	 * Look for an active BSD/386 partition in the DOS partition label.
	 * If no active BSDI partition is found, then pick the first
	 * BSDI partition.
	 */
	xorl	%eax,%eax
	movl	$BOOTADDR+MB_PARTOFF,%ebp	/* base of partition table */
	movl	$4,%ecx

srchpart:
	cmpb	$MBS_BSDI,4(%ebp)
	jnz	cont
	cmpb	$MBA_ACTIVE,(%ebp)
	jz	found
	cmpl	$0,%eax			/* first BSDI partition? */
	jne	cont
	movl	%ebp,%eax
	cmpb	$MBA_NOTACTIVE,(%ebp)	/* not active or inactive: not fdisk */
	jnz	doit
cont:
	addl	$16,%ebp
	loop	srchpart

	cmpl	$0,%eax			/* found a BSDI partition? */
	je	doit			/* not found */
	movl	%eax,%ebp		/* use first found */

found:
	movb	2(%ebp),%bl		/* sector and 2 high cyl bits */
	shll	$2,%ebx			/* move high bits into place */
	movb	3(%ebp),%bl		/* add in low cyl. bits. */

doit:
	PUT(0x43,2)
	/* load remaining BOOTBLOCKS sectors off disk */
	movl	$BOOTADDR,%edi
	movl	$IO_WD1+wd_seccnt,%edx
	movb	$BOOTBLOCKS,%al
	outb	%al,%dx
	incl	%edx		/* wd_sector = 2 */
	movb	$2,%al
	outb	%al,%dx
	incl	%edx		/* wd_cyl_lo = %bl */
	movb	%bl,%al
	outb	%al,%dx
	incl	%edx		/* wd_cyl_hi = %bh */
	movb	%bh,%al
	outb	%al,%dx
	/* use existing value for wd_sdh */
	movl	$BOOTADDR+BOOTBLOCKS*512-1,%esi
	call	doread

	popl	%eax		/* retrieve bootdev */
	pushl	%ebx		/* cyloffset */

	pushl	%eax		/* bootdev */
	
	pushl	$RB_AUTOBOOT	/* howto */

	pushl	$BOOTSTART
	PUT(0x44,3)
	ret	/* main (howto, dev, off) */

/*
 * Initiate a read command with parameters already set up;
 * wait for command completion and data request.
 * On entry, %edi points at the buffer start, %esi at the buffer end.
 */
doread:
	movl	$IO_WD1+wd_command,%edx
	movb	$WDCC_READ,%al
	outb	%al,%dx
	NOP
	cld
	
	/* wait for BUSY to clear and DRQ to set (data ready to read) */
readblk:
	movl	$60,%eax	/* Delay ~ 60 usec */
	call	delay
	movl	$IO_WD1+wd_status,%edx
	inb	%dx,%al
	NOP
	testb	$WDCS_BUSY,%al
	jnz	readblk
	/*
	 * Re-read status as the ATA spec states that the other bits in the
	 * status register may not be valid until 400ns after busy has cleared.
	 * We wait much longer; some Quantum drives seem to require this.
	 */
	movl	$60,%eax	/* Delay ~ 60 usec */
	call	delay
	inb	%dx,%al
	NOP
	testb	$WDCS_DRQ,%al
	jz	readblk
	
	/* read a block into final position in memory */

	movl	$IO_WD1+wd_data,%edx
	movl	$256,%ecx
	.byte 0x66,0xf2,0x6d	# rep insw
	NOP

	/* need more blocks to be read in? */

	cmpl	%esi,%edi
	jl	readblk
	ret

/*
 * Delay: parameter in %eax.
 * Probably delays about %eax/3 to %eax microseconds.
 */
delay:	pushl	%ecx
	movl	%eax,%ecx
dloop:	outb	%al,$0x80	/* %eax * ~1 usec per outb */
	loop	dloop
	popl	%ecx
	ret

	/*
	 * The contents of the Global Descriptor Table.
	 */

#define ATTR0(present, dpl, dtype, type) \
	(((present)<<7)+((dpl)<<5)+((dtype)<<4)+(type))
#define	ATTR1(granularity4k, default32, limit16_19) \
	(((granularity4k)<<7)+((default32)<<6)+(limit16_19))

	.align 2
GDT:
	/* the null segment (selector 0) */
	.long 0x0,0x0
	/* the code segment (selector 0x8) */
	.word 0xffff		/* limit 0:15 = everything */
	.word 0x0		/* base address 0:15 = 0 */
	.byte 0x0		/* base address 16:23 = 0 */
	.byte ATTR0(1,0,1,0xf)	/* present, dpl = 0, dtype = mem, type = R/X */
	.byte ATTR1(1,1,0xf)	/* 4k, 32-bit, limit 16:19 = everything */
	.byte 0x0		/* base address 24:31 = 0 */
	/* the data segment (selector 0x10) */
	.word 0xffff		/* limit 0:15 = everything */
	.word 0x0		/* base address 0:15 = 0 */
	.byte 0x0		/* base address 16:23 = 0 */
	.byte ATTR0(1,0,1,0x3)	/* present, dpl = 0, dtype = mem, type = R/W */
	.byte ATTR1(1,1,0xf)	/* 4k, 32-bit, limit 16:19 = everything */
	.byte 0x0		/* base address 24:31 = 0 */

GDTaddr:
	.word 23		/* sizeof (gdt) - 1 */
	.long GDT		/* &gdt */

ebootblkcode:

	/* remaining space usable for a disk label */
	
	.org	510
	.word	0xaa55		/* signature -- used by BIOS ROM */

ebootblk: 			/* MUST BE EXACTLY 0x200 BIG FOR SURE */
