/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/Class1Recv.c++,v 1.1.1.1 1994/01/14 23:09:45 torek Exp $
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
 *
 * Receive protocol.
 */
#include <stdio.h>
#include "Class1.h"
#include "ModemConfig.h"
#include "HDLCFrame.h"
#include "StackBuffer.h"		// XXX

#include "t.30.h"
#include <stdlib.h>
#include <time.h>

/*
 * Tell the modem to answer the phone.  We override
 * this method so that we can force the terminal's
 * flow control state to be setup to our liking.
 */
CallType
Class1Modem::answerCall(AnswerType type, fxStr& emsg)
{
    if (flowControl == FLOW_XONXOFF)
	setXONXOFF(FLOW_NONE, FLOW_NONE, ACT_FLUSH);
    return FaxModem::answerCall(type, emsg);
}

/*
 * Process an answer response from the modem.
 * Since some Class 1 modems do not give a connect
 * message that distinguishes between DATA and FAX,
 * we override the default handling of "CONNECT"
 * message here to force the high level code to
 * probe further.
 */
const AnswerMsg*
Class1Modem::findAnswer(const char* s)
{
    static const AnswerMsg answer = {
	"CONNECT", 7,
	FaxModem::AT_NOTHING, FaxModem::OK, FaxModem::CALLTYPE_UNKNOWN
    };
    return strneq(s, answer.msg, answer.len) ?
	&answer : FaxModem::findAnswer(s);
}

/*
 * Begin the receive protocol.
 */
fxBool
Class1Modem::recvBegin(fxStr& emsg)
{
    setInputBuffering(FALSE);
    prevPage = FALSE;				// no previous page received
    pageGood = FALSE;				// quality of received page

    return recvIdentification(
	FCF_CSI|FCF_RCVR, lid, FCF_DIS|FCF_RCVR, modemDIS(),
	conf.class1RecvIdentTimer, emsg);
}

/*
 * Transmit local identification and wait for the
 * remote side to respond with their identification.
 */
fxBool
Class1Modem::recvIdentification(u_int f1, const fxStr& id, u_int f2, u_int dics,
    u_int timer, fxStr& emsg)
{
    u_int t1 = howmany(timer, 1000);		// in seconds
    u_int trecovery = howmany(conf.class1TrainingRecovery, 1000);
    time_t start = time(0);
    HDLCFrame frame(conf.class1FrameOverhead);

    emsg = "No answer (T.30 T1 timeout)";
    /*
     * Transmit (NSF) (CSI) DIS frames when the receiving
     * station or (NSC) (CIG) DTC when initiating a poll.
     */
    startTimeout(3000);
    fxBool framesSent = sendFrame(f1, id, FALSE);
    stopTimeout("sending id frame");
    for (;;) {
	if (framesSent) {
	    startTimeout(2550);
	    framesSent = sendFrame(f2, dics);
	    stopTimeout("sending DIS/DCS frame");
	}
	if (framesSent) {
	    /*
	     * Wait for a response to be received.
	     */
	    if (recvFrame(frame, conf.t4Timer)) {
		do {
		    /*
		     * Verify a DCS command response and, if
		     * all is correct, receive phasing/training.
		     */
		    if (!recvDCSFrames(frame)) {
			u_int fcf = frame.getFCF() &~ FCF_SNDR;
			if (fcf == FCF_DCN)
			    emsg = "RSPREC error/got DCN";
			else			// XXX DTC/DIS not handled
			    emsg = "RSPREC invalid response received";
			break;
		    }
		    if (recvTraining()) {
			emsg = "";
			return (TRUE);
		    }
		    emsg = "Failure to train modems";
		    /*
		     * Reset the timeout to insure the T1 timer is
		     * used.  This is done because the adaptive answer
		     * strategy may setup a shorter timeout that's
		     * used to wait for the initial identification
		     * frame.  If we get here then we know the remote
		     * side is a fax machine and so we should wait
		     * the full T1 timeout, as specified by the protocol.
		     */
		    t1 = howmany(conf.t1Timer, 1000);
		} while (recvFrame(frame, conf.t2Timer));
	    }
	}
	/*
	 * We failed to send our frames or failed to receive
	 * DCS from the other side.  First verify there is
	 * time to make another attempt...
	 */
	if (time(0)+trecovery-start >= t1)
	    break;
	/*
	 * Delay long enough to miss any training that the
	 * other side might have sent us.  Otherwise the
	 * caller will miss our retransmission since it'll
	 * be in the process of sending training.
	 */
	pause(conf.class1TrainingRecovery);
	/*
	 * Retransmit ident frames.
	 */
        framesSent = transmitFrame(f1, id, FALSE);
    }
    return (FALSE);
}

/*
 * Receive DCS preceded by any optional frames.
 */
fxBool
Class1Modem::recvDCSFrames(HDLCFrame& frame)
{
    fxStr tsi;
    do {
	switch (frame.getFCF()) {
	case FCF_NSS|FCF_SNDR:
	    protoTrace("REMOTE NSS %#x", frame.getDataWord());
	    break;
	case FCF_TSI|FCF_SNDR:
	    decodeTSI(tsi, frame);
	    recvCheckTSI(tsi);
	    break;
	case FCF_DCS|FCF_SNDR:
	    processDCSFrame(frame);
	    break;
	}
    } while (frame.moreFrames() && recvFrame(frame, conf.t4Timer));
    return (frame.isOK() && frame.getFCF() == (FCF_DCS|FCF_SNDR));
}

/*
 * Receive training and analyze TCF.
 */
fxBool
Class1Modem::recvTraining()
{
    protoTrace("RECV training at %s %s",
	modulationNames[curcap->mod],
	Class2Params::bitRateNames[curcap->br]);
    HDLCFrame buf(conf.class1FrameOverhead);
    fxBool ok = recvTCF(curcap->value, buf, frameRev, 4500);
    if (ok) {					// check TCF data
	u_int n = buf.getLength();
	u_int nonzero = 0;
	u_int zerorun = 0;
	u_int i = 0;
	/*
	 * Determine number of non-zero bytes and
	 * the longest zero-fill run in the data.
	 */
	while (i < n) {
	    u_int j;
	    for (; i < n && buf[i] != 0; i++)
		nonzero++;
	    for (j = i; j < n && buf[j] == 0; j++)
		;
	    if (j-i > zerorun)
		zerorun = j-i;
	    i = j;
	}
	/*
	 * Our criteria for accepting is that there must be
	 * no more than 10% non-zero (bad) data and the longest
	 * zero-run must be at least at least 2/3'rds of the
	 * expected TCF duration.  This is a hack, but seems
	 * to work well enough.  What would be better is to
	 * anaylze the bit error distribution and decide whether
	 * or not we would receive page data with <N% error,
	 * where N is probably ~5.  If we had access to the
	 * modem hardware, the best thing that we could probably
	 * do is read the Eye Quality register (or similar)
	 * and derive an indicator of the real S/N ratio.
	 */
	u_int minrun = params.transferSize((2*TCF_DURATION)/3);
	nonzero = (100*nonzero) / (n == 0 ? 1 : n);
	protoTrace("RECV: TCF %u bytes, %u%% non-zero, %u zero-run",
	    n, nonzero, zerorun);
	if (nonzero > 10 || zerorun < minrun)
	    ok = FALSE;
	(void) waitFor(AT_NOCARRIER);	// wait for message carrier to drop
    }
    /*
     * Send training response; we follow the spec
     * by delaying 75ms before switching carriers.
     */
    pause(conf.class1TCFResponseDelay);
    if (ok) {
	transmitFrame(FCF_CFR|FCF_RCVR);
	protoTrace("TRAINING succeeded");
    } else {
	transmitFrame(FCF_FTT|FCF_RCVR);
	protoTrace("TRAINING failed");
    }
    return (ok);
}

/*
 * Process a received DCS frame.
 */
void
Class1Modem::processDCSFrame(const HDLCFrame& frame)
{
    u_int dcs = frame.getDIS();			// NB: really DCS
    params.setFromDCS(dcs, frame.getXINFO());
    setDataTimeout(60, params.br);
    curcap = findSRCapability(dcs&DCS_SIGRATE, recvCaps);
    recvDCS(params);				// pass to server
}

const u_int Class1Modem::modemPPMCodes[8] = {
    0,			// 0
    PPM_EOM,		// FCF_EOM+FCF_PRI_EOM
    PPM_MPS,		// FCF_MPS+FCF_PRI_MPS
    0,			// 3
    PPM_EOP,		// FCF_EOP+FCF_PRI_EOP
    0,			// 5
    0,			// 6
    0,			// 7
};
const char* Class1Modem::ppmNames[16] = {
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
 *
 * This routine is called after receiving training or after
 * sending a post-page response in a multi-page document.
 */
fxBool
Class1Modem::recvPage(TIFF* tif, int& ppm, fxStr& emsg)
{
    fxBool messageReceived = FALSE;		// message carrier received
    do {
	u_int timer = conf.t2Timer;
	if (!messageReceived) {
	    /*
	     * Look for message carrier and receive Phase C data.
	     */
	    setInputBuffering(TRUE);
	    if (flowControl == FLOW_XONXOFF)
		(void) setXONXOFF(FLOW_NONE, FLOW_XONXOFF, ACT_FLUSH);
	    /*
	     * Set high speed carrier & start receive.  If the
	     * negotiated modulation technique includes short
	     * training, then we use it here (it's used for all
	     * high speed carrier traffic other than the TCF).
	     */
	    int speed = curcap[HasShortTraining(curcap)].value;
	    (void) class1Cmd("RM", speed, AT_NOTHING);
	    ATResponse rmResponse = atResponse(rbuf, conf.t2Timer);
	    if (rmResponse == AT_CONNECT) {
		/*
		 * The message carrier was recognized;
		 * receive the Phase C data.
		 */
		protoTrace("RECV: begin page");
		recvSetupPage(tif, 0, FILLORDER_LSB2MSB);
		pageGood = recvPhaseC(tif, emsg);
		protoTrace("RECV: end page");
		if (!wasTimeout()) {
		    /*
		     * The data was received correctly, wait
		     * for the modem to signal carrier drop.
		     */
		    messageReceived = waitFor(AT_NOCARRIER, 2*1000);
		    if (messageReceived)
			prevPage = TRUE;
		    timer = conf.t1Timer;		// wait longer for PPM
		}
	    }
	    if (flowControl == FLOW_XONXOFF)
		(void) setXONXOFF(FLOW_NONE, FLOW_NONE, ACT_DRAIN);
	    setInputBuffering(FALSE);
	    if (!messageReceived && rmResponse != AT_FCERROR) {
		/*
		 * One of many things may have happened:
		 * o if we lost carrier, then some modems will return
		 *   AT_NOCARRIER or AT_EMPTYLINE in response to the
		 *   AT+FRM request.
		 * o otherwise, there may have been a timeout receiving
		 *   the message data, or there was a timeout waiting
		 *   for the carrier to drop.  Anything unexpected causes
		 *   us abort the receive to avoid looping.
		 * The only case that we don't abort on is that we found
		 * the wrong carrier, which means that there is an HDLC
		 * frame waiting for us--in which case it should get
		 * picked up below.
		 */
		break;
	    }
	}
	/*
	 * Do command received logic.
	 */
	HDLCFrame frame(conf.class1FrameOverhead);
	if (recvFrame(frame, timer)) {
	    u_int fcf = frame.getFCF();
	    switch (fcf) {
	    case FCF_DTC:			// XXX no support
	    case FCF_DIS:			// XXX no support
		protoTrace("RECV DIS/DTC");
		emsg = "Can not continue after DIS/DTC";
		return (FALSE);
	    case FCF_NSS|FCF_SNDR:
	    case FCF_TSI|FCF_SNDR:
	    case FCF_DCS|FCF_SNDR:
		if (recvDCSFrames(frame))
		    (void) recvTraining();
		messageReceived = FALSE;	// look for message carrier
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
		    (void) transmitFrame(FCF_MCF|FCF_RCVR);
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
		    (void) transmitFrame(FCF_RTN|FCF_RCVR);
		    /*
		     * Reset the TIFF-related state so that subsequent
		     * writes will overwrite the previous data.
		     */
		    recvResetPage(tif);
		    messageReceived = TRUE;	// expect DCS next
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
	}
    } while (!wasTimeout() && lastResponse != AT_EMPTYLINE);
    emsg = "T.30 T2 timeout, expected page not received";
    return (FALSE);
}

/*
 * Receive Phase C data.
 */
fxBool
Class1Modem::recvPhaseC(TIFF* tif, fxStr& emsg)
{
    startPageRecv();

    fxBool prematureEOF = FALSE;
    u_char buf[16*1024];
    int n = 0;
    for (;;) {
	int b = getModemDataChar();
	if (b == EOF) {
	    protoTrace("RECV: premature EOF");
	    prematureEOF = TRUE;
	    break;
	}
	if (b == DLE) {
	    b = getModemDataChar();
	    if (b == EOF || b == ETX) {
		if (b == EOF) {
		    prematureEOF = TRUE;
		    emsg = "Premature EOF";
		    protoTrace("RECV: %s", (char*) emsg);
		}
		break;
	    }
	    if (b != DLE) {
		if (n == sizeof (buf))
		    recvData(tif, buf, sizeof (buf)), n = 0;
		buf[n++] = DLE;
	    }
	}
	if (n == sizeof (buf))
	    recvData(tif, buf, sizeof (buf)), n = 0;
	buf[n++] = b;
    }
    if (n > 0)
	recvData(tif, buf, n);
    endPageRecv(params);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, getRecvEOLCount());
#ifdef notdef
    TIFFSetField(tif, TIFFTAG_CLEANFAXDATA, blc ?
	CLEANFAXDATA_REGENERATED : CLEANFAXDATA_CLEAN);
    if (blc) {
	TIFFSetField(tif, TIFFTAG_BADFAXLINES, (u_long) blc);
	TIFFSetField(tif, TIFFTAG_CONSECUTIVEBADFAXLINES, cblc);
    }
#endif
    if (prematureEOF) {
	emsg = "Missing EOL after 5 seconds";
	char c = CAN;			// anything other than DC1/DC3
	putModem(&c, 1, 1);
    }
    return (TRUE);
}

/*
 * Write received data to the current file.
 */
void
Class1Modem::recvData(TIFF* tif, u_char* bp, int n)
{
    /*
     * Always put data out to the file in LSB2MSB bit order.
     * We do this because some TIFF readers (mostly on the PC)
     * don't understand MSB2LSB and/or the FillOrder tag.
     */
    if (conf.recvFillOrder != FILLORDER_LSB2MSB)
	TIFFReverseBits(bp, n);
    FaxModem::recvPageData(tif, bp, n);
}

/*
 * Complete a receive session.
 */
fxBool
Class1Modem::recvEnd(fxStr&)
{
    u_int t1 = howmany(conf.t1Timer, 1000);	// T1 timer in seconds
    time_t start = time(0);
    /*
     * Wait for DCN and retransmit ack of EOP if needed.
     */
    HDLCFrame frame(conf.class1FrameOverhead);
    do {
	if (recvFrame(frame, conf.t2Timer)) {
	    switch (frame.getFCF()) {
	    case FCF_EOP|FCF_SNDR:
		protoTrace("RECV PPM %s", ppmNames[FCF_EOP&0xf]);
		(void) transmitFrame(FCF_MCF|FCF_RCVR);
		break;
	    case FCF_DCN|FCF_SNDR:
		break;
	    default:
		transmitFrame(FCF_DCN|FCF_RCVR);
		break;
	    }
	} else if (!wasTimeout() && lastResponse != AT_FCERROR) {
	    /*
	     * Beware of unexpected responses from the modem.  If
	     * we lose carrier, then we can loop here if we accept
	     * null responses, or the like.
	     */
	    break;
	}
    } while (time(0)-start < t1 &&
	(!frame.isOK() || frame.getFCF() == (FCF_EOP|FCF_SNDR)));
    setInputBuffering(TRUE);
    return (TRUE);
}
