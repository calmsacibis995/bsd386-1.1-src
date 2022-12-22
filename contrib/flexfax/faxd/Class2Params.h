/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/Class2Params.h,v 1.1.1.1 1994/01/14 23:09:43 torek Exp $
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
#ifndef _Class2Params_
#define	_Class2Params_
/*
 * Class 2 T.30 parameter wrapper.
 */
#include "class2.h"
#include "Str.h"

struct Class2Params {
    u_int vr;		// vertical resolution (VR_*)
    u_int br;		// bit rate (BR_*)
    u_int wd;		// page width (WD_*)
    u_int ln;		// page length (LN_*)
    u_int df;		// data format (DF_*)
    u_int ec;		// error correction protocol (EC_*)
    u_int bf;		// binary file transfer protocol (BF_*)
    u_int st;		// minimum scanline time (ST_*)

// tables for mapping Class 2 codes to T.30 DIS/DCS codes
    static u_int vrDISTab[2];		// vertical resolution
    static u_int dfDISTab[4];		// data compression format
    static u_int wdDISTab[8];		// page width
    static u_int lnDISTab[3];		// page length
    static u_int stDISTab[8];		// min scanline time (DIS specific)
    static u_int stDCSTab[8];		// min scanline time (DCS specific)
    static u_int brDISTab[8];		// bit rate (DIS specific)
    static u_int brDCSTab[8];		// bit rate (DCS specific)
// tables for mapping T.30 DIS/DCS values to Class 2 codes
    static u_int DISvrTab[2];		// vertical resolution
    static u_int DISdfTab[2];		// data compression format
    static u_int DISwdTab[4];		// page width
    static u_int DISlnTab[4];		// page length
    static u_int DISstTab[8];		// min scanline time
    static u_int DISbrTab[16];		// DIS best bit rate
    static u_int DCSbrTab[16];		// DIS actual bit rate

// tables for printing class 2 subparameter information
    static const char* bitRateNames[8];		// br codes
    static const char* dataFormatNames[4];	// df codes
    static const char* vresNames[2];		// vr codes
    static const char* pageWidthNames[8];	// wd codes
    static const char* pageLengthNames[4];	// ln codes
    static const char* scanlineTimeNames[8];	// st codes

    Class2Params();

    int operator==(const Class2Params&) const;
    int operator!=(const Class2Params&) const;

    fxStr cmd() const;	// format AT+F string for current parameters
    void setFromDIS(u_int dis, u_int xinfo = 0);
    void setFromDCS(u_int dcs, u_int xinfo = 0);
    u_int getDCS() const;
    fxBool is2D() const;

    u_int transferSize(u_int ms) const;
    u_int minScanlineSize() const;
};
#endif /* _Class2Params_ */
