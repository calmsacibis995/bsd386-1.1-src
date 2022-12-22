/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: if_eoreg.h,v 1.1 1992/08/22 03:43:55 karels Exp $
 */

/*
 * 3Corp. EtherLink (3C501) Ethernet Adapter definitions.
 */

#define EO_NPORTS	16	/* Number of i/o ports */

#define EO_BUFSIZE	2048	/* Size of the buffer memory */

/*
 * I/O Ports
 */
#define	EO_ADDR(i)	(i)	/* Station Address byte i */
#define EO_RSR		0x6	/* Receive Status Register (R) */
#define EO_RCR		0x6	/* Receive Command Register (W) */
#define EO_TSR		0x7	/* Transmit Status Register (R) */
#define EO_TCR		0x7	/* Transmit Command Register (W) */
#define	EO_GPBPL	0x8	/* GP Buffer Pointer (low byte) */
#define EO_GPBPH	0x9	/* GP Buffer Pointer (high byte) */
#define EO_RCVBPL	0xa	/* Receive Buffer Pointer (low) (R) */
#define EO_RBPCLR	0xa	/* Receive Buffer Pointer Clear (W) */
#define EO_RCVBPH	0xb	/* Receive Buffer Pointer (high) (R) */
#define EO_EAPW		0xc	/* Ethernet Address PROM Window */
#define EO_ASR		0xe	/* Auxilary Status Register (R) */
#define EO_ACR		0xe	/* Auxilary Command Register (W) */
#define EO_BUF		0xf	/* Data Buffer */

/*
 * Receive Status Register bits
 */
#define EORS_OVFLO	0x01	/* Overflow Error */
#define EORS_FCS	0x02	/* FCS Error */
#define EORS_DRIBBLE	0x04	/* Dribble Error */
#define EORS_SHORT	0x08	/* Short Frame */
#define EORS_NOVFLO	0x10	/* Received packet w/o Overflow error */
#define EORS_GOOD	0x20	/* Received good packet */
#define EORS_STALE	0x80	/* Stale Reveive Status */

/*
 * Receive Command Register bits
 */
#define EORC_DOV	0x01	/* Detect Overflow */
#define EORC_DFCS	0x02	/* Detect FCS Errors */
#define EORC_DDR	0x04	/* Detect Dribble Errors */
#define EORC_DSF	0x08	/* Detect Short Frames */
#define EORC_DFNOV	0x10	/* Detect Frames w/o Overflow Error */
#define EORC_AGF	0x20	/* Accept Good Frames */
#define EORC_AMM	0xc0	/* Address Match Mode */
#define  EORC_RDIS	 0x00	 /* Receiver Disabled */
#define  EORC_PROMISC	 0x40	 /* Promiscuous Mode */
#define  EORC_AB	 0x80	 /* Accept Address and Broadcast */
#define  EORC_AM	 0xc0	 /* Accept Address and Multicasts */

/*
 * Transmit Status Register bits
 */
#define EOTS_UFLOW	0x01	/* Underflow */
#define EOTS_COLL	0x02	/* Collision */
#define EOTS_C16	0x04	/* Collision 16 */
#define EOTS_RNF	0x08	/* Ready for New Frame */

/*
 * Transmit Command Register
 */
#define EOTC_DUFLO	0x01	/* Detect Underflow */
#define EOTC_DCOLL	0x02	/* Detect Collision */
#define EOTC_DC16	0x04	/* Detect Collision 16 */
#define EOTC_DST	0x08	/* Detect Successful Transmission */

/*
 * Auxilary Status Register
 */
#define EOAS_RXBUSY	0x01	/* Receive Busy */
	/* bits 1-3 are the same as in EOAC_TXBADFCS, EOAC_PBC */
#define EOAS_DMADONE	0x10	/* DMA Done */
	/* bits 5-6 is the same as EOAC_DMAREQ, EOAC_RIDE */
#define EOAS_TXBUSY	0x80	/* Transmit Busy */

/*
 * Auxilary Command Register
 */
#define EOAC_IRE	0x01	/* Interrupt Request Enable */
#define EOAC_TXBADFCS	0x02	/* Transmit packets with bad FCS */
#define EOAC_PCB	0x0c	/* Packet Buffer Control */
#define  EOAC_HOST	 0x00	 /* System Bus has access to the buffer */
#define  EOAC_TFR	 0x04	 /* Transmit followed by receive */
#define  EOAC_RCV	 0x08	 /* Receive */
#define  EOAC_LOOP	 0x0c	 /* Loopback */
#define EOAC_DMAREQ	0x20	/* DMA Request */
#define EOAC_RIDE	0x40	/* Interrupt and DMA Requests Enabled */
#define EOAC_RESET	0x80	/* Reset */

/*
 * Checks for valid IRQs, DRQs and base addresses
 */
#define EO_IOBASEVALID(i)	(((i) & ~0x3f0) == 0)
#define EO_IRQVALID(i)		(0x2fc & (1<<(i)))	/* 2-7 & 9 */
#define EO_DRQVALID(i)		(1 <= (i) && (i) <= 3)
