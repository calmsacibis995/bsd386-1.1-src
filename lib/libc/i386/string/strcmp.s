/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

#include "DEFS.h"

#ifdef LIBC_SCCS
	.asciz	"BSDI $Id: strcmp.s,v 1.2 1993/11/18 20:23:54 donn Exp $"
#endif

/*
 * Compare two strings and return >0/0/<0 depending on string order.
 *
 * int strcmp(const char *s1, const char *s2);
 */
ENTRY(strcmp)
	pushl %esi
	pushl %edi

	movl 12(%esp),%esi
	movl 16(%esp),%edi

	cld

Lagain:
	cmpsb
	jne Lfound
	cmpb $0,-1(%edi)
	jnz Lagain

	xorl %eax,%eax

Lexit:
	popl %edi
	popl %esi
	ret

Lfound:
	movzbl -1(%esi),%eax
	movzbl -1(%edi),%ecx
	subl %ecx,%eax
	jmp Lexit
