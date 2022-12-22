/*
 * Copyright (c) 1992, 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	$Id: fdboot.s,v 1.10 1994/02/04 04:08:54 karels Exp $
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
 *	@(#)fdbootblk.c	7.2 (Berkeley) 5/4/91
 */

/*
 * fdboot.s:
 *	Initial block boot for AT/386 with typical stupid NEC controller
 *	Works only with 3.5 inch diskettes that have 16 or greater sectors/side
 *	or (if FD5BOOT is defined) with 5.25 inch diskettes with 15 sectors/trk
 *
 *	Goal is to read in sucessive 7.5Kbytes of bootstrap to
 *	execute.
 *
 *	No attempt is made to handle disk errors.
 */

#include "reboot.h"

#include "i386/isa/isa.h"
#include "i386/isa/fdreg.h"

#define	NOP	jmp 1f ; nop ; 1:
#define BIOSRELOC	0x7c00

#define	BOOTADDR	(SMRELOC)	/* where we load the 15-sector boot */
#define	BOOTSTART	(SMRELOC+0x200)	/* where we start the bootstrap */
#define	BOOTBLOCKS	15

#define	PROTENABLE	0x1	/* protection enable bit in cr0 */

#define	CODESEL		0x08	/* code segment selector */
#define	DATASEL		0x10	/* data segment selector */

#define	FD_MAJORDEV	2	/* from conf.c */

/* #define FDDEBUG */
#ifdef FDDEBUG
#define CRT     (0xb8000+(24*80*2))	/* start of last line of display */
#define	PUT(c,col) \
	movw	$(0x1f<<8)+(c),CRT+2*(col)
#else
#define	PUT(c,col)	/* void */
#endif /* FDDEBUG */
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

	/* load remaining 15 sectors off disk */
	movl	$BOOTADDR,%edi

#ifdef WATCH_ICU
	movb	$0x20,%al	# do a eoi
	outb	%al,$0x20

	NOP
	movb	$0xbf,%al	# enable floppy interrupt, mask out rest
	outb	%al,$0x21
	NOP
#endif /* WATCH_ICU */
	xorl	%ebx,%ebx
	incb	%bl		# start with sector 2 (we are sector 1)
Lnextsec:
	incb	%bl		# pre-increment
	movb	%bl,sec
	movl	%edi,%ecx

	/* Set read/write bytes */
	xorl	%edx,%edx
	movb	$0x0c,%dl	# outb(0xC,0x46); outb(0xB,0x46);
	movb	$0x46,%al
	outb	%al,%dx
	NOP
	decb	%dx
	outb	%al,%dx

	/* Send start address */
	movb	$0x04,%dl	# outb(0x4, addr);
	movb	%cl,%al
	outb	%al,%dx
	NOP
	movb	%ch,%al		# outb(0x4, addr>>8);
	outb	%al,%dx
	NOP
	rorl	$8,%ecx		# outb(0x81, addr>>16);
	movb	%ch,%al
	outb	%al,$0x81
	NOP

	/* Send count */
	movb	$0x05,%dl	# outb(0x5, 0);
	xorl	%eax,%eax
	outb	%al,%dx
	NOP
	movb	$2,%al		# outb(0x5,2);
	outb	%al,%dx
	NOP

	/* set channel 2 */
	# movb	$2,%al		# outb(0x0A,2);
	outb	%al,$0x0A
	NOP
	PUT(0x42,1)

	/* issue read command to fdc */
	movw	$(IO_FD1+fdsts),%dx
	movl	$readcmd,%esi
	xorl	%ecx,%ecx
	movb	$9,%cl

 2:	inb	%dx,%al
	NOP
	testb	$0x80,%al
	jz 2b

	incb	%dx
	movl	(%esi),%al
	outb	%al,%dx
	NOP
	incl	%esi
	decb	%dx
	loop	 2b

#ifdef WATCH_ICU	/* doesn't work on some machines */
	/* watch the icu looking for an interrupt signalling completion */
	xorl	%edx,%edx
	movb	$0x20,%dl
 2:	movb	$0xc,%al
	outb	%al,%dx
	NOP
	inb	%dx,%al
	NOP
	andb	$0x7f,%al
	cmpb	$6,%al
	jne	2b
	movb	$0x20,%al	# do a eoi
	outb	%al,%dx
	NOP
#endif /* WATCH_ICU */

	/*
	 * Wait for read to complete (if not done above)
	 * by watching for availability of status info.
	 * Then extract the status bytes.
	 */
	movl	$(IO_FD1+fdsts),%edx
	xorl	%ecx,%ecx
	movb	$7,%cl
 2:	inb	%dx,%al
	NOP
	andb	$0xC0,%al
	outb	%al,$0x80	/* delay needed on some 50MHz machines??? */
	cmpb	$0xC0,%al
	jne	2b
	incb	%dx
	inb	%dx,%al
	decb	%dx
	loop	2b
	PUT(0x43,2)

	addw	$0x200,%edi	# next addr to load to
#ifndef FD5BOOT
	/*
	 * On 3 1/2" HD diskettes, we read physical sectors 2-16
	 * from track 0 head 0.
	 */
	cmpb	$16,%bl
	jl	Lnextsec

#else /* FD5BOOT */
	/*
	 * Hack: on 5 1/4" HD diskettes, we read physical sectors 2-15
	 * from track 0 head 0, then the last sector is sector 1, head 1.
	 * This is wired into this logic.
	 */
	cmpb	$1,%bl		/* done with everything? */
	je	3f
	cmpb	$15,%bl		/* done with first track? */
	jl	Lnextsec
	xorl	%ebx,%ebx	/* back to sector 1 with pre-increment */
	movb	$1,head	
	movb	$(1<<2),hddr	/* head 1 */
	jmp	Lnextsec
#endif /* FD5BOOT */

3:
	pushl	$0		/* cylinder */
#ifndef FD5BOOT
	pushl	$MAKEBOOTDEV(FD_MAJORDEV, 0, 0, 0, 0)	/* unit 0, type 0 */
#else /* FD5BOOT */
	pushl	$MAKEBOOTDEV(FD_MAJORDEV, 1, 0, 0, 0)	/* unit 0, type 1 */
#endif /* FD5BOOT */
	
	pushl	$RB_AUTOBOOT		/* howto */

	pushl	$BOOTSTART
	PUT(0x44,3)
	ret	/* main (howto, dev, off) */

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

/*
 * The following array contains the parameters for a read command,
 * in the order they are fed to the controller chip.
 */
readcmd:.byte	0xe6
hddr:	.byte	0		/* head << 2 | drive */
trk:	.byte	0
head:	.byte	0
sec:	.byte	0
secsz:	.byte	2		/* == 512 */
#ifndef FD5BOOT
sectrk:	.byte	18		/* for 3 1/2" HD */
#else /* FD5BOOT */
sectrk:	.byte	15		/* for 5 1/4" HD */
#endif /* FD5BOOT */
gap:	.byte	0x1b
len:	.byte	0xff

ebootblkcode:

	/* remaining space usable for a disk label */
	
	.org	510
	.word	0xaa55		/* signature -- used by BIOS ROM */

ebootblk: 			/* MUST BE EXACTLY 0x200 BIG FOR SURE */
