/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

#include "DEFS.h"

#ifdef LIBC_SCCS
	.asciz	"BSDI $Id: strchr.s,v 1.1 1993/11/12 10:27:51 donn Exp $"
#endif

/*
 * Look for a byte c with a particular value in a string s.
 * Since we don't have to check the entire string before finding the byte,
 * we don't want to use the REP SCAS technique to locate it.
 *
 * char *strchr(const char *s, int c);
 */
ENTRY(strchr)
	pushl %esi

	movl 8(%esp),%esi
	movl 12(%esp),%edx

	cld

Lagain:
	lodsb
	cmpb %al,%dl
	je Lfound
	testb %al,%al
	jnz Lagain

	xorl %eax,%eax

Lexit:
	popl %esi
	ret

Lfound:
	leal -1(%esi),%eax
	jmp Lexit
