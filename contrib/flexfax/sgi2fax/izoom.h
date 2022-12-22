/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/sgi2fax/izoom.h,v 1.1.1.1 1994/01/14 23:10:44 torek Exp $
/*
 * Copyright (c) 1990, 1991, 1992, 1993 Sam Leffler
 * Copyright (c) 1991, 1992, 1993 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */
#ifndef IZOOMDEF
#define IZOOMDEF

#define IMPULSE         1
#define BOX             2
#define TRIANGLE        3
#define QUADRATIC       4
#define MITCHELL        5

typedef struct FILTER {
    int n, totw;
    short *dat;
    short *w;
} FILTER;

typedef struct zoom {
    int (*getfunc)();
    short *abuf;
    short *bbuf;
    int anx, any;
    int bnx, bny;
    short **xmap;
    int type;
    int curay;
    int y;
    FILTER *xfilt, *yfilt;      /* stuff for fitered zoom */
    short *tbuf;
    int nrows, clamp, ay;
    short **filtrows;
    int *accrow;
} zoom;

zoom *newzoom();

#endif /* IZOOMDEF */
