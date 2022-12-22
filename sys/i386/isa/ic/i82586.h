/*
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: i82586.h,v 1.3 1993/12/19 04:50:01 karels Exp $
 */


/*
 * Intel 82586 Ethernet LAN coprocessor definitons
 */

/* System Control block Pointer */
#define IEL_SCPOFF	0xfff6
struct iel_scp { 
	char	bustype;	/* Bus type (0 for 16bit, 1 for 8bit) */
	char	unused[5];
	u_short iscp_off;	/* ISCP offset */
	u_short iscp_base;
};
	
/* Intermediate System Control block Pointer */
struct iel_iscp {
	u_short	busy;		/* 82586 is doing intialization */
	u_short scboff;		/* SCB offset */
	u_short base1;
	u_short base2;
};

/* System Control Block */
struct iel_scb {
	u_short	status;		/* Status Field */
	u_short cmd;		/* Command Field */
	u_short cmdlist;	/* Commands List Pointer */
	u_short rfa;		/* Receive Frame Area Pointer */
	u_short crcerrs;	/* Number of CRC errors */
	u_short alignerrs;	/* Number of alignment errors */
	u_short ndrops;		/* Number of dropped packets */
	u_short overruns;	/* Number of overruns */
};

/* SCB Status bits */
#define IEL_RUS		0x70	/* Receive Unit status */
# define IEL_RU_IDLE	 0x0	/* RU is idle */
# define IEL_RU_SUSP	 0x10	/* RU is suspended */
# define IEL_RU_NRES	 0x20	/* RU lacks resources */
# define IEL_RU_RDY	 0x40	/* RU is ready */
#define IEL_CUS		0x700	/* Command Unit status */
# define IEL_CU_IDLE	 0x0	/* CU is idle */
# define IEL_CU_SUSP	 0x100	/* CU is suspended */
# define IEL_CU_ACT 	 0x200	/* CU is active */
#define IEL_RNR		0x1000	/* Receiver Not Ready */
#define IEL_CNA		0x2000	/* CU left the active state */
#define IEL_FR		0x4000	/* Frame has been received */
#define	IEL_CX		0x8000	/* Command with I bit (interrupt) executed */

/* SCB Command bits */
#define IEL_RUC		0x70	/* Receiver Unit Commands */
# define IEL_RUC_NOP	 0x0	/* NOP */
# define IEL_RUC_START	 0x10	/* Start receiption of frames */
# define IEL_RUC_RES	 0x20	/* Resume frame receiption */
# define IEL_RUC_SUSP	 0x30	/* Suspend frame receiption */
# define IEL_RUC_ABORT	 0x40	/* Abort receiption immediately */
#define IEL_RESET 	0x80	/* Reset the chip */
#define IEL_CUC		0x700	/* Command Unit Commands */
# define IEL_CUC_NOP	 0x0	/* NOP */
# define IEL_CUC_START	 0x100	/* Start execution commands */
# define IEL_CUC_RES	 0x200	/* Resume execution of commands */
# define IEL_CUC_SUSP	 0x300	/* Suspend execution of commands */
# define IEL_CUC_ABORT	 0x400	/* Abort execution of commands */
#define IEL_ACK_RNR	0x1000	/* RNR status acknowledgement */
#define IEL_ACK_CNA	0x2000	/* CNA status acknowledgement */
#define IEL_ACK_FR	0x4000	/* FR status acknowledgement */
#define IEL_ACK_CX	0x8000	/* CX status acknowledgement */

#define IEL_ACK (IEL_ACK_RNR|IEL_ACK_CNA|IEL_ACK_FR|IEL_ACK_CX)

/*
 * i82586 commands/received frames 
 */

/* Status word bits */
#define IEL_NCOL 0xf		/* Number of collisions on transmission */
#define IEL_S5  0x20		/* Too many collisions on transmitting */
#define IEL_S6	0x40		/* Heart Beat -- something was on CD pin during TX  */
				/* No EOF detected during receiving */
#define IEL_S7	0x80		/* Transmission had to defer to traffic */
				/* Received frame is too short */
#define IEL_S8  0x100		/* DMA underrun on TX / overrun on RX */
#define IEL_S9	0x200		/* Lost clear-to-send signal during TX */
				/* RU ran out of resources on receiving */
#define IEL_S10	0x400		/* No carrier sense during transmission */
				/* Alignment error on receiving */
#define IEL_S11 0x800		/* CRC error on receiving */
				/* Fail on DIAGNOSE */
#define IEL_A	0x1000		/* Command aborted */
#define IEL_OK	0x2000		/* This command completed w/o errors */
#define IEL_B	0x4000		/* Busy executing this command */
#define	IEL_C	0x8000		/* Command Completed */

/* Command word bits */
#define IEL_CMD	0x7		/* Command code */
#define IEL_I	0x2000		/* Interrupt after completion */
#define IEL_S	0x4000		/* Suspend after completion */
#define IEL_EL	0x8000		/* End of the command list/last recv frame in the list  */

/* NOP */
#define IEL_NOP		0
struct iel_nop {
	u_short	status;		/* Status of the command */
	u_short cmd;		/* The command */
	u_short link;		/* Link to the next command */
};

/* Individual Address Setup */
#define IEL_IA_SETUP	1
struct iel_ia_setup {
	u_short	status;		/* Status of the command */
	u_short cmd;		/* The command */
	u_short link;		/* Link to the next command */
	u_char	eaddr[ETHER_ADDR_LEN];	/* Ethernet Address */
};

/* Configure */
#define IEL_CONFIG	2
struct iel_config {
	u_short	status;		/* Status of the command */
	u_short cmd;		/* The command */
	u_short link;		/* Link to the next command */
	u_char	bytecnt;	/* Param block byte counter */
	u_char	fifolim;	/* FIFO threshold value */
	u_short	misc1;		/* Miscellaneous params */
	u_char  misc2;		/* Miscellaneous params */
	u_char	ifspc;		/* Interframe spacing */
	u_short	sltime;		/* Slot time/Retry num */
	u_short misc3;		/* Miscellaneous params */
	u_char	minfrm;		/* Min. Frame length */
	u_char	zero;		/* Should be zero */
};

	/* Misc params #1 */
#define IEL_SRDY	0x40	/* External synchronization */
#define IEL_SAVBF	0x80	/* Save bad frames */
#define IEL_ADDRLEN(n)	((n)<<8) /* Number of address bits */
#define IEL_ALLOC	0x800	/* Address and length fields belong to data */
#define IEL_PREAMLEN2   0x0     /* Length of preambula is 2 bytes */
#define IEL_PREAMLEN4   0x1000	/* Length of preambula is 4 bytes */
#define IEL_PREAMLEN8   0x2000	/* Length of preambula is 8 bytes */
#define IEL_PREAMLEN16  0x3000	/* Length of preambula is 16 bytes */
#define IEL_INTLOOP	0x4000	/* Internal Loopback */
#define IEL_EXTLOOP	0x8000	/* External Loopback */

	/* Misc params #2 */
#define IEL_LINPRIO(n)	(n)	/* Linear Priority */
#define IEL_ACR(n)	((n)<<4) /* Accelerated Contention Resolution factor */
#define IEL_BOFMET	0x80	/* Alternate (not IEEE 802.3) exp. backoff */

	/* Slot time/number of retries */
#define IEL_SLOTTIM(n)	(n)	/* Slot time */
#define IEL_NRTRY(n)	((n)<<12) /* Maximum Number of retries on collisions */

	/* Misc params #3 */
#define IEL_PRM		0x1	/* Promiscuous mode */
#define IEL_BCDIS	0x2	/* Broadcast disable */
#define IEL_MANCH	0x4	/* Manchester (!NRZ) encoding */
#define IEL_TONOCRS	0x8	/* Transmit even if no carrier sense */
#define IEL_NCRCINS	0x10	/* No CRC insertion */
#define IEL_CRC16	0x20	/* Use 16bit CRC (instead of 32bit Autodin 2) */
#define IEL_BTSTF	0x40	/* Enable HDLC-like bitstuffing */
#define IEL_PAD		0x80	/* Perform padding for the rest of slot time */
#define IEL_CRSF(n)	((n)<<8) /* Carrier Sense Filter Time (in bits) */
#define IEL_CRSSRC	0x800	/* Carrier Source Sense is internal */
#define IEL_CDTF(n)	((n)<<12) /* Collision Detect Filter Time (in bits) */
#define IEL_CDTSRC	0x8000	/* Collision Detect Source is internal */

#ifndef NO_MULTICAST
#define NMCADDR 64
#else
#define NMCADDR 1
#endif

/* Multicast Addresses Setup */
#define IEL_MCSETUP	3
struct iel_mc_setup {
	u_short	status;		/* Status of the command */
	u_short cmd;		/* The command */
	u_short link;		/* Link to the next command */
	u_short mccnt;		/* The number of mulicast addresses */
	u_char	mcids[NMCADDR*ETHER_ADDR_LEN]; /* Multicast IDs (var. length) */
};

/* Transmit */
#define IEL_TRANSMIT	4
struct iel_transmit {
	u_short	status;		/* Status of the command */
	u_short cmd;		/* The command */
	u_short link;		/* Link to the next command */
	u_short tbd;		/* Block of data to transmit */
	u_char	dstaddr[6];	/* Destination address */
	u_short txlen;		/* Frame length */
};

	/* Transmit Buffer Descriptor */
struct iel_tbd {
	u_short	actcnt;		/* Actual number of data bytes in the buffer */
	u_short	nextbd;		/* link to the next DB */
	u_long	buf;		/* Absolute buffer address */
};

	/* Actcnt bits (good for rbd as well) */
#define IEL_CNT 0x3fff		/* The byte count */
#define IEL_F	0x4000		/* Actcnt is valid (on RX) */
#define IEL_EOF	0x8000		/* End-Of-Frame in actcnt */

/* Time Domain Reflectometer */
#define IEL_TDR		5
struct iel_tdr {
	u_short	status;		/* Status of the command */
	u_short cmd;		/* The command */
	u_short link;		/* Link to the next command */
	u_short result;		/* The result field */
};

	/* The result bits */
#define IEL_TDRTIME	0x7ff	/* Distance to a problem (in TX clocks) */
#define IEL_ET_SRT	0x1000	/* Short on the link */
#define IEL_ET_OPN	0x2000	/* Open on the link */
#define IEL_XCVR_PRB	0x4000	/* Problems with tranceiver cable */
#define IEL_NOPROBLEM	0x8000	/* No Link Problems identified */

/* Dump */
#define IEL_DUMP	6
struct iel_dump {
	u_short	status;		/* Status of the command */
	u_short cmd;		/* The command */
	u_short link;		/* Link to the next command */
	u_short dumpbuf;	/* Dump buffer offset */
};

/* Diagnose */
#define IEL_DIAGNOSE	7
struct iel_diagnose {
	u_short	status;		/* Status of the command */
	u_short cmd;		/* The command */
	u_short link;		/* Link to the next command */
};

/* Receive Frame Descriptor */
struct iel_rfd {
	u_short status;		/* Status of the frame */
	u_short cmd;		/* Action on completion */
	u_short link;		/* The next buffer */
	u_short rbd;		/* Receive buffer descriptor */
	u_char	dstaddr[6];	/* Destination address */
	u_char	srcaddr[6];	/* Source address */
	u_short	rxlen;		/* Length of the received data */
};

	/* Receive Buffer Descriptor */
struct iel_rbd {
	u_short	actcnt;		/* Actual number of data bytes in the buffer */
	u_short	nextbd;		/* link to the next DB */
	u_long	buf;		/* Absolute buffer address */
	u_short elsize;		/* Size of the buffer/EL flag */
};
