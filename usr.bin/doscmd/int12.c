/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	Krystal $Id: int12.c,v 1.1 1994/01/14 23:32:13 polk Exp $ */
#include "doscmd.h"

void
int12(tf)
struct trapframe *tf;
{
    	tf->tf_ax = 640;
}
