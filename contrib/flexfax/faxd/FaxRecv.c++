/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/FaxRecv.c++,v 1.1.1.1 1994/01/14 23:09:47 torek Exp $
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
#include <unistd.h>
#include <osfcn.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdio.h>

#include <Dispatch/dispatcher.h>

#include "tiffio.h"
#include "FaxServer.h"
#include "FaxRecvInfo.h"
#include "RegExArray.h"
#include "faxServerApp.h"
#include "t.30.h"
#include "config.h"
#include "flock.h"

/*
 * FAX Server Reception Protocol.
 */

fxBool
FaxServer::recvFax()
{
    fxStr emsg;
    changeState(RECEIVING);
    traceStatus(FAXTRACE_PROTOCOL, "RECV: begin");
    okToRecv = (qualifyTSI == "");	// anything ok if not qualifying
    recvTSI = "";			// sender's identity initially unknown
    FaxRecvInfoArray docs;
    FaxRecvInfo info;
    fxBool faxRecognized = FALSE;

    /*
     * Create the first file ahead of time to avoid timing
     * problems with Class 1 modems.  (Creating the file
     * after recvBegin can cause part of the first page to
     * be lost.)
     */
    TIFF* tif = setupForRecv("RECV", info, docs);
    if (tif) {
	recvStart = time(0);		// count initial negotiation
	if (faxRecognized = modem->recvBegin(emsg)) {
	    (void) recvDocuments("RECV", tif, info, docs, emsg);
	    if (!modem->recvEnd(emsg))
		traceStatus(FAXTRACE_PROTOCOL, "RECV: %s (end)", (char*) emsg);
	} else {
	    traceStatus(FAXTRACE_PROTOCOL, "RECV: %s (begin)", (char*) emsg);
	    TIFFClose(tif);
	}
    }
    /*
     * Now that the session is completed, do local processing
     * that might otherwise slow down the protocol (and potentially
     * cause timing problems).
     */
    for (u_int i = 0, n = docs.length(); i < n; i++) {
	const FaxRecvInfo& ri = docs[i];
	if (ri.npages > 0) {
	    (void) chmod((char*) ri.qfile, recvFileMode);
	    app->notifyRecvDone(ri);
	} else {
	    traceStatus(FAXTRACE_SERVER, "RECV: empty file \"%s\" deleted",
		(char*) ri.qfile);
	    unlink((char*) ri.qfile);
	}
    }
    traceStatus(FAXTRACE_PROTOCOL, "RECV: end");
    return (faxRecognized);
}

/*
 * Create and lock a temp file for receiving data.
 */
TIFF*
FaxServer::setupForRecv(const char* op, FaxRecvInfo& ri, FaxRecvInfoArray& docs)
{
    ri.qfile = FAX_RECVDIR;
    ri.qfile.append("/faxXXXXXX");
    int fd = mkstemp((char*) ri.qfile);
    if (fd >= 0) {
	ri.npages = 0;			// mark it to be deleted...
	docs.append(ri);		// ...add it in to the set
	TIFF* tif = TIFFFdOpen(fd, ri.qfile, "w");
	if (tif != NULL) {
	    (void) flock(TIFFFileno(tif), LOCK_EX|LOCK_NB);
	    return (tif);
	}
    }
    traceStatus(FAXTRACE_SERVER,
	"%s: Unable to create file \"%s\" for received data", op,
	(char*) ri.qfile);
    return (NULL);
}

/*
 * Receive one or more documents.
 */
fxBool
FaxServer::recvDocuments(const char* op, TIFF* tif, FaxRecvInfo& info, FaxRecvInfoArray& docs, fxStr& emsg)
{
    fxBool recvOK;
    int ppm;
    for (;;) {
	 if (!okToRecv) {
	    traceStatus(FAXTRACE_SERVER,
		"%s: Permission denied (unacceptable client TSI)", op);
	    TIFFClose(tif);
	    return (FALSE);
	}
	traceStatus(FAXTRACE_SERVER, "RECV data in \"%s\"", (char*) info.qfile);
	npages = 0;
	time_t recvStart = time(0);
	recvOK = recvFaxPhaseB(tif, ppm, emsg);
	if (!recvOK)
	    traceStatus(FAXTRACE_PROTOCOL, "%s: %s (Phase B)", op, (char*)emsg);
	TIFFClose(tif);
	recvComplete(info, time(0) - recvStart, emsg);
	docs[docs.length()-1] = info;
	if (!recvOK || ppm == PPM_EOP)
	    return (recvOK);
	/*
	 * Setup state for another file.
	 */
	tif = setupForRecv(op, info, docs);
	if (tif == NULL)
	    return (FALSE);
	recvStart = time(0);
    }
    /*NOTREACHED*/
}

/*
 * Receive Phase B protocol processing.
 */
fxBool
FaxServer::recvFaxPhaseB(TIFF* tif, int& ppm, fxStr& emsg)
{
    ppm = PPM_EOP;
    do {
	if (!modem->recvPage(tif, ppm, emsg))
	    return (FALSE);
	// XXX handle PRI
	if (PPM_PRI_MPS <= ppm && ppm <= PPM_PRI_EOP)
	    traceStatus(FAXTRACE_PROTOCOL, "RECV interrupt request ignored");
    } while (ppm == PPM_MPS || ppm == PPM_PRI_MPS);
    return (TRUE);
}

/*
 * Fill in a receive information block
 * from the server's current receive state.
 */
void
FaxServer::recvComplete(FaxRecvInfo& info, time_t recvTime, const fxStr& emsg)
{
    info.time = recvTime;
    info.npages = npages;
    info.pagewidth = pageWidthCodes[clientParams.wd];
    info.pagelength = pageLengthCodes[clientParams.ln];
    info.sigrate = atoi(Class2Params::bitRateNames[clientParams.br]);
    info.protocol = Class2Params::dataFormatNames[clientParams.df];
    info.resolution = (clientParams.vr == VR_FINE ? 196. : 98.);
    info.sender = recvTSI;
    info.reason = emsg;
}

/*
 * Process a received DCS.
 */
void
FaxServer::recvDCS(const Class2Params& params)
{
    clientParams = params;

    traceStatus(FAXTRACE_PROTOCOL, "REMOTE wants %s",
	Class2Params::bitRateNames[params.br]);
    traceStatus(FAXTRACE_PROTOCOL, "REMOTE wants %s",
	Class2Params::pageWidthNames[params.wd]);
    traceStatus(FAXTRACE_PROTOCOL, "REMOTE wants %s",
	Class2Params::pageLengthNames[params.ln]);
    traceStatus(FAXTRACE_PROTOCOL, "REMOTE wants %s",
	Class2Params::vresNames[params.vr]);
    traceStatus(FAXTRACE_PROTOCOL, "REMOTE wants %s",
	Class2Params::dataFormatNames[params.df]);
}

/*
 * Process a received Non-Standard-Facilities message.
 */
void
FaxServer::recvNSF(u_int)
{
}

/*
 * Check a received TSI against any list of acceptable
 * TSI patterns defined for the server.  This form of
 * access control depends on the sender passing a valid
 * TSI.  With caller-ID, this access control can be made
 * more reliable.
 */
fxBool
FaxServer::recvCheckTSI(const fxStr& tsi)
{
    recvTSI = tsi;
    traceStatus(FAXTRACE_PROTOCOL, "REMOTE TSI \"%s\"", (char*) tsi);
    updateTSIPatterns();
    if (qualifyTSI != "") {	// check against database of acceptable tsi's
	okToRecv = FALSE;	// reject if no patterns!
	if (tsiPats) {
	    for (u_int i = 0; i < tsiPats->length(); i++) {
		RegEx* pat = (*tsiPats)[i];
		if (pat->Search(tsi, tsi.length(), 0, tsi.length()) >= 0) {
		    okToRecv = TRUE;
		    break;
		}
	    }
	}
    } else
	okToRecv = TRUE;
    traceStatus(FAXTRACE_SERVER, "%s TSI \"%s\"",
	okToRecv ? "ACCEPT" : "REJECT", (char*) tsi);
    if (okToRecv)
	setServerStatus("Receiving from \"%s\"", (char*) tsi);
    return (okToRecv);
}

/*
 * Prepare for the reception of page data by setting the
 * TIFF tags to reflect the data characteristics.
 */
void
FaxServer::recvSetupPage(TIFF* tif, long group3opts, int fillOrder)
{
    TIFFSetField(tif, TIFFTAG_SUBFILETYPE,	FILETYPE_PAGE);
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH,
	(u_long) pageWidthCodes[clientParams.wd]);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE,	1);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC,	PHOTOMETRIC_MINISWHITE);
    TIFFSetField(tif, TIFFTAG_ORIENTATION,	ORIENTATION_TOPLEFT);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL,	1);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP,	-1L);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG,	PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_FILLORDER,	fillOrder);
    TIFFSetField(tif, TIFFTAG_XRESOLUTION,	204.);
    TIFFSetField(tif, TIFFTAG_YRESOLUTION,
	(clientParams.vr == VR_FINE) ? 196. : 98.);
    TIFFSetField(tif, TIFFTAG_RESOLUTIONUNIT,	RESUNIT_INCH);
    TIFFSetField(tif, TIFFTAG_SOFTWARE,		"FlexFAX Version 2.2");
    TIFFSetField(tif, TIFFTAG_IMAGEDESCRIPTION,	(char*) recvTSI);
    switch (clientParams.df) {
    case DF_2DMMR:
	TIFFSetField(tif, TIFFTAG_COMPRESSION,	COMPRESSION_CCITTFAX4);
	break;
    case DF_2DMRUNCOMP:
	TIFFSetField(tif, TIFFTAG_COMPRESSION,	COMPRESSION_CCITTFAX3);
	group3opts |= GROUP3OPT_2DENCODING|GROUP3OPT_UNCOMPRESSED;
	TIFFSetField(tif, TIFFTAG_GROUP3OPTIONS,group3opts);
	break;
    case DF_2DMR:
	TIFFSetField(tif, TIFFTAG_COMPRESSION,	COMPRESSION_CCITTFAX3);
	group3opts |= GROUP3OPT_2DENCODING;
	TIFFSetField(tif, TIFFTAG_GROUP3OPTIONS,group3opts);
	break;
    case DF_1DMR:
	TIFFSetField(tif, TIFFTAG_COMPRESSION,	COMPRESSION_CCITTFAX3);
	group3opts &= ~GROUP3OPT_2DENCODING;
	TIFFSetField(tif, TIFFTAG_GROUP3OPTIONS,group3opts);
	break;
    }
    char dateTime[24];
    time_t now = time(0);
    strftime(dateTime, sizeof (dateTime), "%Y:%m:%d %H:%M:%S", localtime(&now));
    TIFFSetField(tif, TIFFTAG_DATETIME,	    dateTime);
    TIFFSetField(tif, TIFFTAG_MAKE,	    (char*) modem->getManufacturer());
    TIFFSetField(tif, TIFFTAG_MODEL,	    (char*) modem->getModel());
    TIFFSetField(tif, TIFFTAG_HOSTCOMPUTER, (char*) hostname);
}

/*
 * Update the TSI pattern array if the file
 * of TSI patterns has been changed since the last
 * time we read it.
 */
void
FaxServer::updateTSIPatterns()
{
    FILE* fd = fopen((char*) qualifyTSI, "r");
    if (fd != NULL) {
	struct stat sb;
	if (fstat(fileno(fd), &sb) >= 0 && sb.st_mtime >= lastPatModTime) {
	    RegExArray* pats = readTSIPatterns(fd);
	    if (tsiPats)
		delete tsiPats;
	    tsiPats = pats;
	    lastPatModTime = sb.st_mtime;
	}
	fclose(fd);
    } else if (tsiPats)
	// file's been removed, delete any existing info
	delete tsiPats, tsiPats = 0;
}

/*
 * Read the file of TSI patterns into an array.
 */
RegExArray*
FaxServer::readTSIPatterns(FILE* fd)
{
    RegExArray* pats = new RegExArray;
    char line[256];

    while (fgets(line, sizeof (line)-1, fd)) {
	char* cp = cp = strchr(line, '#');
	if (cp || (cp = strchr(line, '\n')))
	    *cp = '\0';
	/* trim off trailing white space */
	for (cp = strchr(line, '\0'); cp > line; cp--)
	    if (!isspace(cp[-1]))
		break;
	*cp = '\0';
	if (line[0] != '\0')
	    pats->append(new RegEx(line));
    }
    return (pats);
}
