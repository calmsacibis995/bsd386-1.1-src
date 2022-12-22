/*
 * Copyright (c) 1991 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	$Id: fpe_fmt.c,v 1.6 1992/09/04 21:50:23 karels Exp $
 */

/*
 * 80387 subset emulator (no transcendental functions)
 *
 * conversions (to/from accumulator format)
 * rounding
 */

#include "param.h"
#include "machine/reg.h"
#include "fpe.h"
#include "fp_reg.h"


/*
 * fp_a to fp_[bdefiqs] conversions
 */

void
fp_b2a(bp, ap)
	register fp_b *bp;
	register fp_a *ap;
{

	fp_q q;
	fp_b2uq(bp, &q);
	fp_q2a(&q, ap);
	if (bp->sign & 0x80)
		ap->sign = -1;
}

void
fp_d2a(dp, ap)
	register fp_d *dp;
	register fp_a *ap;
{

	ap->sign = dp->sign ? -1 : 0;
	if (dp->exp == 0) {
		if (dp->mant0 == 0 && dp->mant1 == 0) {
			ap->exp = 0;
			ap->mant0 = ap->mant = 0;
			return;
		}
		ap->mant0 = ((unsigned)dp->mant0 << BIT_OFFSET_d)
			+ BITS_HI2LOW(dp->mant1, BIT_OFFSET_d);
		ap->mant1 = (unsigned)dp->mant1 << BIT_OFFSET_d;
		ap->exp = FP_EXP_BIAS_a - FP_EXP_BIAS_d;
		fp_norm(ap);
		return;
	}
	if (dp->exp == FP_EXP_MAX_d)
		ap->exp = FP_EXP_MAX_a;
	else
		ap->exp = (ushort)dp->exp + FP_EXP_BIAS_a - FP_EXP_BIAS_d;

	ap->mant0 = HI_BIT_a + ((unsigned)dp->mant0 << (BIT_OFFSET_d - 1))
		+ BITS_HI2LOW(dp->mant1, (BIT_OFFSET_d - 1));
	ap->mant1 = (unsigned)dp->mant1 << (BIT_OFFSET_d - 1);
}

void
fp_e2a(ep, ap)
	register fp_e *ep;
	register fp_a *ap;
{

	ap->sign = (ep->exp & HI_BIT_s) ? -1 : 0;
	ap->exp = (ep->exp &~HI_BIT_s) + FP_EXP_BIAS_a - FP_EXP_BIAS_e;
	ap->mant0 = ep->mant0;
	ap->mant1 = ep->mant1;
}

void
fp_f2a(fp, ap)
	register fp_f *fp;
	register fp_a *ap;
{

	ap->sign = fp->sign ? -1 : 0;
	ap->mant1 = 0;
	if (fp->exp == 0) {
		if (fp->mant0 == 0) {
			ap->exp = 0;
			ap->mant0 = 0;
			return;
		}
		ap->mant0 = (unsigned)fp->mant0 << BIT_OFFSET_f;
		ap->exp = FP_EXP_BIAS_a - FP_EXP_BIAS_f;
		fp_norm(ap);
		return;
	}
	if (fp->exp == FP_EXP_MAX_f)
		ap->exp = FP_EXP_MAX_a;
	else
		ap->exp = (ushort)fp->exp + FP_EXP_BIAS_a - FP_EXP_BIAS_f;
	ap->mant0 = HI_BIT_a + ((unsigned)fp->mant0 << (BIT_OFFSET_f - 1));
}

void
fp_i2a(ip, ap)
	int *ip;
	register fp_a *ap;
{
	register i = *ip;

	ap->sign = 0;
	if (i < 0) {
		i = -i;
		ap->sign = -1;
	}
	ap->exp = FP_EXP_BIAS_a + BITS(int) - 1;
	ap->mant0 = i;
	ap->mant1 = 0;
	fp_norm(ap);
}

void
fp_q2a(qp, ap)
	register fp_q *qp;
	register fp_a *ap;
{

	ap->sign = 0;
	if (qp->mant0 & HI_BIT_i) {
		ap->sign = -1;
		fp_chs_m(qp);
	}
	ap->exp = FP_EXP_BIAS_a + BITS(fp_q) - 1;
	ap->mant = qp->mant;
	ap->mant0 = qp->mant0;
	fp_norm(ap);
}

void
fp_s2a(sp, ap)
	short *sp;
	register fp_a *ap;
{
	register s = *sp;

	ap->sign = 0;
	if (s < 0) {
		s = -s;
		ap->sign = -1;
	}
	ap->exp = FP_EXP_BIAS_a + BITS(short) - 1;
	ap->mant0 = s << BITS(short);
	ap->mant1 = 0;
	fp_norm(ap);
}


/*
 * fp_[bdefiqs] to fp_a conversions
 */

struct nd {
	int bias;
	int maxexp;
	int mantlen;
};

enum { Zero, Cont, Overflow };
enum { integer, floating };

void
fp_a2b(fp_emp, ap, bp)
	struct fp_em *fp_emp;
	register fp_a *ap;
	register fp_b *bp;
{
	fp_q q;
	static fp_b round_ovr[] = {
			/* - infinity,  max negative num */
		{0x99999999, 0x99999999, 0x99, 0x80},
			/* max positive num, + infinity */
		{0x99999999, 0x99999999, 0x99, 0}
	};

	bp->sign = 0;
	if (ap->sign) {
		bp->sign |= 0x80;
		ap->sign = 0;
	}

	fp_a2q(fp_emp, ap, &q);
	CHECK_UNMASKED_EXC;
	if (q.mant0 & HI_BIT_i) {
		/* less than zero and ap->sign was 0 ==>
		 * special case - indefinite: {undef, undef, -1, -1}
		 */
		bp->sign = bp->bc = -1;
		return;
	}
	if (q.mant0 >= 0xDE0B6B3) {
		if (q.mant0 > 0xDE0B6B3 || q.mant > 0xA763FFFF) {
			if (!fp_left_intact(fp_emp))
				*bp = round_ovr[bp->sign ? 0 : 1];
			return;
		}
	}
	fp_uq2b(&q, ap);
}

void
fp_a2d(fp_emp, ap, dp)
	struct fp_em *fp_emp;
	register fp_a *ap;
	register fp_d *dp;
{
	static fp_d round_ovr[] = {
		{0, 0,  FP_EXP_MAX_d,	1},	/* - infinity */
		{-1, ALL_BITS_d, FP_EXP_MAX_d - 1, 1},	/* max negative num */
		{-1, ALL_BITS_d, FP_EXP_MAX_d - 1, 0},	/* max positive num */
		{0, 0,  FP_EXP_MAX_d,	0}	/* + infinity */
	};
	static struct nd nd =
		{FP_EXP_BIAS_d, FP_EXP_MAX_d, BITS_MANT_d};
	int r;	

	switch (fp_what_class(ap)) {
	case fp_zero:
		*((int *)dp) = 0;
		*((int *)dp + 1) = 0;
		return;
	case fp_denormal:
		fp_exc_denormal(fp_emp);
		CHECK_UNMASKED_EXC;
		
		/*FALLTHROUGH*/
	case fp_normal:
		r = fp_round(fp_emp, ap, &nd, floating);
		CHECK_UNMASKED_EXC;
		switch (r) {
		case Zero:
			*((int *)dp) = 0;
			*((int *)dp + 1) = 0;
			return;
		case Overflow:
			if (!fp_left_intact(fp_emp))
				*dp = round_ovr[fp_round_ovr(fp_emp, ap->sign)];
			return;
		}
		dp->exp = ap->exp + FP_EXP_BIAS_d - FP_EXP_BIAS_a;
		break;
	default:
		dp->exp = FP_EXP_MAX_d;
	}
	dp->sign = ap->sign;
	 /* to throw away high bit automatically */
	if (fp_shr_m(&ap->mant, BIT_OFFSET_d - 1)) {
		fp_exc_precision(fp_emp);
		CHECK_UNMASKED_EXC;
	}
	dp->mant0 = ap->mant0;
	dp->mant = ap->mant;
}

void
fp_a2e(fp_emp, ap, ep)
	struct fp_em *fp_emp;
	register fp_a *ap;
	register fp_e *ep;
{

	ep->exp = ap->exp + FP_EXP_BIAS_e - FP_EXP_BIAS_a;
	if (ap->sign)	/* set sign */
		ep->exp	|= HI_BIT_s;
	ep->mant0 = ap->mant0;
	ep->mant1 = ap->mant1;
}

void
fp_a2f(fp_emp, ap, fp)
	struct fp_em *fp_emp;
	register fp_a *ap;
	register fp_f *fp;
{
	static fp_f round_ovr[] = {
		{0,	FP_EXP_MAX_f,		1},	/* - infinity */
		{ALL_BITS_f, FP_EXP_MAX_f - 1,	1},	/* max negative num */
		{ALL_BITS_f, FP_EXP_MAX_f - 1,	0},	/* max positive num */
		{0,	FP_EXP_MAX_f,		0}	/* + infinity */
	};
	static struct nd nd =
		{FP_EXP_BIAS_f, FP_EXP_MAX_f, BITS_MANT_f};
	int r;	

	switch (fp_what_class(ap)) {
	case fp_zero:
		*((int *)fp) = 0;
		return;
	case fp_denormal:
		fp_exc_denormal(fp_emp);
		CHECK_UNMASKED_EXC;
		
		/*FALLTHROUGH*/
	case fp_normal:
		r = fp_round(fp_emp, ap, &nd, floating);
		CHECK_UNMASKED_EXC;
		switch (r) {
		case Zero:
			*((int *)fp) = 0;
			return;
		case Overflow:
			if (!fp_left_intact(fp_emp))
				*fp = round_ovr[fp_round_ovr(fp_emp, ap->sign)];
			return;
		}
		fp->exp = ap->exp + FP_EXP_BIAS_f - FP_EXP_BIAS_a;
		break;
	default:
		fp->exp = FP_EXP_MAX_f;
	}
	fp->sign = ap->sign;
	if (fp_shr_m(&ap->mant, BIT_OFFSET_f - 1)) {
		fp_exc_precision(fp_emp);
		CHECK_UNMASKED_EXC;
	}	
	fp->mant0 = ap->mant0;
}

void
fp_a2i(fp_emp, ap, ip)
	struct fp_em *fp_emp;
	register fp_a *ap;
	register int *ip;
{
	static int round_ovr[] = {
		{HI_BIT_i},	/* - infinity,  max negative num */
		{HI_BIT_i},	/* max negative num */
		{~HI_BIT_i},	/* max positive num */
		{HI_BIT_i}	/* + infinity */
	};
	static struct nd nd =
		{0, BITS(int), BITS(int)};
	int r;	

	switch (fp_what_class(ap)) {
	case fp_zero:
		*ip = 0;
		return;
	case fp_unsupported:
	case fp_signaling:
		*ip = HI_BIT_i;
		return;
	case fp_denormal:
		fp_exc_denormal(fp_emp);
		CHECK_UNMASKED_EXC;
	}
	r = fp_round(fp_emp, ap, &nd, integer);
	CHECK_UNMASKED_EXC;
	switch (r) {
	case Zero:
		*ip = 0;
		return;
	case Overflow:
		if (!fp_left_intact(fp_emp))
			*ip = round_ovr[fp_round_ovr(fp_emp, ap->sign)];
		return;
	}
	if (fp_shr_m(&ap->mant, BITS_MANT_a - 1 - ap->exp + FP_EXP_BIAS_a)) {
		fp_exc_precision(fp_emp);
		CHECK_UNMASKED_EXC;
	}	
	*ip = ap->mant;
	if (ap->sign)
		*ip = -(*ip);
}

void
fp_a2q(fp_emp, ap, qp)
	struct fp_em *fp_emp;
	register fp_a *ap;
	register fp_q *qp;
{
	static fp_q round_ovr[] = {
		{0, HI_BIT_i},	/* - infinity,  max negative num */
		{0, HI_BIT_i},	/* max negative num */
		{-1, ~HI_BIT_i},	/* max positive num */
		{0, HI_BIT_i}	/* + infinity */
	};
	static struct nd nd =
		{0, BITS(fp_q), BITS(fp_q)};
	int r;	

	switch (fp_what_class(ap)) {
	case fp_zero:
		qp->mant = qp->mant0 = 0;
		return;
	case fp_unsupported:
	case fp_signaling:
		qp->mant = 0;
		qp->mant0 = HI_BIT_i;
		return;
	case fp_denormal:
		fp_exc_denormal(fp_emp);
		CHECK_UNMASKED_EXC;
	}
	r = fp_round(fp_emp, ap, &nd, integer);
	CHECK_UNMASKED_EXC;
	switch (r) {
	case Zero:
		qp->mant = qp->mant0 = 0;
		return;
	case Overflow:
		if (!fp_left_intact(fp_emp))
			*qp = round_ovr[fp_round_ovr(fp_emp, ap->sign)];
		return;
	}
	if (fp_shr_m(&ap->mant, BITS_MANT_a - 1 - ap->exp + FP_EXP_BIAS_a)) {
		fp_exc_precision(fp_emp);
		CHECK_UNMASKED_EXC;
	}	
	if (ap->sign)
		fp_chs_m(&ap->mant);
	qp->mant = ap->mant;
	qp->mant0 = ap->mant0;
}

void
fp_a2s(fp_emp, ap, sp)
	struct fp_em *fp_emp;
	register fp_a *ap;
	register short *sp;
{
	static short round_ovr[] = {
		{HI_BIT_s},	/* - infinity,  max negative num */
		{HI_BIT_s},	/* max negative num */
		{~HI_BIT_s},	/* max positive num */
		{HI_BIT_s}	/* + infinity */
	};
	static struct nd nd = { 0, BITS(short), BITS(short)};
	int r;	

	switch (fp_what_class(ap)) {
	case fp_zero:
		*sp = 0;
		return;
	case fp_unsupported:
	case fp_signaling:
		*sp = HI_BIT_s;
		return;
	case fp_denormal:
		fp_exc_denormal(fp_emp);
		CHECK_UNMASKED_EXC;
	}
	r = fp_round(fp_emp, ap, &nd, integer);
	CHECK_UNMASKED_EXC;
	switch (r) {
	case Zero:
		*sp = 0;
		return;
	case Overflow:
		if (!fp_left_intact(fp_emp))
			*sp = round_ovr[fp_round_ovr(fp_emp, ap->sign)];
		return;
	}
	if (fp_shr_m(&ap->mant, BITS_MANT_a - 1 - ap->exp + FP_EXP_BIAS_a)) {
		fp_exc_precision(fp_emp);
		CHECK_UNMASKED_EXC;
	}	
	*sp = (short)ap->mant;
	if (ap->sign)
		*sp = -(*sp);
}

/*
 * rounding
 */
fp_round_ovr(fp_emp, sign)
	struct fp_em *fp_emp;
	int sign;
{
	switch (env(fp_control) & FPC_RC) {
	case FPC_RC_RN:
		return (sign ? 0 : 3);
	case FPC_RC_RD:
		return (sign ? 0 : 2);
	case FPC_RC_RU:
		return (sign ? 1 : 3);
	case FPC_RC_CHOP:
		return (sign ? 1 : 2);
	}
}

#define bit(ap, i) (i >= BITS(ap->mant) ? \
	ap->mant0 & (1 << (i - BITS(ap->mant))) : \
	ap->mant & (1 << i))


/*
 * add 1 in i-th position - which is next after last position
 *	for rounding (e.g. 0.5 for integer)
 */
fp_round_add(ap, i)
	register fp_a *ap;
	register i;
{
	fp_q mm;

	if (i >= BITS(mm.mant)) {
		mm.mant0 = 1 << (i - BITS(mm.mant));
		mm.mant = 0;
	} else {
		mm.mant0 = 0;
		mm.mant = 1 << i;
	}
	if (fp_add_norm_mm(&ap->mant, &mm))
		ap->exp += 1;
}

/*
 * add 1 in i-th position - which is next after last position
 *	for rounding (e.g. 0.5 for integer)
 * don't add to numbers with rational < 0.5
 * don't add to even with rational part = 0.5 (e.g. 2.5, 4.5...)
 */
void
fp_round_nearest(ap, i)
	register fp_a *ap;
	register int i;
{
	fp_q mm;

	if (!bit(ap, i))
		return;
	/* check for rational exact 0.5 - round to even */
	if (i > 0 && (i >= BITS_MANT_a - 1 || bit(ap, i + 1) == 0)) {
		mm.mant0 = ap->mant0;
		mm.mant = ap->mant;
		if (fp_shr_m(&mm, i) == 0)
			return;
	} else if (bit(ap, i + 1) == 0)		/* i == 0 */
		return;
	fp_round_add(ap, i);
}

fp_round(fp_emp, ap, np, class)
	struct fp_em *fp_emp;
	register fp_a *ap;
	register struct nd *np;
	int class;
{
	int re;
	register i;

	if (!(ap->mant0 & HI_BIT_a))
		ap->exp -= fp_norm_m(ap);
	re = ap->exp - FP_EXP_BIAS_a + np->bias;
	if (class == integer)
		i = BITS_MANT_a - 1 - re - 1;
	else {	/* (class == floating) */
		i = BITS_MANT_a - np->mantlen - 2;
		if (i < 0)		/* fp_e */
			return (Cont);
		if (re < 0)
			/*
			 * (1 - re) because denormal keep
			 * leading (63th) bit
			 */
			i += 1 - re;
	}
	if (i >= 0 && i < BITS_MANT_a)
		switch (env(fp_control) & FPC_RC) {
		case FPC_RC_RN:
			fp_round_nearest(ap, i);
			break;
		case FPC_RC_RD:
			if (ap->sign && bit(ap, i))
				fp_round_add(ap, i);
			break;
		case FPC_RC_RU:
			if (!ap->sign && bit(ap, i))
				fp_round_add(ap, i);
			break;
		case FPC_RC_CHOP:	/* nothing to do */
			break;
	}
	re = ap->exp - FP_EXP_BIAS_a + np->bias;
	if (re >= np->maxexp) {
		fp_exc_overflow(fp_emp, ap);
		CHECK_UNMASKED_EXC;
		re = ap->exp - FP_EXP_BIAS_a + np->bias;
		if (re >= np->maxexp)
			return (Overflow);
	} else if (re < 0) {
		fp_exc_underflow(fp_emp, ap);
		CHECK_UNMASKED_EXC;
		re = ap->exp - FP_EXP_BIAS_a + np->bias;
		if (re > 0)
			return (Cont);
		if (re < - np->mantlen || class == integer)
			return (Zero);
		else {	/* denormalization for float/double */
			ap->exp -= re;
			/*
			 * (1 - re) because denormal keep
			 * leading (63th) bit
			 */
			if (fp_shr_m(&ap->mant, 1 - re)) {
				fp_exc_precision(fp_emp);
				CHECK_UNMASKED_EXC;
			}
			return (Cont);
		}
	}
	return (Cont);
}

#undef bit

/*
 * normalize accumulator format number
 * ret: fp_zero, fp_denormal or fp_normal
 */
fp_norm(ap)
	register fp_a *ap;
{
	register i = fp_norm_m(&ap->mant);

	if (i == -1) {
		ap->exp = 0;
		return (fp_zero);
	} else
		ap->exp -= i;
	if (ap->exp < 0) {
		fp_shr_m(&ap->mant, - ap->exp);
		ap->exp = 0;
		return (fp_denormal);
	}
	return (fp_normal);
}
