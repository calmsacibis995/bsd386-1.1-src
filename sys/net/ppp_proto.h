/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: ppp_proto.h,v 1.1 1993/02/21 00:06:06 karels Exp $
 */

/*
 * This code is partially derived from CMU PPP.
 *
 * Copyright (c) 1989 Carnegie Mellon University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by Carnegie Mellon University.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*
 * The PPP definitions (RFC 1331)
 */

/* Frame encapsulation stuff */
#define PPP_ADDRESS     0xff    /* The address byte value */
#define PPP_CONTROL     0x03    /* The control byte value */

struct ppp_header {
	u_char  phdr_addr;      /* address byte */
	u_char  phdr_ctl;       /* control byte */
	u_short phdr_type;      /* protocol selector */
};

/* Protocol numbers */
#define PPP_IP          0x0021  /* Raw IP */
#define PPP_OSI         0x0023  /* OSI Network Layer */
#define PPP_NS          0x0025  /* Xerox NS IDP */
#define PPP_DECNET      0x0027  /* DECnet Phase IV */
#define PPP_APPLE       0x0029  /* Appletalk */
#define PPP_IPX         0x002b  /* Novell IPX */
#define PPP_VJC         0x002d  /* Van Jacobson Compressed TCP/IP */
#define PPP_VJNC        0x002f  /* Van Jacobson Uncompressed TCP/IP */
#define PPP_BRPDU       0x0031  /* Bridging PDU */
#define PPP_STII        0x0033  /* Stream Protocol (ST-II) */
#define PPP_VINES       0x0035  /* Banyan Vines */

#define PPP_HELLO       0x0201  /* 802.1d Hello Packets */
#define PPP_LUXCOM      0x0231  /* Luxcom */
#define PPP_SNS         0x0233  /* Sigma Network Systems */

#define PPP_IPCP        0x8021  /* IP Control Protocol */
#define PPP_OSICP       0x8023  /* OSI Network Layer Control Protocol */
#define PPP_NSCP        0x8025  /* Xerox NS IDP Control Protocol */
#define PPP_DECNETCP    0x8027  /* DECnet Control Protocol */
#define PPP_APPLECP     0x8029  /* Appletalk Control Protocol */
#define PPP_IPXCP       0x802b  /* Novell IPX Control Protocol */
#define PPP_STIICP      0x8033  /* Strean Protocol Control Protocol */
#define PPP_VINESCP     0x8035  /* Banyan Vines Control Protocol */

#define PPP_LCP         0xc021  /* Link Control Protocol */
#define PPP_PAP         0xc023  /* Password Authentication Protocol */
#define PPP_LQM         0xc025  /* Link Quality Monitoring */
#define PPP_CHAP        0xc223  /* Challenge Handshake Authentication Protocol */

/*
 * PPP Control Protocols definitions
 */

/* LCP finite state automaton states */
#define CP_INITIAL      0
#define CP_STARTING     1
#define CP_CLOSED       2
#define CP_STOPPED      3
#define CP_CLOSING      4
#define CP_STOPPING     5
#define CP_REQSENT      6
#define CP_ACKRCVD      7
#define CP_ACKSENT      8
#define CP_OPENED       9

struct ppp_cp_hdr {
	u_char  cp_code;        /* packet code */
	u_char  cp_id;          /* packet id */
	u_short cp_length;      /* CP packet length */
};

/* Control Protocols packet codes */
#define CP_CONF_REQ     1       /* Configure-Request */
#define CP_CONF_ACK     2       /* Configure-Ack */
#define CP_CONF_NAK     3       /* Configure-Nak */
#define CP_CONF_REJ     4       /* Configure-Reject */
#define CP_TERM_REQ     5       /* Terminate-Request */
#define CP_TERM_ACK     6       /* Terminate-Ack */
#define CP_CODE_REJ     7       /* Code-Reject */
#define LCP_PROTO_REJ   8       /* Protocol-Reject */
#define LCP_ECHO_REQ    9       /* Echo-Request */
#define LCP_ECHO_REPL   10      /* Echo-Reply */
#define LCP_DISC_REQ    11      /* Discard-Request */

/*
 * CP Options
 */
struct ppp_option {
	u_short po_PAD;         /* padding option structure to align lcp_data */
	u_char  po_type;        /* option type */
	u_char  po_len;         /* option length */
	union {
		u_short s;      /* short */
		u_long  l;      /* long */
		u_char  c[32];  /* chars */
	}       po_data;        /* option data (variable length) */
};

#define LCP_OPT_MAXSIZE 34

/*
 * Line Control Protocol definitions
 */
/* LCP Option defaults */
#define LCP_DFLT_RMU    1500    /* our default packet size */
#define LCP_MIN_RMU     128     /* minimal packet size the remote side should accept */

/* LCP option types */
#define LCP_MRU         1       /* Maximum-Receive-Unit */
#define LCP_ACCM        2       /* Async-Control-Character-Map */
#define LCP_AP          3       /* Authentication-Protocol */
#define LCP_QP          4       /* Quality-Protocol */
#define LCP_MAGIC       5       /* Magic-Number */
#define LCP_PFC         7       /* Protocol-Field-Compression */
#define LCP_ACFC        8       /* Address-and-Control-Field-Compression */

/*
 * PPP Network Control Protocol for IP (IPCP) definitions (RFC 1332)
 */

/* IPCP option types */
#define IPCP_ADDRS      1       /* IP-Addresses */
#define IPCP_CPROT      2       /* IP-Compression-Protocol */
#define IPCP_ADDR       3       /* IP-Address */
