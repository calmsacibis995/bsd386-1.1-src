/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/Class1.c++,v 1.1.1.1 1994/01/14 23:09:45 torek Exp $
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

/*
 * EIA/TIA-578 (Class 1) Modem Driver.
 */
#include "Class1.h"
#include "ModemConfig.h"
#include "HDLCFrame.h"
#include "t.30.h"
#include "tiffio.h"

#include <stdlib.h>
#include <ctype.h>

const char* Class1Modem::modulationNames[6] = {
    "v.21, chan 2",
    "v.27ter fallback mode",
    "v.27ter",
    "v.29",
    "v.17",
    "v.33",
};
/*
 * Modem capabilities are queried at startup and a
 * table based on this is created for each modem.
 * This information is used to negotiate T.30 session
 * parameters (e.g. signalling rate).
 *
 * NB: v.17 w/ long training is the same as v.33, at
 *     least at 12000 and 14400.
 */
const Class1Cap Class1Modem::basicCaps[15] = {
    {  3,  0,	 	0,		     V21, FALSE }, // v.21
    {  24, BR_2400,	DCSSIGRATE_2400V27,  V27, FALSE }, // v.27 ter
    {  48, BR_4800,	DCSSIGRATE_4800V27,  V27, FALSE }, // v.27 ter
    {  72, BR_7200,	DCSSIGRATE_7200V29,  V29, FALSE }, // v.29
    {  73, BR_7200,	DCSSIGRATE_7200V17,  V17, FALSE }, // v.17
    {  74, BR_7200,	DCSSIGRATE_7200V17,  V17, FALSE }, // v.17 w/st
    {  96, BR_9600,	DCSSIGRATE_9600V29,  V29, FALSE }, // v.29
    {  97, BR_9600,	DCSSIGRATE_9600V17,  V17, FALSE }, // v.17
    {  98, BR_9600,	DCSSIGRATE_9600V17,  V17, FALSE }, // v.17 w/st
    { 121, BR_12000,	DCSSIGRATE_12000V33, V33, FALSE }, // v.33
    { 121, BR_12000,	DCSSIGRATE_12000V17, V17, FALSE }, // v.17
    { 122, BR_12000,	DCSSIGRATE_12000V17, V17, FALSE }, // v.17 w/st
    { 145, BR_14400,	DCSSIGRATE_14400V33, V33, FALSE }, // v.33
    { 145, BR_14400,	DCSSIGRATE_14400V17, V17, FALSE }, // v.17
    { 146, BR_14400,	DCSSIGRATE_14400V17, V17, FALSE }, // v.17 w/st
};
#define	NCAPS	(sizeof (basicCaps) / sizeof (basicCaps[0]))

Class1Modem::Class1Modem(FaxServer& s, const ModemConfig& c) : FaxModem(s,c)
{
    group3opts = 0;
    memcpy(xmitCaps, basicCaps, sizeof (basicCaps));
    memcpy(recvCaps, basicCaps, sizeof (basicCaps));
}

Class1Modem::~Class1Modem()
{
}

/*
 * Check if the modem is a Class 1 modem and,
 * if so, configure it for use.
 */
fxBool
Class1Modem::setupModem()
{
    if (!selectBaudRate(conf.maxRate, conf.flowControl, conf.flowControl))
	return (FALSE);
    // Query service support information
    if (atQuery("+FCLASS=?", modemServices))
	traceBits(modemServices & SERVICE_ALL, serviceNames);
    if ((modemServices & SERVICE_CLASS1) == 0)
	return (FALSE);
    atCmd(conf.class1Cmd);

    /*
     * Query manufacturer, model, and firmware revision.
     * We use the manufacturer especially as a key to
     * working around firmware bugs (yech!).
     */
    if (setupManufacturer(modemMfr)) {
	modemCapability("Mfr \"%s\"", (char*) modemMfr);
	modemMfr.raisecase();
    }
    (void) setupModel(modemModel);
    (void) setupRevision(modemRevision);
    if (modemModel != "")
	modemCapability("Model \"%s\"", (char*) modemModel);
    if (modemRevision != "")
	modemCapability("Revision \"%s\"", (char*) modemRevision);

    /*
     * Get modem capabilities and calculate best signalling
     * rate, data formatting capabilities, etc. for use in
     * T.30 negotiations.
     */
    if (!class1Query("+FTM=?", xmitCaps)) {
	serverTrace("Error parsing \"+FTM\" query response: \"%s\"", rbuf);
	return (FALSE);
    }
    modemParams.br = 0;
    u_int i;
    for (i = 1; i < NCAPS; i++)
	if (xmitCaps[i].ok)
	    modemParams.br |= BIT(xmitCaps[i].br);
    modemParams.vr = VR_ALL;
    modemParams.wd = BIT(WD_1728) | BIT(WD_2048) | BIT(WD_2432);
    modemParams.ln = LN_ALL;
    modemParams.df = BIT(DF_1DMR) | BIT(DF_2DMR);
    modemParams.ec = 0;
    modemParams.bf = 0;
    modemParams.st = ST_ALL;
    traceModemParams();
    /*
     * Receive capabilities are maintained separately from
     * transmit capabilities because we need to know more
     * than the signalling rate to formulate the DIS.
     */ 
    if (!class1Query("+FRM=?", recvCaps)) {
	serverTrace("Error parsing \"+FRM\" query response: \"%s\"", rbuf);
	return (FALSE);
    }
    u_int mods = 0;
    for (i = 1; i < NCAPS; i++)
	if (recvCaps[i].ok)
	    mods |= BIT(recvCaps[i].mod);
    switch (mods) {
    case BIT(V27FB):
	discap = DISSIGRATE_V27FB;
	break;
    case BIT(V27):
	discap = DISSIGRATE_V27;
	break;
    case BIT(V29):
	discap = DISSIGRATE_V29;
	break;
    case BIT(V27)|BIT(V29):
	discap = DISSIGRATE_V27|DISSIGRATE_V29;
	break;
    case BIT(V27)|BIT(V29)|BIT(V33):
	discap = 0xE;
	break;
    case BIT(V27)|BIT(V29)|BIT(V33)|BIT(V17):
	discap = 0xD;
	break;
    }
    frameRev = TIFFGetBitRevTable(conf.hostFillOrder != conf.frameFillOrder);

    setupClass1Parameters();
    return (TRUE);
}

/*
 * Send the modem the Class 1 configuration commands.
 */
fxBool
Class1Modem::setupClass1Parameters()
{
    if (modemServices & SERVICE_CLASS1) {
	atCmd(conf.class1Cmd);
	if (conf.setupAACmd != "")
	    atCmd(conf.setupAACmd);
    }
    return (TRUE);
}

/*
 * Set the local subscriber identification.
 */
void
Class1Modem::setLID(const fxStr& number)
{
    encodeTSI(lid, number);
}

/*
 * Construct a binary TSI/CIG string for transmission.  This
 * is encoded as a string of hex digits according to Table 3/T.30
 * (see the spec).  Hyphen ('-') and period are converted to space;
 * otherwise invalid characters are ignored in the conversion.
 * The string may be at most 20 characters (according to the spec).
 */
void
Class1Modem::encodeTSI(fxStr& binary, const fxStr& ascii)
{
    u_char buf[20];
    u_int n = fxmin(ascii.length(),(u_int) 20);
    const u_char* bitrev =
	TIFFGetBitRevTable(conf.hostFillOrder != FILLORDER_LSB2MSB);
    for (u_int i = 0, j = 0; i < n; i++) {
	for (const char* dp = "00112233445566778899  ++- . "; *dp; dp += 2)
	    if (dp[0] == ascii[i]) {
		buf[j++] = bitrev[dp[1]];
		break;
	    }
    }
    /*
     * Now ``reverse copy'' the string.
     */
    binary.resize(20);
    for (i = 0; j > 0; i++, j--)
	binary[i] = buf[j-1];
    for (; i < 20; i++)
	binary[i] = bitrev[' '];	// blank pad remainder
}

/*
 * Do the inverse of encodeTSI; i.e. convert a binary
 * string of encoded digits into the equivalent ascii.
 */
void
Class1Modem::decodeTSI(fxStr& ascii, const HDLCFrame& binary)
{
    int n = binary.getFrameDataLength();
    if (n > 20)			// spec says no more than 20 digits
	n = 20;
    ascii.resize(n);
    const u_char* bitrev =
	TIFFGetBitRevTable(conf.hostFillOrder != FILLORDER_LSB2MSB);
    u_int d = 0;
    fxBool seenDigit = FALSE;
    for (const u_char* cp = binary.getFrameData() + n-1; n > 0; cp--, n--) {
	/*
	 * Accept any printable ascii.
	 */
	u_char c = bitrev[*cp];
        if (isprint(c) || c == ' ') {
	    if (c != ' ')
		seenDigit = TRUE;
	    if (seenDigit)
		ascii[d++] = c;
	}
    }
    ascii.resize(d);
}

/*
 * Pass data to modem, filtering DLE's and
 * optionally including the end-of-data
 * marker <DLE><ETX>.
 */
fxBool
Class1Modem::sendClass1Data(const u_char* data, u_int cc,
    const u_char* bitrev, fxBool eod)
{
    if (!putModemDLEData(data, cc, bitrev, getDataTimeout()))
	return (FALSE);
    if (eod) {
	u_char buf[2];
	buf[0] = DLE;
	buf[1] = ETX;
	return (putModemData(buf, 2));
    } else
	return (TRUE);
}

/*
 * Receive timed out, abort the operation
 * and resynchronize before returning.
 */
void
Class1Modem::abortReceive()
{
    fxBool b = wasTimeout();
    char c = CAN;			// anything other than DC1/DC3
    putModem(&c, 1, 1);
    /*
     * If the modem handles abort properly, then just
     * wait for an OK result.  Otherwise, wait a short
     * period of time, flush any input, and then send
     * "AT" and wait for the return "OK".
     */
    if (conf.class1RecvAbortOK == 0) {	// modem doesn't send OK response
	pause(200);
	flushModemInput();
	(void) atCmd("", AT_OK, 100);
    } else
	(void) waitFor(AT_OK, conf.class1RecvAbortOK);
    setTimeout(b);			// XXX putModem clobbers timeout state
}

/*
 * Receive an HDLC frame.  The timeout is against
 * the receipt of the HDLC flags; the frame itself must
 * be received within 3 seconds (per the spec).
 * If a carrier is received, but the complete frame
 * is not received before the timeout, the receive
 * is aborted.
 */
fxBool
Class1Modem::recvRawFrame(HDLCFrame& frame)
{
    int c;
    /*
     * Search for HDLC frame flags.  The
     * timeout is to reception of the flags.
     */
    do {
	c = getModemChar(0);
    } while (c != EOF && c != 0xff);
    stopTimeout("waiting for HDLC flags");
    if (c == 0xff) {			// flags received
	/*
	 * The spec says that a frame that takes between
	 * 2.55 and 3.45 seconds to be received may be
	 * discarded; we use 3.1 seconds as a compromise.
	 */
	startTimeout(3100);
	do {
	    if (c == DLE) {
		c = getModemChar(0);
		if (c == ETX || c == EOF)
		    break;
	    }
	    frame.put(frameRev[c]);
	} while ((c = getModemChar(0)) != EOF);
	stopTimeout("receiving HDLC frame data");
    }
    if (wasTimeout()) {
	abortReceive();
	return (FALSE);
    }
    traceHDLCFrame("-->", frame);
    /*
     * Now collect the "OK", "ERROR", or "FCERROR"
     * response telling whether or not the FCS was
     * legitimate.
     */
    if (!waitFor(AT_OK)) {
	if (lastResponse == AT_FCERROR)
	    protoTrace("FCS error");
	return (FALSE);
    }
    if (frame.getFrameDataLength() < 1) {
	protoTrace("HDLC frame too short (%u bytes)", frame.getLength());
	return (FALSE);
    }
    if ((frame[1]&0xf7) != 0xc0) {
	protoTrace("HDLC frame with bad control field %#x", frame[1]);
	return (FALSE);
    }
    frame.setOK(TRUE);
    return (TRUE);
}

#include "StackBuffer.h"

/*
 * Log an HLDC frame along with a time stamp (secs.10ms).
 */
void
Class1Modem::traceHDLCFrame(const char* direction, const HDLCFrame& frame)
{
    if (!getHDLCTracing())
	return;
    const char* hexdigits = "0123456789ABCDEF";
    fxStackBuffer buf;
    for (u_int i = 0; i < frame.getLength(); i++) {
	u_char b = frame[i];
	if (i > 0)
	    buf.put(' ');
	buf.put(hexdigits[b>>4]);
	buf.put(hexdigits[b&0xf]);
    }
    protoTrace("%s HDLC<%u:%.*s>", direction,
	frame.getLength(), buf.getLength(), (char*) buf);
}

/*
 * Send a raw HDLC frame and wait for the modem response.
 * The modem is expected to be an HDLC frame sending mode
 * (i.e. +FTH=3 or similar has already be sent to the modem).
 * The T.30 max frame length is enforced with a 3 second
 * timeout on the send.
 */
fxBool
Class1Modem::sendRawFrame(HDLCFrame& frame)
{
    traceHDLCFrame("<--", frame);
    if (frame.getLength() < 3) {
	protoTrace("HDLC frame too short (%u bytes)", frame.getLength());
	return (FALSE);
    }
    if (frame[0] != 0xff) {
	protoTrace("HDLC frame with bad address field %#x", frame[0]);
	return (FALSE);
    }
    if ((frame[1]&0xf7) != 0xc0) {
	protoTrace("HDLC frame with bad control field %#x", frame[1]);
	return (FALSE);
    }
    static u_char buf[2] = { DLE, ETX };
    return (putModemDLEData(frame, frame.getLength(), frameRev, 0) &&
	putModem(buf, 2, 0) &&
	waitFor(frame.moreFrames() ? AT_CONNECT : AT_OK, 0));
}

/*
 * Send a single byte frame.
 */
fxBool
Class1Modem::sendFrame(u_char fcf, fxBool lastFrame)
{
    HDLCFrame frame(conf.class1FrameOverhead);
    frame.put(0xff);
    frame.put(lastFrame ? 0xc8 : 0xc0);
    frame.put(fcf);
    return (sendRawFrame(frame));
}

/*
 * Send a frame with DCS/DIS.
 */
fxBool
Class1Modem::sendFrame(u_char fcf, u_int dcs, fxBool lastFrame)
{
    HDLCFrame frame(conf.class1FrameOverhead);
    frame.put(0xff);
    frame.put(lastFrame ? 0xc8 : 0xc0);
    frame.put(fcf);
    frame.put(dcs>>24);
    frame.put(dcs>>16);
    frame.put(dcs>>8);
    if (dcs&(DCS_XTNDFIELD<<8))
	frame.put(dcs);
    return (sendRawFrame(frame));
}

/*
 * Send a frame with TSI/CSI.
 */
fxBool
Class1Modem::sendFrame(u_char fcf, const fxStr& tsi, fxBool lastFrame)
{
    HDLCFrame frame(conf.class1FrameOverhead);
    frame.put(0xff);
    frame.put(lastFrame ? 0xc8 : 0xc0);
    frame.put(fcf);
    frame.put((u_char*)(char*)tsi, tsi.length());
    return (sendRawFrame(frame));
}

fxBool
Class1Modem::transmitFrame(u_char fcf, fxBool lastFrame)
{
    startTimeout(2550);			// 3.0 - 15% = 2.55 secs
    fxBool frameSent =
	class1Cmd("TH", 3, AT_NOTHING) &&
	atResponse(rbuf, 0) == AT_CONNECT &&
	sendFrame(fcf, lastFrame);
    stopTimeout("sending HDLC frame");
    return (frameSent);
}

fxBool
Class1Modem::transmitFrame(u_char fcf, u_int dcs, fxBool lastFrame)
{
    /*
     * The T.30 spec says no frame can take more than 3 seconds
     * (+/- 15%) to transmit.  We take the conservative approach.
     * and guard against the send exceeding the lower bound.
     */
    startTimeout(2550);			// 3.0 - 15% = 2.55 secs
    fxBool frameSent =
	class1Cmd("TH", 3, AT_NOTHING) &&
	atResponse(rbuf, 0) == AT_CONNECT &&
	sendFrame(fcf, dcs, lastFrame);
    stopTimeout("sending HDLC frame");
    return (frameSent);
}

fxBool
Class1Modem::transmitFrame(u_char fcf, const fxStr& tsi, fxBool lastFrame)
{
    startTimeout(3000);			// give more time than others
    fxBool frameSent =
	class1Cmd("TH", 3, AT_NOTHING) &&
	atResponse(rbuf, 0) == AT_CONNECT &&
	sendFrame(fcf, tsi, lastFrame);
    stopTimeout("sending HDLC frame");
    return (frameSent);
}

/*
 * Send data using the specified signalling rate.
 */
fxBool
Class1Modem::transmitData(int br, u_char* data, u_int cc,
    const u_char* bitrev, fxBool eod)
{
    if (flowControl == FLOW_XONXOFF)
	setXONXOFF(FLOW_XONXOFF, FLOW_NONE, ACT_FLUSH);
    fxBool ok = (class1Cmd("TM", br, AT_CONNECT) &&
	sendClass1Data(data, cc, bitrev, eod) &&
	(eod ? waitFor(AT_OK) : TRUE));
    if (flowControl == FLOW_XONXOFF)
	setXONXOFF(FLOW_NONE, FLOW_NONE, ACT_DRAIN);
    return (ok);
}

/*
 * Receive an HDLC frame.  The timeout is against
 * the receipt of the HDLC flags; the frame itself must
 * be received within 3 seconds (per the spec).
 * If a carrier is received, but the complete frame
 * is not received before the timeout, the receive
 * is aborted.
 */
fxBool
Class1Modem::recvFrame(HDLCFrame& frame, long ms)
{
    frame.reset();
    startTimeout(ms);
    fxBool readPending = class1Cmd("RH", 3, AT_NOTHING);
    if (readPending && waitFor(AT_CONNECT,0))
	return recvRawFrame(frame);		// NB: stops inherited timeout
    stopTimeout("waiting for v.21 carrier");
    if (readPending && wasTimeout())
	abortReceive();
    return (FALSE);
}

/*
 * Receive TCF data using the specified signalling rate.
 */
fxBool
Class1Modem::recvTCF(int br, HDLCFrame& buf, const u_char* bitrev, long ms)
{
    buf.reset();
    if (flowControl == FLOW_XONXOFF)
	setXONXOFF(FLOW_NONE, FLOW_XONXOFF, ACT_DRAIN);
    startTimeout(ms);
    /*
     * Loop waiting for carrier or timeout.
     */
    fxBool readPending, gotCarrier;
    do {
	readPending = class1Cmd("RM", br, AT_NOTHING);
	gotCarrier = readPending && waitFor(AT_CONNECT, 0);
    } while (readPending && !gotCarrier && lastResponse == AT_FCERROR);
    /*
     * If carrier was recognized, collect the data.
     */
    fxBool gotData = FALSE;
    if (gotCarrier) {
	int c = getModemChar(0);		// NB: timeout is to first byte
	stopTimeout("receiving TCF");
	if (c != EOF) {
	    buf.reset();
	    /*
	     * Use a 2 second timer to receive the 1.5
	     * second TCF--perhaps this is too long to
	     * permit us to send the nak in time?
	     */
	    startTimeout(2000);
	    do {
		if (c == DLE) {
		    c = getModemChar(0);
		    if (c == ETX) {
			gotData = TRUE;
			break;
		    }
		}
		buf.put(bitrev[c]);
	    } while ((c = getModemChar(0)) != EOF);
	}
    }
    stopTimeout("receiving TCF");
    /*
     * If the +FRM is still pending, abort it.
     */
    if (readPending && wasTimeout())
	abortReceive();
    if (flowControl == FLOW_XONXOFF)
	setXONXOFF(FLOW_NONE, FLOW_NONE, ACT_DRAIN);
    return (gotData);
}

/* 
 * Modem manipulation support.
 */

/*
 * Reset a Class 1 modem.
 */
fxBool
Class1Modem::reset(long ms)
{
    return (FaxModem::reset(ms) && setupClass1Parameters());
}

ATResponse
Class1Modem::atResponse(char* buf, long ms)
{
    if (FaxModem::atResponse(buf, ms) == AT_OTHER && strneq(buf, "+FCERROR", 8))
	lastResponse = AT_FCERROR;
    return (lastResponse);
}

/*
 * Wait (carefully) for some response from the modem.
 */
fxBool
Class1Modem::waitFor(ATResponse wanted, long ms)
{
    for (;;) {
	ATResponse response = atResponse(rbuf, ms);
	if (response == wanted)
	    return (TRUE);
	switch (response) {
	case AT_TIMEOUT:
	case AT_EMPTYLINE:
	case AT_ERROR:
	case AT_NOCARRIER:
	case AT_NODIALTONE:
	case AT_NOANSWER:
	case AT_OFFHOOK:
	    modemTrace("ERROR: %s", ATresponses[response]);
	    /* fall thru... */
	case AT_FCERROR:
	    return (FALSE);
	}
    }
}

/*
 * Interfaces for sending a Class 1 command; i.e, AT+F<cmd>
 */

/*
 * Send <cmd> and wait for expect
 */
fxBool
Class1Modem::class1Cmd(const char* cmd, ATResponse expect)
{
     return vatFaxCmd(expect, "%s", cmd);
}

/*
 * Send <cmd>=<a0> and wait for expect
 */
fxBool
Class1Modem::class1Cmd(const char* cmd, int a0, ATResponse expect)
{
    return vatFaxCmd(expect, "%s=%u", cmd, a0);
}

/*
 * Send <what>=? and get a range response.
 */
fxBool
Class1Modem::class1Query(const char* what, Class1Cap caps[])
{
    char response[1024];
    if (atCmd(what, AT_NOTHING) && atResponse(response) == AT_OTHER) {
	sync(5000);
	return (parseQuery(response, caps));
    }
    return (FALSE);
}

/*
 * Map the DCS signalling rate to the appropriate
 * Class 1 capability entry.
 */
const Class1Cap*
Class1Modem::findSRCapability(u_short sr, const Class1Cap caps[])
{
    for (u_int i = NCAPS-1; i > 0; i--) {
	const Class1Cap* cap = &caps[i];
	if (cap->sr == sr) {
	    if (cap->mod == V17 && HasShortTraining(cap-1))
		cap--;
	    return (cap);
	}
    }
    // XXX should not happen...
    protoTrace("MODEM: unknown signalling rate %#x, using 9600 v.29", sr);
    return findSRCapability(DCSSIGRATE_9600V29, caps);
}

/*
 * Map the Class 2 bit rate to the best
 * signalling rate capability of the modem.
 */
const Class1Cap*
Class1Modem::findBRCapability(u_short br, const Class1Cap caps[])
{
    for (u_int i = NCAPS-1; i > 0; i--) {
	const Class1Cap* cap = &caps[i];
	if (cap->br == br && cap->ok) {
	    if (cap->mod == V17 && HasShortTraining(cap-1))
		cap--;
	    return (cap);
	}
    }
    protoTrace("MODEM: unsupported baud rate %#x", br);
    return NULL;
}

/*
 * Override the DIS signalling rate capabilities because
 * they are defined from the Class 2 parameter information
 * and do not include the modulation technique.
 */
u_int
Class1Modem::modemDIS() const
{
    // NB: DIS is in 32-bit format
    return (FaxModem::modemDIS() &~ (DIS_SIGRATE<<8)) | (discap<<18);
}

const char COMMA = ',';
const char SPACE = ' ';

/*
 * Parse a Class 1 parameter string.
 */
fxBool
Class1Modem::parseQuery(const char* cp, Class1Cap caps[])
{
    while (cp[0]) {
	if (cp[0] == SPACE) {		// ignore white space
	    cp++;
	    continue;
	}
	if (!isdigit(cp[0]))
	    return (FALSE);
	int v = 0;
	do {
	    v = v*10 + (cp[0] - '0');
	} while (isdigit((++cp)[0]));
	for (u_int i = 0; i < NCAPS; i++)
	    if (caps[i].value == v) {
		caps[i].ok = TRUE;
		break;
	    }
	if (cp[0] == COMMA)		// <item>,<item>...
	    cp++;
    }
    return (TRUE);
}
