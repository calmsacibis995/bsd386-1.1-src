/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxstat/RecvStatus.h,v 1.1.1.1 1994/01/14 23:09:56 torek Exp $
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
#ifndef _FaxRecvStatus_
#define	_FaxRecvStatus_

#include "Array.h"
#include "Str.h"

struct FaxRecvStatus : public fxObj {
    fxStr	date;
    fxStr	time;
    fxStr	sender;
    u_int	npages;
    u_int	pageWidth;
    u_int	pageLength;
    float	resolution;
    int		beingReceived;
    fxStr	host;

    FaxRecvStatus();
    ~FaxRecvStatus();

    fxBool scan(const char* buf);
    fxBool parse(const char*);

    int compare(const FaxRecvStatus* other) const;

    void print(FILE*, fxBool showHost);
    static void printHeader(FILE*, fxBool showHost);
};
fxDECLARE_ObjArray(FaxRecvStatusArray, FaxRecvStatus);
#endif /* _FaxRecvStatus_ */
