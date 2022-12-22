/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/FaxTrace.h,v 1.1.1.1 1994/01/14 23:09:44 torek Exp $
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
#ifndef _FaxTrace_
#define	_FaxTrace_
/*
 * Fax Server Tracing Definitions.
 */
const int FAXTRACE_SERVER	= 0x0001;	// server operation
const int FAXTRACE_PROTOCOL	= 0x0002;	// fax protocol
const int FAXTRACE_MODEMOPS	= 0x0004;	// modem operations
const int FAXTRACE_MODEMCOM	= 0x0008;	// modem communication
const int FAXTRACE_TIMEOUTS	= 0x0010;	// all timeouts
const int FAXTRACE_MODEMCAP	= 0x0020;	// modem capabilities
const int FAXTRACE_HDLC		= 0x0040;	// HDLC protocol frames
const int FAXTRACE_MODEMIO	= 0x0080;	// binary modem i/o
const int FAXTRACE_STATETRANS	= 0x0100;	// server state transitions
const int FAXTRACE_QUEUEMGMT	= 0x0200;	// job queue management
const int FAXTRACE_ANY		= 0xffffffff;

const int FAXTRACE_MASK		= 0xffff;
#endif /* _FaxTrace_ */
