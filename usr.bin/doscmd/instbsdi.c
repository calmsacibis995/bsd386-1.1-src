/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
#include <dos.h>
#include <string.h>

/*	Krystal $Id: instbsdi.c,v 1.1 1994/01/14 23:31:07 polk Exp $ */

main(int ac, char **av)
{
    union REGS in, out, tmp;
    struct SREGS seg, stmp;

    memset(&out, 0, sizeof(out));
    out.h.ah = 0x52;
    int86x(0x21, &out, &tmp, &stmp);

    seg.es = stmp.es;
    in.x.di = tmp.x.bx;

    out.x.ax = 0x5D06;
    int86x(0x21, &out, &tmp, &stmp);

    seg.ds = stmp.ds;
    in.x.si = tmp.x.si;

    int86x(0xff, &in, &out, &seg);
}
