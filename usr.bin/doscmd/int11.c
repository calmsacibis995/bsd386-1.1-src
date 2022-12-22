/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	Krystal $Id: int11.c,v 1.1 1994/01/14 23:31:57 polk Exp $ */
#include "doscmd.h"

void
int11 (tf)
struct trapframe *tf;
{
	/*
	 * If we are an XT
	 */
	tf->tf_ax = 0x0001		/* has a disk */
		  | 0x000c		/* 64K on main board */
		  | 0x0020		/* 80x25 color card */
		  | 0x0080		/* 3 disks */
		  | (0x0 << 9)		/* number rs232 ports */
		  | (0x0 << 14);	/* number of printers */
	/*
	 * If we are an AT
	 */
	tf->tf_ax = nfloppies ? 0x0001 : 0x0000		/* has a disk */
		  | (0 << 1)		/* No mouse */
		  | (0x3 << 2)		/* at least 64 K on board */
		  | (0x2 << 4)		/* 80x25 color card */
		  | ((nfloppies - 1) <<	6)	/* has 1 disk */
		  | (0x0 << 8)		/* No DMA */
		  | (nserial << 9)	/* number rs232 ports */
		  | (0x0 << 12)		/* No game card */
		  | (0x0 << 13)		/* No serial printer/internal modem */
		  | (nparallel << 14);	/* number of printers */
}
