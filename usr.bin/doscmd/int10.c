/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
 
/*	Krystal $Id: int10.c,v 1.1 1994/01/14 23:31:41 polk Exp $ */

#include "doscmd.h"

/*
 * 0040:0060 contains the start and end of the cursor
 */                 
#define curs_end (*(u_char *)0x460)
#define curs_start (*(u_char *)0x461)

void
int10 (tf)
struct trapframe *tf;
{
	char *addr;
	struct byteregs *b = (struct byteregs *)&tf->tf_bx;
	int i, j;
	int sr, sc;

	/*
	 * Any call to the video BIOS is enough to reset the poll
	 * count on the keyboard.
	 */
	reset_poll();

	switch (b->tf_ah) {
	case 0x00: /* Set display mode */
		debug(D_HALF, "Set video mode to %02x\n", b->tf_al);
		break;
	case 0x01: /* Define cursor */
		curs_start = b->tf_ch;
		curs_end = b->tf_cl;
		break;
	case 0x02: /* Position cursor */
		if (!xmode)
			goto unsupported;
		tty_move(b->tf_dh, b->tf_dl);
		break;
	case 0x03: /* Read cursor position */
		if (!xmode)
			goto unsupported;
		tty_report(&i, &j);
		b->tf_dh = i;
		b->tf_dl = j;
		b->tf_ch = curs_start;
		b->tf_cl = curs_end;
		break;
	case 0x05:
		debug(D_HALF, "Select current display page %d\n", b->tf_al);
		break;
	case 0x06: /* initialize window/scroll text upward */
		if (!xmode)
			goto unsupported;
		tty_scroll(b->tf_ch, b->tf_cl, b->tf_dh, b->tf_dl,
			   b->tf_al, b->tf_bh << 8);
		break;
	case 0x07: /* initialize window/scroll text downward */
		if (!xmode)
			goto unsupported;
		tty_rscroll(b->tf_ch, b->tf_cl, b->tf_dh, b->tf_dl,
			   b->tf_al, b->tf_bh << 8);
		break;
	case 0x08: /* read character/attribute */
		if (!xmode)
			goto unsupported;
		i = tty_char(-1, -1);
		b->tf_al = i & 0xff;
		b->tf_ah = i >> 8;
		break;
	case 0x09: /* write character/attribute */
		if (!xmode)
			goto unsupported;
		for (i = 0; i < tf->tf_cx; ++i)
			tty_write(b->tf_al, b->tf_bl << 8);
		break;
	case 0x0a: /* write character */
		if (!xmode)
			goto unsupported;
		for (i = 0; i < tf->tf_cx; ++i)
			tty_write(b->tf_al, -1);
		break;
	case 0x0b: /* set border color */
		if (!xmode)
			goto unsupported;
		video_setborder(b->tf_bl);
		break;
	case 0x0e: /* write character */
		tty_write(b->tf_al, -1);
		break;
	case 0x0f: /* get display mode */
		b->tf_ah = 80; /* number of columns */
		b->tf_al = 3; /* color */
		b->tf_bh = 0; /* display page */
		break;
	case 0x10:
		switch (b->tf_al) {
		case 0x01:
			video_setborder(b->tf_bh & 0x0f);
			break;
		case 0x02:		/* Set pallete registers */
			debug(D_HALF, "INT 10 10:02 Set all palette registers\n");
			break;
		case 0x03:		/* Enable/Disable blinking mode */
			video_blink(b->tf_bl ? 1 : 0);
			break;
		case 0x13:
			debug(D_HALF,
			      "INT 10 10:13 Select color or DAC (%02x, %02x)\n",
				      b->tf_bl, b->tf_bh);
			break;
		default:
			unknown_int3(0x10, 0x10, b->tf_al, tf);
			break;
		}
		break;
#if 1
	case 0x11:
		switch (b->tf_al) {
		case 0x00: printf("Tried to load user defined font.\n"); break;
		case 0x01: printf("Tried to load 8x14 font.\n"); break;
		case 0x02: printf("Tried to load 8x8 font.\n"); break;
		case 0x03: printf("Tried to activate character set\n"); break;
		case 0x04: printf("Tried to load 8x16 font.\n"); break;
		case 0x10: printf("Tried to load and activate user defined font\n"); break;
		case 0x11: printf("Tried to load and activate 8x14 font.\n"); break;
		case 0x12: printf("Tried to load and activate 8x8 font.\n"); break;
		case 0x14: printf("Tried to load and activate 8x16 font.\n"); break;
		case 0x30:
			addr = (char *)MAKE_ADDR (tf->tf_es, tf->tf_bp);
			tf->tf_cx = 14;
			b->tf_dl = 24;
			switch(b->tf_bh) {
			case 0: *(long *)addr = ivec[0x1f]; break;
			case 1: *(long *)addr = ivec[0x43]; break;
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7: *(long *)addr = 0;
				debug(D_HALF,
				      "INT 10 11:30 Request font address %02x",
				      b->tf_bh);
				break;
			default:
				unknown_int4(0x10, 0x11, 0x30, b->tf_bh, tf);
				break;
			}
			break;
		default:
			unknown_int3(0x10, 0x11, b->tf_al, tf);
			break;
		}
		break;
#endif
	case 0x12: /* Load multiple DAC color register */
		if (!xmode)
			goto unsupported;
		switch (b->tf_bl) {
		case 0x10:	/* Read EGA/VGA config */
			b->tf_bh = 0;	/* Color */
			b->tf_bl = 0;	/* 64K */
			break;
		default:
			goto unknown;
		}
		break;
	case 0x13: /* write character string */
		if (!xmode)
			goto unsupported;
                addr = (char *)MAKE_ADDR (tf->tf_es, tf->tf_bp);
		switch (b->tf_al & 0x03) {
		case 0:
			tty_report(&sr, &sc);
			tty_move(b->tf_dh, b->tf_dl);
			for (i = 0; i < tf->tf_cx; ++i)
				tty_write(*addr++, b->tf_bl << 8);
			tty_move(sr, sc);
			break;
		case 1:
			tty_move(b->tf_dh, b->tf_dl);
			for (i = 0; i < tf->tf_cx; ++i)
				tty_write(*addr++, b->tf_bl << 8);
			break;
		case 2:
			tty_report(&sr, &sc);
			tty_move(b->tf_dh, b->tf_dl);
			for (i = 0; i < tf->tf_cx; ++i) {
				tty_write(addr[0], addr[1]);
				addr += 2;
			}
			tty_move(sr, sc);
			break;
		case 3:
			tty_move(b->tf_dh, b->tf_dl);
			for (i = 0; i < tf->tf_cx; ++i) {
				tty_write(addr[0], addr[1]);
				addr += 2;
			}
			break;
		}
		break;
	case 0x1a:
		if (!xmode)
			goto unsupported;
		b->tf_al = 0x1a;	/* I am VGA */
		b->tf_bl = 8;		/* Color VGA */
		b->tf_bh = 0;		/* No other card */
		break;

	case 0xef:
	case 0xfe:	/* Get video buffer */
		break;
    	case 0xff:	/* Update real screen from video buffer */
		/* XXX - we should allow secondary buffer here and then
			 update it as the user requests. */
		break;

    	unsupported:
		if (vflag) dump_regs(tf);
		fatal ("int10 function 0x%02x:%02x only available in X mode\n",
			b->tf_ah, b->tf_al);
    	unknown:
	default:
		unknown_int2(0x10, b->tf_ah, tf);
		break;
	}
}
