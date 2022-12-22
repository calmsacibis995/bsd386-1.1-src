/*
 * Copyright (c) 1991 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	$Id: fpe_s.s,v 1.2 1992/02/20 22:14:47 karels Exp $
 */

/*
 * 80387 subset emulator (no transcendental functions)
 *
 * arithmetic operations, shifts, normalization
 *
 */
#include "asm.h"

/*
 * void fp_chs_m(fp_q *ap)
 * int fp_norm_m(fp_q *ap)
 * void fp_shr_m(fp_q *ap, int count)
 *
 * int fp_sub_mm(fp_q *ap, fp_q *bp)
 * int fp_add_norm_mm(fp_q *ap, fp_q *bp)
 * int fp_mul_mm(fp_q *ap, fp_q *bp)
 * int fp_div_mm(fp_q *ap, fp_q *bp)
 *
 * void fp_uq2b(fp_q *qp, fp_b *bp)
 * void fp_b2uq(fp_b *bp, fp_q *qp)
 */

/*
 * void fp_chs_m(fp_q *ap)
 */
ENTRY(fp_chs_m)
	FRAME
	movl	B_ARG0, %eax
#define	a1	(%eax)
#define	a0	4(%eax)
	notl	a0
	notl	a1
	addl	$1, a1
	adcl	$0, a0
#undef	a1
#undef	a0
 	EMARF
	ret

/*
 * int fp_norm_m(fp_q *ap)
 * normalize mantissa (shift left of mantissa)
 * ret: number of binary digits shifted
 *	or (-1) if mantissa iz zero
 *	(*ap) shifted so that high bit set
 */
ENTRY(fp_norm_m)
	FRAME
	movl	B_ARG0, %edx
#define	a1	(%edx)
#define	a0	4(%edx)
	bsrl	a0, %ecx
	jz	LBf(L1,1)
	subb	$31, %cl
	negb	%cl
	movl	a1, %eax
	SHLDL(	%cl, %eax, a0)
	shll	%cl, a1
	movl	%ecx, %eax
	jmp	LBf(L3,3)
LB_(L1,1)
	bsrl	a1, %ecx
	jz 	LBf(L2,2)
	subb	$31, %cl
	negb	%cl
	movl	a1, %eax
	shll	%cl, %eax
	movl	%eax, a0
	movl	$0, a1
	leal	32(%ecx), %eax
	jmp	LBf(L3,3)
LB_(L2,2)
	movl	$-1, %eax
LB_(L3,3)
#undef	a1
#undef	a0
 	EMARF
	ret

/*
 * void fp_shr_m(fp_q *ap, int count)
 * shift right of mantissa
 * 0 <= count < 64
 * ret: non-zero - shift out part wasn't zero
 *	0 - otherwise
 */
ENTRY(fp_shr_m)
	FRAME
	pushl	%ebx
	movl	B_ARG0, %ebx
	movl	B_ARG1, %ecx
#define	a1	(%ebx)
#define	a0	4(%ebx)
	movl	$0, %eax
	jcxz	LBf(L13,3)
	cmpb	$32, %cl
	jz	LBf(L11,1)
	jg	LBf(L12,2)
	/* %cl < 32 */

	movl	a1, %edx
	SHRDL(	%cl, %edx, %eax)
	movl	a0, %edx
	SHRDL(	%cl, %edx, a1)
	shrl	%cl, a0
	jmp	LBf(L15,5)
LB_(L11,1)	/* %cl = 32 */
	movl	a1, %eax
	movl	a0, %edx
	movl	%edx, a1
	movl	$0, a0
	jmp	LBf(L15,5)
LB_(L12,2)	/* %cl > 32 */
	andb	$31, %cl	/* not necessary (shr mod 32), yet */
	movl	a0, %edx
	cmpl	$0, a1
	jz 	LBf(L13,3)
	movl	a1, %eax
	jmp	LBf(L14,4)
LB_(L13,3)
	SHRDL(	%cl, %edx, %eax)
LB_(L14,4)
	shrl	%cl, %edx
	movl	%edx, a1
	movl	$0, a0
#undef	a1
#undef	a0
LB_(L15,5)
	popl	%ebx
 	EMARF
	ret

/*
 * int fp_sub_mm(fp_q *ap, fp_q *bp)
 * sub two mantisses of accumulator format numbers
 * ret: 1 - was overflow,
 *	0 - otherwise
 */
ENTRY(fp_sub_mm)
	FRAME
	pushl	%ebx
	movl	B_ARG0, %ebx
	movl	B_ARG1, %ecx
#define	a1	(%ebx)
#define	a0	4(%ebx)
#define	b1	(%ecx)
#define	b0	4(%ecx)
	movl	$0, %edx
	movl	a1, %eax
	movl	a0, %edx
	subl	b1, %eax
	sbbl	b0, %edx

	movl	%eax, a1
	movl	%edx, a0

	movl	$0, %eax
	jnc	LBf(L21,1)
	incl	%eax
LB_(L21,1)
#undef	a1
#undef	a0
#undef	b1
#undef	b0
	popl	%ebx
 	EMARF
	ret

/*
 * int fp_add_norm_mm(fp_q *ap, fp_q *bp)
 * add two mantisses of accumulator format numbers
 * and normalize:
 *	on overflow shift result right one bit & insert 1 to high bit
 * ret: 1 - was overflow,
 *	3 - was overflow and lost precision bit
 *	0 - otherwise
 */
ENTRY(fp_add_norm_mm)
	FRAME
	pushl	%ebx
	movl	B_ARG0, %ebx
	movl	B_ARG1, %ecx
#define	a1	(%ebx)
#define	a0	4(%ebx)
#define	b1	(%ecx)
#define	b0	4(%ecx)
	movl	a0, %edx
	movl	a1, %eax
	addl	b1, %eax
	adcl	b0, %edx
	movl	%eax, a1
	movl	%edx, a0
	movl	$0, %eax
	jnc	LBf(L31,1)
	incl	%eax
	rcrl	$1, a0
	rcrl	$1, a1
	jnc	LBf(L31,1)
	orb	$2, %al
LB_(L31,1)
#undef	a1
#undef	a0
#undef	b1
#undef	b0
	popl	%ebx
 	EMARF
	ret

/*
 * int fp_mul_mm(fp_q *ap, fp_q *bp)
 * multiply two mantisses of accumulator format numbers
 * ret: 1 - was overflow,
 *	2 - lost precision bit(s)
 *	3 - was overflow and lost precision bit(s)
 *	0 - otherwise
 *	(*ap) - result (normalized)
 */
ENTRY(fp_mul_mm)
	FRAME
	subl	$16, %esp
	pushl	%ebx
#define r3	-16(%ebp)
#define r2	-12(%ebp)
#define r1	-8(%ebp)
#define r0	-4(%ebp)

	movl	$0, r0
	movl	$0, r1
	movl	B_ARG0, %ebx
	movl	B_ARG1, %ecx
#define	a1	(%ebx)
#define	a0	4(%ebx)
#define	b1	(%ecx)
#define	b0	4(%ecx)

	movl	a1, %eax
	mull	b1
	movl	%eax, r3
	movl	%edx, r2

	movl	a1, %eax
	mull	b0
	addl	%eax, r2
	adcl	%edx, r1
	adcl	$0, r0

	movl	a0, %eax
	mull	b1
	addl	%eax, r2
	adcl	%edx, r1
	adcl	$0, r0

	movl	a0, %eax
	mull	b0
	addl	%eax, r1
	adcl	%edx, r0

	jns	LBf(L41,1)
	movl	r1, %eax
	movl	%eax, a1
	movl	r0, %eax
	movl	%eax, a0

	movl	$1, %eax
	jmp	LBf(L42,2)
LB_(L41,1)
	shll	$1, r3
	rcll	$1, r2
	movl	r1, %eax
	rcll	$1, %eax
	movl	%eax, a1

	movl	r0, %eax
	rcll	$1, %eax
	movl	%eax, a0

	movl	$0, %eax
LB_(L42,2)
	cmpl	$0, r3
	jnz	LBf(L43,3)
	cmpl	$0, r2
	jz	LBf(L44,4)
LB_(L43,3)
	orb	$2, %al
LB_(L44,4)
#undef	a1
#undef	a0
#undef	b1
#undef	b0
#undef r0
#undef r1
#undef r2
#undef r3
	popl	%ebx
 	EMARF
	ret

/*
 * int fp_div_mm(fp_q *ap, fp_q *bp)
 * divide two mantisses of accumulator format numbers
 *	(*ap)/(*bp)
 * ret: non-zero - precision lost, 0 otherwise
 * side effect: *ap = result
 */
ENTRY(fp_div_mm)
	FRAME
	subl	$12, %esp
#define r0	-4(%ebp)
#define r1	-8(%ebp)
#define op_add	-9(%ebp)	/* 0 - sub, else(>0) - add : byte*/
#define i	-10(%ebp)	/* 0..64 : byte */

	pushl	%ebx
	pushl	%esi
	pushl	%edi
 	movl	B_ARG0, %esi
 	movl	B_ARG1, %edi
#define	a1	(%esi)
#define	a0	4(%esi)
#define	b1	(%edi)
#define	b0	4(%edi)

 	movb	$63, i
 	movb	$0, op_add
 	movl	$0, r0
 	movl	$0, r1

	shrl	$1, b0
	rcrl	$1, b1

	shrl	$1, a0
	rcrl	$1, a1

LCL_(wh1)				/* while (i > 0) */
	cmpl	$0, a0
	jnz	LBf(L50, 1)
	cmpl	$0, a1
	jz	LCL(ewh1)

LB_(L50, 1)
	decb 	i
 	jl	LCL(out)

	movl	b1, %eax
	movl	b0, %edx

	cmpb	$0, op_add
	jne	LBf(L51,1)

	/* substract branch */
	subl	%eax, a1
	sbbl	%edx, a0
	jc	LBf(L53,3)		/* sub, carry => add, r <- 0 */
	jmp	LBf(L52,2)		/* sub, nocarry => sub, r <- 1 */

LB_(L51,1)
	/* add branch */
	addl	%eax, a1
	adcl	%edx, a0
	jc	LBf(L52,2)		/* add, carry => sub, r <- 1 */
	/* jnc	LBf(L53,3) */		/* add, nocarry => add, r <- 0 */

LB_(L53,3)		/* set next add, insert 0 to r */
 	incb	op_add		/* next time op - add */
	shll	$1, r1		/* r <- 0 */
	rcll	$1, r0

	shll	$1, a1		/* a <- */
	rcll	$1, a0
	jmp	LCL(wh1)

LB_(L52,2)		/* set next sub, insert 1 to r on left shift */
	shll	$1, a1		/* a <- */
	rcll	$1, a0

 	movb	$0, op_add		/* next time op - sub */
	stc			/* r <- 1 */
	rcll	$1, r1
	rcll	$1, r0

	/* normalize a, then shift r as a, and correct i */
	bsrl	a0, %ecx
	jz	LBf(L54,4)
	subb	$30, %cl
	jge	LCL(wh1)
	negb	%cl

	cmpb	i, %cl
	jg	LBf(L521, 1)
	subb	%cl, i
	jmp	LBf(L525, 5)
LB_(L521, 1)
	movb	i, %cl
	movb	$0, i
LB_(L525, 5)
	movl	a1, %eax
	SHLDL(	%cl, %eax, a0 )
	shll	%cl, a1
	movl	r1, %eax
	SHLDL(	%cl, %eax, r0 )
	shll	%cl, r1
	jmp	LCL(wh1)

LB_(L54,4)
	/* a0 == 0 */
	bsrl	a1, %ecx
	jz	LCL(ewh1)
	subb	$62, %cl
	negb	%cl

	cmpb	i, %cl
	jg	LBf(L541, 1)
	subb	%cl, i
	jmp	LBf(L542, 2)
LB_(L541, 1)
	movb	i, %cl
	movb	$0, i
LB_(L542, 2)
	cmpb	$32, %cl
	jl	LBb(L525, 5)
	subb	$32, %cl
	movl	a1, %eax
	shll	%cl, %eax
	movl	%eax, a0
	movl	$0, a1
	movl	r1, %eax
	shll	%cl, %eax
	movl	%eax, r0
	movl	$0, r1
	jmp	LCL(wh1)

LCL_(ewh1)
	movb	i, %cl
	cmpb	$32, %cl
	jge	LBf(L514,4)
	movl	r1, %eax
	SHLDL(	%cl, %eax, r0 )
	shll	%cl, r1
	jmp	LCL(out)
LB_(L514,4)
	subb	$32, %cl
	movl	r1, %eax
	shll	%cl, %eax
	movl	%eax, r0
	movl	$0, r1

LCL_(out)
	movl	$1, %eax
	cmpl	$0, a0
	jnz	LBf(L512,2)
	cmpl	$0, a1
	jnz	LBf(L512,2)
	decl	%eax
LB_(L512, 2)

	movl	r0, %ebx
	movl	%ebx, a0
	movl	r1, %ebx
	movl	%ebx, a1

	shll	a1
	rcll	a0

	popl	%edi
	popl	%esi
	popl	%ebx
#undef	a1
#undef	a0
#undef	b1
#undef	b0
#undef	i
#undef	r0
#undef	r1
#undef	op
	EMARF
	ret

/*
 * conversions between BCD fp_b and unsigned fp_q
 */

/*
 * void fp_b2uq(fp_b *bp, fp_q *qp)
 */
ENTRY(fp_b2uq)
	FRAME
	pushl	%ebx
	pushl	%esi
	pushl	%edi

	movl	B_ARG0, %esi
#define	bc	8(%esi)
#define	b0	4(%esi)
#define	b0lsb	7(%esi)
#define	b1	(%esi)

	movl	B_ARG1, %edi
#define	q1	(%edi)
#define	q0	4(%edi)

	movb	$4, %cl

	movb	bc, %al
	movl	b0, %ebx
	SHRDL(	%cl, %eax, %ebx)
	andl	$0x0f, %eax
	call	LCL(Lunpack9)

	mull	tenInNinth	/* 10**9 */
	movl	%edx, q0
	movl	%eax, q1

	movb	b0lsb, %al
	movl	b1, %ebx
	andl	$0x0f, %eax
	call	LCL(Lunpack9)
	addl	%eax, q1
	adcl	$0, q0

	popl	%edi
	popl	%esi
	popl	%ebx
	EMARF
	ret

/*
 * convert 2-digit 1-byte packed bcd to binary
 * %cl == 4
 * %al - packed bcd - 1 byte - 2 digit
 *
 * ret: %al - binary
 */
LCL_(unpack_al)
	shll	%cl, %eax
	shrb	%cl, %al
	andb	$0x0f, %ah
	aad
	ret

/*
 * convert 9 digit packed bcd
 * to binary
 * %cl == 4
 * one (not two!) most significant digit in %eax, 8 digits in %ebx
 * ret: %eax
 */
LCL_(Lunpack9)
	pushl	%eax	/* to reserve place */
	movb	$5, %ch
LB_(L61, 1)
	mull	hundred
	movl	%eax, (%esp)
	movb	%bl, %al
	call	LCL(unpack_al)
	andl	$0xff, %eax
	addl	(%esp), %eax
	decb	%ch
	jz	LBf(L62, 2)
	shrl	%cl, %ebx
	shrl	%cl, %ebx
	jmp	LBb(L61, 1)
LB_(L62, 2)
	popl	%edx	/* dummy: only to restore esp */
	ret



/*
 * void fp_uq2b(fp_q *qp, fp_b *bp)
 */
ENTRY(fp_uq2b)
	FRAME
	pushl	%ebx
	pushl	%esi
	pushl	%edi

	movl	B_ARG1, %esi
#define	bc	8(%esi)
#define	b0	4(%esi)
#define	b0lsb	7(%esi)
#define	b1	(%esi)

	movl	B_ARG0, %edi
#define	q1	(%edi)
#define	q0	4(%edi)

	movb	$4, %cl

	movl	q0, %edx
	movl	q1, %eax
	divl	tenInNinth		/* 10**9 */

	pushl	%edx		/* temporarily save */

	movl	$0, %edx
	divl	tenInSeventh	/* 10**7 */
	/* remainder in %edx - keep till pack8() */
	aam
	shlb	%cl, %ah
	orb	%ah, %al
	movb	%al, bc

	call	LCL(Lpack8)
	shll	%cl, %ebx
	movl	%ebx, b0

	popl	%eax		/* restore saved */
	movl	$0, %edx
	idivl	ten
		/* remainder in %edx - keep till pack8() */
	andb	$0xf, %al
	orb	%al, b0lsb

	call	LCL(Lpack8)
	movl	%ebx, b1

	popl	%edi
	popl	%esi
	popl	%ebx
	EMARF
	ret

/*
 * in %edx - binary (< 10**8)
 * %cl == $4
 * out %ebx - bcd
 */
LCL_(Lpack8)
	movb	$0, %ebx
	movb	$8, %ch
LB_(L83, 3)
	movl	%edx, %eax
	movl	$0, %edx
	divl	ten
	andb	%al, %bl
	decb	%ch
	jz	LBf(L84, 4)
	shll	%cl, %ebx
	jmp	LBb(L83, 3)
LB_(L84, 4)
	ret

	.data

tenInNinth:	.long	1000000000
tenInSeventh:	.long	10000000
hundred:	.long	100
ten:		.long	10
