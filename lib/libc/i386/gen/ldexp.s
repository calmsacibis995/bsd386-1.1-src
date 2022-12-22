/*
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: ldexp.s,v 1.1 1993/02/05 21:34:22 polk Exp $
 */

#include <DEFS.h>

/*
 * ANSI conformance requires that we notify on overflow.
 */
	.globl _errno
#define	ERANGE			34

ENTRY(ldexp)
	fildl 12(%esp)
	fldl 4(%esp)
	fscale
	fstl -8(%esp)		/* force double precision overflow detection */
	fwait			/* paranoia */
	fnstsw %ax
	testb $8,%al		/* check for overflow */
	jz 0f
	movl $ERANGE,_errno	/* %st should have +/- Inf */
0:
	fstp %st(1)		/* reset the stack */
	ret
