/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	Krystal $Id: int2f.c,v 1.1 1994/01/14 23:35:22 polk Exp $ */
#include "doscmd.h"

int int2f_11(struct trapframe *);

void
int2f(struct trapframe *tf)
{               
	int c;
        struct byteregs *b = (struct byteregs *)&tf->tf_bx;

	switch(b->tf_ah) {
	case 0x00:
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
		debug (D_FILE_OPS, "Called printer function 0x%02x", b->tf_ah);
		tf->tf_eflags |= PSL_C;
		b->tf_al = FUNC_NUM_IVALID;
		break;
	case 0x12:
		switch (b->tf_al) {
		case 0x2e:
		    /* XXX - GET/SET ERROR TABLE ADDRESSES */
		    debug (D_HALF, "GET/SET ERROR TABLE %d\n", b->tf_dl);
		    tf->tf_eflags |= PSL_C;
		    break;
		default:
		    unknown_int4(0x2f, 0x12, b->tf_al, b->tf_dl, tf);
		    break;
    	    	}
		break;
	case 0x43:
		switch (b->tf_al) {
		case 0x00:
			debug (D_HALF, "Get XMS status\n");
			b->tf_al = 0;
			break;
		default:
			unknown_int3(0x2f, 0x43, b->tf_al, tf);
			break;
		}
		break;
	case 0xb7:
		switch (b->tf_al) {
		case 0x00:
			debug (D_HALF, "Get APPEND status\n");
			b->tf_al = 0;
			break;
		case 0x02:
			debug (D_HALF, "Verify DOS 5.0 APPEND compatibility\n");
			tf->tf_ax = 0;
			break;
		case 0x04:
			debug (D_HALF, "Get APPEND path\n");
			tf->tf_es = 0;
			tf->tf_di = 0;
			break;
		case 0x06:
			debug (D_HALF, "Deterimine APPEND mode\n");
			tf->tf_bx = 0;
			break;
		case 0x07:
			debug (D_HALF, "Set APPEND mode to %04x\n", tf->tf_bx);
			break;
		default:
			if (vflag) dump_regs(tf);
			fatal ("unknown int2f:b7 func 0x%x\n", b->tf_al);
			break;
		}
		break;
    	case 0x11:
    	case 0x16:
    	case 0x48:	/* Read command line */
    	case 0x55:
    	case 0xae:
	default:
		unknown_int2(0x2f, b->tf_ah, tf);
	}
}
