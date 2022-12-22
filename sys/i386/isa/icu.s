/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: icu.s,v 1.20 1994/01/05 21:00:27 karels Exp $
 */

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * %sccs.include.386.c%
 *
 *	%W% (Berkeley) %G%
 */

/*
 * AT/386
 * Vector interrupt control section
 * Copyright (C) 1989,90 W. Jolitz
 */

#include        "isa.h"
#include        "icu.h"

#include "machine/psl.h"

/* 
 * Interrupt masks.  These are the masks used by splxxx;
 * they are set up during autoconfiguration.  They are bit masks
 * that will be ORed with the previous priority and then set into
 * the 8259A ICU.  nonemask will contain the mask of all unused
 * interrupts.  As we configure interrupts, the corresponding bit in
 * nonemask (and protomask) is cleared.
 */
	.data
	.align	2
	.globl	_cpl
_cpl:		.long	IRQ_ALLHW
	.globl	_nonemask
_nonemask:	.long	IRQ_ALLHW
	.globl	_highmask
_highmask:	.long	(IRQ_ALLHW|IRQSOFT)
	.globl	_ttymask
_ttymask:	.long	IRQSOFT
	.globl	_biomask
_biomask:	.long	IRQSOFT
	.globl	_impmask
_impmask:	.long	IRQSOFT
	.globl	_protomask
_protomask:	.long	(IRQ_ALLHW|IRQSOFT)
	.globl	_isa_intr
_isa_intr:	.space	16*4

	.text
	.align 2
/*
 * Handle return from interrupt after device handler finishes.
 */
doreti:
	cli
	popl	%ebx			# remove intr number
	popl	%eax			# get previous priority
	# now interrupt frame is a trap frame!
	movl	%eax,%ecx
	outb	%al,$IO_ICU1+1		# re-enable intr?
	movb	%ah,%al
	outb	%al,$IO_ICU2+1
	movl	%ecx,_cpl

	cmpl	_nonemask,%ecx		# returning to zero?
	je	9f

	/*
	 * Not returning to spl0, just pop registers and return.
	 */
	pop	%es
	pop	%ds
	popal
	addl	$8,%esp
	iret

	/*
	 * Set protomask priority, enabling hardware interrupts.
	 * The rest of this section is protected against reentrance
	 * by the non-nonemask cpl (IRQSOFT set).
	 */
9:	orl	$IRQSOFT,%ecx		/* block software interrupts */
	movl	%ecx,_cpl
	sti

	cmpl	$0,_netisr		# check for softints/traps
	je	2f			# if none, maybe ast's

#include "../net/netisr.h"

#define DONET(s, c)	; \
	.globl	c;  \
	btrl	$s,_netisr;  \
	jnb	1f; \
	call	c; \
1:
#ifdef INET
	DONET(NETISR_IP,_ipintr)
#if 0
	DONET(NETISR_ARP,_arpintr)
#endif
#endif
#ifdef IMP
	DONET(NETISR_IMP,_impintr)
#endif
#ifdef NS
	DONET(NETISR_NS,_nsintr)
#endif
#ifdef ISO
	DONET(NETISR_ISO,_clnlintr)
#endif
#ifdef CCITT
	DONET(NETISR_CCITT,_hdintr)
#endif
	DONET(NETISR_RAW,_rawintr)

	btrl	$NETISR_SCLK,_netisr
	jnb	2f
	# back to an interrupt frame for a moment
	pushl	_nonemask
	pushl	$0	# dummy unit
	call	_softclock
	popl	%eax
	popl	%eax

2:
	.globl	_wayout_list, _dowayout
	cmpl	$0,_wayout_list
	je	2f
	call	_dowayout

2:
	cmpl	$0,_astpending
	je	9f
	testl	$PSL_VM,14*4(%esp)	# vm86 set
	jne	3f			# yes - means user
	cmpw	$0x1f,13*4(%esp)	# returning to user? (check tf_cs)
	jne	9f			# nope, leave
3:	call	_trap

	/*
	 * Finally ready to return.
	 * Disable interrupts, then clear the IRQSOFT bit
	 * so that we will again be at spl0 once we return.
	 */
9:	cli
	andl	$(~IRQSOFT),_cpl	/* splx(nonemask); */
	pop	%es			# finally return
	pop	%ds
	popal
	addl	$8,%esp
	iret

#ifndef INLINE_SPL
/*
 * Interrupt priority mechanism
 * May be done in-line or using these functions,
 * except for spl0, which is always done as a function.
 */
ALTENTRY(splclock, _splhigh)
ENTRY(splhigh)
	ORPL(_highmask)
	ret

/* spltty is normally a macro that calls _spltty if necessary */
ALTENTRY(spltty, __spltty)
ENTRY(_spltty)
	ORPL(_ttymask)
	ret

ENTRY(splbio)
	ORPL(_biomask)
	ret

ENTRY(splimp)
	ORPL(_impmask)
	ret

ENTRY(splnet)
	/* ORPL(_protomask) */
	movl	_cpl,%eax
	orl	$IRQSOFT,_cpl
	ret

ENTRY(splsoftclock)
	SETPL(_protomask)
	ret
#endif /* !INLINE_SPL */


ENTRY(splx)
	movl	4(%esp),%ecx		# new priority level
	cmpl	_nonemask,%ecx
	je	spl0			# going to "zero level" is special

	SETPL(%ecx)
	ret

ENTRY(spl0)
spl0:
	SETPL(_protomask)
	cmpl	$0,_netisr
	je	2f

	DONET(NETISR_RAW,_rawintr)
#ifdef INET
	DONET(NETISR_IP,_ipintr)
#if 0
	DONET(NETISR_ARP,_arpintr)
#endif
#endif
#ifdef IMP
	DONET(NETISR_IMP,_impintr)
#endif
#ifdef NS
	DONET(NETISR_NS,_nsintr)
#endif
#ifdef ISO
	DONET(NETISR_ISO,_clnlintr)
#endif
#ifdef CCITT
	DONET(NETISR_CCITT,_hdintr)
#endif

2:
	.globl	_wayout_list, _dowayout
	cmpl	$0,_wayout_list
	je	2f
	call	_dowayout

2:
	andl	$(~IRQSOFT),_cpl	/* splx(nonemask); */
	ret

	/* hardware interrupt catcher (IDT 32 - 47) */
	.globl	_isa_intrswitch
/*
 * Hack: in order to avoid calling isa_intrswitch, then returning here
 * for just one instruction (jmp doreti), we do the call by hand
 * to fudge the return address to "return" to doreti.
 */

	/* hard-wire clock interrupt for a little extra performance */
IDTVEC(clkintr)
	INTR1(0, _highmask); pushl $doreti; jmp _hardclock

#if 0
IDTVEC(intr0)
	INTR1(0, _mask0); pushl $doreti; jmp _isa_intrswitch
#endif

IDTVEC(intr1)
	INTR1(1, _mask1); pushl $doreti; jmp _isa_intrswitch

IDTVEC(intr2)
	INTR1(2, _mask2); pushl $doreti; jmp _isa_intrswitch

IDTVEC(intr3)
	INTR1(3, _mask3); pushl $doreti; jmp _isa_intrswitch

IDTVEC(intr4)
	INTR1(4, _mask4); pushl $doreti; jmp _isa_intrswitch

IDTVEC(intr5)
	INTR1(5, _mask5); pushl $doreti; jmp _isa_intrswitch

IDTVEC(intr6)
	INTR1(6, _mask6); pushl $doreti; jmp _isa_intrswitch

IDTVEC(intr7)
	INTR1_7(7, _mask7); pushl $doreti; jmp _isa_intrswitch


IDTVEC(intr8)
	INTR2(8, _mask8); pushl $doreti; jmp _isa_intrswitch

IDTVEC(intr9)
	INTR2(9, _mask9); pushl $doreti; jmp _isa_intrswitch

IDTVEC(intr10)
	INTR2(10, _mask10); pushl $doreti; jmp _isa_intrswitch

IDTVEC(intr11)
	INTR2(11, _mask11); pushl $doreti; jmp _isa_intrswitch

IDTVEC(intr12)
	INTR2(12, _mask12); pushl $doreti; jmp _isa_intrswitch

IDTVEC(intr13)
	INTR2(13, _mask13); pushl $doreti; jmp _isa_intrswitch

IDTVEC(intr14)
	INTR2(14, _mask14); pushl $doreti; jmp _isa_intrswitch

IDTVEC(intr15)
	INTR2_15(15, _mask15); pushl $doreti; jmp _isa_intrswitch


	.globl	_isa_adintr

/*
 * entry for interrupt during autoconfig interrupt probe, all except 7, 15.
 * Like INTR2, but we take an extra jmp and use the same mask
 * to save on code space.
 */
#define	ADINTR(unit) \
	pushl $0;		/* fake error code */ \
	pushl $T_ASTFLT;	/* fake trap number */ \
	pushal; \
	movl $unit,%ecx;	/* save unit number */ \
	jmp adintr		/* rest is common */

/*
 * common code for adintr* interrupts other than 7, 15;
 * unit/irq number is in %ecx.
 */
adintr:
	movb $0x20,%al
	outb %al,$IO_ICU2	/* OCW2: non-specific EOI */
	outb %al,$IO_ICU1	/* EOI for slave interrupt in master */

	pushl %ds
	pushl %es
	movw $0x10,%ax		/* GSEL(GDATA_SEL, SEL_KPL) */
	movw %ax,%ds
	movw %ax,%es

	INTRICU(_highmask)
	pushl %ecx		/* unit/irq number */
	call _isa_adintr
	jmp doreti

IDTVEC(adintr0)
	ADINTR(0)

IDTVEC(adintr1)
	ADINTR(1)

IDTVEC(adintr2)
	ADINTR(2)

IDTVEC(adintr3)
	ADINTR(3)

IDTVEC(adintr4)
	ADINTR(4)

IDTVEC(adintr5)
	ADINTR(5)

IDTVEC(adintr6)
	ADINTR(6)

IDTVEC(adintr7)
	INTR1_7(7, _highmask); call _isa_adintr; jmp doreti


IDTVEC(adintr8)
	ADINTR(8)

IDTVEC(adintr9)
	ADINTR(9)

IDTVEC(adintr10)
	ADINTR(10)

IDTVEC(adintr11)
	ADINTR(11)

IDTVEC(adintr12)
	ADINTR(12)

IDTVEC(adintr13)
	ADINTR(13)

IDTVEC(adintr14)
	ADINTR(14)

IDTVEC(adintr15)
	INTR2_15(15, _highmask); call _isa_adintr; jmp doreti
