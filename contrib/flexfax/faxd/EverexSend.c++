/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/EverexSend.c++,v 1.1.1.1 1994/01/14 23:09:47 torek Exp $
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
#include "Everex.h"
#include "ModemConfig.h"

#include "t.30.h"
#include "everex.h"

/*
 * Send Protocol for ``old'' Class-1-style Everex modems.
 */

CallStatus
EverexModem::dialResponse()
{
    for (;;) {
	switch (atResponse(rbuf, conf.dialResponseTimeout)) {
	case AT_EMPTYLINE:	return (FAILURE);
	case AT_TIMEOUT:	return (FAILURE);
	case AT_BUSY:		return (BUSY);
	case AT_NOCARRIER:	return (NOCARRIER);
	case AT_NODIALTONE:	return (NODIALTONE);
	case AT_NOANSWER:	return (NOANSWER);
	case AT_ERROR:		return (ERROR);
	case AT_OTHER:
	    if (strneq(rbuf, "FAX", 3))
		return (OK);
	    break;
	case AT_OK:
	    return (type == EV958 ? OK : FAILURE);	// XXX data connection?
	}
    }
}

/*
 * Begin a send operation.
 */
void
EverexModem::sendBegin()
{
    atCmd("#S8=10#S9=30");	// s8 = retry count, s9 = retry interval
    if (type == EV958)		// send 1100Hz tone
	atCmd("#R#T1100=5");
    params.br = (u_int) -1;	// force initial dcs setup
}

/*
 * Get the initial DIS and NSF information.
 */
fxBool
EverexModem::getPrologue(Class2Params& params, u_int& nsf, fxStr& csi, fxBool& hasDoc)
{
    if (!atCmd("#FR01?", AT_NOTHING))	// fetch DIS bytes
	return (FALSE);
    nsf = 0;				// XXX AT#FR04?
    csi = "";
    switch (atResponse(rbuf, 10*1000)) {
    case AT_OTHER:
	break;
    default:
	(void) sync(2*1000);
	/* fall thru... */
    case AT_OK:
    case AT_ERROR:
	return (FALSE);
    }
    int n = strlen(rbuf);
    dis = fromHex(rbuf, 6);
    u_int xinfo = (dis&DIS_XTNDFIELD) && n >= 8 ? fromHex(rbuf+6, 2) : 0;
    params.setFromDIS(dis, xinfo);
    hasDoc = (dis & DIS_T4XMTR) != 0;		// documents to poll
    // XXX get CSI
    return (sync(2*1000) && (dis&DIS_T4RCVR));
}

/*
 * Setup file-independent parameters prior
 * to entering Phase B of the send protocol.
 */
void
EverexModem::sendSetupPhaseB()
{
    setupFrame(FCF_TSI|FCF_SNDR, (char*) lid);
}

u_int EverexModem::modemPFMCodes[8] = {
    FCF_MPS,		// PPM_MPS
    FCF_EOM,		// PPM_EOM
    FCF_EOP,		// PPM_EOP
    FCF_EOP,		// 3 ??? XXX
    FCF_PRI_MPS,	// PPM_PRI_MPS
    FCF_PRI_EOM,	// PPM_PRI_EOM
    FCF_PRI_EOP,	// PPM_PRI_EOP
    FCF_EOP,		// 7 ??? XXX
};
int EverexModem::modemRateCodes[8] = {
    S4_2400V27,		// V.27 2400
    S4_4800V27,		// V.27 4800 XXX (maybe V29 instead?)
    S4_7200V29,		// V.29 7200
    S4_9600V29,		// V.29 9600
    S4_9600V29,		// V.29 9600 (map 12000)
    S4_9600V29,		// V.29 9600 (map 14400)
    S4_9600V29,		// undefined
    S4_9600V29,		// undefined
};
int EverexModem::modemScanCodes[8] = {
    0,			// ST_0MS
    5,			// ST_5MS
    10,			// ST_10MS2
    10,			// ST_10MS
    20,			// ST_20MS2
    20,			// ST_20MS
    40,			// ST_40MS2
    40,			// ST_40MS
};

/*
 * Send the specified document using the supplied
 * parameters.  The pph is the post-page-handling
 * indicators calculated prior to intiating the call.
 */
fxBool
EverexModem::sendPhaseB(TIFF* tif, Class2Params& next, FaxMachineInfo& info,
    fxStr& pph, fxStr& emsg)
{
    int ntrys = 0;			// # retraining/command repeats
    fxBool transferOK = TRUE;
    fxBool morePages = TRUE;

    do {
	transferOK = !abortRequested();
	if (!transferOK)
	     break;
	/*
	 * Check the next page to see if the transfer
	 * characteristics change.  If so, update the
	 * current T.30 session parameters.
	 */
	if (params != next) {
	    if (!sendTraining(next, emsg))
		return (FALSE);
	    params = next;
	}
	transferOK = sendPage(tif, params, emsg);
	if (!transferOK)
	    break;
	/*
	 * Delay before switching to the low speed carrier to
	 * send the post-page-message frame.
	 */
	pause(75);
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
	u_int fcf;
	do {
	    transferOK = sendPPM(cmd, emsg);
	    if (!transferOK)
		break;
	    switch (fcf = fromHex(rbuf+2, 2)) {
	    case FCF_RTP:		// ack, continue after retraining
		transferOK = sendTraining(params, emsg);
		next = params;		// avoid retraining above
		/* fall thru... */
	    case FCF_MCF:		// ack confirmation
	    case FCF_PIP:		// ack, w/ operator intervention
		countPage();		// update server
		pph.remove(0);		// discard post-page-handling
		ntrys = 0;
		if (morePages)
		    transferOK = (TIFFReadDirectory(tif) &&
			sendSetupParams(tif, next, info, emsg));
		break;
	    case FCF_DCN:		// disconnect, abort
		emsg = "Remote fax disconnected prematurely";
		transferOK = FALSE;
		break;
	    case FCF_RTN:		// nak, retry after retraining
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
		    params.br--;
		}
		if (transferOK = sendTraining(params, emsg)) {
		    morePages = TRUE;	// force continuation
		    next = params;	// avoid retraining above
		}
		break;
	    case FCF_PIN:		// nak, retry w/ operator intervention
		emsg = "Unable to transmit page"
		       " (NAK with operator intervention)";
		transferOK = FALSE;
		break;
	    case FCF_CRP:
		break;
	    default:			// unexpected abort
		emsg = "Fax protocol error (unknown frame received)";
		transferOK = FALSE;
		break;
	    }
	} while (fcf == FCF_CRP && ++ncrp < 3);
	if (ncrp == 3) {
	    emsg = "Fax protocol error (command repeated 3 times)";
	    transferOK = FALSE;
	}
    } while (transferOK && morePages);
    return (transferOK);
}

/*
 * Send capabilities and do training.
 */
fxBool
EverexModem::sendTraining(Class2Params& params, fxStr& emsg)
{
    do {
	u_int dcs = params.getDCS();
	setupFrame(FCF_DCS|FCF_SNDR, dcs);
	fxStr br(modemRateCodes[params.br&7], "#S4=%u");
	fxStr st(modemScanCodes[params.st&7], "#S5=%u");
	atCmd(br | st);
	int t = 2;
	do {
	    protoTrace("SEND training at %s",
		Class2Params::bitRateNames[params.br]);
	    if (!sendFrame(FCF_TSI|FCF_SNDR, FCF_DCS|FCF_SNDR)) {
		if (abortRequested())
		    return (FALSE);
		protoTrace("Error sending T.30 prologue frames");
		continue;
	    }
	    /*
	     * Delay before sending the TCF.
	     */
	    pause(75);
	    if (!atCmd("#P1")) {
		if (abortRequested())
		    return (FALSE);
		protoTrace("Problem sending TCF data");
	    }
	    char buf[1024];
	    if (getModemLine(buf, conf.t4Timer) > 2 && strneq(buf, "R ", 2))
		switch (fromHex(buf+2, 2)) {
		case FCF_CFR:		// training confirmed
		    protoTrace("TRAINING succeeded");
		    setDataTimeout(60, params.br);
		    return (TRUE);
		case FCF_FTT:		// failure to train, retry
		    break;
		case FCF_DCN|FCF_RCVR:	// disconnect
		    emsg = "Remote fax disconnected during training";
		    protoTrace("TRAINING failed");
		    goto done;
		}
	    pause(1500);		// give other side time to reset
	} while (--t > 0);
	/*
	 * Two attempts at the current speed failed, drop
	 * the signalling rate to the next lower rate supported
	 * by the local & remote sides and try again.
	 */
    } while (dropToNextBR(params));
    emsg = "Failure to train remote modem";
done:
    protoTrace("TRAINING failed");
    return (FALSE);
}

#define	isCapable(br,dis)	((dis & Class2Params::brDISTab[br]) != 0)

/*
 * Select the next lower signalling rate that's
 * acceptable to both local and remote sides.
 */
fxBool
EverexModem::dropToNextBR(Class2Params& params)
{
    do {
	if (params.br == BR_2400)
	    return (FALSE);
	params.br--;
    } while (selectSignallingRate(params.br) != params.br ||
	!isCapable(params.br, dis));
    return (TRUE);
}

/*
 * Wait for a signal from the modem that the high
 * speed carrier was dropped (``M 4'').
 */
fxBool
EverexModem::waitForCarrierToDrop(fxStr& emsg)
{
    int t = 3;
    do {
	if (getModemLine(rbuf, getDataTimeout()) > 2 && streq(rbuf, "M 4"))
	    return (TRUE);
    } while (--t > 0);
    emsg = "Unable to reestablish low speed carrier";
    return (FALSE);
}

const int EverexSendMode =
    (S2_HOSTCTL|S2_DEFMSGSYS|S2_FLOWATBUF|S2_PADEOLS|S2_19200);

/*
 * Send a page of facsimile data (pre-encoded).
 */
fxBool
EverexModem::sendPage(TIFF* tif, const Class2Params& params, fxStr& emsg)
{
    fxBool rc = FALSE;
    if (!modemFaxConfigure(EverexSendMode)) {
	emsg = "Unable to configure modem";
	return (rc);
    }
    if (!atCmd("#P5#P3#P6")) {		// switch modem & start transfer
	emsg = "Unable to initiate page transfer";
	return (rc);
    }
    if (setBaudRate(BR19200, FLOW_XONXOFF, FLOW_NONE)) {
	protoTrace("SEND begin page");
	u_long* stripbytecount;
	(void) TIFFGetField(tif, TIFFTAG_STRIPBYTECOUNTS, &stripbytecount);
	int totbytes = (int) stripbytecount[0];
	if (totbytes > 0) {
	    u_char* buf = new u_char[totbytes];
	    if (TIFFReadRawStrip(tif, 0, buf, totbytes) >= 0) {
		int sentbytes = 0;
		/*
		 * Correct bit order of data if not what modem requires.
		 */
		u_short fillorder = FILLORDER_MSB2LSB;
		(void) TIFFGetField(tif, TIFFTAG_FILLORDER, &fillorder);
		if (fillorder != FILLORDER_LSB2MSB)
		    TIFFReverseBits(buf, totbytes);
		/*
		 * Pass data to modem, being careful not to hang.
		 */
		beginTimedTransfer();
		u_char* cp = buf;
		while (totbytes > 0 && !wasTimeout() && !abortRequested()) {
		    int n = fxmin(totbytes, 1024);
		    if (!putModemData(cp, n))
			break;
		    sentbytes += n;
		    cp += n;
		    totbytes -= n;
		    // XXX check to make sure RTC isn't in data
		}
		endTimedTransfer();
		// return TRUE if no timeout
		rc = (totbytes == 0) && !wasTimeout() && !abortRequested();
		if (!rc)
		    emsg = "Timeout during transfer of page data";
		protoTrace("SENT %d bytes of data", sentbytes);
	    }
	    delete buf;
	}
	if (rc) {
	    rc = sendRTC(params.is2D());
	    // TRUE =>'s be careful about flushing data
	    (void) sendBreak(TRUE);
	} else
	    (void) sendBreak(FALSE);
	(void) setBaudRate(EVEREX_CMDBRATE, FLOW_CURRENT, FLOW_XONXOFF);
	protoTrace("SEND end page");
    }
    return (rc ? waitForCarrierToDrop(emsg) : FALSE);
}

/*
 * Send a 1d or 2d encoded RTC.
 */
fxBool
EverexModem::sendRTC(fxBool is2D)
{
    static char RTC1D[] =
	{ 0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0x00, 0x08, 0x80 };
    static char RTC2D[] =
	{ 0x00, 0x18, 0x00, 0x03, 0x60, 0x00, 0x0C, 0x80, 0x01, 0x30 };
    protoTrace("SEND %s RTC", is2D ? "2D" : "1D");
    return (is2D ?
	putModemData(RTC2D, sizeof (RTC2D)) :
	putModemData(RTC1D, sizeof (RTC1D)));
}

/*
 * Send the post-page-message and wait for a response.
 */
fxBool
EverexModem::sendPPM(int ppm, fxStr& emsg)
{
    int t = 3;
    do {
	if (!sendFrame(ppm|FCF_SNDR)) {
	    if (!abortRequested())
		emsg = "Problem sending post-page message frame";
	    return (FALSE);
	}
	if (getModemLine(rbuf, conf.t4Timer) > 2 && strneq(rbuf, "R ", 2))
	    return (TRUE);
    } while (--t > 0);
    emsg = "No response to MPS or EOP repeated 3 tries";
    return (FALSE);
}

/*
 * Terminate a send operation.
 */
void
EverexModem::sendEnd()
{
    sendFrame(FCF_DCN|FCF_SNDR);		// terminate session
}
