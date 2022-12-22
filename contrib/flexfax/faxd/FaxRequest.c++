/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/FaxRequest.c++,v 1.1.1.1 1994/01/14 23:09:48 torek Exp $
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
#include <ctype.h>
#include <unistd.h>
#include <syslog.h>
#include <osfcn.h>

#include "FaxRequest.h"
#include "config.h"

Regexp FaxRequest::jobidPat(FAX_QFILEPREF);

FaxRequest::FaxRequest(const fxStr& qf) : qfile(qf)
{
    tts = 0;
    killtime = 0;
    (void) jobidPat.Match(qf, qf.length(), 0);
    int qfileTail = jobidPat.EndOfMatch();
    jobid = qf.extract(qfileTail, qf.length()-qfileTail);
    fp = NULL;
    status = send_retry;
    npages = 0;
    dirnum = 0;
    ntries = 0;
    ndials = 0;
    totdials = 0;
    maxdials = (u_short) FAX_RETRIES;
    notify = no_notice;
}

FaxRequest::~FaxRequest()
{
    if (fp)
	fclose(fp);
}

fxBool
FaxRequest::checkFile(const char* file)
{
    // XXX scan full pathname to avoid security holes (e.g. foo/../../..)
    if (file[0] == '.' || file[0] == '/') {
	syslog(LOG_ERR, "%s: Invalid data file \"%s\"; not in same directory",
	    (char*) qfile, file);
	return FALSE;
    }
    int fd = open(file, 0);
    if (fd == -1) {
	syslog(LOG_ERR, "%s: Can not access data file \"%s\"",
	    (char*) qfile, file);
	return FALSE;
    }
    close(fd);
    return TRUE;
}

#define	isCmd(cmd)	(strcasecmp(line, cmd) == 0)

// Return True for success, False for failure
fxBool
FaxRequest::readQFile(int fd)
{
    if (fd > -1)
	fp = fdopen(fd, "r+w");
    else
	fp = fopen((char*) qfile, "r+w");
    if (fp == NULL) {
	syslog(LOG_ERR, "%s: open: %m", (char*) qfile);
	return FALSE;
    }
    char line[1024];
    char* cp;
    while (fgets(line, sizeof (line) - 1, fp)) {
	if (line[0] == '#')
	    continue;
	if (cp = strchr(line, '\n'))
	    *cp = '\0';
	char* tag = strchr(line, ':');
	if (!tag) {
	    syslog(LOG_ERR, "%s: Malformed line \"%s\"", (char*) qfile, line);
	    return FALSE;
	}
	*tag++ = '\0';
	while (isspace(*tag))
	    tag++;
	if (isCmd("tts")) {		// time to send
	    tts = atoi(tag);
	    if (tts == 0)
		tts = time(0);		// distinguish "now" from unset
	} else if (isCmd("killtime")) {	// time to kill job
	    killtime = atoi(tag);
	} else if (isCmd("number")) {	// dialstring
	    number = tag;
	} else if (isCmd("external")) {	// displayable phone number
	    external = tag;
	} else if (isCmd("sender")) {	// sender's name
	    sender = tag;
	} else if (isCmd("mailaddr")) {	// return mail address
	    mailaddr = tag;
	} else if (isCmd("jobtag")) {	// user-specified job tag
	    jobtag = tag;
	} else if (isCmd("status")) {	// reason job is queued
	    notice = tag;
	} else if (isCmd("resolution")) {// vertical resolution in dpi
	    resolution = atof(tag);
	} else if (isCmd("npages")) {	// number of pages
	    npages = atoi(tag);
	} else if (isCmd("dirnum")) {	// directory of next page to send
	    dirnum = atoi(tag);
	} else if (isCmd("ntries")) {	// # attempts to send current page
	    ntries = atoi(tag);
	} else if (isCmd("ndials")) {	// # consecutive failing calls
	    ndials = atoi(tag);
	} else if (isCmd("totdials")) {	// total # calls
	    totdials = atoi(tag);
	} else if (isCmd("maxdials")) {	// max total # calls
	    maxdials = atoi(tag);
	} else if (isCmd("pagewidth")) {// page width in pixels
	    pagewidth = atoi(tag);
	} else if (isCmd("pagelength")) {// page length in mm
	    pagelength = atof(tag);
	} else if (isCmd("pagehandling")) {// page analysis info
	    pagehandling = tag;
	} else if (isCmd("modem")) {	// outgoing modem to use
	    modem = tag;
	} else if (isCmd("notify")) {	// email notification
	    if (strcmp(tag, "when done") == 0) {
		notify = when_done;
	    } else if (strcmp(tag, "when requeued") == 0) {
		notify = when_requeued;
	    } else if (strcmp(tag, "none") == 0) {
		notify = no_notice;
	    } else {
		syslog(LOG_ERR, "job %s: Invalid notify value \"%s\"\n",
		    (char*) qfile, tag);
	    }
	} else if (isCmd("postscript")) {	// postscript document
	    if (!checkFile(tag))
		return FALSE;
	    files.append(tag);
	    ops.append(FaxSendOp(send_postscript));
	} else if (isCmd("!postscript")) {	// saved postscript document
	    if (!checkFile(tag))
		return FALSE;
	    files.append(tag);
	    ops.append(FaxSendOp(send_postscript_saved));
	} else if (isCmd("tiff")) {		// tiff document
	    if (!checkFile(tag))
		return FALSE;
	    files.append(tag);
	    ops.append(FaxSendOp(send_tiff));
	} else if (isCmd("!tiff")) {		// saved tiff document
	    if (!checkFile(tag))
		return FALSE;
	    files.append(tag);
	    ops.append(FaxSendOp(send_tiff_saved));
	} else if (isCmd("poll")) {		// polling request
	    files.append(tag);
	    ops.append(FaxSendOp(send_poll));
	} else
	    syslog(LOG_INFO, "%s: ignoring unknown command line \"%s: %s\"",
		(char*) qfile, line, tag);
    }
    if (tts == 0) {
	syslog(LOG_INFO, "%s: not ready to send yet", (char*) qfile);
	return FALSE;
    }
    if (number == "" || sender == "" || mailaddr == "") {
	syslog(LOG_ERR, "%s: Malformed job description, "
	    "missing number|sender|mailaddr", (char*) qfile);
	return FALSE;
    }
    if (killtime == 0) {
	syslog(LOG_ERR, "%s: No kill time, or time is zero", (char*) qfile);
	return (FALSE);
    }
    if (modem == "")
	modem = MODEM_ANY;
    if (files.length() > 0)
	return TRUE;
    syslog(LOG_ERR, "%s: No files to send (number \"%s\")",
	(char*) qfile, (char*) number);
    return FALSE;
}

void
FaxRequest::writeQFile()
{
    rewind(fp);
    fprintf(fp, "modem:%s\n", (char*) modem);
    fprintf(fp, "tts:%u\n", tts);
    fprintf(fp, "killtime:%u\n", killtime);
    fprintf(fp, "number:%s\n", (char*) number);
    fprintf(fp, "external:%s\n", (char*) external);
    fprintf(fp, "sender:%s\n", (char*) sender);
    fprintf(fp, "resolution:%g\n", resolution);
    fprintf(fp, "npages:%d\n", npages);
    fprintf(fp, "dirnum:%d\n", dirnum);
    fprintf(fp, "ntries:%d\n", ntries);
    fprintf(fp, "ndials:%d\n", ndials);
    fprintf(fp, "totdials:%d\n", totdials);
    fprintf(fp, "maxdials:%d\n", maxdials);
    fprintf(fp, "pagewidth:%d\n", pagewidth);
    fprintf(fp, "pagelength:%g\n", pagelength);
    fprintf(fp, "pagehandling:%s\n", (char*) pagehandling);
    fprintf(fp, "status:%s\n", (char*) notice);
    fprintf(fp, "mailaddr:%s\n", (char*) mailaddr);
    fprintf(fp, "jobtag:%s\n", (char*) jobtag);
    if (notify == when_requeued)
	fprintf(fp, "notify:when requeued\n");
    else if (notify == when_done)
	fprintf(fp, "notify:when done\n");
    static const char* opNames[3] = { "postscript", "tiff", "poll" };
    for (u_int i = 0, n = files.length(); i < n; i++) {
	const char* op = "";
	switch (ops[i]) {
	case send_tiff:			op = "tiff"; break;
	case send_tiff_saved:		op = "!tiff"; break;
	case send_postscript:		op = "postscript"; break;
	case send_postscript_saved:	op = "!postscript"; break;
	case send_poll:			op = "poll"; break;
	}
	fprintf(fp, "%s:%s\n", op, (char*) files[i]);
    }
    fflush(fp);
    if (ftruncate(fileno(fp), ftell(fp)) < 0)
	syslog(LOG_ERR, "%s: truncate failed: %m", (char*) qfile);
    // XXX probably should fsync, but not especially portable
}

fxIMPLEMENT_PrimArray(FaxSendOpArray, FaxSendOp);
