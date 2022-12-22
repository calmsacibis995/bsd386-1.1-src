/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxstat/SendStatus.c++,v 1.1.1.1 1994/01/14 23:09:57 torek Exp $
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
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "SendStatus.h"

FaxSendStatus::FaxSendStatus()
{
   isLocked = FALSE;
   isSendAt = FALSE;
}
FaxSendStatus::~FaxSendStatus() {}

int
FaxSendStatus::compare(const FaxSendStatus* other) const
{
    int c = isLocked - other->isLocked;
    if (c == 0)
	c = dts.compare(&other->dts);
    if (c == 0)
	c = tts.compare(&other->tts);
    if (c == 0)
	c = sender.compare(&other->sender);
    if (c == 0)
	c = number.compare(&other->number);
    return c;
}

#define nextTag(s, what)					\
	cp = tag; tag = strchr(cp, ':'); if (!tag) { what; }	\
	s.append(cp, tag-cp);					\
	for (tag++; isspace(*tag); tag++);
fxBool
FaxSendStatus::parse(const char* tag)
{
    const char* cp;

    nextTag(jobname, return (FALSE));
    nextTag(sender, return (FALSE));
    nextTag(tts, return (FALSE));
    nextTag(number, return (FALSE));
    nextTag(modem, return (FALSE));
    if (!isSendAt) {
	status = tag;
	isLocked = (tts == "locked");
	if (*tag == '\0')
	    status = isLocked ? "Being processed" : "Queued and waiting";
	if (!isLocked && tts != "asap") {
	    dts = tts.cut(0, 10);
	    /*
	     * The server sends ':''s as '.'s to simplify parsing.
	     * Now that we're done parsing, change them back.
	     */
	    tts.remove(0);		// leading blank
	    tts[2] = ':';
	    tts[5] = ':';
	}
    } else
	status = "submitted to at and waiting";
    return (TRUE);
}
#undef nextTag

void
FaxSendStatus::printHeader(FILE* fp, int, fxBool showHost)
{
    fprintf(fp, "%-4s %-5s %-15s %-16s %-14s",
	"Job", "Modem", "Destination", "Time-To-Send", "Sender");
    if (showHost)
	fprintf(fp, " %-10s", "Host");
    fprintf(fp, " Status\n");
}

void
FaxSendStatus::print(FILE* fp, int ncols, fxBool showHost)
{
    fprintf(fp, "%-4s %-5s %-15.15s"
	, (char*) jobname
	, (char*) modem
	, (char*) number
    );
    if (!isSendAt) {
	fprintf(fp, " %5.5s %10.10s"
	    , (isLocked || tts == "asap" ? "" : (char*) tts)
	    , (isLocked || tts == "asap" ? "" : (char*) dts)
	);
    } else
	fprintf(fp, " %-16.16s" , (char*) tts);
    fprintf(fp, " %-14.14s", (char*) sender);
    ncols -= 59;
    if (showHost) {
	fprintf(fp, " %-10.10s", (char*) host);
	ncols -= 11;
    }
    if (ncols < 10)
	ncols = 10;
    if (status.length() > ncols-1)
	fprintf(fp, " %.*s...\n", ncols-4, (char*) status);
    else
	fprintf(fp, " %s\n", (char*) status);
}

FaxSendAtStatus::FaxSendAtStatus()
{
   isLocked = FALSE;
   isSendAt = TRUE;
}
FaxSendAtStatus::~FaxSendAtStatus() {}

fxIMPLEMENT_ObjArray(FaxSendStatusArray, FaxSendStatus);
