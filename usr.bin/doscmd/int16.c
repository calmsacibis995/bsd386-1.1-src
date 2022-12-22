/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	Krystal $Id: int16.c,v 1.3 1994/01/15 21:28:51 polk Exp $ */
#include "doscmd.h"

volatile int poll_cnt = 16;

void
wakeup_poll()
{
    if (poll_cnt <= 0)
      poll_cnt = 1;
}

void
reset_poll()
{
    poll_cnt = 16;
}

void
sleep_poll()
{
    if (--poll_cnt <= 0) {
	poll_cnt = 0;
	while (KbdEmpty() && poll_cnt <= 0)
	    tty_pause();
    }
}

void
int16 (tf)
struct trapframe *tf;
{               
	int c;
        struct byteregs *b = (struct byteregs *)&tf->tf_bx;

    	if (!xmode) {
		if (vflag) dump_regs(tf);
		fatal ("int16 func 0x%x only supported in X mode\n", b->tf_ah);
    	}
	switch( b->tf_ah) {
	case 0x00:
	case 0x10: /* Get enhanced keystroke */
		poll_cnt = 16;
		while (KbdEmpty())
		    tty_pause();
		c = KbdRead();
		b->tf_al = c & 0xff;
		b->tf_ah = (c >> 8) & 0xff;
		break;
	case 0x01: /* Get keystroke */
	case 0x11: /* Get enhanced keystroke */
		sleep_poll();

		if (KbdEmpty()) {
		    tf->tf_eflags |= PSL_Z;
		    break;
		}
		tf->tf_eflags &= ~PSL_Z;
		c = KbdPeek();
		b->tf_al = c & 0xff;
		b->tf_ah = (c >> 8) & 0xff;
		break;
	case 0x02:
		b->tf_al = tty_state();
		break;
	case 0x05:
		KbdWrite(tf->tf_cx);
		break;
    	case 0x12:
    	    	b->tf_al = tty_state();
    	    	b->tf_ah = tty_estate();
		break;
    	case 0x03:	/* Set typematic and delay rate */
		break;
	default:
		unknown_int2(0x16, b->tf_ah, tf);
		break;
	}
}
