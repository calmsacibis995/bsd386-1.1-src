/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	Krystal $Id: int15.c,v 1.2 1994/01/15 21:18:14 polk Exp $ */
#include "doscmd.h"

void
int15(tf)
struct trapframe *tf;
{
	struct byteregs *b = (struct byteregs *)&tf->tf_bx;
	int cond;
	int count;

        tf->tf_eflags &= ~PSL_C;

	switch (b->tf_ah) {
	case 0x00:	/* Get Cassette Status */
		b->tf_ah = 0x86;
		tf->tf_eflags |= PSL_C;	/* We don't support a cassette */
		break;
	case 0x04:	/* Set ABIOS table */
		tf->tf_eflags |= PSL_C;	/* We don't support it */
		break;
	case 0x41:
		if (b->tf_al & 0x10) {
			debug (D_HALF, "WAIT on port %d", tf->tf_dx);
		} else {
			debug (D_HALF, "WAIT");
		}
		cond = b->tf_al;
		count = b->tf_bl;
		for (;;) {
		    struct timeval tv;
		    if (cond & 0x10) {
			inb(&tf);
		    } else {
			b->tf_al = *(u_char *)MAKE_ADDR(tf->tf_es, tf->tf_di);
		    }
		    switch (cond & 7) {
/* next line as per prb recommendation mcl 01/01/94 */
/*                  case 0: goto out; break; */
                    case 0:
                        if (tf->tf_dx == 0x82) {
                                goto out; break;
                        }
                        fprintf(stderr, "waiting for external event\n");
                        fprintf(stderr,
                                "ax %04x, bx %04x, dx %04x, es:di %04x:%04x\n",
                                tf->tf_ax, tf->tf_bx, tf->tf_dx, tf->tf_es,
                                tf->tf_di);
                        break;
		    case 1: if (b->tf_al == b->tf_bh) goto out; break;
		    case 2: if (b->tf_al != b->tf_bh) goto out; break;
		    case 3: if (b->tf_al != 0) goto out; break;
		    case 4: if (b->tf_al == 0) goto out; break;
		    }
		    
		    tv.tv_sec = 0;
		    tv.tv_usec = 10000000 / 182;
		    select(0, 0, 0, 0, &tv);
		    if (count && --count == 0)
			break;
		}
	    out:
		b->tf_al = cond;
		break;
    	case 0x4f:
		/*
		 * XXX - Check scan code in b->tf_al.
		 */
		break;
	case 0x88:
		tf->tf_ax = 0;	/* memory past 1M */
		break;
	case 0xc0:	/* get configuration */
		debug (D_TRAPS|0x15, "Get configuration", tf->tf_dx);
    	    	tf->tf_es = rom_config >> 16;
    	    	tf->tf_bx = rom_config & 0xffff;
		break;
    	case 0xc1:	/* get extended BIOS data area */
                tf->tf_eflags |= PSL_C;
		break;
	default:
		unknown_int2(0x15, b->tf_ah, tf);
		break;
	}
}
