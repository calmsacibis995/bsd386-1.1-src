/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

#include "DEFS.h"
#include "COPY.h"

#ifdef LIBC_SCCS
	.asciz	"BSDI $Id: strncpy.s,v 1.1 1993/11/12 10:29:49 donn Exp $"
#endif

/*
 * Copy a string src into a buffer dst of the given size len.
 * If the string was shorter than the buffer, pad with nuls.
 * Return the address of the buffer.
 *
 * char *strncpy(char *dst, const char *src, size_t len);
 */
ENTRY(strncpy)
	STRN_PROLOGUE()

	cmpl %esi,%edi
	jbe Lforward	/* if destination precedes source, copy forward */

	COPY_REVERSE()
	jmp Lpad

Lforward:
	COPY()

Lpad:			/* pad with nuls */
	movl 12(%esp),%edi
	addl %edx,%edi
	movl 20(%esp),%ecx
	subl %edx,%ecx
	xorl %eax,%eax
	repe; stosb

	movl 12(%esp),%eax
	COPY_EPILOGUE()
