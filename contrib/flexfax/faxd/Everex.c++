/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/Everex.c++,v 1.1.1.1 1994/01/14 23:09:46 torek Exp $
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
#include "Everex.h"
#include "ModemConfig.h"

#include "everex.h"
#include "t.30.h"

#include <stdlib.h>

/*
 * Everex Modem Driver.
 */
EverexModem::EverexModem(FaxServer& s, const ModemConfig& c) : FaxModem(s,c)
{
    type = UNKNOWN;
}

EverexModem::~EverexModem()
{
}

fxBool
EverexModem::dataService()
{
    return (FALSE);
}

#define	isCapable(what)		(capabilities & BIT(what))

fxBool
EverexModem::setupModem()
{
    if (!selectBaudRate(EVEREX_CMDBRATE, FLOW_XONXOFF, FLOW_XONXOFF))
	return (FALSE);
    type = UNKNOWN;
    if (atCmd("I4?", AT_NOTHING) && atResponse(rbuf) == AT_OTHER) {
	modemModel = rbuf;
	type = (modemModel == "EV958") ? EV958
	     : (modemModel == "EV968") ? EV968
	     :			     	 ABATON;	// generic
	modemMfr = "Everex";
	(void) sync(500);
    }
    if (type == UNKNOWN)
	return (FALSE);
    if (conf.flowControl != FLOW_XONXOFF) {
	serverTrace("Sorry, Everex driver requires XON/XOFF flow control");
	return (FALSE);
    }
    // get modem capabilities bit mask
    u_int capabilities;
    if (atCmd("#S3?", AT_NOTHING) && atResponse(rbuf) == AT_OTHER) {
	capabilities = fromHex(rbuf);
	(void) sync(500);
    } else
	return (FALSE);
    if (atCmd("I7?", AT_NOTHING) && atResponse(rbuf) == AT_OTHER) {
	modemRevision = rbuf;
	(void) sync(500);
    }
    modemServices = SERVICE_DATA|SERVICE_CLASS1;	// XXX bit of a cheat
    modemParams.vr = VR_ALL;
    modemParams.br = 0;
    if (isCapable(S4_9600V29))
	modemParams.br |= BIT(BR_9600);
    if (isCapable(S4_7200V29))
	modemParams.br |= BIT(BR_7200);
    if (isCapable(S4_4800V27) || isCapable(S4_4800V29))
	modemParams.br |= BIT(BR_4800);
    if (isCapable(S4_2400V27))
	modemParams.br |= BIT(BR_2400);
    modemParams.wd = BIT(WD_1728) | BIT(WD_2048) | BIT(WD_2432);
    modemParams.ln = LN_ALL;
    modemParams.df = BIT(DF_1DMR) | BIT(DF_2DMR);
    modemParams.ec = 0;
    modemParams.bf = 0;
    modemParams.st = ST_ALL;
    traceBits(modemServices, serviceNames);
    traceModemParams();
    return TRUE;
}

fxBool
EverexModem::supportsEOLPadding() const	
{
    return TRUE;
}

void
EverexModem::setLID(const fxStr& number)
{
    encodeCSI(lid, number);
}

/*
 * Construct a Calling Station Identifier (CSI) string
 * for the modem.  This is encoded as a string of hex digits
 * according to Table 3/T.30 (see the spec).  Hyphen ('-')
 * and period are converted to space; otherwise invalid
 * characters are ignored in the conversion.  The string may
 * be at most 20 characters (according to the spec).
 */
void
EverexModem::encodeCSI(fxStr& binary, const fxStr& ascii)
{
    char buf[40];
    const char* hexDigits = "0123456789ABCDEF";
    u_int n = fxmin(ascii.length(),(u_int) 20);
    for (u_int i = 0, j = 0; i < n; i++) {
	int c = ascii[i];
	for (const u_char* dp = digitMap; *dp; dp += 2)
	    if (c == dp[1]) {
		buf[j++] = hexDigits[dp[0]>>4];
		buf[j++] = hexDigits[dp[0]&0xf];
		break;
	    }
    }
    /*
     * Now ``reverse copy'' the string.
     */
    binary.resize(40);
    for (i = 0; j > 1; i += 2, j -= 2) {
	binary[i] = buf[j-2];
	binary[i+1] = buf[j-1];
    }
    for (; i < 40; i+= 2)
	binary[i] = '0', binary[i+1] = '4';	// blank pad remainder
}

/*
 * Do the inverse of encodeCSI; i.e. convert a string of
 * hex digits from the modem into the equivalent ASCII.
 */
void
EverexModem::decodeCSI(fxStr& ascii, const fxStr& binary)
{
    int n = binary.length();
    if (n > 40)		// spec says no more than 20 digits
	n = 40;
    ascii.resize((n+1)/2);
    u_int d = 0;
    fxBool seenDigit = FALSE;
    for (char* cp = (char *)binary + n-2; n > 1; cp -= 2, n -= 2) {
       int digit = fromHex(cp, 2);
       for (const u_char* dp = digitMap; *dp != '\0'; dp += 2)
	    if (dp[0] == digit) {
		if (dp[1] != ' ')
		    seenDigit = TRUE;
		if (seenDigit)
		    ascii[d++] = dp[1];
		break;
	    }
    }
    ascii.resize(d);
}

fxBool
EverexModem::sendFrame(int f1)
{
    return (atCmd("#FT=" | toHex(f1,2)));
}

fxBool
EverexModem::sendFrame(int f1, int f2)
{
    return (atCmd("#FT=" | toHex(f1,2) | toHex(f2,2)));
}

fxBool
EverexModem::sendFrame(int f1, int f2, int f3)
{
    return (atCmd("#FT=" | toHex(f1,2) | toHex(f2,2) | toHex(f3,2)));
}

fxBool
EverexModem::setupFrame(int f, int v)
{
    return (atCmd("#FT" | toHex(f, 2) | "=" | toHex(v, 8)));
}

fxBool
EverexModem::setupFrame(int f, const char* v)
{
    return (atCmd("#FT" | toHex(f, 2) | "=" | v));
}

fxBool
EverexModem::recvFrame(char* buf, long ms)
{
    return (getModemLine(buf, ms) > 2 && strneq(buf, "R ", 2));
}

/* 
 * Modem manipulation support.
 */

fxBool
EverexModem::reset(long ms)
{
    // &C0	DCD always on
    // &D3	force modem reset when DTR is toggled
    // #V	enable verbose codes
    // #Z	reset fax state
    return (FaxModem::reset(ms) && atCmd("&C0&D3#V#Z"));
}

fxBool
EverexModem::abort(long ms)
{
    // XXX maybe should not always send DCN
    sendFrame(FCF_DCN|FCF_SNDR);		// terminate session
    return (atCmd("#ZH0", AT_OK, ms));
}

fxBool
EverexModem::modemFaxConfigure(int bits)
{
    return (atCmd("#S2=" | fxStr(bits, "%d")));
}
