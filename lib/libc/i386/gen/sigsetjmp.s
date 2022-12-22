/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: sigsetjmp.s,v 1.1 1993/02/06 00:00:58 polk Exp $
 */

#include "DEFS.h"

#define	_JBLEN	10	/* XXX from setjmp.h */

ENTRY(sigsetjmp)
	movl 4(%esp),%edx
	movl 8(%esp),%eax
	movl %eax,(_JBLEN*4)(%edx)
	testl %eax,%eax
	jz __setjmp
	jmp _setjmp

ENTRY(siglongjmp)
	movl 4(%esp),%edx
	cmpl $0,(_JBLEN*4)(%edx)
	je __longjmp
	jmp _longjmp
