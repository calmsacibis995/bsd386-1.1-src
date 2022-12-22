/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/Class2.c++,v 1.1.1.1 1994/01/14 23:09:46 torek Exp $
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
#include "Class2.h"
#include "ModemConfig.h"

#include <stdlib.h>
#include <ctype.h>

Class2Modem::Class2Modem(FaxServer& s, const ModemConfig& c) : FaxModem(s,c)
{
    hangupCode[0] = '\0';
    group3opts = 0;
    // supply defaults if not specified
    if (mfrQueryCmd == "")
	mfrQueryCmd = "+FMFR?";
    if (modelQueryCmd == "")
	modelQueryCmd = "+FMDL?";
    if (revQueryCmd == "")
	revQueryCmd = "+FREV?";
}

Class2Modem::~Class2Modem()
{
}

/*
 * Check if the modem is a Class 2 modem and, if so,
 * configure it for use.  We try to confine a lot of
 * the manufacturer-specific bogosity here and leave
 * the remainder of the Class 2 support fairly generic.
 */
fxBool
Class2Modem::setupModem()
{
    if (!selectBaudRate(conf.maxRate, conf.flowControl, conf.flowControl))
	return (FALSE);
    // Query service support information
    if (atQuery("+FCLASS=?", modemServices))
	traceBits(modemServices & SERVICE_ALL, serviceNames);
    if ((modemServices & SERVICE_CLASS2) == 0)
	return (FALSE);
    atCmd(conf.class2Cmd);

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
    if (atCmd("+FDCC=?", AT_NOTHING)) {
	/*
	 * Syntax: (vr),(br),(wd),(ln),(df),(ec),(bf),(st)
	 * where,
	 *	vr	vertical resolution
	 *	br	bit rate
	 *	wd	page width
	 *	ln	page length
	 *	df	data compression
	 *	ec	error correction
	 *	bf	binary file transfer
	 *	st	scan time/line
	 */
	if (getModemLine(rbuf) > 0) {
	    if (!parseRange(rbuf, modemParams)) {
		serverTrace("Error parsing \"+FDCC\" query response: \"%s\"",
		    rbuf);
		return (FALSE);
	    } else
		traceModemParams();
	}
	// NB: some ZyXEL firmware does not send "OK"
	sync(500);
    }
    /*
     * Check to see if the modem supports copy quality
     * checking.  As of today, the only modem we know
     * that does this is the Everex 24/96D.
     */
    cqCmds = "";
    u_int modemCQ = 0;
    if (atQuery("+FCQ=?", modemCQ)) {
	if (modemCQ & BIT(1)) {
	    cqCmds = conf.class2CQCmd;
	    modemSupports("1-D copy quality checking");
	} else
	    modemSupports("no copy quality checking");
    }
    /*
     * Define the code to send to the modem to trigger
     * the transfer of received Phase C data to the host.
     * The ZyXEL and certain Rockwell RC32ACL-based modems
     * use DC2 as specified in 2388-B.  Everyone else uses DC1
     * as specified in 2388-A.  Isn't that nice?
     */
    if (conf.class2RecvDataTrigger == "") {
	if (modemMfr == "ZYXEL" || modemModel == "RC32ACL")
	    recvDataTrigger = DC2;
	else
	    recvDataTrigger = DC1;
    } else
	recvDataTrigger = conf.class2RecvDataTrigger[0];
    /*
     * Prior to rev 6.01 ZyXEL's claim to support 2D MR, but
     * there are problems.  Thus we override it here to reflect
     * what actually works.  Also, before 6.01 +FDIS commands
     * were ignored; this makes it impossible to select high
     * res or low res on a page-by-page basis so we are forced
     * to restrict usage to only low res transfers.
     *
     * As of rev 2.17, PPI modems ignore +FDIS commands; thus
     * we disable 2D and high res capabilities.
     */
    if ((modemMfr == "ZYXEL" && modemRevision < "V 6.01") ||
      strneq(modemMfr, "(c) Practical", 13)) {
	modemParams.df = BIT(DF_1DMR);
	modemParams.vr = BIT(VR_NORMAL);
	serverTrace("Warning, disabling 2D encoding and 196 lpi capabilities");
    }
    setupClass2Parameters();			// send parameters to the modem
    return (TRUE);
}

/*
 * Send the modem the Class 2 configuration commands.
 */
fxBool
Class2Modem::setupClass2Parameters()
{
    if (modemServices & SERVICE_CLASS2) {	// when resetting at startup
	atCmd(conf.class2Cmd);
	class2Cmd("TBC", 0);			// stream mode
	class2Cmd(conf.class2BORCmd);
	class2Cmd("CR", 1);			// enable receiving
	/*
	 * Set Phase C data transfer timeout parameter.
	 * Note that we also have our own timeout parameter
	 * that should be at least as large (though it
	 * probably doesn't matter).  Some modem manufacturers
	 * do not support this command (or perhaps they
	 * hide it under a different name).
	 */
	(void) class2Cmd("PHCTO", 30);
	/*
	 * Try to setup byte-alignment of received EOL's.
	 * As always, this is problematic.  If the modem
	 * does not support this, but accepts the command
	 * (as many do!), then received facsimile will be
	 * incorrectly tagged as having byte-aligned EOL
	 * codes in them--not usually much of a problem.
	 */
	if (conf.class2RELCmd != "" && class2Cmd(conf.class2RELCmd))
	    group3opts |= GROUP3OPT_FILLBITS;
	else
	    group3opts &= ~GROUP3OPT_FILLBITS;
	if (cqCmds != "")			// copy quality checking
	    (void) class2Cmd(cqCmds);
	if (getHDLCTracing())			// HDLC frame tracing
	    class2Cmd("BUG", 1);
	/*
	 * Force the DCC so that we override whatever
	 * the modem defaults are.
	 */
	setupDCC();
	/*
	 * Enable adaptive-answer support.  If we're configured,
	 * we'll act like getty and initiate a login session if
	 * we get a data connection.  Note that we do this last
	 * so that the modem can be left in a state other than
	 * +FCLASS=2 (e.g. Rockwell-based modems often need to be
	 * in Class 0).
	 */
	if (conf.setupAACmd != "")
	    (void) atCmd(conf.setupAACmd);
    }
    return (TRUE);
}

/*
 * Setup DCC to reflect best capabilities of the server.
 */
fxBool
Class2Modem::setupDCC()
{
    params.vr = getBestVRes();
    params.br = getBestSignallingRate();
    params.wd = getBestPageWidth();
    params.ln = getBestPageLength();
    params.df = getBestDataFormat();
    params.ec = EC_DISABLE;		// XXX
    params.bf = BF_DISABLE;
    params.st = getBestScanlineTime();
    return class2Cmd("DCC", params);
}

/*
 * Parse a ``capabilities'' string from the modem and
 * return the values through the params parameter.
 */
fxBool
Class2Modem::parseClass2Capabilities(const char* cap, Class2Params& params)
{
    int n = sscanf(cap, "%d,%d,%d,%d,%d,%d,%d,%d",
	&params.vr, &params.br, &params.wd, &params.ln,
	&params.df, &params.ec, &params.bf, &params.st);
    if (n == 8) {
	/*
	 * Clamp values to insure modem doesn't feed us
	 * nonsense; should log bogus stuff also.
	 */
	params.vr = fxmin(params.vr, (u_int) VR_FINE);
	params.br = fxmin(params.br, (u_int) BR_14400);
	params.wd = fxmin(params.wd, (u_int) WD_864);
	params.ln = fxmin(params.ln, (u_int) LN_INF);
	params.df = fxmin(params.df, (u_int) DF_2DMMR);
	if (params.ec > EC_ENABLE)
	    params.ec = EC_DISABLE;
	if (params.bf > BF_ENABLE)
	    params.bf = BF_DISABLE;
	params.st = fxmin(params.st, (u_int) ST_40MS);
	return (TRUE);
    } else {
	protoTrace("MODEM protocol botch, can not parse \"%s\"", cap);
	return (FALSE);
    }
}

/*
 * Set the modem into data service after auto-detecting
 * an incoming data connection.  Class 2 says this should
 * be automatically done for us.
 */
fxBool
Class2Modem::dataService()
{
    return (TRUE);				// nothing to do
}

/*
 * Set the modem into voice service after auto-detecting
 * an incoming voice call.  ZyXEL (only modem that supports
 * this) say that it should be done automatically.
 */
fxBool
Class2Modem::voiceService()
{
    return (TRUE);
}

fxBool
Class2Modem::setupRevision(fxStr& revision)
{
    if (FaxModem::setupRevision(revision)) {
	/*
	 * Cleanup ZyXEL response (argh), modem gives:
	 * +FREV?	"U1496E   V 5.02 M    "
	 */
	if (modemMfr == "ZYXEL") {
	    u_int pos = modemRevision.next(0, ' ');
	    if (pos != modemRevision.length()) {	// rev. has model 1st
		pos = modemRevision.skip(pos, ' ');
		modemRevision.remove(0, pos);
	    }
	}
	return (TRUE);
    } else
	return (FALSE);
}

fxBool
Class2Modem::setupModel(fxStr& model)
{
    if (FaxModem::setupModel(model)) {
	/*
	 * Cleanup ZyXEL response (argh), modem gives:
	 * +FMDL?	"U1496E   V 5.02 M    "
	 */
	if (modemMfr == "ZYXEL")
	    modemModel.resize(modemModel.next(0, ' '));	// model is first word
	return (TRUE);
    } else
	return (FALSE);
}

/*
 * Strip any quote marks from a string.  This
 * is used for received TSI+CSI strings passed
 * to the server.
 */
fxStr
Class2Modem::stripQuotes(const char* cp)
{
    fxStr s(cp);
    u_int pos = s.next(0, '"');
    if (pos != s.length())
	s.remove(0,pos+1);
    pos = s.next(0, '"');
    if (pos != s.length())
	s.remove(pos, s.length()-pos);
    return (s);
}

/*
 * Construct the Calling Station Identifier (CSI) string
 * for the modem.  This is encoded as an ASCII string
 * according to Table 3/T.30 (see the spec).  Hyphen ('-')
 * and period are converted to space; otherwise invalid
 * characters are ignored in the conversion.  The string may
 * be at most 20 characters (according to the spec).
 */
void
Class2Modem::setLID(const fxStr& number)
{
    fxStr csi;
    u_int n = fxmin((u_int) number.length(), (u_int) 20);
    for (u_int i = 0; i < n; i++) {
	char c = number[i];
	if (c == ' ' || c == '-' || c == '.')
	    csi.append(' ');
	else if (c == '+' || isdigit(c))
	    csi.append(c);
    }
    class2Cmd("LID", (char*) csi);
}

/* 
 * Modem manipulation support.
 */

/*
 * Reset a Class 2 modem.
 */
fxBool
Class2Modem::reset(long ms)
{
    return (FaxModem::reset(ms) && setupClass2Parameters());
}

/*
 * Abort an active Class 2 session.
 */
fxBool
Class2Modem::abort(long)
{
    return (class2Cmd(conf.class2AbortCmd));
}

ATResponse
Class2Modem::atResponse(char* buf, long ms)
{
    if (FaxModem::atResponse(buf, ms) == AT_OTHER &&
      (buf[0] == '+' && buf[1] == 'F')) {
	if (strneq(buf, "+FHNG:", 6)) {
	    processHangup(buf+6);
	    lastResponse = AT_FHNG;
	} else if (strneq(buf, "+FCON", 5))
	    lastResponse = AT_FCON;
	else if (strneq(buf, "+FPOLL", 6))
	    lastResponse = AT_FPOLL;
	else if (strneq(buf, "+FDIS:", 6))
	    lastResponse = AT_FDIS;
	else if (strneq(buf, "+FNSF:", 6))
	    lastResponse = AT_FNSF;
	else if (strneq(buf, "+FCSI:", 6))
	    lastResponse = AT_FCSI;
	else if (strneq(buf, "+FPTS:", 6))
	    lastResponse = AT_FPTS;
	else if (strneq(buf, "+FDCS:", 6))
	    lastResponse = AT_FDCS;
	else if (strneq(buf, "+FNSS:", 6))
	    lastResponse = AT_FNSS;
	else if (strneq(buf, "+FTSI:", 6))
	    lastResponse = AT_FTSI;
	else if (strneq(buf, "+FET:", 5))
	    lastResponse = AT_FET;
    }
    return (lastResponse);
}

/*
 * Wait (carefully) for some response from the modem.
 * In particular, beware of unsolicited hangup messages
 * from the modem.  Some modems seem to not use the
 * Class 2 required +FHNG response--and instead give
 * us an unsolicited NO CARRIER message.  Isn't life
 * wondeful?
 */
fxBool
Class2Modem::waitFor(ATResponse wanted, long ms)
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
	    modemTrace("ERROR: %s", ATresponses[response]);
	    return (FALSE);
	case AT_FHNG:
	    // return hangup status, but try to wait for requested response
	    { char buf[1024]; (void) atResponse(buf, 2*1000); }
	    return (isNormalHangup());
	}
    }
}

/*
 * Interfaces for sending a Class 2 command; i.e, AT+F<cmd>
 */

/*
 * Send <cmd> and wait for "OK"
 */
fxBool
Class2Modem::class2Cmd(const char* cmd)
{
    return vatFaxCmd(AT_OK, "%s", cmd);
}

/*
 * Send <cmd>=<a0> and wait for "OK"
 */
fxBool
Class2Modem::class2Cmd(const char* cmd, int a0)
{
    return vatFaxCmd(AT_OK, "%s=%u", cmd, a0);
}

/*
 * Send AT+F<cmd>=<t.30 parameters> and wait for "OK".
 */
fxBool
Class2Modem::class2Cmd(const char* cmd, const Class2Params& p)
{
    const fxStr& params = p.cmd();
    return vatFaxCmd(AT_OK, "%s=%s", cmd, (char*) params);
}

/*
 * Send AT+F<cmd>="<s>" and wait for "OK".
 */
fxBool
Class2Modem::class2Cmd(const char* cmd, const char* s)
{
    return vatFaxCmd(AT_OK, "%s=\"%s\"", cmd, s);	// XXX handle escapes
}

/*
 * Parse a Class 2 parameter specification
 * and return the resulting bit masks.
 */
fxBool
Class2Modem::parseRange(const char* cp, Class2Params& p)
{
    if (!vparseRange(cp, 8, &p.vr,&p.br,&p.wd,&p.ln,&p.df,&p.ec,&p.bf,&p.st))
	return (FALSE);
    p.vr &= VR_ALL;
    p.br &= BR_ALL;
    p.wd &= WD_ALL;
    p.ln &= LN_ALL;
    p.df &= DF_ALL;
    p.ec &= EC_ALL;
    p.bf &= BF_ALL;
    p.st &= ST_ALL;
    return TRUE;
}

/*
 * Hangup codes are broken up according to:
 *   2388/89
 *   2388/90 and 2388-A
 *   2388-B
 * The table to search is based on the modem type deduced
 * at modem configuration time.
 *
 * Note that we keep the codes as strings to avoid having
 * to distinguish the decimal numbers of 2388-A from the
 * hexadecimal renumbering done in 2388-B!
 */
static struct HangupCode {
    const char*	code[3];	// from 2388/89, 2388/90, 2388-A, and 2388-B
    const char*	message;	// what code means
} hangupCodes[] = {
// Call placement and termination
    {{  "0",  "0",  "0" }, "Normal and proper end of connection" },
    {{  "1",  "1",  "1" }, "Ring detect without successful handshake" },
    {{  "2",  "2",  "2" }, "Call aborted,  from +FK or <CAN>" },
    {{ NULL,  "3",  "3" }, "No loop current" },
    {{ NULL, NULL,  "4" }, "Ringback detected, no answer (timeout)" },
    {{ NULL, NULL,  "5" }, "Ringback detected, no answer without CED" },
// Transmit Phase A & miscellaneous errors
    {{ "10", "10", "10" }, "Unspecified Phase A error" },
    {{ "11", "11", "11" }, "No answer (T.30 T1 timeout)" },
// Transmit Phase B
    {{ "20", "20", "20" }, "Unspecified Transmit Phase B error" },
    {{ "21", "21", "21" }, "Remote cannot be polled" },
    {{ "22", "22", "22" }, "COMREC error in transmit Phase B/got DCN" },
    {{ "23", "23", "23" }, "COMREC invalid command received/no DIS or DTC" },
    {{ "24", "24", "24" }, "RSPREC error/got DCN" },
    {{ "25", "25", "25" }, "DCS sent 3 times without response" },
    {{ "26", "26", "26" }, "DIS/DTC received 3 times; DCS not recognized" },
    {{ "27", "27", "27" }, "Failure to train at 2400 bps or +FMINSP value" },
    {{ "28", "28", "28" }, "RSPREC invalid response received" },
// Transmit Phase C
    {{ "30", "40", "40" }, "Unspecified Transmit Phase C error" },
    {{ NULL, NULL, "41" }, "Unspecified Image format error" },
    {{ NULL, NULL, "42" }, "Image conversion error" },
    {{ "33", "43", "43" }, "DTE to DCE data underflow" },
    {{ NULL, NULL, "44" }, "Unrecognized Transparent data command" },
    {{ NULL, NULL, "45" }, "Image error, line length wrong" },
    {{ NULL, NULL, "46" }, "Image error, page length wrong" },
    {{ NULL, NULL, "47" }, "Image error, wrong compression code" },
// Transmit Phase D
    {{ "40", "50", "50" }, "Unspecified Transmit Phase D error, including"
			   " +FPHCTO timeout between data and +FET command" },
    {{ "41", "51", "51" }, "RSPREC error/got DCN" },
    {{ "42", "52", "52" }, "No response to MPS repeated 3 times" },
    {{ "43", "53", "53" }, "Invalid response to MPS" },
    {{ "44", "54", "54" }, "No response to EOP repeated 3 times" },
    {{ "45", "55", "55" }, "Invalid response to EOP" },
    {{ "46", "56", "56" }, "No response to EOM repeated 3 times" },
    {{ "47", "57", "57" }, "Invalid response to EOM" },
    {{ "48", "58", "58" }, "Unable to continue after PIN or PIP" },
// Received Phase B
    {{ "50", "70", "70" }, "Unspecified Receive Phase B error" },
    {{ "51", "71", "71" }, "RSPREC error/got DCN" },
    {{ "52", "72", "72" }, "COMREC error" },
    {{ "53", "73", "73" }, "T.30 T2 timeout, expected page not received" },
    {{ "54", "74", "74" }, "T.30 T1 timeout after EOM received" },
// Receive Phase C
    {{ "60", "90", "90" }, "Unspecified Phase C error, including too much delay"
			   " between TCF and +FDR command" },
    {{ "61", "91", "91" }, "Missing EOL after 5 seconds (section 3.2/T.4)" },
    {{ "63", "93", "93" }, "DCE to DTE buffer overflow" },
    {{ "64", "94", "92" }, "Bad CRC or frame (ECM or BFT modes)" },
// Receive Phase D
    {{ "70","100", "A0" }, "Unspecified Phase D error" },
    {{ "71","101", "A1" }, "RSPREC invalid response received" },
    {{ "72","102", "A2" }, "COMREC invalid response received" },
    {{ "73","103", "A3" }, "Unable to continue after PIN or PIP, no PRI-Q" },
// Everex proprietary error codes (9/28/90)
    {{ NULL,"128", NULL }, "Cannot send: +FMINSP > remote's +FDIS(BR) code" },
    {{ NULL,"129", NULL }, "Cannot send: remote is V.29 only,"
			   " local DCE constrained to 2400 or 4800 bps" },
    {{ NULL,"130", NULL }, "Remote station cannot receive (DIS bit 10)" },
    {{ NULL,"131", NULL }, "+FK aborted or <CAN> aborted" },
    {{ NULL,"132", NULL }, "+Format conversion error in +FDT=DF,VR, WD,LN"
			   " Incompatible and inconvertable data format" },
    {{ NULL,"133", NULL }, "Remote cannot receive" },
    {{ NULL,"134", NULL }, "After +FDR, DCE waited more than 30 seconds for"
			   " XON from DTE after XOFF from DTE" },
    {{ NULL,"135", NULL }, "In Polling Phase B, remote cannot be polled" },
};
#define	NCODES	(sizeof (hangupCodes) / sizeof (hangupCodes[0]))

/*
 * Given a hangup code from a modem return a
 * descriptive string.  We use strings here to
 * avoid having to know what type of modem we're
 * talking to (Rev A or Rev B or SP-2388).  This
 * works right now becase the codes don't overlap
 * (as strings).  Hopefully this'll continue to
 * be true.
 */
const char*
Class2Modem::hangupCause(const char* code)
{
    for (u_int i = 0; i < NCODES; i++) {
	const HangupCode& c = hangupCodes[i];
	if ((c.code[1] != NULL && strcasecmp(code, c.code[1]) == 0) ||
	    (c.code[2] != NULL && strcasecmp(code, c.code[2]) == 0))
	    return (c.message);
    }
    return ("Unknown code");
}

/*
 * Process a hangup code string.
 */
void
Class2Modem::processHangup(const char* cp)
{
    while (isspace(*cp))			// strip leading white space
	cp++;
    while (*cp == '0' && cp[1] != '\0')		// strip leading 0's
	cp++;
    strncpy(hangupCode, cp, sizeof (hangupCode));
    protoTrace("REMOTE HANGUP: %s (code %s)", hangupCause(hangupCode), hangupCode);
}

/*
 * Does the current hangup code string indicate
 * that the remote side hung up the phone in a
 * "normal and proper way".
 */
fxBool
Class2Modem::isNormalHangup()
{
    // normal hangup is "", "0", or "00"
    return (hangupCode[0] == '\0' ||
	(hangupCode[0] == '0' &&
	 (hangupCode[1] == '0' || hangupCode[1] == '\0')));
}

static const char* ppmCodes[] = {
    "MPS (more pages, same document)",
    "EOM (more documents)",
    "EOP (no more pages or documents)",
    "PRI-MPS (more pages after procedure interrupt)",
    "PRI-EOM (more documents after procedure interrupt)",
    "PRI-EOP (all done after procedure interrupt)",
};
#define	NPPMS	(sizeof (ppmCodes) / sizeof (ppmCodes[0]))

/*
 * Return a string the describes the post-page-message code.
 */
const char*
Class2Modem::ppmCodeName(int code)
{
    return (code < NPPMS ? ppmCodes[code] : "???");
}

static const char* pprCodes[] = {
    "#0 (unknown code)",
    "MCF (page good)",
    "RTN (page bad, retrain requested)",
    "RTP (page good, retrain requested)",
    "PIN (page bad, interrupt requested)",
    "PIP (page good, interrupt requested)",
};
#define	NPPRS	(sizeof (pprCodes) / sizeof (pprCodes[0]))

/*
 * Return a string the describes the post-page-response code.
 */
const char*
Class2Modem::pprCodeName(int code)
{
    return (code < NPPRS ? pprCodes[code] : "???");
}
