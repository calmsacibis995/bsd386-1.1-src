/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/Class1Send.c++,v 1.1.1.1 1994/01/14 23:09:46 torek Exp $
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
 * Send protocol.
 */
#include <stdio.h>
#include <time.h>
#include "Class1.h"
#include "ModemConfig.h"
#include "HDLCFrame.h"
#include "t.30.h"

/*
 * Dial the phone number.  We override this method
 * so that we can force the terminal into a known
 * flow control state.
 */
CallStatus
Class1Modem::dial(const char* number)
{
    if (flowControl == FLOW_XONXOFF)
	setXONXOFF(FLOW_NONE, FLOW_NONE, ACT_FLUSH);
    return FaxModem::dial(number);
}

/*
 * Wait-for and process a dial command response.
 */
CallStatus
Class1Modem::dialResponse()
{
    int ntrys = 0;
    for (;;) {
	switch (atResponse(rbuf, conf.dialResponseTimeout)) {
	case AT_BUSY:	return (BUSY);
	case AT_NODIALTONE:	return (NODIALTONE);
	case AT_NOANSWER:	return (NOANSWER);
	case AT_ERROR:		return (ERROR);
	case AT_CONNECT:	return (OK);
	case AT_NOCARRIER:	return (NOCARRIER);
	default:		return (FAILURE);
	case AT_FCERROR:
	    /*
	     * Some modems that support adaptive-answer assert a data
	     * carrier first and then fall back to answering as a fax
	     * modem.  For these modems we'll get an initial +FCERROR
	     * (wrong carrier) result that we ignore.  We actually
	     * accept three of these in case the modem switches carriers
	     * several times (haven't yet encountered anyone that does).
	     */
	    if (++ntrys == 3)
		return (FAILURE);
	    break;
	}
    }
}

void
Class1Modem::sendBegin()
{
    setInputBuffering(FALSE);
    // polling accounting?
    params.br = (u_int) -1;			// force initial training
}

/*
 * Get the initial DIS command.
 */
fxBool
Class1Modem::getPrologue(Class2Params& params, u_int& nsf, fxStr& csi, fxBool& hasDoc)
{
    u_int t1 = howmany(conf.t1Timer, 1000);	// T1 timer in seconds
    time_t start = time(0);
    HDLCFrame frame(conf.class1FrameOverhead);

    /*
     * Wait up to T1 for an initial frame.
     */
    startTimeout(conf.t4Timer);
    if (!recvRawFrame(frame)) {
	do {
	    if (time(0)-start >= t1)
		return (FALSE);
	} while (!recvFrame(frame, conf.t4Timer));
    }
    /*
     * An HDLC frame was received; process any
     * optional frames that precede the DIS.
     */
    csi = "";
    nsf = 0;
    do {
	switch (frame.getFCF()) {
	case FCF_NSF:
	    nsf = frame.getDataWord();
	    break;
	case FCF_CSI:
	    decodeTSI(csi, frame);
	    recvCSI(csi);
	    break;
	case FCF_DIS:
	    dis = frame.getDIS();
	    params.setFromDIS(dis, frame.getXINFO());
	    curcap = NULL;		// force setup
	    break;
	}
    } while (frame.moreFrames() && recvFrame(frame, conf.t4Timer));
    hasDoc = (dis & DIS_T4XMTR) != 0;		// documents to poll
    /*
     * NB: we don't handle DTC, only DIS.
     */
    return (frame.isOK() && frame.getFCF() == FCF_DIS && (dis&DIS_T4RCVR) != 0);
}

/*
 * Setup file-independent parameters prior
 * to entering Phase B of the send protocol.
 */
void
Class1Modem::sendSetupPhaseB()
{
}

const u_int Class1Modem::modemPFMCodes[8] = {
    FCF_MPS,		// PPM_MPS
    FCF_EOM,		// PPM_EOM
    FCF_EOP,		// PPM_EOP
    FCF_EOP,		// 3 ??? XXX
    FCF_PRI_MPS,	// PPM_PRI_MPS
    FCF_PRI_EOM,	// PPM_PRI_EOM
    FCF_PRI_EOP,	// PPM_PRI_EOP
    FCF_EOP,		// 7 ??? XXX
};
const char* Class1Modem::pprNames[16] = {
    NULL,					// 0
    "MCF (message confirmation)",		// FCF_MCF
    "RTN (retrain negative)",			// FCF_RTN
    "RTP (retrain positive)",			// FCF_RTP
    "PIN (procedural interrupt negative)",	// FCF_PIN
    "PIP (procedural interrupt positive)",	// FCF_PIP
    NULL,					// 6
    NULL,					// 7
    "CRP (command repeat)",			// FCF_CRP
    NULL,					// 9
    NULL,					// 10
    NULL,					// 11
    NULL,					// 12
    NULL,					// 13
    NULL,					// 14
    "DCN (disconnect)",				// 15
};

/*
 * Send the specified document using the supplied
 * parameters.  The pph is the post-page-handling
 * indicators calculated prior to intiating the call.
 */
fxBool
Class1Modem::sendPhaseB(TIFF* tif, Class2Params& next, FaxMachineInfo& info,
    fxStr& pph, fxStr& emsg)
{
    int ntrys = 0;			// # retraining/command repeats
    fxBool morePages = TRUE;		// more pages still to send
    fxBool transferOK = TRUE;
    HDLCFrame frame(conf.class1FrameOverhead);

    do {
	transferOK = !abortRequested();
	if (!transferOK)
	    break;
	/*
	 * Check the next page to see if the transfer
	 * characteristics change.  If so, update the
	 * T.30 session parameters and do training.
	 * Note that since the initial parameters are
	 * setup to be "undefined", training will be
	 * sent for the first page after receiving the
	 * DIS frame.
	 */
	if (params != next) {
	    transferOK = sendTraining(next, 3, emsg);
	    if (!transferOK)
		break;
	    params = next;
	}
	/*
	 * Transmit the facsimile message/Phase C.
	 */
	transferOK = sendPage(tif, params, emsg);
	if (!transferOK)
	    break;			// a problem, disconnect
	/*
	 * Everything went ok, look for the next page to send.
	 */
	morePages = !TIFFLastDirectory(tif);
	int cmd;
	// XXX check pph length
	switch (pph[0]) {
	default:
	    morePages = FALSE;
	    emsg = "Unknown post-page-handling indicator \"" | pph | "\"";
	    /* fall thru... */
	case 'P': cmd = FCF_EOP; break;
	case 'M': cmd = FCF_EOM; break;
	case 'S': cmd = FCF_MPS; break;
	}
	int ncrp = 0;
	/*
	 * Delay before switching to the low speed carrier to
	 * send the post-page-message frame.  We follow the spec
	 * in delaying 75ms before switching carriers, except
	 * when at EOP in which case we delay longer because,
	 * empirically, some machines need more time.  Beware
	 * that, reportedly, lengthening this delay too much can
	 * permit echo suppressors to kick in with bad results.
	 *
	 * NB: We do not use +FTS because many modems are slow
	 *     to send the OK result and so using pause is more
	 *     accurate.
	 */
	pause(cmd == FCF_MPS ? conf.class1SendPPMDelay : 95);
	do {
	    /*
	     * Send post-page message and get response.
	     */
	    protoTrace("SEND send %s", ppmNames[cmd&0xf]);
	    transferOK = sendPPM(cmd, frame, emsg);
	    if (!transferOK)
		break;
	    u_int ppr = frame.getFCF();
	    if (pprNames[ppr&0xf])
		protoTrace("SEND recv %s", pprNames[ppr&0xf]);
	    else
		protoTrace("SEND recv unknown FCF %#x", ppr);
	    switch (ppr) {
	    case FCF_RTP|FCF_RCVR:	// ack, continue after retraining
		params.br = (u_int) -1;	// force retraining above
		/* fall thru... */
	    case FCF_MCF|FCF_RCVR:	// ack confirmation
	    case FCF_PIP|FCF_RCVR:	// ack, w/ operator intervention
		countPage();		// update server
		pph.remove(0);		// discard post-page-handling
		ntrys = 0;
		if (morePages && transferOK)
		    transferOK = (TIFFReadDirectory(tif) &&
			sendSetupParams(tif, next, info, emsg));
		break;
	    case FCF_DCN|FCF_RCVR:	// disconnect, abort
		emsg = "Remote fax disconnected prematurely";
		transferOK = FALSE;
		break;
	    case FCF_RTN|FCF_RCVR:	// nak, retry after retraining
		if ((++ntrys % 2) == 0) {
		    /*
		     * Drop to a lower signalling rate and retry.
		     */
		    if (params.br == BR_2400) {
			emsg = "Unable to transmit page"
			       " (NAK at all possible signalling rates)";
			transferOK = FALSE;
			break;
		    }
		    --params.br;
		    curcap = NULL;	// force sendTraining to reselect
		}
		if (transferOK = sendTraining(params, 3, emsg)) {
		    morePages = TRUE;	// force continuation
		    next = params;	// avoid retraining above
		}
		break;
	    case FCF_PIN|FCF_RCVR:	// nak, retry w/ operator intervention
		emsg = "Unable to transmit page"
		       " (NAK with operator intervention)";
		transferOK = FALSE;
		break;
	    case FCF_CRP|FCF_RCVR:
		break;
	    default:			// unexpected abort
		emsg = "Fax protocol error (unknown frame received)";
		transferOK = FALSE;
		break;
	    }
	} while (frame.getFCF() == FCF_CRP && ++ncrp < 3);
	if (ncrp == 3) {
	    emsg = "Fax protocol error (command repeated 3 times)";
	    transferOK = FALSE;
	}
    } while (transferOK && morePages);
    return (transferOK);
}

/*
 * Send ms's worth of zero's at the current signalling rate.
 * Note that we send real zero data here rather than using
 * the Class 1 modem facility to do zero fill.
 */
fxBool
Class1Modem::sendTCF(const Class2Params& params, u_int ms)
{
    u_int tcfLen = params.transferSize(ms);
    u_char* tcf = new u_char[tcfLen];
    memset(tcf, 0, tcfLen);
    fxBool ok = transmitData(curcap->value, tcf, tcfLen, frameRev, TRUE);
    delete tcf;
    return ok;
}

/*
 * Send the training prologue frames; TSI and DCS.
 */
fxBool
Class1Modem::sendPrologue(u_int dcs, const fxStr& tsi)
{
    if (transmitFrame(FCF_TSI|FCF_SNDR, tsi, FALSE)) {
	startTimeout(2550);			// 3.0 - 15% = 2.55 secs
	fxBool frameSent = sendFrame(FCF_DCS|FCF_SNDR, dcs);
	stopTimeout("sending DCS frame");
	return (frameSent);
    } else
	return (FALSE);
}

/*
 * Return whether or not the previously received DIS indicates
 * the remote side is capable of the T.30 DCS signalling rate.
 */
static fxBool
isCapable(u_int sr, u_int dis)
{
    dis = (dis & DIS_SIGRATE) >> 10;
    switch (sr) {
    case DCSSIGRATE_2400V27:
    case DCSSIGRATE_4800V27:
	return ((dis & (DISSIGRATE_V27|DISSIGRATE_V27FB)) != 0);
    case DCSSIGRATE_9600V29:
    case DCSSIGRATE_7200V29:
	return ((dis & DISSIGRATE_V29) != 0);
    case DCSSIGRATE_14400V33:
    case DCSSIGRATE_12000V33:
	if (dis == 0xE)
	    return (TRUE);
	/* fall thru... */
    case DCSSIGRATE_14400V17:
    case DCSSIGRATE_12000V17:
    case DCSSIGRATE_9600V17:
    case DCSSIGRATE_7200V17:
	return (dis == 0xD);
    }
    return (FALSE);
}

/*
 * Send capabilities and do training.
 */
fxBool
Class1Modem::sendTraining(Class2Params& params, int tries, fxStr& emsg)
{
    if (tries == 0) {
	emsg = "DIS/DTC received 3 times; DCS not recognized";
	return (FALSE);
    }
    u_int dcs = params.getDCS();		// NB: 32-bit DCS
    if (!curcap) {
	/*
	 * Select Class 1 capability: use params.br to hunt
	 * for the best signalling scheme acceptable to both
	 * local and remote (based on received DIS and modem
	 * capabilities gleaned at modem setup time).
	 */
	params.br++;				// XXX can go out of range
	if (!dropToNextBR(params))
	    goto failed;
    }
    do {
	/*
	 * Override the Class 2 parameter bit rate
	 * capability and use the signalling rate
	 * calculated from the modem's capabilities
	 * and the received DIS.  This is because
	 * the Class 2 state does not include the
	 * modulation technique (v.27, v.29, v.17, v.33).
	 */
	params.br = curcap->br;
	// NB: <<8's are because dcs is in 32-bit format
	dcs = (dcs &~ (DCS_SIGRATE<<8)) | (curcap->sr<<8);
	int t = 2;
	do {
	    protoTrace("SEND training at %s %s",
		modulationNames[curcap->mod],
		Class2Params::bitRateNames[curcap->br]);
	    if (!sendPrologue(dcs, lid)) {
		if (abortRequested())
		    goto done;
		protoTrace("Error sending T.30 prologue frames");
		continue;
	    }
	    /*
	     * Delay before switching to high speed carrier
	     * to send the TCF data.  Note that we use pause
	     * instead of +FTS because many modems are slow
	     * to return the OK result and this can screw up
	     * timing.  Reportedly some modems won't work
	     * properly unless they see +FTS before the TCF,
	     * but for now we stick with what appears to work
	     * the best with the modems we use.
	     */
	    pause(conf.class1SendTCFDelay);
	    if (!sendTCF(params, TCF_DURATION)) {
		if (abortRequested())
		    goto done;
		protoTrace("Problem sending TCF data");
	    }
	    /*
	     * Receive response to training.  Acceptable
	     * responses are: DIS or DTC (DTC not handled),
	     * FTT, or CFR; and also a premature DCN.
	     */
	    HDLCFrame frame(conf.class1FrameOverhead);
	    if (recvFrame(frame, conf.t4Timer)) {
		do {
		    switch (frame.getFCF()) {
		    case FCF_NSF|FCF_RCVR:
			{ u_int nsf = frame.getDataWord(); }
			break;
		    case FCF_CSI|FCF_RCVR:
			{ fxStr csi; decodeTSI(csi, frame); recvCSI(csi); }
			break;
		    }
		} while (frame.moreFrames() && recvFrame(frame, conf.t4Timer));
	    }
	    if (frame.isOK()) {
		switch (frame.getFCF()) {
		case FCF_CFR|FCF_RCVR:		// training confirmed
		    protoTrace("TRAINING succeeded");
		    setDataTimeout(60, params.br);
		    return (TRUE);
		case FCF_FTT|FCF_RCVR:		// failure to train, retry
		    break;
		case FCF_DIS|FCF_RCVR:		// new capabilities, maybe
		    { u_int newDIS = frame.getDIS();
		      if (newDIS != dis) {
			dis = newDIS;
			params.setFromDIS(dis, frame.getXINFO());
			curcap = NULL;
		      }
		    }
		    return (sendTraining(params, --tries, emsg));
		default:
		    if (frame.getFCF() == (FCF_DCN|FCF_RCVR))
			emsg = "RSRPEC error/got DCN";
		    else
			emsg = "RSPREC invalid response received";
		    goto done;
		}
	    }
	    // delay to give other side time to reset
	    pause(conf.class1TrainingRecovery);
	} while (--t > 0);
	/*
	 * Two attempts at the current speed failed, drop
	 * the signalling rate to the next lower rate supported
	 * by the local & remote sides and try again.
	 */
    } while (dropToNextBR(params));
failed:
    emsg = "Failure to train remote modem";
done:
    protoTrace("TRAINING failed");
    return (FALSE);
}

/*
 * Select the next lower signalling rate that's
 * acceptable to both local and remote machines.
 */
fxBool
Class1Modem::dropToNextBR(Class2Params& params)
{
    for (;;) {
	if (params.br == BR_2400)
	    return (FALSE);
	// get ``best capability'' of modem at this baud rate
	curcap = findBRCapability(--params.br, xmitCaps);
	if (curcap) {
	    // hunt for compatibility with remote at this baud rate
	    do {
		if (isCapable(curcap->sr, dis))
		    return (TRUE);
		curcap--;
	    } while (curcap->br == params.br);
	}
    }
    /*NOTREACHED*/
}

/*
 * Select the next higher signalling rate that's
 * acceptable to both local and remote machines.
 */
fxBool
Class1Modem::raiseToNextBR(Class2Params& params)
{
    for (;;) {
	if (params.br == BR_14400)	// highest speed
	    return (FALSE);
	// get ``best capability'' of modem at this baud rate
	curcap = findBRCapability(++params.br, xmitCaps);
	if (curcap) {
	    // hunt for compatibility with remote at this baud rate
	    do {
		if (isCapable(curcap->sr, dis))
		    return (TRUE);
		curcap--;
	    } while (curcap->br == params.br);
	}
    }
    /*NOTREACHED*/
}

/*
 * Send data for the current page.
 */
fxBool
Class1Modem::sendPageData(u_char* data, u_int cc, const u_char* bitrev)
{
    beginTimedTransfer();
    fxBool rc = sendClass1Data(data, cc, bitrev, FALSE);
    endTimedTransfer();
    protoTrace("SENT %u bytes of data", cc);
    return rc;
}

/*
 * Send RTC to terminate a page.  Note that we pad the
 * RTC with zero fill to "push it out on the wire".  It
 * seems that some Class 1 modems do not immediately
 * send all the data they are presented.
 */
fxBool
Class1Modem::sendRTC(fxBool is2D)
{
    static const u_char RTC1D[9+20] =
	{ 0x00,0x10,0x01,0x00,0x10,0x01,0x00,0x10,0x01 };
    static const u_char RTC2D[10+20] =
	{ 0x00,0x18,0x00,0xC0,0x06,0x00,0x30,0x01,0x80,0x0C };
    protoTrace("SEND %s RTC", is2D ? "2D" : "1D");
    if (is2D)
	return sendClass1Data(RTC2D, sizeof (RTC2D), frameRev, TRUE);
    else
	return sendClass1Data(RTC1D, sizeof (RTC1D), frameRev, TRUE);
}

/*
 * Send a page of data.
 */
fxBool
Class1Modem::sendPage(TIFF* tif, const Class2Params& params, fxStr& emsg)
{
    /*
     * Set high speed carrier & start transfer.  If the
     * negotiated modulation technique includes short
     * training, then we use it here (it's used for all
     * high speed carrier traffic other than the TCF).
     */
    int speed = curcap[HasShortTraining(curcap)].value;
    if (!class1Cmd("TM", speed, AT_CONNECT)) {
	emsg = "Unable to establish message carrier";
	return (FALSE);
    }
    fxBool rc = TRUE;
    protoTrace("SEND begin page");
    if (flowControl == FLOW_XONXOFF)
	setXONXOFF(FLOW_XONXOFF, FLOW_NONE, ACT_FLUSH);
    /*
     * Correct bit order of data if not what modem expects.
     */
    u_short fillorder;
    TIFFGetFieldDefaulted(tif, TIFFTAG_FILLORDER, &fillorder);
    const u_char* bitrev =
	TIFFGetBitRevTable(conf.sendFillOrder != FILLORDER_LSB2MSB);
    u_long* stripbytecount;
    (void) TIFFGetField(tif, TIFFTAG_STRIPBYTECOUNTS, &stripbytecount);
    u_char* fill;
    u_char* eoFill;
    u_long w = 0xffffff;
    u_int minLen = params.minScanlineSize();
    if (minLen > 0) {
	/*
	 * Client requires a non-zero min-scanline time.  We
	 * comply by zero-padding scanlines that have <minLen
	 * bytes of data to send.  To minimize underrun we
	 * do this padding in a strip-sized buffer.
	 */
	u_long rowsperstrip;
	TIFFGetFieldDefaulted(tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);
	if (rowsperstrip == (u_long) -1)
	     TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &rowsperstrip);
	fill = new u_char[minLen*rowsperstrip];
	eoFill = fill + minLen*rowsperstrip;
    }
    u_char* fp = fill;
    for (u_int strip = 0; strip < TIFFNumberOfStrips(tif) && rc; strip++) {
	u_int totbytes = (u_int) stripbytecount[strip];
	if (totbytes > 0) {
	    u_char* buf = new u_char[totbytes];
	    if (TIFFReadRawStrip(tif, strip, buf, totbytes) >= 0) {
		/*
		 * Send the strip of data.  This is slightly complicated
		 * by the fact that we may have to add zero-fill before the
		 * EOL codes to bring the transmit time for each scanline
		 * up to the negotiated min-scanline time.
		 *
		 * Note that we blindly force the data to be in LSB2MSB bit
		 * order so that the EOL locating code works (if needed).
		 * This may result in two extraneous bit reversals if the
		 * modem wants the data in MSB2LSB order, but for now we'll
		 * avoid the temptation to optimize.
		 */
		if (fillorder != FILLORDER_LSB2MSB)
		    TIFFReverseBits(buf, totbytes);
		if (minLen > 0) {
		    u_char* bp = buf;
		    u_char* ep = buf+totbytes;
		    do {
			u_char* bol = bp;
			do {
			    w = (w<<8) | *bp++;
			} while (!EOLcode(w) && bp < ep);
			/*
			 * We're either at an EOL code or at the end of data.
			 * If necessary, insert zero-fill before the last byte
			 * in the EOL code so that we comply with the
			 * negotiated min-scanline time.
			 */
			u_int lineLen = bp - bol;
			if (fp + fxmax(lineLen, minLen) >= eoFill) {
			    /*
			     * Not enough space for this scanline, flush
			     * the current data and reset the pointer into
			     * the zero fill buffer.
			     */
			    rc = sendPageData(fill, fp-fill, bitrev);
			    fp = fill;
			    if (!rc)			// error writing data
				break;
			}
			memcpy(fp, bol, lineLen);	// first part of line
			fp += lineLen;
			if (lineLen < minLen) {		// must zero-fill
			    u_int zeroLen = minLen - lineLen;
			    memset(fp-1, 0, zeroLen);	// zero padding
			    fp += zeroLen;
			    fp[-1] = bp[-1];		// last byte in EOL
			}
		    } while (bp < ep);
		} else {
		    /*
		     * No EOL-padding needed, just jam the bytes.
		     */
		    rc = sendPageData(buf, totbytes, bitrev);
		}
	    }
	    delete buf;
	}
    }
    if (minLen > 0) {
	/*
	 * Flush anything that was not sent above.
	 */
	if (fp > fill)
	    rc = sendPageData(fill, fp-fill, bitrev);
	delete fill;
    }
    if (rc || abortRequested())
	rc = sendRTC(params.is2D());
    protoTrace("SEND end page");
    if (rc) {
	/*
	 * Wait for transmit buffer to empty.
	 */
	ATResponse r;
	while ((r = atResponse(rbuf, getDataTimeout())) == AT_OTHER)
	    ;
	rc = (r == AT_OK);
    }
    if (flowControl == FLOW_XONXOFF)
	setXONXOFF(FLOW_NONE, FLOW_NONE, ACT_DRAIN);
    if (!rc)
	emsg = "Unspecified Transmit Phase C error";	// XXX
    return (rc);
}

/*
 * Send the post-page-message and wait for a response.
 */
fxBool
Class1Modem::sendPPM(u_int ppm, HDLCFrame& mcf, fxStr& emsg)
{
    for (int t = 0; t < 3; t++) {
	if (!transmitFrame(ppm|FCF_SNDR)) {
	    if (!abortRequested())
		emsg = "Error transmitting post-page message";
	    return (FALSE);
	}
	if (recvFrame(mcf, conf.t4Timer))
	    return (TRUE);
    }
    emsg = "No response to MPS or EOP repeated 3 tries";
    return (FALSE);
}

/*
 * Terminate a send session.
 */
void
Class1Modem::sendEnd()
{
    transmitFrame(FCF_DCN|FCF_SNDR);		// disconnect
    setInputBuffering(TRUE);
}
