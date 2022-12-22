/*
 * Copyright (c) 1993 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	$Id: assert.c,v 1.1 1993/12/16 01:12:49 donn Exp $
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

__dead void
__assert(e, f, l)
	const char *e, *f;
	int l;
{

	(void)fprintf(stderr,
	    "assertion \"%s\" failed: file \"%s\", line %d\n", e, f, l);
	abort();
}
