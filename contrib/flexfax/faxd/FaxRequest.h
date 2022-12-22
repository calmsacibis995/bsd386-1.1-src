/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/FaxRequest.h,v 1.1.1.1 1994/01/14 23:09:44 torek Exp $
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
#ifndef _FaxRequest_
#define	_FaxRequest_
/*
 * Fax Send/Poll Request Structure.
 */
#include "StrArray.h"
#include <time.h>
#include <stdio.h>

#include <InterViews/regexp.h>

enum FaxSendOp {
    send_tiff,			// send tiff file
    send_tiff_saved,		// saved tiff file (converted)
    send_postscript,		// send postscript file
    send_postscript_saved,	// saved postscript file (convert to tiff)
    send_poll,			// make poll request
};
fxDECLARE_PrimArray(FaxSendOpArray, FaxSendOp);

enum FaxSendStatus {
    send_retry,			// waiting for retry
    send_failed,		// finished w/o success
    send_done,			// completed successfully
};

/*
 * This structure is passed from the queue manager (faxServerApp)
 * to the fax modem+protocol service (FaxServer) for each send/poll
 * operation to be done.  This class also supports the read and
 * writing of this information to an external file.
 */
struct FaxRequest {
    enum FaxNotify {		// notification control
	no_notice,		// no notifications
	when_done,		// notify when send completed
	when_requeued		// notify if job requeued
    };

    fxStr	qfile;		// associated queue file name
    fxStr	jobid;		// job identifier
    FILE*	fp;		// open+locked queue file
    FaxSendStatus status;	// request status indicator
    u_short	dirnum;		// directory of next page to send
    u_short	npages;		// total pages sent/received
    u_short	ntries;		// # tries to send current page
    u_short	ndials;		// # consecutive failed tries to call dest
    u_short	totdials;	// total # calls to dest
    u_short	maxdials;	// max # calls to make
    u_short	pagewidth;	// desired output page width (mm)
    float	pagelength;	// desired output page length (mm)
    float	resolution;	// desired vertical resolution (lpi)
    time_t	tts;		// time to send
    time_t	killtime;	// time to kill job
    fxStr	sender;		// sender's name
    fxStr	mailaddr;	// return mail address
    fxStr	jobtag;		// user-specified job tag
    fxStr	number;		// dialstring for fax machine
    fxStr	external;	// displayable phone number for fax machine
    FaxSendOpArray ops;		// send-related ops to do
    fxStrArray	files;		// associated files to transmit or polling id's
    FaxNotify	notify;		// email notification indicator
    fxStr	notice;		// message to send for notification
    fxStr	modem;		// outgoing modem to use
    fxStr	pagehandling;	// page analysis information

    fxBool checkFile(const char* file);

    static Regexp jobidPat;

    FaxRequest(const fxStr& qf);
    ~FaxRequest();
    fxBool readQFile(int fd);
    void writeQFile();
};
#endif /* _FaxRequest_ */
