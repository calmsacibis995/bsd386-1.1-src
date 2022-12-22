/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxstat/RecvStatus.c++,v 1.1.1.1 1994/01/14 23:09:57 torek Exp $
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
#include "RecvStatus.h"
#include "PageSize.h"

FaxRecvStatus::FaxRecvStatus() {}
FaxRecvStatus::~FaxRecvStatus() {}

fxBool
FaxRecvStatus::scan(const char* buf)
{
    return sscanf(buf, "%d:%u:%u:%f:%u",
	&beingReceived, &pageWidth, &pageLength, &resolution, &npages) == 5;
}

int
FaxRecvStatus::compare(const FaxRecvStatus* other) const
{
    int c = -date.compare(&other->date);
    if (c == 0)
	c = -time.compare(&other->time);
    if (c == 0)
	c = sender.compare(&other->sender);
    return c == 0 ? (npages - other->npages) : c;
}

fxBool
FaxRecvStatus::parse(const char* tag)
{
    if (scan(tag)) {
	for (const char* cp = strchr(tag, '\0'); cp > tag && *cp != ':'; cp--)
	    ;
	sender.append(cp+1, 16);
	cp = cp-20+1;
	if (cp > tag) {
	    date.append(cp, 10);
	    time.append(cp+11, 8);
	} else {
	    date = "????:??:??";
	    time = "??:??:??";
	}
	return (TRUE);
    } else
	return (FALSE);
}

void
FaxRecvStatus::printHeader(FILE* fp, fxBool showHost)
{
    fprintf(fp, "%-14s %-16s %10s %-7s %s\n",
	"Sender", "Received At", "Pages", "Quality",
	(showHost ? "(Host)" : ""));
}

void
FaxRecvStatus::print(FILE* fp, fxBool showHost)
{
    float w = (pageWidth / 204.) * 25.4;
    float h = pageLength / (resolution < 100 ? 3.85 : 7.7);
    PageSizeInfo* info = PageSizeInfo::getPageSizeBySize(w, h);
    fprintf(fp, "%-14.14s %5.5s %10.10s %3d %6.6s %-7.7s %s\n"
	, (char*) sender
	, (!beingReceived ? (char*) time : "")
	, (!beingReceived ? (char*) date : "")
	, npages
	, info ? info->abbrev() : ""
	, (resolution > 150 ? "fine" : "normal")
	, (showHost ? (char*) host : "")
    );
}

fxIMPLEMENT_ObjArray(FaxRecvStatusArray, FaxRecvStatus);
