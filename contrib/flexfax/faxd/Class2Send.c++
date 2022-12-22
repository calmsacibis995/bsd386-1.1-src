/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/Class2Send.c++,v 1.1.1.1 1994/01/14 23:09:46 torek Exp $
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
#include "Class2.h"
#include "ModemConfig.h"

/*
 * Send Protocol for Class-2-style modems.
 */

/*
 * Status messages to ignore when dialing.
 */
static fxBool
isNoise(const char* s)
{
    static const char* noiseMsgs[] = {
	"CED",		// RC32ACL-based modems send this before +FCON
	"DIALING",
	"RRING",	// Telebit
	"RINGING",	// ZyXEL
	NULL
    };

    for (u_int i = 0; noiseMsgs[i] != NULL; i++)
	if (strneq(s, noiseMsgs[i], strlen(noiseMsgs[i])))
	    return (TRUE);
    return (FALSE);
}

/*
 * Process the response to a dial command.
 */
CallStatus
Class2Modem::dialResponse()
{
    ATResponse r;

    do {
	/*
	 * Use a dead-man timeout since some
	 * modems seem to get hosed and lockup.
	 */
	r = atResponse(rbuf, conf.dialResponseTimeout);
	switch (r) {
	case AT_ERROR:	    return (ERROR);	// error in dial command
	case AT_BUSY:	    return (BUSY);	// busy signal
	case AT_NOCARRIER:  return (NOCARRIER);	// no carrier detected
	case AT_NODIALTONE: return (NODIALTONE);// local phone connection hosed
	case AT_NOANSWER:   return (NOANSWER);	// no answer or ring back
	case AT_FHNG:	    return (NOANSWER);	// usually T1 timeout
	case AT_FCON:	    return (OK);	// fax connection

	case AT_TIMEOUT:			// timed out w/o response
	case AT_CONNECT:			// modem thinks data connection
	    hangup();
	    break;
	}
    } while (r == AT_OTHER && isNoise(rbuf));
    return (FAILURE);
}

/*
 * Process the string of session-related information
 * sent to the caller on connecting to a fax machine.
 */
fxBool
Class2Modem::getPrologue(Class2Params& dis, u_int& nsf, fxStr& csi, fxBool& hasDoc)
{
    fxBool gotParams = FALSE;
    hasDoc = FALSE;
    nsf = 0;
    for (;;) {
	switch (atResponse(rbuf, conf.t1Timer)) {
	case AT_TIMEOUT:
	case AT_EMPTYLINE:
	case AT_NOCARRIER:
	case AT_NODIALTONE:
	case AT_NOANSWER:
	case AT_ERROR:
	    processHangup("10");		// Unspecified Phase A error
	    return (FALSE);
	case AT_FPOLL:
	    hasDoc = TRUE;
	    protoTrace("REMOTE has document to POLL");
	    break;
	case AT_FDIS:
	    gotParams = parseClass2Capabilities(rbuf+6, dis);
	    break;
	case AT_FNSF:
	    { char* cp = rbuf+6;
	      nsf = fromHex(cp, strlen(cp));	// extract NSF information
	      protoTrace("REMOTE NSF \"%s\"", cp);
	    }
	    break;
	case AT_FCSI:
	    csi = stripQuotes(rbuf+6);
	    recvCSI(csi);
	    break;
	case AT_FHNG:
	    return (FALSE);
	case AT_OK:
	    return (gotParams);
	}
    }
}

void
Class2Modem::sendBegin()
{
    // polling accounting?
}

/*
 * Initiate data transfer from the host to the modem when
 * doing a send.  Note that Class 2.0 says that we're
 * supposed to wait for an XON from the modem in response
 * to the +FDT, before actually sending any data.  This
 * is not the case with most modems however, so whether or
 * not we do this is a configuration parameter.
 */
fxBool
Class2Modem::dataTransfer()
{
    return (vatFaxCmd(AT_NOTHING, "DT") &&
	waitFor(AT_CONNECT, conf.pageStartTimeout) &&
	(!conf.class2XmitWaitForXON || modemStopOutput()));
}

/*
 * Send the specified document using the supplied
 * parameters.  The pph is the post-page-handling
 * indicators calculated prior to intiating the call.
 */
fxBool
Class2Modem::sendPhaseB(TIFF* tif, Class2Params& next, FaxMachineInfo& info,
    fxStr& pph, fxStr& emsg)
{
    int ntrys = 0;			// # retraining/command repeats

    setDataTimeout(60, next.br);	// 60 seconds for 1024 byte writes
    hangupCode[0] = '\0';

    fxBool transferOK;
    fxBool morePages = FALSE;
    do {
	transferOK = FALSE;
	if (abortRequested())
	     break;
	/*
	 * Check the next page to see if the transfer
	 * characteristics change.  If so, update the
	 * current T.30 session parameters.
	 */
	if (params != next) {
	    if (!class2Cmd("DIS", next))
		break;
	    params = next;
	}
	if (dataTransfer() && sendPage(tif)) {
	    /*
	     * Page transferred, process post page response from
	     * remote station (XXX need to deal with PRI requests).).
	     */
	    morePages = !TIFFLastDirectory(tif);
	    u_int ppm;
	    // XXX check pph length
	    switch (pph[0]) {
	    default:
		morePages = FALSE;
		emsg = "Unknown post-page-handling indicator \"" | pph | "\"";
		/* fall thru... */
	    case 'P': ppm = PPM_EOP; break;
	    case 'M': ppm = PPM_EOM; break;
	    case 'S': ppm = PPM_MPS; break;
	    }
	    u_int ppr;
	    if (pageDone(ppm, ppr)) {
		switch (ppr) {
		case PPR_MCF:		// page good
		case PPR_PIP:		// page good, interrupt requested
		case PPR_RTP:		// page good, retrain requested
		    countPage();
		    pph.remove(0);	// discard post-page-handling
		    ntrys = 0;
		    if (morePages) {
			if (!TIFFReadDirectory(tif)) {
			    emsg = "Problem reading document directory";
			    break;
			}
			if (!sendSetupParams(tif, next, info, emsg))
			    break;
		    }
		    transferOK = TRUE;
		    break;
		case PPR_RTN:		// page bad, retrain requested
		    if (++ntrys >= 3) {
			emsg = "Unable to transmit page"
			       " (giving up after 3 attempts)";
			transferOK = FALSE;
			break;
		    }
		    morePages = TRUE;	// retransmit page
		    transferOK = TRUE;
		    break;
		case PPR_PIN:		// page bad, interrupt requested
		    break;
		}
	    }
	}
    } while (transferOK && morePages);
    if (!transferOK) {
	if (emsg == "") {
	    if (hangupCode[0])
		emsg = hangupCause(hangupCode);
	    else
		emsg = "Communication failure during Phase B/C";
	}
	abort();			// terminate session
    }
    return (transferOK);
}

/*
 * Handle the page-end protocol.
 */
fxBool
Class2Modem::pageDone(u_int ppm, u_int& ppr)
{
    ppr = 0;			// something invalid
    if (vatFaxCmd(AT_NOTHING, "ET=%u", ppm)) {
	for (;;) {
	    switch (atResponse(rbuf, conf.pageDoneTimeout)) {
	    case AT_FPTS:
		if (sscanf(rbuf+6, "%u,", &ppr) != 1) {
		    protoTrace("MODEM protocol botch (\"%s\"), %s",
			rbuf, "can not parse PPM");
		    return (FALSE);		// force termination
		}
		break;
	    case AT_OK:
		return (TRUE);
	    case AT_FHNG:
		return (isNormalHangup());
	    case AT_EMPTYLINE:
		/*
		 * The ZyXEL modem appears to drop DCD when the
		 * remote side drops carrier, no matter whether
		 * DCD is configured to follow carrier or not.
		 * This results in a stream of empty lines,
		 * *sometimes* followed by the requisite trailing OK.
		 * As a hack workaround to deal with the situation
		 * we accept the post page response if this is the
		 * last page that we're sending and the page is
		 * good (i.e. we would hang up immediately anyway).
		 */
		if (ppm == PPM_EOP && ppr == PPR_MCF)
		    return (TRUE);
		/* fall thru... */
	    case AT_TIMEOUT:
	    case AT_NOCARRIER:
	    case AT_NODIALTONE:
	    case AT_NOANSWER:
	    case AT_ERROR:
		goto bad;
	    }
	}
    }
bad:
    processHangup("50");		// Unspecified Phase D error
    return (FALSE);
}

/*
 * Abort a data transfer in progress.
 */
void
Class2Modem::abortDataTransfer()
{
    char c = CAN;
    putModemData(&c, 1);
}

/*
 * Send a page of data using the ``stream interface''.
 */
fxBool
Class2Modem::sendPage(TIFF* tif)
{
    fxBool rc = TRUE;
    protoTrace("SEND begin page");
    if (flowControl == FLOW_XONXOFF)
	setXONXOFF(FLOW_XONXOFF, FLOW_NONE, ACT_FLUSH);
    /*
     * Correct bit order of data if not what modem expects.
     */
    u_short fillorder;
    TIFFGetFieldDefaulted(tif, TIFFTAG_FILLORDER, &fillorder);
    const u_char* bitrev = TIFFGetBitRevTable(fillorder != conf.sendFillOrder);

    u_long* stripbytecount;
    (void) TIFFGetField(tif, TIFFTAG_STRIPBYTECOUNTS, &stripbytecount);
    for (u_int strip = 0; strip < TIFFNumberOfStrips(tif) && rc; strip++) {
	u_int totbytes = (u_int) stripbytecount[strip];
	if (totbytes > 0) {
	    u_char* data = new u_char[totbytes];
	    if (TIFFReadRawStrip(tif, strip, data, totbytes) >= 0) {
		/*
		 * Pass data to modem, filtering DLE's and
		 * being careful not to get hung up.
		 */
		beginTimedTransfer();
		rc = putModemDLEData(data, totbytes, bitrev, getDataTimeout());
		endTimedTransfer();
		protoTrace("SENT %d bytes of data", totbytes);
	    }
	    delete data;
	}
    }
    if (rc)
	rc = sendEOT();
    else
	abortDataTransfer();
    if (flowControl == FLOW_XONXOFF)
	setXONXOFF(FLOW_CURRENT, FLOW_XONXOFF, ACT_DRAIN);
    protoTrace("SEND end page");
    return (rc ? (waitFor(AT_OK) && hangupCode[0] == '\0') : rc);
}

/*
 * Send an end-of-transmission signal to the modem.
 */
fxBool
Class2Modem::sendEOT()
{
    static char EOT[] = { DLE, ETX };
    return (putModemData(EOT, sizeof (EOT)));
}
