/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	Krystal $Id: int33.c,v 1.1 1994/01/14 23:35:33 polk Exp $ */
#include "doscmd.h"

void
int33 (tf)
struct trapframe *tf;
{
	tf->tf_eflags |= PSL_C;	/* We don't support a mouse */
}
