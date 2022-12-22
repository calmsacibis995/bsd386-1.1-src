/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/Class1Poll.c++,v 1.1.1.1 1994/01/14 23:09:45 torek Exp $
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
#include "Class1.h"
#include "ModemConfig.h"
#include "HDLCFrame.h"
#include "t.30.h"

fxBool
Class1Modem::requestToPoll()
{
    return TRUE;
}

fxBool
Class1Modem::pollBegin(const fxStr& pollID, fxStr& emsg)
{
    fxStr cig;
    u_int dtc = modemDIS();
    encodeTSI(cig, pollID);

    setInputBuffering(FALSE);
    prevPage = FALSE;				// no previous page received
    pageGood = FALSE;				// quality of received page

    return class1Cmd("TH", 3, AT_NOTHING) &&
	atResponse(rbuf, 2550) == AT_CONNECT &&
	recvIdentification(FCF_CIG, cig, FCF_DTC, dtc, conf.t1Timer, emsg);
}
