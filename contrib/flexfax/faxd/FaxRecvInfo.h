/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/FaxRecvInfo.h,v 1.1.1.1 1994/01/14 23:09:44 torek Exp $
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
#ifndef _FaxRecvInfo_
#define	_FaxRecvInfo_
/*
 * Wrapper class for passing information
 * about a received file from the FaxServer
 * to the faxServerApp.
 */
#include "Array.h"
#include "Str.h"

class FaxRecvInfo : public fxObj {
public:
    fxStr	qfile;		// file containing data
    u_short	npages;		// number of total pages
    u_short	pagewidth;	// page width (pixels)
    u_short	sigrate;	// signalling rate
    float	pagelength;	// page length (mm)
    float	resolution;	// resolution (dpi)
    fxStr	protocol;	// communication protocol
    fxStr	sender;		// sender's TSI
    float	time;		// time on the phone
    fxStr	reason;		// reason for failure

    FaxRecvInfo();
    ~FaxRecvInfo();
};
fxDECLARE_ObjArray(FaxRecvInfoArray, FaxRecvInfo);
#endif /* _FaxRecvInfo_ */
