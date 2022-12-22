/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/everex.h,v 1.1.1.1 1994/01/14 23:09:45 torek Exp $
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
#ifndef _everex_
#define	_everex_
/*
 * Everex FAX Modem Definitions.
 */
#define	EVEREX_CMDBRATE	BR2400		// cmd interface is fixed at 2400 baud

// fax configuration (#S2 register)
#define	S2_HOSTCTL	0x00		// host control
#define	S2_MODEMCTL	0x01		// modem control
#define	S2_DEFMSGSYS	0x00		// default message system
#define	S2_ALTMSGSYS	0x04		// alternate message system
#define	S2_FLOWATBUF	0x00		// xon/xoff at buffer transitions
#define	S2_FLOW1SEC	0x08		// xon/xoff once a second
#define	S2_19200	0x00		// 19.2Kb T4 host-modem link speed
#define	S2_9600		0x10		// 9.6Kb T4 host-modem link speed
#define	S2_PADEOLS	0x00		// byte align and zero stuff eols
#define	S2_RAWDATA	0x20		// pass data unaltered
#define	S2_RESET	0x80		// reset detected by host

// high speed carrier select values (#S4 register)
#define	S4_GROUP2	0		// Group 2
#define	S4_2400V27	1		// V.27 2400
#define	S4_4800V27	2		// V.27 4800
#define	S4_4800V29	3		// V.29 4800
#define	S4_7200V29	4		// V.29 7200
#define	S4_9600V29	5		// V.29 9600

#endif /* _everex_ */
