/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/FaxModem.c++,v 1.1.1.1 1994/01/14 23:09:47 torek Exp $
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
#include <ctype.h>
#include <stdlib.h>

#include "Everex.h"
#include "Class1.h"
#include "Class2.h"
#include "FaxServer.h"
#include "t.30.h"

/*
 * Call status description strings.
 */
const char* FaxModem::callStatus[7] = {
    "Call successful",				// OK
    "Busy signal detected",			// BUSY
    "No carrier detected",			// NOCARRIER
    "No answer from remote",			// NOANSWER
    "No local dialtone",			// NODIALTONE
    "Invalid dialing command",			// ERROR
    "Unknown problem (check modem power)",	// FAILURE
};
/*
 * Service class descriptions.  The first three
 * correspond to the EIA/TIA definitions.  The
 * voice class is for ZyXEL modems.
 */
const char* FaxModem::serviceNames[9] = {
    "\"Data\"",			// SERVICE_DATA
    "\"Class 1\"",		// SERVICE_CLASS1
    "\"Class 2\"",		// SERVICE_CLASS2
    "",				// 3
    "",				// 4
    "",				// 5
    "",				// 6
    "",				// 7
    "\"Voice\"",		// SERVICE_VOICE
};
const char* FaxModem::ATresponses[13] = {
    "Nothing",			// AT_NOTHING
    "OK",			// AT_OK
    "Connection established",	// AT_CONNECT
    "No answer or ring back",	// AT_NOANSWER
    "No carrier",		// AT_NOCARRIER
    "No dial tone",		// AT_NODIALTONE
    "Busy",			// AT_BUSY
    "Phone off-hook",		// AT_OFFHOOK
    "Ring",			// AT_RING
    "Command error",		// AT_ERROR
    "<Empty line>",		// AT_EMPTYLINE
    "<Timeout>",		// AT_TIMEOUT
    "<Unknown response>"	// AT_OTHER
};

const u_char FaxModem::digitMap[12*2+1] = {
    0x04, ' ', 0x0C, '0', 0x8C, '1', 0x4C, '2',
    0xCC, '3', 0x2C, '4', 0xAC, '5', 0x6C, '6',
    0xEC, '7', 0x1C, '8', 0x9C, '9', 0xD4, '+',
    '\0'
};

FaxModem::FaxModem(FaxServer& s, const ModemConfig& c)
    : server(s)
    , conf(c)
    , mfrQueryCmd(c.mfrQueryCmd)
    , modelQueryCmd(c.modelQueryCmd)
    , revQueryCmd(c.revQueryCmd)
{
    /*
     * The modem drivers and main server code require:
     *
     * echoOff		command echo disabled
     * verboseResults	verbose command result strings
     * resultCodes	result codes enabled
     * onHook		modem initially on hook (hung up)
     * noAutoAnswer	no auto-answer (we do it manually)
     *
     * In addition the following configuration is included
     * in the reset command set:
     *
     * flowControl	DCE-DTE flow control method
     * setupDTR		DTR management technique
     * setupDCD		DCD management technique
     * pauseTime	time to pause for "," when dialing
     * waitTime		time to wait for carrier when dialing
     *
     * Any other modem-specific configuration that needs to
     * be done at reset time should be implemented by overriding
     * the FaxModem::reset method.
     */
    resetCmds = conf.resetCmds		// prepend to insure our needs
	      | conf.echoOffCmd
	      | conf.noAutoAnswerCmd
	      | conf.verboseResultsCmd
	      | conf.resultCodesCmd
	      | conf.onHookCmd
	      | conf.flowControlCmd
	      | conf.setupDTRCmd
	      | conf.setupDCDCmd
	      | conf.pauseTimeCmd
	      | conf.waitTimeCmd
	      ;
    modemServices = 0;
    rate = BR0;
    flowControl = conf.flowControl;
    iFlow = FLOW_CURRENT;
    oFlow = FLOW_CURRENT;
}

FaxModem::~FaxModem()
{
}

/*
 * Deduce the type of modem supplied to the server
 * and return an instance of the appropriate modem
 * driver class.
 */
FaxModem*
FaxModem::deduceModem(FaxServer& s, const ModemConfig& conf)
{
    fxStr h(conf.type);
    h.raisecase();
    if (h != "CLASS2" && h != "CLASS1" && h != "EVEREX" && h != "ABATON") {
	if (h != "" && h != "UNKNOWN")
	    s.traceStatus(FAXTRACE_SERVER, "MODEM: Unknown type \"%s\" ignored",
		(char*) conf.type);
	h = "UNKNOWN";
    }
    /*
     * Probe for modem using type, if specified; otherwise
     * try Class 2, Class 1, and then old Everex types.
     */
    FaxModem* modem;
    fxBool nothingTried = TRUE;
    if (h == "CLASS2" || h == "UNKNOWN") {
	nothingTried = FALSE;
	modem = new Class2Modem(s, conf);
	if (modem) {
	    if (modem->setupModem())
		return modem;
	    delete modem;
	}
    }
    if (h == "CLASS1" || h == "UNKNOWN") {
	nothingTried = FALSE;
	modem = new Class1Modem(s, conf);
	if (modem) {
	    if (modem->setupModem())
		return modem;
	    delete modem;
	}
    }
    if (h == "EVEREX" || h == "ABATON" || h == "UNKNOWN") {
	nothingTried = FALSE;
	modem = new EverexModem(s, conf);
	if (modem) {
	    if (modem->setupModem())
		return modem;
	    delete modem;
	}
    }
    return (NULL);
}

/*
 * Default methods for modem driver interface.
 */
fxBool
FaxModem::dataService()
{
    return (FALSE);
}

fxBool
FaxModem::voiceService()
{
    return (FALSE);
}

CallStatus
FaxModem::dial(const char* number)
{
    protoTrace("DIAL %s", number);
    char buf[256];
    sprintf(buf, (const char*) conf.dialCmd, number);
    return (atCmd(buf, AT_NOTHING) ? dialResponse() : FAILURE);
}

/*
 * Set of status codes we expect to receive
 * from a modem in response to an A (answer
 * the phone) command.
 */
static const AnswerMsg answerMsgs[] = {
{ "CONNECT FAX",11,
   FaxModem::AT_NOTHING,	FaxModem::OK,	     FaxModem::CALLTYPE_FAX },
{ "CONNECT",     7,
   FaxModem::AT_NOTHING,	FaxModem::OK,	     FaxModem::CALLTYPE_DATA },
{ "NO ANSWER",   9,
   FaxModem::AT_NOTHING,	FaxModem::NOANSWER,  FaxModem::CALLTYPE_ERROR },
{ "NO CARRIER", 10,
   FaxModem::AT_NOTHING,	FaxModem::NOCARRIER, FaxModem::CALLTYPE_ERROR },
{ "NO DIALTONE",11,
   FaxModem::AT_NOTHING,	FaxModem::NODIALTONE,FaxModem::CALLTYPE_ERROR },
{ "ERROR",       5,
   FaxModem::AT_NOTHING,	FaxModem::ERROR,     FaxModem::CALLTYPE_ERROR },
{ "FAX",	 3,
   FaxModem::AT_CONNECT,	FaxModem::OK,	     FaxModem::CALLTYPE_FAX },
{ "DATA",	 4,
   FaxModem::AT_CONNECT,	FaxModem::OK,	     FaxModem::CALLTYPE_DATA },
};
#define	NANSWERS	(sizeof (answerMsgs) / sizeof (answerMsgs[0]))

const AnswerMsg*
FaxModem::findAnswer(const char* s)
{
    for (u_int i = 0; i < NANSWERS; i++)
	if (strneq(s, answerMsgs[i].msg, answerMsgs[i].len))
	    return (&answerMsgs[i]);
    return (NULL);
}

/*
 * Deduce connection kind: fax, data, or voice.
 */
CallType
FaxModem::answerResponse(fxStr& emsg)
{
    CallStatus cs = FAILURE;
    ATResponse r;
    do {
	r = atResponse(rbuf, conf.answerResponseTimeout);
again:
	if (r == AT_TIMEOUT) {
	    emsg = "Ring detected without successful handshake"; 
	    return (CALLTYPE_ERROR);
	}
	const AnswerMsg* am = findAnswer(rbuf);
	if (am != NULL) {
	    if (am->expect != AT_NOTHING && conf.waitForConnect) {
		/*
		 * Response string is an intermediate result that
		 * is only meaningful if followed by AT response
		 * am->next.  Read the next response from the modem
		 * and if it's the expected one, use the message
		 * to intuit the call type.  Otherwise, discard
		 * the intermediate response string and process the
		 * call according to the newly read response.
		 * This is intended to deal with modems that send
		 *   <something>
		 *   CONNECT
		 * (such as the Boca 14.4).
		 */
		r = atResponse(rbuf, conf.answerResponseTimeout);
		if (r != am->expect)
		    goto again;
	    }
	    if (am->status == OK)		// success
		return (am->type);
	    cs = am->status;
	    break;
	}
    } while (r != AT_EMPTYLINE);
    emsg = callStatus[cs];
    return (CALLTYPE_ERROR);
}

CallType
FaxModem::answerCall(AnswerType atype, fxStr& emsg)
{
    CallType ctype = CALLTYPE_ERROR;
    /*
     * If the request has no type-specific commands
     * to use, then just use the normal commands
     * intended for answering any type of call.
     */
    AnswerType t = atype;
    if (t != ANSTYPE_ANY && conf.answerCmd[t] == "")
	t = ANSTYPE_ANY;
    if (atCmd(conf.answerCmd[t], AT_NOTHING)) {
	ctype = answerResponse(emsg);
	if (ctype == CALLTYPE_UNKNOWN) {
	    /*
	     * The response does not uniquely identify the type
	     * of call; assume the type corresponds to the type
	     * of the answer request.
	     */
	    static CallType unknownCall[] = {
		CALLTYPE_FAX,	// ANSTYPE_ANY (default)
		CALLTYPE_DATA,	// ANSTYPE_DATA
		CALLTYPE_FAX,	// ANSTYPE_FAX
		CALLTYPE_VOICE,	// ANSTYPE_VOICE
	    };
	    ctype = unknownCall[atype];
	}
	/*
	 * Send any configured commands to the modem once the
	 * type of the call has been established.  These commands
	 * normally configure flow control and buad rate for
	 * modems that, for example, require a fixed baud rate
	 * and flow control scheme when receiving fax.
	 */ 
	if (conf.answerBeginCmd[ctype] != "")
	    (void) atCmd(conf.answerBeginCmd[ctype]);
    }
    return (ctype);
}

void FaxModem::sendBegin()	{}
void FaxModem::sendSetupPhaseB(){}
void FaxModem::sendEnd()	{}

/*
 * Modem capability (and related) query interfaces.
 */

static u_int
bestBit(u_int bits, u_int top, u_int bot)
{
    while (top > bot && (bits & BIT(top)) == 0)
	top--;
    return (top);
}

/*
 * Return Class 2 code for best modem signalling rate.
 */
u_int
FaxModem::getBestSignallingRate() const
{
    return bestBit(modemParams.br, BR_14400, BR_2400);
}

/*
 * Compare the requested signalling rate against
 * those the modem can do and return the appropriate
 * Class 2 bit rate code.
 */
int
FaxModem::selectSignallingRate(int br) const
{
    for (; br >= 0 && (modemParams.br & BIT(br)) == 0; br--)
	;
    return (br);
}

/*
 * Set data transfer timeout and adjust according
 * to the negotiated bit rate.
 */
void
FaxModem::setDataTimeout(long secs, u_int br)
{
    dataTimeout = secs*1000;	// 9600 baud timeout/data write (ms)
    switch (br) {
    case BR_2400:	dataTimeout *= 4; break;
    case BR_4800:	dataTimeout *= 2; break;
    case BR_9600:	dataTimeout = (4*dataTimeout)/3; break;
    // could shrink timeout for br > 9600
    }
}

/*
 * Compare the requested min scanline time
 * to what the modem can do and return the
 * lowest time the modem can do.
 */
int
FaxModem::selectScanlineTime(int st) const
{
    for (; st < ST_40MS && (modemParams.st & BIT(st)) == 0; st++)
	;
    return (st);
}

/*
 * Return the best min scanline time the modem
 * is capable of supporting.
 */
u_int
FaxModem::getBestScanlineTime() const
{
    u_int st;
    for (st = ST_0MS; st < ST_40MS; st++)
	if (modemParams.st & BIT(st))
	    break;
    return st;
}

/*
 * Return the best vres the modem supports.
 */
u_int
FaxModem::getBestVRes() const
{
    return bestBit(modemParams.vr, VR_FINE, VR_NORMAL);
}

/*
 * Return the best page width the modem supports.
 */
u_int
FaxModem::getBestPageWidth() const
{
    // XXX NB: we don't use anything > WD_2432
    return bestBit(modemParams.wd, WD_2432, WD_1728);
}

/*
 * Return the best page length the modem supports.
 */
u_int
FaxModem::getBestPageLength() const
{
    return bestBit(modemParams.ln, LN_INF, LN_A4);
}

/*
 * Return the best data format the modem supports.
 */
u_int
FaxModem::getBestDataFormat() const
{
    return bestBit(modemParams.df, DF_2DMMR, DF_1DMR);
}

/*
 * Return whether or not the modem supports 2DMR.
 */
fxBool
FaxModem::supports2D() const
{
    return modemParams.df & BIT(DF_2DMR);
}

/*
 * Return whether or not received EOLs are byte aligned.
 */
fxBool
FaxModem::supportsEOLPadding() const
{
    return FALSE;
}

/*
 * Return whether or not the modem supports the
 * specified vertical resolution.  Note that we're
 * rather tolerant because of potential precision
 * problems and general sloppiness on the part of
 * applications writing TIFF files.
 */
fxBool
FaxModem::supportsVRes(float res) const
{
    if (75 <= res && res < 120)
	return modemParams.vr & BIT(VR_NORMAL);
    else if (150 <= res && res < 250)
	return modemParams.vr & BIT(VR_FINE);
    else
	return FALSE;
}

/*
 * Return whether or not the modem supports the
 * specified page width.
 */
fxBool
FaxModem::supportsPageWidth(u_int w) const
{
    switch (w) {
    case 1728:	return modemParams.wd & BIT(WD_1728);
    case 2048:	return modemParams.wd & BIT(WD_2048);
    case 2432:	return modemParams.wd & BIT(WD_2432);
    case 1216:	return modemParams.wd & BIT(WD_1216);
    case 864:	return modemParams.wd & BIT(WD_864);
    }
    return FALSE;
}

/*
 * Return whether or not the modem supports the
 * specified page length.  As above for vertical
 * resolution we're lenient in what we accept.
 */
fxBool
FaxModem::supportsPageLength(u_int l) const
{
    // XXX probably need to be more forgiving with values
    if (270 < l && l <= 330)
	return modemParams.ln & (BIT(LN_A4)|BIT(LN_INF));
    else if (330 < l && l <= 390)
	return modemParams.ln & (BIT(LN_B4)|BIT(LN_INF));
    else
	return modemParams.ln & BIT(LN_INF);
}

/*
 * Return modems best capabilities for setting up
 * the initial T.30 DIS when receiving data.
 */
u_int
FaxModem::modemDIS() const
{
    u_int DIS = DIS_T4RCVR
	      | Class2Params::vrDISTab[getBestVRes()]
	      | Class2Params::brDISTab[getBestSignallingRate()]
	      | Class2Params::wdDISTab[getBestPageWidth()]
	      | Class2Params::lnDISTab[getBestPageWidth()]
	      | Class2Params::dfDISTab[getBestDataFormat()]
	      | Class2Params::stDISTab[getBestScanlineTime()]
	      ;
    // tack on one extension byte
    DIS = (DIS | DIS_XTNDFIELD) << 8;
    if (modemParams.df >= DF_2DMRUNCOMP)
	DIS |= DIS_2DUNCOMP;
    if (modemParams.df >= DF_2DMMR)
	DIS |= DIS_G4COMP;
    return (DIS);
}

/*
 * Tracing support.
 */

/*
 * Trace a MODEM-communication-related activity.
 */
void
FaxModem::modemTrace(const char* va_alist ...)
#define	fmt va_alist
{
    va_list ap;
    va_start(ap, fmt);
    char buf[1024];
    strcpy(buf, "MODEM ");
    strcat(buf, fmt);
    server.vtraceStatus(FAXTRACE_MODEMCOM, buf, ap);
    va_end(ap);
}
#undef fmt

/*
 * Trace a modem capability.
 */
void
FaxModem::modemCapability(const char* va_alist ...)
#define	fmt va_alist
{
    va_list ap;
    va_start(ap, fmt);
    char buf[1024];
    strcpy(buf, "MODEM ");
    strcat(buf, fmt);
    server.vtraceStatus(FAXTRACE_MODEMCAP, buf, ap);
    va_end(ap);
}
#undef fmt

/*
 * Indicate a modem supports some capability.
 */
void
FaxModem::modemSupports(const char* va_alist ...)
#define	fmt va_alist
{
    va_list ap;
    va_start(ap, fmt);
    char buf[1024];
    strcpy(buf, "MODEM Supports ");
    strcat(buf, fmt);
    server.vtraceStatus(FAXTRACE_MODEMCAP, buf, ap);
    va_end(ap);
}
#undef fmt

/*
 * Trace a protocol-related activity.
 */
void
FaxModem::protoTrace(const char* va_alist ...)
#define	fmt va_alist
{
    va_list ap;
    va_start(ap, fmt);
    server.vtraceStatus(FAXTRACE_PROTOCOL, fmt, ap);
    va_end(ap);
}
#undef fmt

/*
 * Trace a server-level activity.
 */
void
FaxModem::serverTrace(const char* va_alist ...)
#define	fmt va_alist
{
    va_list ap;
    va_start(ap, fmt);
    server.vtraceStatus(FAXTRACE_SERVER, fmt, ap);
    va_end(ap);
}
#undef fmt

/*
 * Trace a modem capability bit mask.
 */
void
FaxModem::traceBits(u_int bits, const char* bitNames[])
{
    for (u_int i = 0; bits; i++)
	if (BIT(i) & bits) {
	    modemSupports(bitNames[i]);
	    bits &= ~BIT(i);
	}
}

/*
 * Trace a modem's capabilities.
 */
void
FaxModem::traceModemParams()
{
    traceBits(modemParams.vr, Class2Params::vresNames);
    traceBits(modemParams.br, Class2Params::bitRateNames);
    traceBits(modemParams.wd, Class2Params::pageWidthNames);
    traceBits(modemParams.ln, Class2Params::pageLengthNames);
    traceBits(modemParams.df, Class2Params::dataFormatNames);
    if (modemParams.ec & (BIT(EC_ENABLE)))
	modemSupports("error correction");
    if (modemParams.bf & BIT(BF_ENABLE))
	modemSupports("binary file transfer");
    traceBits(modemParams.st, Class2Params::scanlineTimeNames);
}

#define	EOLcheck(w,mask,code) \
    if ((w & mask) == code) { w |= mask; return (TRUE); }

/*
 * Check the last 24 bits of received T.4-encoded
 * data (presumed to be in LSB2MSB bit order) for
 * an EOL code and, if one is found, foul the data
 * to insure future calls do not re-recognize an
 * EOL code.
 */
fxBool
FaxModem::EOLcode(u_long& w)
{
    if ((w & 0x00f00f) == 0) {
	EOLcheck(w, 0x00f0ff, 0x000080);
	EOLcheck(w, 0x00f87f, 0x000040);
	EOLcheck(w, 0x00fc3f, 0x000020);
	EOLcheck(w, 0x00fe1f, 0x000010);
    }
    if ((w & 0x00ff00) == 0) {
	EOLcheck(w, 0x00ff0f, 0x000008);
	EOLcheck(w, 0x80ff07, 0x000004);
	EOLcheck(w, 0xc0ff03, 0x000002);
	EOLcheck(w, 0xe0ff01, 0x000001);
    }
    if ((w & 0xf00f00) == 0) {
	EOLcheck(w, 0xf0ff00, 0x008000);
	EOLcheck(w, 0xf87f00, 0x004000);
	EOLcheck(w, 0xfc3f00, 0x002000);
	EOLcheck(w, 0xfe1f00, 0x001000);
    }
    return (FALSE);
}
#undef EOLcheck

void
FaxModem::startPageRecv()
{
    recvEOLCount = 0;
    recvWord = 0xffffff;
    memset(RTCbuf, 0, sizeof (RTCbuf));
}

void
FaxModem::recvPageData(TIFF* tif, u_char* bp, int n)
{
    TIFFWriteRawStrip(tif, 0, bp, n);
    /*
     * Capture the last 40 bytes of data so that
     * we can verify the RTC at the end when adjusting
     * the received line count.  10 bytes is all
     * that we need; it is enough to hold a 2D RTC.
     */
    int cc = n - 40;
    if (cc > 0)
	memcpy(RTCbuf, bp+cc, 40);
    else
	memcpy(RTCbuf-cc, bp, n);
    /*
     * Many modems (ZyXEL, Supra) do not return valid line
     * counts, so count them here.  Note that this code
     * assumes data is in LSB2MSB order.
     */
    u_long rows = 0;
    u_long w = recvWord;
    for (cc = n; cc > 0; cc--) {
	w = (w<<8) | *bp++;
	if (EOLcode(w))
	    rows++;
    }
    recvWord = w;
    recvEOLCount += rows;
    protoTrace("RECV: %d bytes of data, %lu lines", n, rows);
}

static const int EOL = 0x001;		// end-of-line code (11 0's + 1)

#define	BITCASE(b)			\
    case b:				\
	code <<= 1;			\
	if (data & b) code |= 1;	\
	len++;				\
	if (code > 0) { bit = (b<<1); break; }

/*
 * Parse RTC and adjust line count.
 */
void
FaxModem::endPageRecv(const Class2Params& params)
{
    fxBool startOfRTC = TRUE;
    /*
     * Locate the start of the RTC by looking for
     * two consecutive EOL codes in the RTC buffer.
     * From that point onward, we deduct any EOL codes
     * that we see from the count of received lines.
     */
    u_int i = 0;
    int bit = 0x01;
    int data = 0;
    fxBool emptyLine = FALSE;
    for (;;) {
	short code = 0;
	short len = 0;
	switch (bit & 0xff) {
    again:
	BITCASE(0x01);
	BITCASE(0x02);
	BITCASE(0x04);
	BITCASE(0x08);
	BITCASE(0x10);
	BITCASE(0x20);
	BITCASE(0x40);
	BITCASE(0x80);
	default:
	    if (i == sizeof (RTCbuf))
		return;
	    data = RTCbuf[i++];
	    goto again;
	}
	if (len >= 12 && code == EOL) {		// EOL code
	    if (params.is2D()) {		// snarf tag bit
		if ((bit & 0xff) == 0) {
		    if (i == sizeof (RTCbuf))
			return;
		    data = RTCbuf[i++];
		    bit = 0x01;
		}
		bit <<= 1;
	    }
	    if (emptyLine) {			// consecutive EOL codes
		recvEOLCount -= 1+startOfRTC;
		startOfRTC = FALSE;
	    } else
		emptyLine = TRUE;
	} else					// something other than EOL
	    emptyLine = FALSE;
    }
}

u_long
FaxModem::getRecvEOLCount() const
{
    return recvEOLCount;
}

/*
 * Modem i/o support.
 */

int
FaxModem::getModemLine(char buf[], long ms)
{
    int n = server.getModemLine(buf, ms);
    if (n > 0)
	trimModemLine(buf, n);
    return (n);
}
int FaxModem::getModemChar(long ms) { return server.getModemChar(ms); }
int FaxModem::getModemDataChar()    { return server.getModemChar(dataTimeout); }

fxBool
FaxModem::putModemDLEData(const u_char* data, u_int cc, const u_char* bitrev, long ms)
{
    u_char dlebuf[2*1024];
    while (cc > 0) {
	if (wasTimeout() || abortRequested())
	    return (FALSE);
	/*
	 * Copy to temp buffer, doubling DLE's.
	 */
	u_int i, j;
	u_int n = fxmin(cc, conf.maxPacketSize);
	n = fxmin(n, sizeof (dlebuf)/2);
	for (i = 0, j = 0; i < n; i++, j++) {
	    dlebuf[j] = bitrev[data[i]];
	    if (dlebuf[j] == DLE)
		dlebuf[++j] = DLE;
	}
	if (!putModem(dlebuf, j, ms))
	    return (FALSE);
	data += n;
	cc -= n;
	if (cc > 0 && conf.interPacketDelay)
	    pause(conf.interPacketDelay);
    }
    return (TRUE);
}

void FaxModem::flushModemInput()
    { server.modemFlushInput(); }
fxBool FaxModem::putModem(void* d, int n, long ms)
    { return server.putModem(d, n, ms); }
fxBool FaxModem::putModemData(void* d, int n)
    { return server.putModem(d, n, dataTimeout); }
void FaxModem::putModemLine(const char* cp)
    { server.putModemLine(cp); }

void FaxModem::startTimeout(long ms) { server.startTimeout(ms); }
void FaxModem::stopTimeout(const char* w){ server.stopTimeout(w); }

const u_int MSEC_PER_SEC = 1000;

#include <osfcn.h>
#include <sys/time.h>

void
FaxModem::pause(u_int ms)
{
    if (ms == 0)
	return;
    protoTrace("DELAY %u ms", ms);
    struct timeval tv;
    tv.tv_sec = ms / MSEC_PER_SEC;
    tv.tv_usec = (ms % MSEC_PER_SEC) * 1000;
    (void) select(0, 0, 0, 0, &tv);
}

/*
 * Reset the modem and set the DTE-DCE rate.
 */
fxBool
FaxModem::selectBaudRate(BaudRate br, FlowControl i, FlowControl o)
{
    rate = br;
    iFlow = i;
    oFlow = o;
    for (u_int n = 3; n != 0; n--)
	if (reset(5*1000))
	    return (TRUE);
    return (FALSE);
}

fxBool FaxModem::sendBreak(fxBool pause)
    { return server.sendBreak(pause); }
fxBool
FaxModem::setBaudRate(BaudRate r)
{
    if (server.setBaudRate(r)) {
	if (conf.baudRateDelay)
	    pause(conf.baudRateDelay);
	return (TRUE);
    } else
	return (FALSE);
}
fxBool
FaxModem::setBaudRate(BaudRate r, FlowControl i, FlowControl o)
{
    if (server.setBaudRate(r,i,o)) {
	if (conf.baudRateDelay)
	    pause(conf.baudRateDelay);
	return (TRUE);
    } else
	return (FALSE);
}
fxBool FaxModem::setXONXOFF(FlowControl i, FlowControl o, SetAction a)
    { return server.setXONXOFF(i,o,a); }
fxBool FaxModem::setDTR(fxBool onoff)
    { return server.setDTR(onoff); }
fxBool FaxModem::setInputBuffering(fxBool onoff)
    { return server.setInputBuffering(onoff); }
fxBool FaxModem::modemStopOutput()
    { return server.modemStopOutput(); }

/*
 * Miscellaneous server interfaces hooks.
 */

fxBool FaxModem::getProtocolTracing()
    { return (server.getSessionTracing() & FAXTRACE_PROTOCOL) != 0; }
fxBool FaxModem::getHDLCTracing()
    { return (server.getSessionTracing() & FAXTRACE_HDLC) != 0; }

fxBool FaxModem::sendSetupParams(TIFF* tif, Class2Params& params,
    FaxMachineInfo& info, fxStr& emsg)
{
    return server.sendSetupParams(tif, params, info, emsg);
}

fxBool
FaxModem::recvCheckTSI(const fxStr& tsi)
{
    fxStr s(tsi);
    s.remove(0, s.skip(0,' '));		// strip leading white space
    u_int pos = s.skipR(s.length(),' ');
    s.remove(pos, s.length() - pos);	// and trailing white space
    return server.recvCheckTSI(s);
}
void
FaxModem::recvCSI(fxStr& csi)
{
    csi.remove(0, csi.skip(0,' '));	// strip leading white space
    u_int pos = csi.skipR(csi.length(),' ');
    csi.remove(pos, csi.length() - pos);// and trailing white space
    protoTrace("REMOTE CSI \"%s\"", (char*) csi);
}
void FaxModem::recvDCS(Class2Params& params)
    { server.recvDCS(params); }
void FaxModem::recvNSF(u_int nsf)
    { server.recvNSF(nsf); }

void
FaxModem::recvSetupPage(TIFF* tif, long group3opts, int fillOrder)
{
    server.recvSetupPage(tif, group3opts, fillOrder);
    /*
     * Record the file offset to the start of the data
     * in the file.  We write zero bytes to force the
     * strip offset to be setup in case this is the first
     * time the strip is being written.
     */
    u_char null[1];
    (void) TIFFWriteRawStrip(tif, 0, null, 0);
    u_long* lp;
    (void) TIFFGetField(tif, TIFFTAG_STRIPOFFSETS, &lp);
    savedWriteOff = lp[0];
}

/*
 * Reset the TIFF state for the current page so that
 * subsequent data overwrites anything previously
 * written.  This is done by reseting the file offset
 * and setting the strip's bytecount and offset to
 * values they had at the start of the page.  This
 * scheme assumes that only page data is written to
 * the file between the time recvSetupPage is called
 * and recvResetPage is called.
 */
void
FaxModem::recvResetPage(TIFF* tif)
{
    u_long* lp;
    TIFFSetWriteOffset(tif, 0);		// force library to reset state
    TIFFGetField(tif, TIFFTAG_STRIPOFFSETS, &lp);	lp[0] = savedWriteOff;
    TIFFGetField(tif, TIFFTAG_STRIPBYTECOUNTS, &lp);	lp[0] = 0;
}

fxBool FaxModem::abortRequested()
    { return server.abortRequested(); }

void FaxModem::beginTimedTransfer()		{ server.timeout = FALSE; }
void FaxModem::endTimedTransfer()		{}
fxBool FaxModem::wasTimeout()			{ return server.timeout; }
void FaxModem::setTimeout(fxBool b)		{ server.timeout = b; }

void FaxModem::countPage()			{ server.npages++; }

/*
 * Parsing support routines.
 */

/*
 * Cleanup a response line from the modem.  This removes
 * leading white space and any prefixing "+F<mumble>=" crap
 * that some Class 2 modems put at the front, as well as
 * any trailing white space.
 */
void
FaxModem::trimModemLine(char buf[], int& cc)
{
    // trim trailing white space
    if (cc > 0 && isspace(buf[cc-1])) {
	do {
	    cc--;
	} while (cc > 0 && isspace(buf[cc-1]));
	buf[cc] = '\0';
    }
    if (cc > 0) {
	u_int i = 0;
	// leading white space
	while (i < cc && isspace(buf[i]))
	    i++;
	// check for a leading +F<mumble>=
	if (i+1 < cc && buf[i] == '+' && buf[i+1] == 'F') {
	    u_int j = i;
	    for (i += 2; i < cc && buf[i] != '='; i++)
		 ;
	    if (i < cc) {	// trim more white space
		for (i++; i < cc && isspace(buf[i]); i++)
		    ;
	    } else		// no '=', back out
		i = j;
	}
	cc -= i;
	memmove(buf, buf+i, cc+1);
    }
}

u_int
FaxModem::fromHex(const char* cp, int n)
{
    if (n == -1)
	n = strlen(cp);
    u_int v = 0;
    while (n-- > 0) {
	int c = *cp++;
	if (isxdigit(c)) {
	    if (isdigit(c))
		c -= '0';
	    else
		c = (c - 'A') + 10;
	    v = (v << 4) + c;
	}
    }
    return (v);
}

fxStr
FaxModem::toHex(int v, int ndigits)
{
    char buf[9];
    assert(ndigits <= 8);
    for (int i = ndigits-1; i >= 0; i--) {
	buf[i] = "0123456789ABCDEF"[v&0xf];
	v >>= 4;
    }
    return (fxStr(buf, ndigits));
}

/* 
 * Hayes-style modem manipulation support.
 */
fxBool
FaxModem::reset(long ms)
{
    setDTR(FALSE);
    pause(conf.resetDelay);		// pause so modem can do reset
    setDTR(TRUE);
    /*
     * On some systems lowering and raising DTR is not done
     * properly (DTR is not raised when requested); thus we
     * reopen the device to insure that DTR is reasserted.
     */
    server.reopenDevice();
    if (!setBaudRate(rate, iFlow, oFlow))
	return (FALSE);
    flushModemInput();
    return atCmd(resetCmds, AT_OK, ms);
}

fxBool
FaxModem::abort(long ms)
{
    return reset(ms);
}

fxBool
FaxModem::sync(long ms)
{
    return waitFor(AT_OK, ms);
}

ATResponse
FaxModem::atResponse(char* buf, long ms)
{
    int n = getModemLine(buf, ms);
    if (n <= 0)
	lastResponse = (wasTimeout() ? AT_TIMEOUT : AT_EMPTYLINE);
    else if (strneq(buf, "OK", 2))
	lastResponse = AT_OK;
    else if (strneq(buf, "NO CARRIER", 10))
	lastResponse = AT_NOCARRIER;
    else if (strneq(buf, "NO DIAL", 7))		// NO DIALTONE or NO DIAL TONE
	lastResponse = AT_NODIALTONE;
    else if (strneq(buf, "NO ANSWER", 9))
	lastResponse = AT_NOANSWER;
    else if (strneq(buf, "ERROR", 5))
	lastResponse = AT_ERROR;
    else if (strneq(buf, "CONNECT", 7))
	lastResponse = AT_CONNECT;
    else if (strneq(buf, "RING", 4))
	lastResponse = AT_RING;
    else if (strneq(buf, "BUSY", 4))
	lastResponse = AT_BUSY;
    else if (strneq(buf, "PHONE OFF-HOOK", 14))
	lastResponse = AT_OFFHOOK;
    else
	lastResponse = AT_OTHER;
    return lastResponse;
}

/*
 * Send an AT command string to the modem and, optionally
 * wait for status responses.  This routine breaks multi-line
 * strings (demarcated by embedded \n's) and waits for each
 * intermediate response.  Embedded escape sequences for
 * changing the DCE-DTE communication rate and/or host-modem
 * flow control scheme are also recognized and handled.
 */
fxBool
FaxModem::atCmd(const fxStr& cmd, ATResponse r, long ms)
{
    u_int cmdlen = cmd.length();
    u_int pos = 0;
    fxBool respPending = FALSE;

    /*
     * Scan string for \n's and escape codes (byte w/ 0x80 set).
     * A \n causes the current string to be sent to the modem and
     * a return status string parsed (and possibly compared to an
     * expected response).  An escape code terminates scanning,
     * with any pending string flushed to the modem before the
     * associated commands are carried out.
     */
    u_int i = 0;
    while (i < cmdlen) {
	if (cmd[i] == '\n') {
	    /*
	     * Send partial string to modem and await
	     * status string if necessary.
	     */
	    putModemLine("AT" | cmd.extract(pos, i-pos));
	    pos = ++i;			// next segment starts after \n
	    if (r != AT_NOTHING) {
		if (!waitFor(r, ms))
		    return (FALSE);
	    } else {
		if (!waitFor(AT_OK, ms))
		    return (FALSE);
	    }
	    respPending = FALSE;
	} else if (cmd[i] & 0x80) {
	    /*
	     * Escape code; flush any partial line, process
	     * escape codes and carry out the associated
	     * actions.  Note that assume there is no benefit
	     * to set flow control independently of baud rate.
	     */
	    if (i > pos) {
		putModemLine("AT" | cmd.extract(pos, i-pos));
		respPending = TRUE;
	    }
	    BaudRate br = rate;
	    FlowControl flow = flowControl;
	    do {
		switch (cmd[i] & 0xff) {
		case ESC_XON:	flow = FLOW_XONXOFF; break;
		case ESC_RTS:	flow = FLOW_RTSCTS; break;
		default:	br = cmd[i] & 0xf; break;
		}
	    } while (++i < cmdlen && (cmd[i] & 0x80));
	    pos = i;			// next segment starts here
	    if (flow != flowControl)
		setBaudRate(br, flow, flow);
	    else
		setBaudRate(br);
	    rate = br; flowControl = flow;
	} else
	    i++;
    }
    /*
     * Flush any pending string to modem.
     */
    if (i > pos) {
	putModemLine("AT" | cmd.extract(pos, i-pos));
	respPending = TRUE;
    }
    /*
     * Wait for any pending response.
     */
    if (respPending) {
	if (r != AT_NOTHING && !waitFor(r, ms))
	    return (FALSE);
    }
    return (TRUE);
}

/*
 * Wait (carefully) for some response from the modem.
 */
fxBool
FaxModem::waitFor(ATResponse wanted, long ms)
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
	    return (FALSE);
	}
    }
}

/*
 * Process a manufacturer/model/revision query.
 */
fxBool
FaxModem::doQuery(fxStr& queryCmd, fxStr& result)
{
    if (queryCmd == "")
	return (TRUE);
    if (queryCmd[0] == '!') {
	/*
	 * ``!mumble'' is interpreted as "return mumble";
	 * this means that you can't send ! to the modem.
	 */
	result = queryCmd.tail(queryCmd.length()-1);
	return (TRUE);
    }
    return (atQuery(queryCmd, result));
}

/*
 * Return modem manufacturer.
 */
fxBool
FaxModem::setupManufacturer(fxStr& mfr)
{
    return doQuery(mfrQueryCmd, mfr);
}

/*
 * Return modem model identification.
 */
fxBool
FaxModem::setupModel(fxStr& model)
{
    return doQuery(modelQueryCmd, model);
}

/*
 * Return modem firmware revision.
 */
fxBool
FaxModem::setupRevision(fxStr& rev)
{
    return doQuery(revQueryCmd, rev);
}

/*
 * Class X-style command support.
 */

/*
 * Send AT+F<cmd> and (potentially) wait for a response.
 */
fxBool
FaxModem::vatFaxCmd(ATResponse resp, const char* fmt ... )
{
    char buf[2048];
    buf[0] = '+'; buf[1] = 'F';
    va_list ap;
    va_start(ap, fmt);
    vsprintf(buf+2, fmt, ap);
    va_end(ap);
    return atCmd(buf, resp);
}

/*
 * Send AT<what>? and get a range response.
 */
fxBool
FaxModem::atQuery(const char* what, u_int& v)
{
    char response[1024];
    if (atCmd(what, AT_NOTHING) && atResponse(response) == AT_OTHER) {
	sync();
	return parseRange(response, v);
    }
    return (FALSE);
}

/*
 * Send AT<what>? and get a string response.
 */
fxBool
FaxModem::atQuery(const char* what, fxStr& v)
{
    ATResponse r = AT_ERROR;
    if (atCmd(what, AT_NOTHING)) {
	v.resize(0);
	while ((r = atResponse(rbuf)) != AT_OK) {
	    if (r == AT_ERROR || r == AT_TIMEOUT || r == AT_EMPTYLINE)
		break;
	    if (v.length())
		v.append('\n');
	    v.append(rbuf);
	}
    }
    return (r == AT_OK);
}

const char OPAREN = '(';
const char CPAREN = ')';
const char COMMA = ',';
const char SPACE = ' ';

/*
 * Parse a Class 2 parameter range string.  This is very
 * forgiving because modem vendors do not exactly follow
 * the syntax specified in the "standard".  Try looking
 * at some of the responses given by rev ~4.04 of the
 * ZyXEL firmware (for example)!
 */
fxBool
FaxModem::vparseRange(const char* cp, int nargs ... )
{
    fxBool b = TRUE;
    va_list ap;
    va_start(ap, nargs);
    while (nargs-- > 0) {
	while (cp[0] == SPACE)
	    cp++;
	char matchc;
	fxBool acceptList;
	if (cp[0] == OPAREN) {				// (<items>)
	    matchc = CPAREN;
	    acceptList = TRUE;
	    cp++;
	} else if (isdigit(cp[0])) {			// <item>
	    matchc = COMMA;
	    acceptList = (nargs == 0);
	} else {
	    b = FALSE;
	    break;
	}
	int mask = 0;
	while (cp[0] && cp[0] != matchc) {
	    if (cp[0] == SPACE) {			// ignore white space
		cp++;
		continue;
	    }
	    if (!isdigit(cp[0])) {
		b = FALSE;
		goto done;
	    }
	    int v = 0;
	    do {
		v = v*10 + (cp[0] - '0');
	    } while (isdigit((++cp)[0]));
	    int r = v;
	    if (cp[0] == '-') {				// <low>-<high>
		cp++;
		if (!isdigit(cp[0])) {
		    b = FALSE;
		    goto done;
		}
		r = 0;
		do {
		    r = r*10 + (cp[0] - '0');
		} while (isdigit((++cp)[0]));
	    } else if (cp[0] == '.') {			// <d.b>
		cp++;
		while (isdigit(cp[0]))			// XXX
		    cp++;
	    }
	    // expand range or list
	    for (; v <= r; v++)
		mask |= 1<<v;
	    if (acceptList && cp[0] == COMMA)		// (<item>,<item>...)
		cp++;
	}
	*va_arg(ap, int*) = mask;
	if (cp[0] == matchc)
	    cp++;
	if (matchc == CPAREN && cp[0] == COMMA)
	    cp++;
    }
done:
    va_end(ap);
    return (b);
}

/*
 * Parse a single Class X range specification
 * and return the resulting bit mask.
 */
fxBool
FaxModem::parseRange(const char* cp, u_int& a0)
{
    return vparseRange(cp, 1, &a0);
}

void
FaxModem::setSpeakerVolume(SpeakerVolume l)
{
    atCmd(conf.setVolumeCmd[l]);
}

void
FaxModem::hangup()
{
    atCmd(conf.onHookCmd, AT_OK, 5000);
}

fxBool
FaxModem::waitForRings(u_int n)
{
    while (n > 0 && atResponse(rbuf, 10*1000) == AT_RING)
	n--;
    return (n <= 0);
}
