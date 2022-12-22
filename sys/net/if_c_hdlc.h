/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: if_c_hdlc.h,v 1.1 1993/02/21 00:04:24 karels Exp $
 */

/*
 * Definitions for the cisco HDLC serial line encapsulation
 */

/*
 * Cisco packet header
 */
struct cisco_hdr {
	u_char  csh_addr;       /* link address */
	u_char  csh_ctl;        /* control -- always zero */
	u_short csh_type;       /* packet type */
};

/*
 * Link addresses
 */
#define CISCO_ADDR_UNICAST      0x0f
#define CISCO_ADDR_BCAST        0x8f

/*
 * Packet type values
 */
#define CISCO_TYPE_INET         0x0800          /* IP */
#define CISCO_TYPE_SLARP        0x8035          /* SLARP control packet */


/*
 * Cisco SLARP control packet structure
 */
struct cisco_slarp {
	long    csl_code;               /* code */
	union {
	    struct {
	    	u_long	addr;		/* IP addr */
	    	u_long	mask;		/* IP addr mask */
	    	u_short	unused[3];
	    } addr;
	    struct {
		long	myseq;		/* my sequence number */
		long	yourseq;	/* your sequence number */
		short	rel;		/* reliability */
		short	t1;		/* time alive (1) */
		short 	t0;		/* time alive (0) */
	    } keep;
	} un;
};

#define	SLARP_SIZE	18	/* sizeof rounds up */

#define	csl_addr	un.addr.addr
#define	csl_mask	un.addr.mask
#define	csl_myseq	un.keep.myseq
#define	csl_yourseq	un.keep.yourseq
#define	csl_rel		un.keep.rel
#define	csl_t1		un.keep.t1
#define	csl_t0		un.keep.t0

/*
 * SLARP control packet codes
 */
#define SLARP_REQUEST		0
#define SLARP_REPLY		1
#define SLARP_KEEPALIVE		2
