/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/ModemConfig.c++,v 1.1.1.1 1994/01/14 23:09:49 torek Exp $
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
#include "ModemConfig.h"
#include "t.30.h"
#include <string.h>
#include <syslog.h>

/*
 * We append the "@" symbol to the dial string so that
 * the modem will wait 5 seconds before attempting to
 * connect.  This makes it possible to get return codes
 * that more indicate if the phone was answered w/o a
 * carrier or wether the phone wasn't answered at all.
 * We then assume that if the phone was answered and
 * there was no carrier that a person must have answered.
 * (Kudos to Stuart Lynne for this trick.)
 *
 * Unfortunately, ZyXEL's don't seem to detect a busy
 * signal when the @ symbol is used to terminate the
 * dialing string.  Instead calls to a busy line come
 * back with a NO CARRIER status which causes us to
 * toss the job.  Consequently we must fall back to the
 * normal method and hope the conservative call status
 * checking in the mainstream server is sufficient.
 */
ModemConfig::ModemConfig()
    : type("unknown")
    , dialCmd("DT%s@")			// %s = phone number
    , noAutoAnswerCmd("S0=0")
    , echoOffCmd("E0")
    , verboseResultsCmd("V1")
    , resultCodesCmd("Q0")
    , onHookCmd("H0")
    , softResetCmd("Z")
    , waitTimeCmd("S7=30")		// wait time is 30 seconds
    , pauseTimeCmd("S8=2")		// comma pause time is 2 seconds
    , class2AbortCmd("K")		// class 2 session abort
    , class1Cmd("+FCLASS=1")		// set class 1 (fax)
    , class2Cmd("+FCLASS=2")		// set class 2 (fax)
{
    class2BORCmd = "BOR=" | fxStr(BOR_C_DIR + BOR_BD_DIR, "%u");
    class2XmitWaitForXON = FALSE;	// default suits most Class 2 modems

    // default volume setting commands
    setVolumeCmds("M0 L0M1 L1M1 L2M1 L3M1");

    answerCmd[FaxModem::ANSTYPE_ANY] = "A";

    flowControl = FaxModem::FLOW_XONXOFF;// expect XON/XOFF flow control
    maxRate = FaxModem::BR19200;	// reasonable for most modems
    sendFillOrder = FILLORDER_LSB2MSB;	// default to CCITT bit order
    recvFillOrder = FILLORDER_LSB2MSB;	// default to CCITT bit order
    frameFillOrder = FILLORDER_LSB2MSB;	// default to CCITT bit order
    hostFillOrder = FILLORDER_MSB2LSB;	// default to most common

    resetDelay = 2600;			// 2.6 second delay after reset
    baudRateDelay = 0;			// delay after setting baud rate

    t1Timer = TIMER_T1;			// T.30 T1 timer (ms)
    t2Timer = TIMER_T2;			// T.30 T2 timer (ms)
    t4Timer = TIMER_T4;			// T.30 T4 timer (ms)
    dialResponseTimeout = 3*60*1000;	// dialing command timeout (ms)
    answerResponseTimeout = 3*60*1000;	// answer command timeout (ms)
    pageStartTimeout = 3*60*1000;	// page send/receive timeout (ms)
    pageDoneTimeout = 3*60*1000;	// page send/receive timeout (ms)

    class1TCFResponseDelay = 75;	// 75ms delay between TCF and ack/nak
    class1SendPPMDelay = 75;		// 75ms delay before sending PPM
    class1SendTCFDelay = 75;		// 75ms delay between sending DCS & TCF
    class1TrainingRecovery = 1500;	// 1.5sec delay after failed training
    class1RecvAbortOK = 200;		// 200ms after abort before flushing OK
    class1FrameOverhead = 4;		// flags + station id + 2-byte FCS
    class1RecvIdentTimer = t1Timer;	// default to standard protocol

    maxPacketSize = 16*1024;		// max write to modem
    interPacketDelay = 0;		// delay between modem writes
    waitForConnect = FALSE;		// unique answer response from modem
}

ModemConfig::~ModemConfig()
{
}

#ifdef streq
#undef streq
#endif
#define	streq(a,b)	(strcasecmp(a,b)==0)

static fxBool getBoolean(const char* cp)
    { return (streq(cp, "on") || streq(cp, "yes")); }

static BaudRate
getRate(const char* cp)
{
    if (streq(cp, "300"))
	return (FaxModem::BR300);
    else if (streq(cp, "1200"))
	return (FaxModem::BR1200);
    else if (streq(cp, "2400"))
	return (FaxModem::BR2400);
    else if (streq(cp, "4800"))
	return (FaxModem::BR4800);
    else if (streq(cp, "9600"))
	return (FaxModem::BR9600);
    else if (streq(cp, "19200"))
	return (FaxModem::BR19200);
    else if (streq(cp, "38400"))
	return (FaxModem::BR38400);
    else if (streq(cp, "57600"))
	return (FaxModem::BR57600);
    else {
	syslog(LOG_ERR, "Unknown baud rate \"%s\", using 19200", cp);
	return (FaxModem::BR19200);	// default
    }
}

static u_int
getFill(const char* cp)
{
    if (streq(cp, "LSB2MSB"))
	return (FILLORDER_LSB2MSB);
    else if (streq(cp, "MSB2LSB"))
	return (FILLORDER_MSB2LSB);
    else {
	syslog(LOG_ERR, "Unknown fill order \"%s\"", cp);
        return ((u_int) -1);
    }
}

static FlowControl
getFlow(const char* cp)
{
    if (streq(cp, "xonxoff"))
	return (FaxModem::FLOW_XONXOFF);
    else if (streq(cp, "rtscts"))
	return (FaxModem::FLOW_RTSCTS);
    else {
	syslog(LOG_ERR, "Unknown flow control \"%s\", using xonxoff", cp);
	return (FaxModem::FLOW_XONXOFF);	// default
    }
}

void
ModemConfig::setVolumeCmds(const fxStr& tag)
{
    u_int l = 0;
    for (int i = FaxModem::OFF; i <= FaxModem::HIGH; i++) {
	fxStr tmp = tag.token(l, " \t");		// NB: for gcc
	setVolumeCmd[i] = parseATCmd(tmp);
    }
}

/*
 * Scan AT command strings and convert <...> escape
 * commands into single-byte escape codes that are
 * interpreted by FaxModem::atCmd.  Note that the
 * baud rate setting commands are carefully ordered
 * so that the desired baud rate can be extracted
 * from the low nibble.
 */
fxStr
ModemConfig::parseATCmd(const char* cp)
{
    fxStr cmd(cp);
    u_int pos = 0;
    while ((pos = cmd.next(pos, '<')) != cmd.length()) {
	u_int epos = pos+1;
	fxStr esc = cmd.token(epos, '>');
	esc.lowercase();

	char ecode = 0;
	if (esc == "xon")
	    ecode = ESC_XON;
	else if (esc == "rts")
	    ecode = ESC_RTS;
	else if (esc == "2400")
	    ecode = ESC_BR2400;
	else if (esc == "4800")
	    ecode = ESC_BR4800;
	else if (esc == "9600")
	    ecode = ESC_BR9600;
	else if (esc == "19200")
	    ecode = ESC_BR19200;
	else if (esc == "38400")
	    ecode = ESC_BR38400;
	else if (esc == "57600")
	    ecode = ESC_BR57600;
	else if (esc == "")		// NB: "<>" => <
	    ecode = '<';
	else {
	    syslog(LOG_ERR, "Unknown AT escape code \"%s\"", (char*) esc);
	    pos = epos;
	}
	if (ecode) {
	    cmd.remove(pos, epos-pos);
	    cmd.insert(ecode, pos);
	}
    }
    return (cmd);
}

fxBool
ModemConfig::parseItem(const char* tag, const char* value)
{
    fxBool recognized = TRUE;

    if (streq(tag, "ModemType"))
	type = value;
    else if (streq(tag, "ModemResetCmds") || streq(tag, "ModemResetCmd"))
	resetCmds = parseATCmd(value);
    else if (streq(tag, "ModemDialCmd"))
	dialCmd = parseATCmd(value);
    else if (streq(tag, "ModemAnswerCmd") || streq(tag, "ModemAnswerAnyCmd"))
	answerCmd[FaxModem::ANSTYPE_ANY] = parseATCmd(value);
    else if (streq(tag, "ModemAnswerFaxCmd"))
	answerCmd[FaxModem::ANSTYPE_FAX] = parseATCmd(value);
    else if (streq(tag, "ModemAnswerDataCmd"))
	answerCmd[FaxModem::ANSTYPE_DATA] = parseATCmd(value);
    else if (streq(tag, "ModemAnswerVoiceCmd"))
	answerCmd[FaxModem::ANSTYPE_VOICE] = parseATCmd(value);
    else if (streq(tag, "ModemAnswerFaxBeginCmd"))
	answerBeginCmd[FaxModem::CALLTYPE_FAX] = parseATCmd(value);
    else if (streq(tag, "ModemAnswerDataBeginCmd"))
	answerBeginCmd[FaxModem::CALLTYPE_DATA] = parseATCmd(value);
    else if (streq(tag, "ModemAnswerVoiceBeginCmd"))
	answerBeginCmd[FaxModem::CALLTYPE_VOICE] = parseATCmd(value);
    else if (streq(tag, "ModemFlowControlCmd"))
	flowControlCmd = parseATCmd(value);
    else if (streq(tag, "ModemSetupAACmd"))
	setupAACmd = parseATCmd(value);
    else if (streq(tag, "ModemSetupDTRCmd"))
	setupDTRCmd = parseATCmd(value);
    else if (streq(tag, "ModemSetupDCDCmd"))
	setupDCDCmd = parseATCmd(value);
    else if (streq(tag, "ModemNoAutoAnswerCmd"))
	noAutoAnswerCmd = parseATCmd(value);
    else if (streq(tag, "ModemSetVolumeCmd"))
	setVolumeCmds(value);
    else if (streq(tag, "ModemEchoOffCmd"))
	echoOffCmd = parseATCmd(value);
    else if (streq(tag, "ModemVerboseResultsCmd"))
	verboseResultsCmd = parseATCmd(value);
    else if (streq(tag, "ModemResultCodesCmd"))
	resultCodesCmd = parseATCmd(value);
    else if (streq(tag, "ModemOnHookCmd"))
	onHookCmd = parseATCmd(value);
    else if (streq(tag, "ModemSoftResetCmd"))
	softResetCmd = parseATCmd(value);
    else if (streq(tag, "ModemWaitTimeCmd"))
	waitTimeCmd = parseATCmd(value);
    else if (streq(tag, "ModemCommaPauseTimeCmd"))
	pauseTimeCmd = parseATCmd(value);
    else if (streq(tag, "ModemFlowControl"))
	flowControl = getFlow(value);
    else if (streq(tag, "ModemMaxRate") || streq(tag, "ModemRate"))
	maxRate = getRate(value);
    else if (streq(tag, "ModemRecvFillOrder"))
	recvFillOrder = getFill(value);
    else if (streq(tag, "ModemSendFillOrder"))
	sendFillOrder = getFill(value);
    else if (streq(tag, "ModemFrameFillOrder"))
	frameFillOrder = getFill(value);
    else if (streq(tag, "ModemHostFillOrder"))
	hostFillOrder = getFill(value);
    else if (streq(tag, "ModemResetDelay"))
	resetDelay = atoi(value);
    else if (streq(tag, "ModemBaudRateDelay"))
	baudRateDelay = atoi(value);
    else if (streq(tag, "ModemMaxPacketSize"))
	maxPacketSize = atoi(value);
    else if (streq(tag, "ModemInterPacketDelay"))
	interPacketDelay = atoi(value);
    else if (streq(tag, "ModemMfrQueryCmd"))
	mfrQueryCmd = parseATCmd(value);
    else if (streq(tag, "ModemModelQueryCmd"))
	modelQueryCmd = parseATCmd(value);
    else if (streq(tag, "ModemRevQueryCmd"))
	revQueryCmd = parseATCmd(value);
    else if (streq(tag, "ModemWaitForConnect"))
	waitForConnect = getBoolean(value);

    else if (streq(tag, "FaxT1Timer"))		t1Timer = atoi(value);
    else if (streq(tag, "FaxT2Timer"))		t2Timer = atoi(value);
    else if (streq(tag, "FaxT4Timer"))		t4Timer = atoi(value);
    else if (streq(tag, "ModemDialResponseTimeout"))
	dialResponseTimeout = atoi(value);
    else if (streq(tag, "ModemAnswerResponseTimeout"))
	answerResponseTimeout = atoi(value);
    else if (streq(tag, "ModemPageStartTimeout"))
	pageStartTimeout = atoi(value);
    else if (streq(tag, "ModemPageDoneTimeout"))
	pageDoneTimeout = atoi(value);

	// Class 1-specific configuration controls
    else if (streq(tag, "Class1Cmd"))
	class1Cmd = parseATCmd(value);
    else if (streq(tag, "Class1TCFResponseDelay"))
	class1TCFResponseDelay = atoi(value);
    else if (streq(tag, "Class1SendPPMDelay"))
	class1SendPPMDelay = atoi(value);
    else if (streq(tag, "Class1SendTCFDelay"))
	class1SendTCFDelay = atoi(value);
    else if (streq(tag, "Class1TrainingRecovery"))
	class1TrainingRecovery = atoi(value);
    else if (streq(tag, "Class1RecvAbortOK"))
	class1RecvAbortOK = atoi(value);
    else if (streq(tag, "Class1FrameOverhead"))
	class1FrameOverhead = atoi(value);
    else if (streq(tag, "Class1RecvIdentTimer"))
	class1RecvIdentTimer = atoi(value);

	// Class 2-specific configuration controls
    else if (streq(tag, "Class2Cmd"))
	class2Cmd = parseATCmd(value);
    else if (streq(tag, "Class2BORCmd"))
	class2BORCmd = parseATCmd(value);
    else if (streq(tag, "Class2RELCmd"))
	class2RELCmd = parseATCmd(value);
    else if (streq(tag, "Class2CQCmd"))
	class2CQCmd = parseATCmd(value);
    else if (streq(tag, "Class2AbortCmd"))
	class2AbortCmd = parseATCmd(value);
    else if (streq(tag, "Class2RecvDataTrigger"))
	class2RecvDataTrigger = value;
    else if (streq(tag, "Class2XmitWaitForXON"))
	class2XmitWaitForXON = getBoolean(value);

	// for backwards compatibility
    else if (streq(tag, "WaitForCarrier"))
	waitTimeCmd = "S7=" | fxStr(value);
    else if (streq(tag, "CommaPauseTime"))
	pauseTimeCmd = "S8=" | fxStr(value);
    else if (streq(tag, "ModemXONXOFF"))	// old way
	flowControl = (getBoolean(value) ?
			FaxModem::FLOW_XONXOFF : FaxModem::FLOW_RTSCTS);
    else
	recognized = FALSE;
    return (recognized);
}
