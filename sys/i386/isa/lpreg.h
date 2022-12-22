/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI:   $Id: lpreg.h,v 1.2 1993/02/21 01:50:32 karels Exp $
 */
 
/*-
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Erik Forsberg.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 *  PC-AT Parallel Printer Port driver.
 *  Written April 8th, 1992 for the BSDI/386 system.
 *  Please send bugs and enhancements to erik@eab.retix.com
 *  Copyright 1992, Erik Forsberg.
 */

#define LP_NPORTS	4

#define LPDATA		0	/* Offset for data latch */
#define LPSTATUS	1	/* Offset for Printer Status */
#define LPCTL		2	/* Offset for Printer Control */

/*
 * Printer Control Port
 */
#define IE	0x10	/* IRQ enable */
#define SLCT	0x08	/* printer SELECT */
#define NOTINIT	0x04	/* printer /INIT, active LOW */
#define AUTO	0x02	/* auto line feed */
#define STROBE	0x01	/* printer strobe */

/*
 * Printer Status Port
 */
#define NOTBUSY	0x80	/* printer /BUSY */
#define NOTACK	0x40	/* printer /ACK, active LOW */
#define PAPER	0x20	/* out of paper */
#define SLCTED	0x10	/* printer SELECTed */
#define NOERROR	0x08	/* printer /ERROR, active low */

/*
 * The following should be elsewhere...
 * Parameters for lphook.
 */
#define	LPHOOK_PROBE	1
#define	LPHOOK_PORT	2
#define	LPHOOK_ATTACH	3
#define	LPHOOK_DETACH	4

#ifdef KERNEL
int lphook __P((int unit, int op, void *softc, int (*intr)()));
#endif
