/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	Krystal $Id: int20.c,v 1.1 1994/01/14 23:34:17 polk Exp $ */
#include "doscmd.h"

void
int20 (tf)
struct trapframe *tf;
{
	/* int 20 = exit(0) */
	done (tf, 0);
}
