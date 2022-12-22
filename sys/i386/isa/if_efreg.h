/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI $Id: if_efreg.h,v 1.5 1993/12/19 04:12:18 karels Exp $
 */

/*
 * 3Com Corp. Etherlink III (3C509) Ethernet adapter driver.
 *
 * History:
 *	9/15/92 [avg]	- initial revision
 */

/*
 * Config macros
 */
#define EF_IOBASEVALID(b)	(((b) & ~0x1f0) == 0x200)

/* 3C509 supports IRQ 3, 5, 7, 9, 10, 11, 12, 15 */
#define EF_IRQS	     0x9eac
#define EF_IRQVALID(i)	((1<<(i)) & EF_IRQS)

#define EF_NPORT		16

/* Manufacturer ID code */
#define EF_MFG_ID	0x6d50

/* ID port address */
#define EF_ID		0x100

/* ID port commands */
#define EFID_RDROM(n)	(0x80 | (n))	/* Read the ROM address n */
#define EFID_RESET	0xc0		/* Global Reset */
#define EFID_TAG(n)	(0xd0 | (n))	/* Set adapter tag register to n */
#define EFID_TESTTAG(n) (0xd8 | (n))	/* Test adapeter for tag n */
#define EFID_ACT(a)	(0xe0 | (((a)>>4) & 0x1f))
					/* Activate adapter with base address a */
#define EFID_ACTIVATE	0xff		/* Activate adapter with EEPROM address */


/* Registers common for all windows */
#define EF_STAT		0xe		/* Status register (R) */
#define EF_CMD		0xe		/* Command register (W) */	

/* Window 0 - Setup */
#define EF0_MID		0x0		/* Manufacturer ID (R) */
#define EF0_PID		0x2		/* Product ID (R) */
#define EF0_CC		0x4		/* Configuration Control */
#define EF0_ACNF	0x6		/* Address Configuration */
#define EF0_RCNF	0x8		/* Resource Configuration */
#define EF0_PROMCMD	0xa		/* EEPROM Command */
#define EF0_PROMDATA	0xc		/* EEPROM Data */

/* Window 1 - Operating Set */
#define EF1_DATA	0x0		/* RX/TX PIO Data */
#define EF1_RXSTAT	0x8		/* RX Status (R) */
#define EF1_TIMER	0xa		/* 32 usec interrupt latency timer */
#define EF1_TXSTAT	0xb		/* TX Status (R) */
#define EF1_TXFREE	0xc		/* Free transmit bytes (R) */

/* Window 2 - Station Address */
#define EF2_ADDR(i)	(i)		/* Station address */

/* Window 3 - FIFO Management */
#define EF3_RXFREE	0xa		/* Free receive bytes (R) */
#define EF3_TXFREE	0xc		/* Free transmit bytes (R) */

/* Window 4 - Diagnostics */
#define EF4_TXDIAG	0x0		/* TX Diagnostic */
#define EF4_HDIAG	0x2		/* Host Diagnostic */
#define EF4_FDIAG	0x4		/* FIFO Diagnostic */
#define EF4_NDIAG	0x6		/* Net Diagnostic */
#define EF4_ECS		0x8		/* Ethernet Controller Status */
#define EF4_MTS		0xa		/* Media Type and Status */

/* Window 5  - Command Results and Internal State */
#define EF5_TXSTART	0x0		/* TX Start threshold + 4 (R) */
#define EF5_TXAVAIL	0x2		/* TX Available threshold (R) */
#define EF5_RXEARLY	0x6		/* RX Early threshold (R) */
#define EF5_RXFILTER	0x8		/* RX Filter (R) */
#define EF5_IMASK	0xa		/* Interrupt Mask (R) */
#define EF5_RZMASK	0xc		/* Read Zero Mask (R) */

/* Window 6 - Statistics */
#define EF6_CSL		0x0		/* Carrier Sense Lost during transmission */
#define EF6_NOCD	0x1		/* Frames transmitted with no CD Heartbeat */
#define EF6_MCOL	0x2		/* Frames transmitted after Multiple Collisions */
#define EF6_OCOL	0x3		/* Frames transmitted after One Collision */
#define EF6_LCOL	0x4		/* Late Collisions on transmit */
#define EF6_OVR		0x5		/* Receive Overruns */
#define EF6_TXOK	0x6		/* Frames Transmitted OK */
#define EF6_RXOK	0x7		/* Frames Received OK */
#define EF6_DEF		0x8		/* Transmit Deferrals */
#define EF6_RXTOTAL	0xa		/* Total Bytes Received OK */
#define EF6_TXTOTAL	0xc		/* Total Bytes Transmitted OK */

/*
 * Commands to write to EF_CMD
 * The argument should be OR-ed with the command word
 */
#define EFC_RESET	0x0000		/* Global Reset */
#define EFC_WINDOW	0x0800		/* Select Window N */
#define EFC_COAXSTA	0x1000		/* Start Coaxial Transceiver */
#define EFC_RXDIS	0x1800		/* Disable Receiver */
#define EFC_RXENB	0x2000		/* Enable Receiver */
#define EFC_RXRST	0x2800		/* Reset Receiver */
#define EFC_RXDTP	0x4000		/* Discard Top Received Packet */
#define EFC_TXENB	0x4800		/* Enable Transmitter */
#define EFC_TXDIS	0x5000		/* Disable Transmitter */
#define EFC_TXRST	0x5800		/* Reset Transmitter */
#define EFC_IREQ	0x6000		/* Request Interrupt */
#define EFC_ACK		0x6800		/* Acknowledge Interrupt (arg bits as in EF_STAT) */
#define EFC_SIM		0x7000		/* Set Interrupt Mask (arg bits as in EF_STAT) */
#define EFC_SZM		0x7800		/* Set Zero Mask (arg bits as in EF_STAT) */
#define EFC_SRXF	0x8000		/* Set RX Filter */
# define EFRXF_ADDR	 0x1		 /* Accept Individual Address */
# define EFRXF_AM	 0x2		 /* Accept Multicast */	
# define EFRXF_AB	 0x4		 /* Accept Broadcast */
# define EFRXF_PROM	 0x8		 /* Promiscuous Mode (accept everything) */
#define EFC_RXEARLY	0x8800		/* Set RX Early Threshold */
#define EFC_TXAVAIL	0x9000		/* Set TX Available Threshold */
#define EFC_TXSTART	0x9800		/* Set TX Start Threshold */
#define EFC_STATENB	0xa800		/* Statistics Enable */
#define EFC_STATDIS	0xb000		/* Statistics Disable */
#define EFC_COAXSTP	0xb800		/* Stop Coaxial Transceiver */

/*
 * Status bits (read from EF_STAT)
 */
#define EFS_IL		0x0001		/* Interrupt Latch */
#define EFS_AF		0x0002		/* Adapter Failure */
#define EFS_TXC		0x0004		/* TX Complete */
#define EFS_TXA		0x0008		/* TX Available */
#define EFS_RXC		0x0010		/* RX Complete */
#define EFS_RXE		0x0020		/* RX Early */
#define EFS_IREQ	0x0040		/* Interrupt Requested */
#define EFS_UPDS	0x0080		/* Update Statistics */
#define EFS_CIP		0x1000		/* Command-In-Progress */
#define EFS_WINDOW	0xe000		/* Window number */

/*
 * Receive Status (read from EF1_RXSTAT)
 */
#define EFRS_BYTES	0x07ff		/* Bytes left in FIFO */
#define EFRS_ETYPE	0x7800		/* Error Type, including EFRS_ERR */
# define EFET_DRIB	  0x1000	  /* Dribble Bit(s) */
# define EFET_OVERRUN	  0x4000	  /* Overrun */
# define EFET_OVRSIZ	  0x4800	  /* Oversize Packet */
# define EFET_RUNT	  0x5800	  /* Runt Packet */
# define EFET_ALIGN	  0x6000	  /* Alignment (Framing) Error */
# define EFET_CRC	  0x6800	  /* CRC Error */
#define EFRS_ERR	0x4000		/* Error */
#define EFRS_INCOMPL	0x8000		/* Incomplete */

/*
 * Transmit Status (read from EF1_TXSTAT)
 */
#define EFTS_SOVFL	0x04		/* TX Status Overflow */
#define EFTS_MAXCOL	0x08		/* Maximum Collisions */
#define EFTS_UNDR	0x10		/* Underrun (TX Reset required) */
#define EFTS_JAB	0x20		/* Jabber Error */
#define EFTS_INT	0x40		/* Interrupt on successful transmission */
#define EFTS_COMPL	0x80		/* TX is complete */

/*
 * Configuration Control Register (EF0_CC)
 */
#define EFCC_ENA	0x0001		/* Enable adapter */
#define EFCC_RST	0x0002		/* Reset adapter to POR */
#define EFCC_ICODEC	0x0100		/* Use internal encoder/decoder (R) */
#define EFCC_TP		0x0200		/* On-board 10BASE-T (TP) is available (R) */
#define EFCC_RXTM	0x0400		/* Receive test mode (R) */
#define EFCC_TXTM	0x0800		/* Transmit test mode (R) */
#define EFCC_NORM	0x0c00		/* Normal mode (R) */
#define EFCC_BNC	0x1000		/* On-board 10BASE2 (BNC) is available (R) */
#define EFCC_AUI 	0x2000		/* AUI connector is available (R) */

/*
 * Address Configuration Register (EF0_ACNF)
 */
#define EFAC_BASE(a)	(((a)>>4) & 0x1f) /* ISA base address */
#define EFAC_EISASLOT	0x001f		/* EISA slot-specific i/o address */

#define EFAC_NOROM	0x0000		/* Boot ROM disabled */
#define EFAC_ROMBASE	0x0f00		/* Boot ROM base bits */
#define EFAC_ROMSIZE	0x3000		/* Boot ROM size bits */

#define EFAC_TP		0x0000		/* Select 10BASE-T (TP) transceiver */
#define EFAC_AUI	0x4000		/* Select AUI connector */
#define EFAC_BNC	0xc000		/* Select 10BASE2 (BNC) transceiver */

/*
 * Media Type and Status Register (EF4_MTS)
 */
#define EFMTS_SQESE     0x0008          /* TP - SQE Statistics Enable */
#define EFMTS_COLL      0x0010          /* Collision (R) */
#define EFMTS_CRS       0x0020          /* Carrier Sense (R) */
#define EFMTS_JABE      0x0040          /* TP - Jabber Error Enable */
#define EFMTS_LBE       0x0080          /* TP - Link Beat Enable */
#define EFMTS_UNSQ      0x0100          /* TP - Unsquelch (R) */
#define EFMTS_JAB       0x0200          /* TP - Jabber (R) */
#define EFMTS_POLSW     0x0400          /* TP - Polarity Swap (R) */
#define EFMTS_LBC       0x0800          /* TP - Link Beat Correct */
#define EFMTS_SQE       0x1000          /* SQE Present (R) */
#define EFMTS_IED       0x2000          /* Internal Encoder/Decoder (R) */
#define EFMTS_BNC       0x4000          /* BNC transceiver (R) */
#define EFMTS_AUID      0x8000          /* AUI disable (R) */

/*
 * Resource Configuration Register (EF0_RCNF)
 */
#define EFRC_IRQ(i)	(((i)<<12) | 0xf00) /* Set IRQ i */

/*
 * EEPROM Command Register (EF0_PROMCMD)
 */
#define EFPC_WDIS	0x00		/* Write/Erase Disable */
#define EFPC_WALL	0x10		/* Write All Registers */
#define EFPC_EALL	0x20		/* Erase All Registers */
#define EFPC_WENB	0x30		/* Write/Erase Enable */
#define EFPC_WREG(a)	(0x40 | (a))	/* Write EEPROM address a */
#define EFPC_RREG(a)	(0x80 | (a))	/* Read EEPROM address a */
#define EFPC_EREG(a)	(0xc0 | (a))	/* Erase EEPROM register a */

#define EFPC_TAG	0x0700		/* Tag register (R) */
#define EFPC_TST	0x4000		/* Test mode jumpered (R) */
#define EFPC_EBY	0x8000		/* EEPROM is busy (R) */

/*
 * EEPROM Data Structure
 */
#define EFROM_ADDR(i)	(0x0 + (i))	/* 3Com station address (word i) */
#define EFROM_PID	0x3		/* 3C509 Product ID (0x9?50) */
#define EFROM_MFG(i)	(0x4 + (i))	/* Manufacturing Data */
#define EFROM_MFC	0x7		/* Manufacturer Code (0x6d50) */
#define EFROM_ACNF	0x8		/* Address Configuration */
#define EFROM_RCNF	0x9		/* Resource Configuration */
#define EFROM_OEMA(i)	(0xa + (i))	/* OEM station address (word i) */
#define EFROM_SI	0xd		/* Software Information */
#define EFROM_CHKSUM	0xf		/* Checksum */
#define EFROM_NMD(i)	(0x10 + (i))	/* Network Management Data (48 words) */
