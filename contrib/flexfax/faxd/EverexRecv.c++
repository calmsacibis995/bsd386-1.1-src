/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/EverexRecv.c++,v 1.1.1.1 1994/01/14 23:09:47 torek Exp $
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
#include <stdio.h>
#include <time.h>
#include "Everex.h"
#include "ModemConfig.h"

#include "t.30.h"
#include "everex.h"
#include <stdlib.h>

/*
 * Recv Protocol for Class-1-style Everex modems.
 */

#define	FAX_RECVMODE	(S2_HOSTCTL|S2_PADEOLS|S2_19200)

CallType
EverexModem::answerCall(AnswerType atype, fxStr& emsg)
{
    if (atype == FaxModem::ANSTYPE_FAX || atype == FaxModem::ANSTYPE_ANY) {
	if (!atCmd("#A#S8=8#S9=45V1&E1S0=0X4"))
	    goto bad;
	if (!modemFaxConfigure(FAX_RECVMODE|S2_RESET))
	    goto bad;
	if (!setupFrame(FCF_DIS|FCF_RCVR, modemDIS()))
	    goto bad;
	if (!setupFrame(FCF_CSI|FCF_RCVR, (char*) lid))
	    goto bad;
	if (!atCmd("#T2100=30")) {
    bad:
	    emsg = "Unspecified Receive Phase B error";
	    return (CALLTYPE_ERROR);
	} else
	    return (CALLTYPE_FAX);
    }
    return (FaxModem::answerCall(atype, emsg));
}

fxBool
EverexModem::recvBegin(fxStr& emsg)
{
    setInputBuffering(FALSE);
    prevPage = FALSE;				// no previous page received
    pageGood = FALSE;				// quality of received page
    return recvIdentification(FCF_CSI|FCF_RCVR, FCF_DIS|FCF_RCVR, emsg);
}

/*
 * Transmit DIS/DTC and wait for a response.
 */
fxBool
EverexModem::recvIdentification(u_int f1, u_int f2, fxStr& emsg)
{
    u_int t1 = howmany(conf.t1Timer, 1000);	// T1 in seconds
    time_t start = time(0);
    emsg = "No answer (T.30 T1 timeout)";
    do {
	/*
	 * Transmit (NSF) (CSI) DIS frames when the receiving
	 * station or (NSC) (CIG) DTC when initiating a poll.
	 */
	if (sendFrame(f1, f2)) {
	    /*
	     * Wait for a response to be received.
	     */
	    if (recvFrame(rbuf, conf.t2Timer)) {// XXX should be TIMER_T4
		do {
		    /*
		     * Verify a DCS command response and, if
		     * all is correct, receive phasing/training.
		     */
		    u_int fcf;
		    if (!recvDCSFrames(rbuf+2, fcf)) {
			fcf &= ~FCF_SNDR;
			if (fcf == FCF_DCN)
			    emsg = "RSPREC error/got DCN";
			else			// XXX DTC/DIS not handled
			    emsg = "RSPREC invalid response received";
			break;
		    }
		    if (recvTraining())
			return (TRUE);
		    emsg = "Failure to train modems";
		} while (recvFrame(rbuf, conf.t2Timer));
	    }
	}
	/*
	 * If we failed to send our frames or failed to
	 * receive DCS from the other side, then delay
	 * long enough to miss any training that the other
	 * side might have sent us.  Otherwise the caller
	 * will miss our retransmission since it'll be
	 * in the process of sending training.
	 */
	pause(conf.class1TrainingRecovery);
    } while (time(0)-start < t1);
    return (FALSE);
}

fxBool
EverexModem::recvDCSFrames(const char* cp, u_int& fcf)
{
    for (; *cp; cp += 3) {
	fcf = fromHex(cp, 2);
	switch (fcf) {
	case FCF_NSS|FCF_SNDR:
	    (void) recvNSS();
	    break;
	case FCF_DCS|FCF_SNDR:
	    (void) recvDCS();
	    break;
	case FCF_TSI|FCF_SNDR:
	    (void) recvTSI();
	    break;
	case FCF_CRP:		// resend last frame
	    break;
	}
    }
    return (fcf == (FCF_DCS|FCF_SNDR));
}

/*
 * Wait-for and decode training response.
 */
fxBool
EverexModem::recvTraining()
{
    protoTrace("RECV training");
    startTimeout(conf.t1Timer);
    char buf[1024];
    while (getModemLine(buf) > 2 && !strneq(buf, "M ", 2))
	;
    stopTimeout("reading from modem");
    pause(conf.class1TCFResponseDelay);	// delay between carrier switch
    fxBool ok = (!wasTimeout() && strneq(buf, "M 3", 3));
    if (ok) {
	sendFrame(FCF_CFR|FCF_RCVR);
	protoTrace("TRAINING succeeded");
    } else {
	sendFrame(FCF_FTT|FCF_RCVR);
	protoTrace("TRAINING failed");
    }
    return (ok);
}

/*
 * Decode a received TSI and ask the server if
 * we should accept documents from the client.
 */
fxBool
EverexModem::recvTSI()
{
    if (atCmd("#FRC2?", AT_NOTHING)) {
	char buf[1024];
	int n = getModemLine(buf);
	if (sync()) {
	    fxStr tsi;
	    if (n > 2)
		decodeCSI(tsi, fxStr(buf, n));
	    (void) recvCheckTSI(tsi);
	    return (TRUE);
	}
    }
    return (FALSE);
}

/*
 * Decode and process a received DCS.
 */
fxBool
EverexModem::recvDCS()
{
    if (atCmd("#FRC1?", AT_NOTHING)) {
	char buf[1024];
	int n = getModemLine(buf);
	if (sync() && n >= 6) {
	    u_int dcs = fromHex(buf, fxmin(n,6));
	    u_int xinfo =
		((dcs & DCS_XTNDFIELD) && n >= 8 ? fromHex(buf+6, 2) : 0);
	    Class2Params params;
	    params.setFromDCS(dcs, xinfo);
	    is2D = (params.df >= DF_2DMR);		// NB: assumes no G4
	    setDataTimeout(60, params.br);
	    FaxModem::recvDCS(params);
	    return (TRUE);
	}
    }
    return (FALSE);
}

/*
 * Process a received NSS.
 */
fxBool
EverexModem::recvNSS()
{
    if (atCmd("#FRC4?", AT_NOTHING)) {
	char buf[1024];
	int n = getModemLine(buf);
	if (sync()) {
	    protoTrace("REMOTE NSS \"%s\"", buf);
	    return (TRUE);
	}
    }
    return (FALSE);
}

const u_int EverexModem::modemPPMCodes[8] = {
    0,			// 0
    PPM_EOM,		// FCF_EOM+FCF_PRI_EOM
    PPM_MPS,		// FCF_MPS+FCF_PRI_MPS
    0,			// 3
    PPM_EOP,		// FCF_EOP+FCF_PRI_EOP
    0,			// 5
    0,			// 6
    0,			// 7
};
const char* EverexModem::ppmNames[16] = {
    NULL,				// 0
    "EOM (more documents)",		// FCF_EOM
    "MPS (more pages, same document)",	// FCF_MPS
    NULL,				// 3
    "EOP (no more pages or documents)",	// FCF_EOP
    NULL,				// 5
    NULL,				// 6
    NULL,				// 7
    "CRP (repeat last command)",	// 8
    "PRI-EOM (more documents after interrupt)",	// FCF_PRI_EOM
    "PRI-MPS (more pages after interrupt)",	// FCF_PRI_MPS
    NULL,				// 11
    "PRI-EOP (nor more pages after interrupt)",	// FCF_PRI_EOP
    NULL,				// 13
    NULL,				// 14
    "DCN (disconnect)",			// 15
};

/*
 * Receive a page of data.
 */
fxBool
EverexModem::recvPage(TIFF* tif, int& ppm, fxStr& emsg)
{
    fxBool messageReceived = FALSE;
    while (getModemLine(rbuf, conf.t1Timer) >= 3) {
	if (strneq(rbuf, "M 1", 3)) {		// message carrier received
	    /*
	     * Message carrier received; receive page.
	     */
	    setInputBuffering(TRUE);
	    protoTrace("RECV: begin page");
	    (void) atCmd("#P7");		// switch to high speed carrier
	    u_int br = (FAX_RECVMODE & S2_9600 ? BR9600 : BR19200);
	    if (setBaudRate(br, FLOW_NONE, FLOW_XONXOFF)) {
		recvSetupPage(tif, GROUP3OPT_FILLBITS, FILLORDER_LSB2MSB);
		pageGood = recvPageData(tif, emsg);
		(void) sendBreak(FALSE);	// back to low speed
		(void) setBaudRate(EVEREX_CMDBRATE, FLOW_XONXOFF, FLOW_XONXOFF);
		protoTrace("RECV: end page");
		if (wasTimeout()) {
		    setInputBuffering(FALSE);
		    return (FALSE);
		}
		/*
		 * The data was received correctly, wait
		 * for the modem to signal carrier drop.
		 */
		messageReceived = waitFor(AT_OK);
		setInputBuffering(FALSE);
		if (!messageReceived)
		    return (FALSE);
		prevPage = TRUE;
	    } else {
		emsg = "Can not change baud rate for Phase C data";
		(void) sendBreak(FALSE);	// back to low speed
	    }
	} else if (strneq(rbuf, "R ", 2)) {	// HDLC frames received
	    /*
	     * Do <command received> logic.
	     */
	    u_int fcf = fromHex(rbuf+2, 2);
	    switch (fcf) {
	    case FCF_DTC:			// XXX no support
	    case FCF_DIS:			// XXX no support
		protoTrace("RECV DIS/DTC");
		emsg = "Can not continue after DIS/DTC";
		return (FALSE);
	    case FCF_NSS|FCF_SNDR:
	    case FCF_TSI|FCF_SNDR:
	    case FCF_DCS|FCF_SNDR:
		if (recvDCSFrames(rbuf+2, fcf))
		    (void) recvTraining();
		messageReceived = FALSE;
		break;
	    case FCF_MPS|FCF_SNDR:		// MPS
	    case FCF_EOM|FCF_SNDR:		// EOM
	    case FCF_EOP|FCF_SNDR:		// EOP
	    case FCF_PRI_MPS|FCF_SNDR:		// PRI-MPS
	    case FCF_PRI_EOM|FCF_SNDR:		// PRI-EOM
	    case FCF_PRI_EOP|FCF_SNDR:		// PRI-EOP
		protoTrace("RECV PPM %s", ppmNames[fcf&0xf]);
		if (!prevPage) {
		    /*
		     * Post page message, but no previous page
		     * was received--this violates the protocol.
		     */
		    emsg = "COMREC invalid response received";
		    return (FALSE);
		}
		/*
		 * [Re]transmit post page response.
		 */
		if (pageGood) {
		    (void) sendFrame(FCF_MCF|FCF_RCVR);
		    /*
		     * If post page message confirms the page
		     * that we just received, write it to disk.
		     */
		    if (messageReceived) {
			TIFFWriteDirectory(tif);
			countPage();
			ppm = modemPPMCodes[fcf&7];
			return (TRUE);
		    }
		} else {
		    /*
		     * Page not received, or unacceptable; tell
		     * other side to retransmit after retrain.
		     */
		    (void) sendFrame(FCF_RTN|FCF_RCVR);
		    messageReceived = FALSE;
		    /*
		     * Reset the TIFF-related state so that subsequent
		     * writes will overwrite the previous data.
		     */
		    recvResetPage(tif);
		}
		break;
	    case FCF_DCN|FCF_SNDR:		// DCN
		protoTrace("RECV %s", ppmNames[fcf&0xf]);
		emsg = "COMREC received DCN";
		return (FALSE);
	    default:
		emsg = "COMREC invalid response received";
		return (FALSE);
	    }
	} else {
	    modemTrace("Unrecognized response \"%s\"", rbuf);
	    emsg = "Unrecognized response from modem";
	    return (FALSE);
	}
    }
    emsg = "T.30 T2 timeout, expected page not received";
    return (FALSE);
}

static const int EOL = 0x001;		// end-of-line code (11 0's + 1)

#define	BITCASE(b)			\
    case b:				\
	c <<= 1;			\
	if (shdata & b) c |= 1;		\
	l++;				\
	if (c > 0) { shbit = (b<<1); break; }

/*
 * Receive a Huffman code.
 */
void
EverexModem::recvCode(int& len, int& code)
{
    short c = 0;
    short l = 0;
    switch (shbit & 0xff) {
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
	shdata = getModemDataChar();
	if (shdata == EOF)
	    longjmp(recvEOF, 1);
	goto again;
    }
    code = c;
    len = l;
}

/*
 * Return the next bit from the modem.
 */
int
EverexModem::recvBit()
{
    if ((shbit & 0xff) == 0) {
	shdata = getModemDataChar();
	if (shdata == EOF)
	    longjmp(recvEOF, 1);
	shbit = 0x01;
    }
    int b = (shdata & shbit) != 0;
    shbit <<= 1;
    return (b);
}

/*
 * Receive a page of G3-encoded data.  EOLs are counted and
 * bad rows are replaced with a previous good rows received.
 */
fxBool
EverexModem::recvPageData(TIFF* tif, fxStr& emsg)
{
    u_long badfaxrows = 0;		// # of rows w/ errors
    int badfaxrun = 0;			// current run of bad rows
    int	maxbadfaxrun = 0;		// longest bad run
    u_char thisrow[howmany(2432,8)];	// current accumulated row
    u_char lastrow[howmany(2432,8)];	// previous row for regeneration
    int eols = 0;			// count of consecutive EOL codes
    int lastlen = 0;			// length of last row received (bytes)
    u_long row = 0;			// received row number
    int tag = 0;			// 2D decoding tag that follows EOL
    u_long totbytes = 0;		// total bytes of data received
    u_char* ep = &thisrow[sizeof (thisrow)-2];

    if (setjmp(recvEOF) != 0) {
	emsg = "Missing EOL after 5 seconds";
	protoTrace("RECV: premature EOF, row %lu", row);
	recvDone(tif, totbytes, row, badfaxrows, maxbadfaxrun);
	return (FALSE);
    }
top:
    u_char* cp = &thisrow[0];		// place to put next byte
    int bit = 0x80;			// current bit of received data
    int data = 0;			// current received data byte
    fxBool emptyLine = TRUE;		// is current line empty?
    /*
     * The "2" here means that we treat 2 consecutive
     * EOL codes (w/o intervening data) as RTC.  This
     * is a cheat, but means that we don't hang if
     * the RTC is chopped by the sender or the modem.
     * Since the many other fax machines do this and
     * the Class 2 spec says that it will always compress
     * consecutive EOL codes to one, this should be ok.
     */
    while (eols < 2) {
	int len, code;
	recvCode(len, code);
	if (len >= 12) {
	    if (code == EOL) {
		/*
		 * Found an EOL, flush the current row and
		 * check for RTC (6 consecutive EOL codes).
		 */
		if (is2D)
		    tag = recvBit();
		if (!emptyLine) {
		    if (bit != 0x80)			// pad to byte boundary
			*cp++ = data;
		    // insert EOL and tag bit if 2D
		    *cp++ = 0x00;
		    if (is2D)
			*cp++ = 0x02|tag;
		    else
			*cp++ = 0x01;
		    lastlen = cp - thisrow;
		    TIFFReverseBits(thisrow, lastlen);
		    TIFFWriteRawStrip(tif, 0, thisrow, lastlen);
		    memcpy(lastrow, thisrow, lastlen);	// save for regeneration
		    row++;
		    totbytes += lastlen;
		    eols = 0;
		} else
		    eols++;
		if (badfaxrun > maxbadfaxrun)
		    maxbadfaxrun = badfaxrun;
		badfaxrun = 0;
		goto top;
	    }
	    if (len > 13) {
		/*
		 * Encountered a bogus code word; skip to the
		 * EOL and regenerate the previous line.   Note
		 * that for 2D data, the regeneration uses the
		 * last line, not the last 1D-encoded line--this
		 * is WRONG (we should also be careful about 2D
		 * lines that follow a regenerated line).
		 */
		protoTrace("RECV: bad code word 0x%x, len %d, row %lu",
		    code, len, row);
		badfaxrows++, badfaxrun++;
		// skip input data to next EOL
		do {
		    recvCode(len, code);
		} while (len < 12 || code != EOL);
		if (is2D)
		    tag = recvBit();
		// regenerate previous row
		TIFFWriteRawStrip(tif, 0, lastrow, lastlen);
		goto top;
	    }
	}
	// shift code into scanline buffer (could unroll expand loop)
	for (u_int mask = 1<<(len-1); mask; mask >>= 1) {
	    if (code & mask)
		data |= bit;
	    if ((bit >>= 1) == 0) {
		if (cp < ep)
		    *cp++ = data;
		data = 0;
		bit = 0x80;
	    }
	}
	emptyLine = FALSE;
    }
    recvDone(tif, totbytes, row, badfaxrows, maxbadfaxrun);
    return (TRUE);		// XXX check copy quality
}

/*
 * Force image length and write data characterization tags.
 */
void
EverexModem::recvDone(TIFF* tif, u_long totbytes, u_long nrows, u_long bad, int maxbad)
{
    protoTrace("RECV: %lu bytes, %lu rows", totbytes, nrows);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, nrows);
    if (bad) {
	TIFFSetField(tif, TIFFTAG_BADFAXLINES,	bad);
	TIFFSetField(tif, TIFFTAG_CLEANFAXDATA,	CLEANFAXDATA_REGENERATED);
	TIFFSetField(tif, TIFFTAG_CONSECUTIVEBADFAXLINES, maxbad);
    } else
	TIFFSetField(tif, TIFFTAG_CLEANFAXDATA,	CLEANFAXDATA_CLEAN);
}

/*
 * Complete a receive session.
 */
fxBool
EverexModem::recvEnd(fxStr&)
{
    (void) recvFrame(rbuf, conf.t2Timer);	// accept any DCN
    u_int t1 = howmany(conf.t1Timer, 1000);	// T1 timer in seconds
    time_t start = time(0);
    /*
     * Wait for DCN and retransmit ack of EOP if needed.
     */
    u_int fcf = FCF_EOP|FCF_SNDR;
    do {
	if (recvFrame(rbuf, conf.t2Timer)) {
	    switch (fcf = fromHex(rbuf+2, 2)) {
	    case FCF_EOP|FCF_SNDR:
		protoTrace("RECV PPM %s", ppmNames[FCF_EOP&0xf]);
		(void) sendFrame(FCF_MCF|FCF_RCVR);
		break;
	    case FCF_DCN|FCF_SNDR:
		break;
	    default:
		sendFrame(FCF_DCN|FCF_RCVR);
		break;
	    }
	}
    } while (time(0)-start < t1 && fcf == (FCF_EOP|FCF_SNDR));
    setInputBuffering(TRUE);
    return (TRUE);
}
