/*	BSDI $Id: if_slvar.h,v 1.6 1994/01/29 01:15:32 karels Exp $	*/

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
 *	@(#)if_slvar.h	7.7 (Berkeley) 5/7/91
 *
 * $Header: /bsdi/MASTER/BSDI_OS/sys/net/if_slvar.h,v 1.6 1994/01/29 01:15:32 karels Exp $
 */

/*
 * Definitions for SLIP interface data structures
 * 
 * (This exists so programs like slstats can get at the definition
 *  of sl_softc.)
 */
struct sl_softc {
	struct	ifnet sc_if;		/* network-visible interface */
	struct	ifqueue sc_fastq;	/* interactive output queue */
	struct	tty *sc_ttyp;		/* pointer to tty structure */
	u_char	*sc_mp;			/* pointer to next available buf char */
	u_char	*sc_ep;			/* pointer to last available buf char */
	u_char	*sc_buf;		/* input buffer */
	u_int	sc_flags;		/* see below */
	u_int	sc_escape;	/* =1 if last char input was FRAME_ESCAPE */
	u_int	sc_bytessent;
	u_int	sc_bytesrcvd;
	long	sc_lasttime;		/* last time a char arrived */
	long	sc_starttime;		/* last time a char arrived */
	long	sc_abortcount;		/* number of abort esacpe chars */
/* the following is not #ifdef INET so that user access to these data
   structures do not need to #define INET */
	struct	slcompress *sc_comp;	/* tcp compression data */
	caddr_t	sc_bpf;			/* bpf magic cookie */
};

/* visible flags */
#define	SC_COMPRESS	IFF_LINK0	/* compress TCP traffic */
#define	SC_NOICMP	IFF_LINK1	/* supress ICMP traffic */
#define	SC_AUTOCOMP	IFF_LINK2	/* auto-enable TCP compression */

/* internal flags (should be separate) */
#define	SC_ABORT	0x10000		/* have been sent an abort request */

/* this stuff doesn't belong here... */
#define	SLIOCGFLAGS	_IOR('t', 90, int)	/* get configuration flags */
#define	SLIOCSFLAGS	_IOW('t', 89, int)	/* set configuration flags */
#define	SLIOCGUNIT	_IOR('t', 88, int)	/* get slip unit number */

/* nor does this stuff; put it in slip.h later. */

/*
 * definitions of the pseudo- link-level header attached to slip
 * packets grabbed by the packet filter (bpf) traffic monitor.
 */
#define SLIP_HDRLEN 16

#define SLX_DIR 0
#define SLX_CHDR 1
#define CHDR_LEN 15

#define SLIPDIR_IN 0
#define SLIPDIR_OUT 1
