/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: if_nereg.h,v 1.4 1993/12/19 04:24:44 karels Exp $
 */

/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
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
 *
 *	@(#)if_nereg.h	7.1 (Berkeley) 5/9/91
 */

/*
 * NE1000 & NE2000 Ethernet Card registers
 * The NE cards use a DS8390 Ethernet controller
 * at the beginning of the I/O space.
 */

#define NE_NPORT	32

#include "ic/ds8390.h"

#define ne_data		0x10	/* Data Transfer port */
#define ne_reset	0x1f	/* Card Reset port */

#define NE_PKTSZ	1536

/* Memory layout on NE-2000 */
#define	NE2000_TBUF	(16 * 1024)		/* Start of Transmit Buffer */
#define	NE2000_RBUF	(NE2000_TBUF+NE_PKTSZ)	/* Start of Receive Buffer */
#define	NE2000_RBUFEND	(32 * 1024)		/* End of Transmit Buffer */

/* Memory layout on NE-1000 */
#define NE1000_TBUF	(8 * 1024)		/* Start of Transmit Buffer */
#define NE1000_RBUF	(NE1000_TBUF+NE_PKTSZ)	/* Start of Receive Buffer */
#define NE1000_RBUFEND	(16 * 1024)		/* End of Transmit Buffer */

/*
 * Is the i/o base valid?
 * (NE1000 and NE2000 support the same range of base addresses)
 */
#define NE_IOBASEVALID(a)	(((a) & 0x1f) == 0x00)
