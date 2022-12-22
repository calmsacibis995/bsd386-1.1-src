/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/HDLCFrame.c++,v 1.1.1.1 1994/01/14 23:09:48 torek Exp $
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
 * Raw HDLC Frame Interface.
 */
#include "HDLCFrame.h"
#include <string.h>
#include <stdlib.h>

HDLCFrame::HDLCFrame(u_int fo)
{
    frameOverhead = fo;
    base = next = buf;
    end = &buf[sizeof(buf)];
    amountToGrowBy = 1024;
    ok = FALSE;
}

HDLCFrame::~HDLCFrame()
{
    if (base != buf) delete base;
}

HDLCFrame::HDLCFrame(const HDLCFrame& other)
{
    u_int size = other.end - other.base;
    u_int len = other.getLength();
    if (size > sizeof(buf)) {
	base = (u_char*) malloc(size);
    } else {
	base = &buf[0];
    }
    end = base + size;
    next = base + len;
    memcpy(base, other.base, len);
    ok = other.ok;
    frameOverhead = other.frameOverhead;
}

void
HDLCFrame::addc(u_char c)
{
    if (next >= end)
	grow(amountToGrowBy);
    *next++ = c;
}

void
HDLCFrame::grow(u_int amount)
{
    // insure an acceptable amount of growth
    if (amount < amountToGrowBy) amount = amountToGrowBy;

    // move data into larger piece of memory
    u_int size = end - base;
    u_int len = getLength();
    u_int newSize = size + amount;
    if (base == buf) {
	base = (u_char*) malloc(newSize);
	memcpy(base, buf, sizeof(buf));
    } else {
	base = (u_char*) realloc(base, newSize);
    }

    // update position pointers
    end = base + newSize;
    next = base + len;
}

void
HDLCFrame::put(const u_char* c, u_int len)
{
    u_int remainingSpace = end - next;
    if  (len > remainingSpace)
	grow(len - remainingSpace);
    memcpy(next, c, len);
    next += len;
}

u_int
HDLCFrame::getDataWord() const
{
    u_int n = getFrameDataLength();
    u_int w = (n > 1) ? (*this)[3] : 0;
    if (n > 2) w = (w<<8)|(*this)[4];
    if (n > 3) w = (w<<8)|(*this)[5];
    if (n > 4) w = (w<<8)|(*this)[6];
    return w;
}

u_int
HDLCFrame::getDIS() const
{
    u_int n = getFrameDataLength();
    u_int dis = (n > 1) ? (*this)[3] : 0;
    dis <<= 8; if (n > 2) dis |= (*this)[4];
    dis <<= 8; if (n > 3) dis |= (*this)[5];
    return dis;
}

u_int
HDLCFrame::getXINFO() const
{
    return (getFrameDataLength() > 4 && ((*this)[5] & 0x1)) ? (*this)[6] : 0;
}
