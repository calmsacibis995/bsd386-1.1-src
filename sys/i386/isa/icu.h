/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: icu.h,v 1.16 1993/12/19 04:10:43 karels Exp $
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
 * AT/386 Interrupt Control constants
 * W. Jolitz 8/89
 * Much modified since then!
 */

#ifndef	__ICU__
#define	__ICU__

#ifndef	LOCORE

/*
 * Interrupt "level" mechanism variables, masks, and macros
 */

/*
 * Interrupt Group Masks
 */
extern	u_int highmask;		/* interrupts masked with splhigh() */
extern	u_int ttymask;		/* interrupts masked with spltty() */
extern	u_int biomask;		/* interrupts masked with splbio() */
extern	u_int impmask;		/* interrupts masked with splimp() */
extern	u_int protomask;	/* interrupts masked with splnet() */
extern	u_int nonemask;		/* interrupts masked with spl0() */
extern	u_int cpl;		/* current interrupt mask */

#define	INTREN(s)	(nonemask &= ~(s), protomask &= ~(s))
#if 0
#define	INTRDIS(s)	nonemask |= (s)
#endif
#define	INTRMASK(msk,s)	msk |= (s)

#else

/*
 * Macros for interrupt level priority masks (used in interrupt vector entry)
 */

/*
 * In interrupt entry, OR mask into ICU interrupt masks,
 * enable interrupts, and push previous mask.
 */
#define	INTRICU(mask) \
	movl mask,%eax;		/* set interrupt mask */ \
	movl _cpl,%edx; \
	orl %edx,%eax;		/* or'ed with old mask */ \
	movl %eax,_cpl; \
	outb %al,$IO_ICU1+1; \
	movb %ah,%al; \
	outb %al,$IO_ICU2+1; \
9:	sti; \
	pushl %edx		/* push previous mask */

/* entry for interrupts 0 through 6 (ICU1) */
#define	INTR1(unit, mask) \
	pushl $0;		/* fake error code */ \
	pushl $T_ASTFLT;	/* fake trap number */ \
	pushal; \
\
	movb $0x20,%al; \
	outb %al,$IO_ICU1;	/* OCW2: non-specific EOI */ \
\
	pushl %ds; \
	pushl %es; \
	movw $0x10,%ax;		/* GSEL(GDATA_SEL, SEL_KPL) */ \
	movw %ax,%ds; \
	movw %ax,%es; \
\
	INTRICU(mask); \
	incl _cnt+V_INTR; \
	incl _isa_intr + ((unit)*4); \
\
	pushl $unit;

/* entry for interrupt 7 (ICU1); need to detect stray interrupts */
#define	INTR1_7(unit, mask) \
	pushl $0;		/* fake error code */ \
	pushl $T_ASTFLT;	/* fake trap number */ \
	pushal; \
\
	inb $IO_ICU1,%al;	/* test In-Service register */ \
	testb %al,%al; \
	jne 0f; \
	movb $0x20,%al; 	/* it's a stray -- ignore it */ \
	outb %al,$IO_ICU1;	/* OCW2: non-specific EOI */ \
	popal; \
	addl $8,%esp; \
	incl %ss:_isa_defaultstray; /* count the glitches; %ds is user's */ \
	iret; \
0: \
	movb $0x20,%al; \
	outb %al,$IO_ICU1;	/* OCW2: non-specific EOI */ \
\
	pushl %ds; \
	pushl %es; \
	movw $0x10,%ax;		/* GSEL(GDATA_SEL, SEL_KPL) */ \
	movw %ax,%ds; \
	movw %ax,%es; \
\
	INTRICU(mask); \
	incl _cnt+V_INTR; \
	incl _isa_intr + ((unit)*4); \
\
	pushl $unit;

/* entry for interrupts 8 through 14 (ICU2) */
#define	INTR2(unit, mask) \
	pushl $0;		/* fake error code */ \
	pushl $T_ASTFLT;	/* fake trap number */ \
	pushal; \
\
	movb $0x20,%al; \
	outb %al,$IO_ICU2;	/* OCW2: non-specific EOI */ \
	outb %al,$IO_ICU1;	/* EOI for slave interrupt in master */ \
\
	pushl %ds; \
	pushl %es; \
	movw $0x10,%ax;		/* GSEL(GDATA_SEL, SEL_KPL) */ \
	movw %ax,%ds; \
	movw %ax,%es; \
\
	INTRICU(mask); \
	incl _cnt+V_INTR; \
	incl _isa_intr + ((unit)*4); \
\
	pushl $unit;

/* entry for interrupt 15 (ICU2); need to detect stray interrupts */
#define	INTR2_15(unit, mask) \
	pushl $0;		/* fake error code */ \
	pushl $T_ASTFLT;	/* fake trap number */ \
	pushal; \
\
	inb $IO_ICU2,%al;	/* test In-Service register */ \
	testb %al,%al; \
	jne 0f; \
	movb $0x20,%al; 	/* it's a stray -- ignore it */ \
	outb %al,$IO_ICU1;	/* EOI for slave interrupt in master */ \
	outb %al,$IO_ICU2;	/* EOI for slave */ \
	popal; \
	addl $8,%esp; \
	incl %ss:_isa_defaultstray + 4; /* count the glitches; %ds is user's */ \
	iret; \
0: \
	movb $0x20,%al; \
	outb %al,$IO_ICU2;	/* OCW2: non-specific EOI */ \
	outb %al,$IO_ICU1;	/* EOI for slave interrupt in master */ \
\
	pushl %ds; \
	pushl %es; \
	movw $0x10,%ax;		/* GSEL(GDATA_SEL, SEL_KPL) */ \
	movw %ax,%ds; \
	movw %ax,%es; \
\
	INTRICU(mask); \
	incl _cnt+V_INTR; \
	incl _isa_intr + ((unit)*4); \
\
	pushl $unit;

/*
 * Macros for setting interrupt masks
 */

/* OR m into current mask */
#define	ORPL(m)	\
	cli;				/* disable interrupts */ \
	movl	m, %eax; 		/* get new mask */ \
	movl	_cpl, %edx;		/* previous mask */ \
	orl	%edx, %eax;		/* or in previous mask */ \
	movl	%eax, _cpl;		/* set new priority */ \
	outb	%al, $IO_ICU1+1; \
	movb	%ah, %al; 		/* high byte of new hw mask */ \
	outb	%al, $IO_ICU2+1; \
	movl	%edx, %eax;		/* return old priority */ \
	sti;				/* enable interrupts */

/* OR m into current mask, checking for new mask == old */
#define	ORPL_LAZY(m)	\
	movl	m, %eax; 		/* get new mask */ \
	movl	_cpl, %edx;		/* previous mask */ \
	orl	%edx, %eax;		/* or in previous mask */ \
	cmpl	%edx, %eax;		/* same as previous? */ \
	je	9f; \
	cli;				/* disable interrupts */ \
	movl	%eax, _cpl;		/* set new priority */ \
	outb	%al, $IO_ICU1+1; \
	movb	%ah, %al; 		/* high byte of new hw mask */ \
	outb	%al, $IO_ICU2+1; \
	movl	%edx, %eax;		/* return old priority */ \
	sti;				/* enable interrupts */ \
9:

/* set mask to v */
#define	SETPL(v)	\
	cli;				/* disable interrupts */ \
	movl	v, %edx; \
	movb	%dl,%al; 		/* low byte of new hw mask */ \
	outb	%al, $IO_ICU1+1; \
	movb	%dh, %al; 		/* high byte of new hw mask */ \
	outb	%al, $IO_ICU2+1; \
	movl	_cpl, %eax;		/* return old priority */ \
	movl	%edx, _cpl;		/* set new priority */ \
	sti;				/* enable interrupts */

#endif

/*
 * Interrupt enable bits -- in order of priority
 */
#define	IRQ0		0x0001		/* highest priority - timer */
#define	IRQ1		0x0002
#define	IRQ_SLAVE	0x0004
#define	IRQ8		0x0100
#define	IRQ9		0x0200
#define	IRQ2		IRQ9
#define	IRQ10		0x0400
#define	IRQ11		0x0800
#define	IRQ12		0x1000
#define	IRQ13		0x2000
#define	IRQ14		0x4000
#define	IRQ15		0x8000
#define	IRQ3		0x0008
#define	IRQ4		0x0010
#define	IRQ5		0x0020
#define	IRQ6		0x0040
#define	IRQ7		0x0080		/* lowest hw - parallel printer */
#define	IRQ_ALLHW	0xffff		/* all hardware interrupts */
#define	IRQSOFT		0x010000	/* software interrupt mask */

#define IRQUNK		0xff		/* use intr discovery to get IRQ */
#define IRQNONE		0		/* use no interrupt */

/*
 * Interrupt Control offset into Interrupt descriptor table (IDT)
 */
#define	ICU_OFFSET	32		/* 0-31 are processor exceptions */
#define	ICU_LEN		16		/* 32-47 are ISA interrupts */

#endif	__ICU__
