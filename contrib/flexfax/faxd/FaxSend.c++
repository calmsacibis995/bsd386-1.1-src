/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/FaxSend.c++,v 1.1.1.1 1994/01/14 23:09:48 torek Exp $
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
#include <osfcn.h>
#include <stdio.h>
#include <sys/stat.h>

#include <Dispatch/dispatcher.h>

#include "tiffio.h"
#include "FaxServer.h"
#include "FaxMachineInfo.h"
#include "FaxRecvInfo.h"
#include "UUCPLock.h"
#include "faxServerApp.h"
#include "t.30.h"
#include "config.h"

/*
 * FAX Server Transmission Protocol.
 */
void
FaxServer::sendFax(FaxRequest& fax, FaxMachineInfo& clientInfo)
{
    assert(fax.files.length() != 0);
    assert(fax.number != "");
    assert(fax.sender != "");
    assert(fax.files.length() == fax.ops.length());
    npages = fax.npages;
    if (modemLock->lock()) {
	if (setupModem()) {
	    changeState(SENDING);
	    IOHandler* handler =
		Dispatcher::instance().handler(modemFd, Dispatcher::ReadMask);
	    if (handler)
		Dispatcher::instance().unlink(modemFd);
	    /*
	     * Prepare the job for transmission by analysing
	     * the page characteristics and determining whether
	     * or not the page transfer parameters will have
	     * to be renegotiated after the page is sent.  This
	     * is done before the call is placed because it can
	     * be slow and there can be timing problems if this
	     * is done during transmission.
	     */
	    fxStr emsg;
	    if (sendPrepareFax(fax, clientInfo, emsg)) {
		/*
		 * Construct the phone number to dial by applying the
		 * dialing rules to the user-specified dialing string.
		 */
		fxStr dial = prepareDialString(fax.number);
		fxStr canon = canonicalizePhoneNumber(fax.number);
		log = new FaxMachineLog(canon, logMode);
		setServerStatus("Sending job %s to %s",
		    (char*) fax.jobid, (char*) fax.external);
		sendFax(fax, clientInfo, dial);
		delete log, log = NULL;
	    } else
		sendFailed(fax, send_failed, emsg);
	    /*
	     * Because some modems are impossible to safely hangup in the
	     * event of a problem, we force a close on the device so that
	     * the modem will see DTR go down and (hopefully) clean up any
	     * bad state its in.  We then wait a couple of seconds before
	     * trying to setup the modem again so that it can have some
	     * time to settle.  We want a handle on the modem so that we
	     * can be prepared to answer incoming phone calls.
	     */
	    discardModem(TRUE);
	    changeState(MODEMWAIT, 5);
	} else {
	    sendFailed(fax, send_retry,
		"Can not setup modem", (u_int) (4*TIMER_POLLMODEM));
	    discardModem(TRUE);
	    changeState(MODEMWAIT, TIMER_POLLMODEM);
	}
	modemLock->unlock();
    } else {
	if (state != LOCKWAIT)
	    sendFailed(fax, send_retry,
		"Can not lock modem device", (u_int) (2*TIMER_POLLMODEM));
	if (state != SENDING && state != ANSWERING)
	    changeState(LOCKWAIT, TIMER_POLLMODEM);
    }
    u_int pagesSent = npages - fax.npages;
    fax.npages = npages;
    app->notifySendDone(&fax,
	pagesSent,
	clientInfo.getCSI(),
	atoi(Class2Params::bitRateNames[clientParams.br & 7]),
	Class2Params::dataFormatNames[clientParams.df & 3]);
    if (fax.status != send_retry)
	clientInfo.setJobInProgress("");
}

void
FaxServer::sendFailed(FaxRequest& fax, FaxSendStatus stat, const char* notice, u_int tts)
{
    fax.status = stat;
    fax.notice = notice;
    fax.tts = time(0) + tts;
    traceStatus(FAXTRACE_SERVER, "SEND FAILED: %s", notice);
}

u_int FaxServer::requeueTTS[7] = {
    0,			// phone answered and carrier received
    5*60,		// destination phone busy
    FAX_REQUEUE,	// no carrier from remote
    FAX_REQUEUE,	// no answer from remote
    FAX_REQUEUE,	// no local dialtone
    FAX_REQUEUE,	// error in dial command
    FAX_REQUEUE,	// other problem (modem turned off)
};

/*
 * Send the specified TIFF files to the FAX
 * agent at the given phone number.
 */
void
FaxServer::sendFax(FaxRequest& fax, FaxMachineInfo& clientInfo, const fxStr& number)
{
    int oTracingLevel = logTracingLevel;	// save default value
    (void) clientInfo.getTracingLevel(logTracingLevel);
    /*
     * Check if this job includes a poll request, and
     * if it does, inform the modem in case it needs to
     * do something to get back status about whether or
     * not documents are available for retrieval.
     */
    FaxSendOp op = send_poll;
    if (fax.ops.find(op) != fx_invalidArrayIndex)
	modem->requestToPoll();
    fax.notice = "";
    abortCall = FALSE;
    clientInfo.setJobInProgress(fax.qfile);
    CallStatus callstat = modem->dial(number);
    if (callstat == FaxModem::OK && !abortRequested()) {
	/*
	 * Call reached a fax machine.  Check remote's
	 * capabilities against those required by the
	 * job and setup for transmission.
	 */
	fax.ndials = 0;
	clientInfo.setCalledBefore(TRUE);
	clientInfo.setDialFailures(0);
	modem->sendBegin();
	fxBool remoteHasDoc = FALSE;
	fxStr notice;
	u_int nsf;
	fxStr csi;
	if (!modem->getPrologue(clientCapabilities, nsf, csi, remoteHasDoc)) {
	    // NB: assume receiver is T.4 compatible...
	    sendFailed(fax, send_retry,
	"Unable to get remote capabilities or remote is not T.4 compatible",
		FAX_REQUEUE);
	} else {
	    clientInfo.setCSI(csi);			// record remote CSI
	    if (!sendClientCapabilitiesOK(clientInfo, nsf, notice)) {
		// NB: mark job completed 'cuz there's no way recover
		sendFailed(fax, send_failed, notice);
	    } else {
		modem->sendSetupPhaseB();
		/*
		 * Group 3 protocol forces any sends to precede any polling.
		 */
		fax.status = send_done;			// be optimistic
		op = send_tiff;
		while (fax.ops.length() > 0) {		// send operations
		    u_int i = fax.ops.find(op);
		    if (i == fx_invalidArrayIndex)
			break;
		    traceStatus(FAXTRACE_PROTOCOL, "SEND file \"%s\"",
			(char*) fax.files[i]);
		    if (!sendFaxPhaseB(fax, clientInfo, fax.files[i]))
			break;
		    /*
		     * The file was delivered, notify the server.
		     * Note that a side effect of the notification
		     * is that this file is deleted from the set of
		     * files to send (so that it's not sent again
		     * if the job is requeued).  This is why we call
		     * find again at the top of the loop
		     */
		    app->notifyDocumentSent(fax, i);
		    fax.dirnum = 0;
		}
		if (fax.status == send_done && fax.ops.length() > 0)
		    sendPoll(fax, remoteHasDoc);
	    }
	}
	modem->sendEnd();
	if (fax.status != send_done) {
	    clientInfo.setSendFailures(clientInfo.getSendFailures()+1);
	    clientInfo.setLastSendFailure(fax.notice);
	} else
	    clientInfo.setSendFailures(0);
    } else {
	/*
	 * Analyze the call status codes and selectively decide if the
	 * job should be retried.  We try to avoid the situations where
	 * we might be calling the wrong number so that we don't end up
	 * harrassing someone w/ repeated calls.
	 */
	fax.ndials++;
	switch (callstat) {
	case FaxModem::NOCARRIER:	// no carrier detected on remote side
	case FaxModem::FAILURE:		// typically from dialing a data modem
	    /*
	     * Since some modems can not distinguish between ``No Carrier''
	     * and ``No Answer'' we offer this configurable hack whereby
	     * we'll retry the job <n> times in the face of ``No Carrier''
	     * dialing errors; if we've never previously reached a facsimile
	     * machine at that number.  This should not be used except if
	     * the modem is incapable of distinguishing betwee ``No Carrier''
	     * and ``No Answer''.
	     */
	    if (!clientInfo.getCalledBefore() && fax.ndials > noCarrierRetrys) {
		sendFailed(fax, send_failed, FaxModem::callStatus[callstat]);
		/*
		 * If the modem appears to distinguish ``No Carrier'' from
		 * ``No Answer'', then also reject future jobs to this number
		 * by setting up a rejection notice in the machine info
		 * database.
		 */
		if (noCarrierRetrys <= 1)
		    clientInfo.setRejectNotice(FaxModem::callStatus[callstat]);
	    } else
		sendFailed(fax, send_retry,
		    FaxModem::callStatus[callstat], requeueTTS[callstat]);
	    break;
	case FaxModem::BUSY:		// busy signal
	case FaxModem::NOANSWER:	// no answer or ring back
	case FaxModem::NODIALTONE:	// no local dialtone, possibly unplugged
	case FaxModem::ERROR:		// modem might just need to be reset
	    sendFailed(fax, send_retry,
		FaxModem::callStatus[callstat], requeueTTS[callstat]);
	    break;
	case FaxModem::OK:		// call was aborted by user
	    break;
	}
	if (callstat != FaxModem::OK) {
	    clientInfo.setDialFailures(clientInfo.getDialFailures()+1);
	    clientInfo.setLastDialFailure(fax.notice);
	}
    }
    if (abortCall)
	sendFailed(fax, send_failed, "Job aborted by user");
    else if (fax.status == send_retry && ++fax.totdials == fax.maxdials)
	sendFailed(fax, send_failed, "Too many attempts to send");
    modem->hangup();
    /*
     * Cleanup after the call.  If we have new information on
     * the client's remote capabilities, the machine info
     * database will be updated when the instance is destroyed.
     */
    logTracingLevel = oTracingLevel;	// restore default value
}

/*
 * Process a polling request.
 */
void
FaxServer::sendPoll(FaxRequest& fax, fxBool remoteHasDoc)
{
    FaxSendOp op = send_poll;
    u_int i = fax.ops.find(op);
    if (i == fx_invalidArrayIndex) {
	fax.notice = "polling operation not done because of internal failure";
	traceStatus(FAXTRACE_SERVER, "internal muckup, lost polling request");
	// NB: job is marked done
    } else if (!remoteHasDoc) {
	fax.notice = "remote has no document to poll";
	traceStatus(FAXTRACE_SERVER, "REJECT: " | fax.notice);
	// override to force status about polling failure
	if (fax.notify == FaxRequest::no_notice)
	    fax.notify = FaxRequest::when_done;
    } else {
	fxStr cig = canonicalizePhoneNumber(fax.files[i]);
	if (cig == "")
	    cig = canonicalizePhoneNumber(FAXNumber);
	traceStatus(FAXTRACE_PROTOCOL, "POLL with CIG \"%s\"", (char*) cig);
	FaxRecvInfoArray docs;
	fax.status =
	    (pollFaxPhaseB(cig, docs, fax.notice) ? send_done : send_retry);
	for (u_int j = 0; j < docs.length(); j++) {
	    const FaxRecvInfo& ri = docs[j];
	    if (ri.npages > 0) {
		(void) chmod((char*) ri.qfile, recvFileMode);
		app->notifyPollRecvd(fax, ri);
	    } else {
		traceStatus(FAXTRACE_SERVER, "POLL: empty file \"%s\" deleted",
		    (char*) ri.qfile);
		unlink((char*) ri.qfile);
	    }
	}
	if (fax.status == send_done)
	    app->notifyPollDone(fax, i);
    }
}

/*
 * Phase B of Group 3 protocol.
 */
fxBool
FaxServer::sendFaxPhaseB(FaxRequest& fax, FaxMachineInfo& clientInfo, const fxStr& file)
{
    fxBool status = FALSE;
    TIFF* tif = TIFFOpen(file, "r");
    if (tif && (fax.dirnum == 0 || TIFFSetDirectory(tif, fax.dirnum))) {
	// set up DCS according to file characteristics
	if (sendSetupParams(tif, clientParams, clientInfo, fax.notice)) {
	    /*
	     * Count pages sent and advance dirnum so that if we
	     * terminate prematurely we'll only transmit what's left
	     * in the current document/file.  Also, if nothing is
	     * sent, bump the counter on the number of times we've
	     * attempted to send the current page.  We don't try
	     * more than 3 times--to avoid looping.
	     */
	    u_int prevPages = npages;
	    status = modem->sendPhaseB(tif, clientParams, clientInfo,
		fax.pagehandling, fax.notice);
	    if (!status)
		fax.status = send_retry;
	    if (npages == prevPages) {
		fax.ntries++;
		if (fax.ntries > 2) {
		    if (fax.notice != "")
			fax.notice.append("; ");
		    fax.notice.append(
			"Giving up after 3 attempts to send same page");
		    traceStatus(FAXTRACE_SERVER, "SEND: %s \"%s\", dirnum %d",
			(char*) fax.notice, (char*) file, fax.dirnum);
		    fax.status = send_failed;
		}
	    } else {
		fax.dirnum += npages - prevPages;
		fax.ntries = 0;
	    }
	} else
	    fax.status = send_failed;
    } else {
	fax.notice = tif ? "Can not set directory in document file" :
			   "Can not open document file";
	traceStatus(FAXTRACE_SERVER, "SEND: %s \"%s\", dirnum %d",
	    (char*) fax.notice, (char*) file, fax.dirnum);
	fax.status = send_failed;
    }
    if (tif)
	TIFFClose(tif);
    return (status);
}

/*
 * Check client's capabilities (DIS) against those of the
 * modem and select the parameters that are best for us.
 */
fxBool
FaxServer::sendClientCapabilitiesOK(FaxMachineInfo& clientInfo, u_int nsf, fxStr& emsg)
{
    /*
     * Select signalling rate and minimum scanline time
     * for the duration of the session.  These are not
     * changed once they are set here.
     */
    clientInfo.setMaxSignallingRate(clientCapabilities.br);
    int signallingRate =
	modem->selectSignallingRate(clientInfo.getMaxSignallingRate());
    if (signallingRate == -1) {
	emsg = "Modem does not support negotiated signalling rate";
	return (FALSE);
    }
    clientParams.br = signallingRate;
    int minScanlineTime = modem->selectScanlineTime(clientCapabilities.st);
    if (minScanlineTime == -1) {
	emsg = "Modem does not support negotiated min scanline time";
	return (FALSE);
    }
    clientParams.st = minScanlineTime;
    clientParams.ec = EC_DISABLE;		// XXX
    clientParams.bf = BF_DISABLE;
    /*
     * Record the remote machine's capabilities for use below in
     * selecting tranfer parameters for each page sent.  The info
     * constructed here is also recorded in a private database for
     * use in pre-formatting documents sent in future conversations.
     */
    clientInfo.setSupportsHighRes(clientCapabilities.vr == VR_FINE);
    clientInfo.setSupports2DEncoding(clientCapabilities.df >= DF_2DMR);
    clientInfo.setMaxPageWidth(pageWidthCodes[clientCapabilities.wd]);
    clientInfo.setMaxPageLength(pageLengthCodes[clientCapabilities.ln]);
    if (nsf) {
	// XXX add Adobe's PostScript protocol
	traceStatus(FAXTRACE_PROTOCOL, "REMOTE NSF %#x", nsf);
    }
    traceStatus(FAXTRACE_PROTOCOL, "REMOTE best rate %s",
	Class2Params::bitRateNames[clientCapabilities.br]);
    traceStatus(FAXTRACE_PROTOCOL, "REMOTE max %s",
	Class2Params::pageWidthNames[clientCapabilities.wd]);
    traceStatus(FAXTRACE_PROTOCOL, "REMOTE max %s",
	Class2Params::pageLengthNames[clientCapabilities.ln]);
    traceStatus(FAXTRACE_PROTOCOL, "REMOTE best vres %s",
	Class2Params::vresNames[clientCapabilities.vr]);
    traceStatus(FAXTRACE_PROTOCOL, "REMOTE best format %s",
	Class2Params::dataFormatNames[clientCapabilities.df]);
    if (clientCapabilities.ec == EC_ENABLE)
	traceStatus(FAXTRACE_PROTOCOL, "REMOTE supports error correction");
    traceStatus(FAXTRACE_PROTOCOL, "REMOTE best %s",
	Class2Params::scanlineTimeNames[clientCapabilities.st]);
    traceStatus(FAXTRACE_PROTOCOL, "REMOTE %s PostScript transfer",
	clientInfo.getSupportsPostScript() ? "supports" : "does not support");

    traceStatus(FAXTRACE_PROTOCOL, "USE %s",
	Class2Params::bitRateNames[clientParams.br]);
    traceStatus(FAXTRACE_PROTOCOL, "USE %s",
	Class2Params::scanlineTimeNames[clientParams.st]);
    return (TRUE);
}

/*
 * Prepare the job for transmission by analysing
 * the page characteristics and determining whether
 * or not the page transfer parameters will have
 * to be renegotiated after the page is sent.  This
 * is done before the call is placed because it can
 * be slow and there can be timing problems if this
 * is done during transmission.
 */
fxBool
FaxServer::sendPrepareFax(FaxRequest& fax, FaxMachineInfo& clientInfo, fxStr& emsg)
{
    if (fax.pagehandling != "")		// already done
	return (TRUE);
    /*
     * Scan the pages and figure out where session parameters
     * will need to be renegotiated.  Construct a string of
     * indicators to use when doing the actual transmission.
     *
     * NB: all files are coalesced into a single fax document
     *     if possible
     */
    Class2Params params;		// current parameters
    Class2Params next;			// parameters for next page
    TIFF* tif = NULL;			// current open TIFF image
    params.df = (u_int) -1;		// indicates first page
    for (u_int i = 0;;) {
	if (!tif || TIFFLastDirectory(tif)) {
	    /*
	     * Locate the next file to be sent.
	     */
	    if (tif)			// close previous file
		TIFFClose(tif), tif = NULL;
	    if (i >= fax.ops.length())
		goto done;
	    FaxSendOp op = send_tiff;
	    i = fax.ops.find(op, i);
	    if (i == fx_invalidArrayIndex)
		goto done;
	    tif = TIFFOpen(fax.files[i], "r");
	    if (tif == NULL) {
		emsg = "Can not open document file";
		goto bad;
	    }
	    i++;			// advance for next find
	} else {
	    /*
	     * Read the next TIFF directory.
	     */
	    if (!TIFFReadDirectory(tif)) {
		emsg = "Problem reading document directory";
		goto bad;
	    }
	}
	next = params;
	if (!sendSetupParams1(tif, next, clientInfo, emsg))
	    goto bad;
	if (params.df != (u_int) -1) {
	    /*
	     * The pagehandling string has:
	     * 'M' = EOM, for when parameters must be renegotiated
	     * 'S' = MPS, for when next page uses the same parameters
	     * 'P' = EOP, for the last page to be transmitted
	     */
	    fax.pagehandling.append(next == params ? 'S' : 'M');
	}
	params = next;
    }
done:
    fax.pagehandling.append('P');		// EOP
    return (TRUE);
bad:
    if (tif)
	TIFFClose(tif);
    return (FALSE);
}

/*
 * Select session parameters according to the info
 * in the TIFF file.  We setup the encoding scheme,
 * page width & length, and vertical-resolution
 * parameters.  If the remote machine is incapable
 * of handling the image, we bail out.
 *
 * Note that we shouldn't be rejecting too many files
 * because we cache the capabilities of the remote machine
 * and use this to image the facsimile.  This work is
 * mainly done to optimize transmission and to reject
 * anything that might sneak by.
 */
fxBool
FaxServer::sendSetupParams1(TIFF* tif, Class2Params& params, FaxMachineInfo& clientInfo, fxStr& emsg)
{
    short compression;
    (void) TIFFGetField(tif, TIFFTAG_COMPRESSION, &compression);
    if (compression != COMPRESSION_CCITTFAX3) {
	emsg = "Document is not in a Group 3-compatible format";
	traceStatus(FAXTRACE_SERVER, "SEND: %s (file %s, compression %d)",
	    (char*) emsg, TIFFFileName(tif), compression);
	return (FALSE);
    }

    // XXX perhaps should verify samples and bits/sample???
    long g3opts;
    if (!TIFFGetField(tif, TIFFTAG_GROUP3OPTIONS, &g3opts))
	g3opts = 0;
    if (g3opts & GROUP3OPT_2DENCODING) {
	if (!clientInfo.getSupports2DEncoding()) {
	    emsg = "Document was encoded with 2DMR,"
		   " but client does not support this data format";
	    traceStatus(FAXTRACE_SERVER, "REJECT: %s", (char*) emsg);
	    return (FALSE);
	}
	params.df = DF_2DMR;
    } else
	params.df = DF_1DMR;

    u_long w;
    (void) TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
    if (w > clientInfo.getMaxPageWidth()) {
	emsg = "Client does not support document page width";
	traceStatus(FAXTRACE_SERVER,
	    "REJECT: %s, max remote page width %u pixels"
		 ", image width %lu pixels",
	    (char*) emsg, clientInfo.getMaxPageWidth(), w);
	return (FALSE);
    }
    // NB: only common values
    params.wd = (w <= 1728 ? WD_1728 : w <= 2048 ? WD_2048 : WD_2432);

    /*
     * Try to deduce the vertical resolution of the image
     * image.  This can be problematical for arbitrary TIFF
     * images 'cuz vendors sometimes don't give the units.
     * We, however, can depend on the info in images that
     * we generate 'cuz we're careful to include valid info.
     */
    float yres;
    if (TIFFGetField(tif, TIFFTAG_YRESOLUTION, &yres)) {
	short resunit = RESUNIT_NONE;
	(void) TIFFGetField(tif, TIFFTAG_RESOLUTIONUNIT, &resunit);
	if (resunit == RESUNIT_INCH)
	    yres /= 25.4;
    } else {
	/*
	 * No vertical resolution is specified, try
	 * to deduce one from the image length.
	 */
	u_long l;
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &l);
	yres = (l < 1450 ? 3.85 : 7.7);		// B4 at 98 lpi is ~1400 lines
    }
    if (yres >= 7.) {
	if (!clientInfo.getSupportsHighRes()) {
	    emsg = "High resolution document is not supported by client";
	    traceStatus(FAXTRACE_SERVER,
		"REJECT: %s, image resolution %g lines/mm",
		(char*) emsg, yres);
	    return (FALSE);
	}
	params.vr = VR_FINE;
    } else
	params.vr = VR_NORMAL;

    /*
     * Select page length according to the image size and
     * vertical resolution.  Note that if the resolution
     * info is bogus, we may select the wrong page size.
     * Note also that we're a bit lenient in places here
     * to take into account sloppy coding practice (e.g.
     * using 200 dpi for high-res facsimile.
     */
    if (clientInfo.getMaxPageLength() != -1) {
	u_long h = 0;
	(void) TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
	float len = h / yres;			// page length in mm
	if (len > clientInfo.getMaxPageLength()) {
	    emsg = "Client does not support document page length";
	    traceStatus(FAXTRACE_SERVER,
		"REJECT: %s, max remote page length %d mm"
		    ", image length %lu rows (%.2f mm)",
		(char*) emsg, clientInfo.getMaxPageLength(), h, len);
	    return (FALSE);
	}
	// 330 is chosen 'cuz it's half way between A4 & B4 lengths
	params.ln = (len < 330 ? LN_A4 : LN_B4);
    } else
	params.ln = LN_INF;
    return (TRUE);
}

fxBool
FaxServer::sendSetupParams(TIFF* tif, Class2Params& params, FaxMachineInfo& clientInfo, fxStr& emsg)
{
    if (sendSetupParams1(tif, params, clientInfo, emsg)) {
	traceStatus(FAXTRACE_PROTOCOL, "USE %s",
	    Class2Params::pageWidthNames[params.wd]);
	traceStatus(FAXTRACE_PROTOCOL, "USE %s",
	    Class2Params::pageLengthNames[params.ln]);
	traceStatus(FAXTRACE_PROTOCOL, "USE %s",
	    Class2Params::vresNames[params.vr]);
	traceStatus(FAXTRACE_PROTOCOL, "USE %s",
	    Class2Params::dataFormatNames[params.df]);
	return (TRUE);
    } else
	return (FALSE);
}
