/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	Krystal $Id: int17.c,v 1.1 1994/01/14 23:33:55 polk Exp $ */
#include "doscmd.h"

void
int17(tf)
struct trapframe *tf;
{
	struct byteregs *b = (struct byteregs *)&tf->tf_bx;

	switch (b->tf_ah) {
	case 0x01:	/* Initialize Printer */
		debug (D_HALF, "Initialize Parallel port %d", tf->tf_dx);
		b->tf_ah = 0x01;	/* Time out */
		break;
	default:
		unknown_int2(0x17, b->tf_ah, tf);
		break;
	}
}
