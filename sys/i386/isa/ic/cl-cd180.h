/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI $Id: cl-cd180.h,v 1.3 1993/11/18 20:56:16 karels Exp $
 */

/*
 * Definitions for Cirrus Logic CL-CD180 8-port async mux chip
 */

#define CD180_NCHAN	8	/* Total number of channels */
#define CD180_TPC	16	/* Ticks per character */

/*
 * Channel Registers
 */
#define CD180_CCR	0x01	/* Channel Command Register */
#define CD180_IER	0x02	/* Interrupt Enable Register */
#define CD180_COR1	0x03	/* Channel Option Register 1 */
#define CD180_COR2	0x04	/* Channel Option Register 1 */
#define CD180_COR3	0x05	/* Channel Option Register 1 */
#define CD180_CCSR	0x06	/* Channel Control STatus Register */
#define CD180_RDCR	0x07	/* Receive Data Count Register */
#define CD180_SCHR1	0x09	/* Special Character Register 1 */
#define CD180_SCHR2	0x0a	/* Special Character Register 2 */
#define CD180_SCHR3	0x0b	/* Special Character Register 3 */
#define CD180_SCHR4	0x0c	/* Special Character Register 4 */
#define CD180_MCOR1	0x10	/* Modem Change Option 1 Register */
#define CD180_MCOR2	0x11	/* Modem Change Option 2 Register */
#define CD180_MCR	0x12	/* Modem Change Register */
#define CD180_RTPR	0x18	/* Receive Timeout Period Register */
#define CD180_MSVR	0x28	/* Modem Signal Value Register */
#define CD180_RBPRH	0x31	/* Receive Baud Rate Period Register High */
#define CD180_RBPRL	0x32	/* Receive Baud Rate Period Register Low */
#define CD180_TBPRH	0x39	/* Transmit Baud Rate Period Register High */
#define CD180_TBPRL	0x3a	/* Transmit Baud Rate Period Register Low */

/*
 * Global registers
 */
#define CD180_GIVR	0x40	/* Global Interrupt Verctor Register */
#define CD180_GICR	0x41	/* Global Interrupting Channel Register */
#define CD180_PILR1	0x61	/* Priority Interrupt Level Register 1 */
#define CD180_PILR2	0x62	/* Priority Interrupt Level Register 2 */
#define CD180_PILR3	0x63	/* Priority Interrupt Level Register 3 */
#define CD180_CAR	0x64	/* Channel Access Register */
#define CD180_GFRCR	0x6b	/* Global Firmware Revision Code Register */
#define CD180_PPRH	0x70	/* Prescaler Period Register High */
#define CD180_PPRL	0x71	/* Prescaler Period Register Low */
#define CD180_RDR	0x78	/* Receiver Data Register */
#define CD180_RCSR	0x7a	/* Receiver Character Status Register */
#define CD180_TDR	0x7b	/* Transmit Data Register */
#define CD180_EOIR	0x7f	/* End of Interrupt Register */

/*
 * Global Interrupt Vector Register
 */
#define GIVR_ITMASK	0x7	/* Interrupt type mask */
#define  GIVR_IT_MODEM	 0x1	 /* Modem Signal Change Interrupt */
#define  GIVR_IT_TX	 0x2	 /* Transmit Data Interrupt */
#define  GIVR_IT_RCV	 0x3	 /* Receive Good Data Interrupt */
#define  GIVR_IT_REXC	 0x7	 /* Receive Exception Interrupt */

/*
 * Global Interrupt Channel Register
 */
#define GICR_CHAN	0x1c	/* Channel Number Mask */
#define GICR_CHAN_OFF	2	/* Channel Number Offset */

/*
 * Channel Address Register
 */
#define CAR_CHAN	0x7	/* Channel Number Mask */
#define CAR_A7		0x8	/* A7 Address Extention (for test purposes) */

/*
 * Receive Character Status Register
 */
#define RCSR_TOUT	0x80	/* Rx Timeout */
#define RCSR_SCDET	0x70	/* Special Character Detected Mask */
#define  RCSR_NO_SC	 0x0	 /* No Special Characters Detected */
#define	 RCSR_SC_1	 0x10	 /* Special Char 1 (1&3) Detected */
#define	 RCSR_SC_2	 0x20	 /* Special Char 2 (2&4) Detected */
#define	 RCSR_SC_3	 0x30	 /* Special Char 3 Detected */
#define	 RCSR_SC_4	 0x40	 /* Special Char 4 Detected */
#define RCSR_BREAK	0x8	/* Break has been detected */
#define RCSR_PE		0x4	/* Parity Error */
#define RCSR_FE		0x2	/* Frame Error */
#define RCSR_OE		0x1	/* Overrun Error */

/*
 * Channel Command Register
 * (commands in groups can be OR-ed)
 */
#define CCR_HARDRESET	0x81	/* Reset the chip */

#define CCR_SOFTRESET	0x80	/* Soft Channel Reset */

#define CCR_CORCHG1	0x42	/* Channel Option Register 1 Changed */
#define CCR_CORCHG2	0x44	/* Channel Option Register 2 Changed */
#define CCR_CORCHG3	0x48	/* Channel Option Register 3 Changed */

#define CCR_SSCH1	0x21	/* Send Special Character 1 */

#define CCR_SSCH2	0x22	/* Send Special Character 2 */

#define CCR_SSCH3	0x23	/* Send Special Character 3 */

#define CCR_SSCH4	0x24	/* Send Special Character 4 */

#define CCR_TXEN	0x18	/* Enable Transmitter */
#define CCR_RXEN	0x12	/* Enable Receiver */

#define CCR_TXDIS	0x14	/* Diasable Transmitter */
#define CCR_RXDIS	0x11	/* Diasable Receiver */

/*
 * Interrupt Enable Register
 */
#define IER_DSR		0x80	/* Enable interrupt on DSR change */
#define IER_CD		0x40	/* Enable interrupt on CD change */
#define IER_CTS		0x20	/* Enable interrupt on CTS change */
#define IER_RXD		0x10	/* Enable interrupt on Receive Data */
#define IER_RXSC	0x8	/* Enable interrupt on Receive Spec. Char */
#define IER_TXRDY	0x4	/* Enable interrupt on TX FIFO empty */
#define IER_TXEMPTY	0x2	/* Enable interrupt on TX completely empty */
#define IER_RET		0x1	/* Enable interrupt on RX Exc. Timeout */

/*
 * Channel Option Register 1
 */
#define COR1_ODDP	0x80	/* Odd Parity */
#define COR1_PARMODE	0x60	/* Parity Mode mask */
#define  COR1_NOPAR	 0x0	 /* No Parity */
#define  COR1_FORCEPAR	 0x20	 /* Force Parity */
#define  COR1_NORMPAR	 0x40	 /* Normal Parity */
#define COR1_IGNORE	0x10	/* Ignore Parity on RX */
#define COR1_STOPBITS	0xc	/* Number of Stop Bits */
#define  COR1_1SB	 0x0	 /* 1 Stop Bit */
#define  COR1_15SB	 0x4	 /* 1.5 Stop Bits */
#define  COR1_2SB	 0x8	 /* 2 Stop Bits */
#define COR1_CHARLEN	0x3	/* Character Length */
#define	 COR1_5BITS	 0x0	 /* 5 bits */
#define	 COR1_6BITS	 0x1	 /* 6 bits */
#define	 COR1_7BITS	 0x2	 /* 7 bits */
#define	 COR1_8BITS	 0x3	 /* 8 bits */

/*
 * Channel Option Register 2
 */
#define COR2_IXM	0x80	/* Implied XON mode */
#define COR2_TXIBE	0x40	/* Enable In-Band (XON/XOFF) Flow Control */
#define COR2_ETC	0x20	/* Embedded Tx Commands Enable */
#define COR2_LLM	0x10	/* Local Loopback Mode */
#define COR2_RLM	0x8	/* Remote Loopback Mode */
#define COR2_RTSAO	0x4	/* RTS Automatic Output Enable */
#define COR2_CTSAE	0x2	/* CTS Automatic Enable */
#define COR2_DSRAE	0x1	/* DSR Automatic Enable */

/*
 * Channel Option Register 3
 */
#define COR3_XONCH	0x80	/* XON is a pair of characters (spec. 1&3) */
#define COR3_XOFFCH	0x40	/* XOFF is a pair of characters (2&4) */
#define COR3_FCT	0x20	/* Flow-Control Transparency Mode */
#define COR3_SCDE	0x10	/* Special Character Detection Enable */
#define COR3_RXTH	0xf	/* RX FIFO Threshold value (1-8) */

/*
 * Channel Control Status Register
 */
#define CCSR_RXEN	0x80	/* Revceiver Enabled */
#define CCSR_RXFLOFF	0x40	/* Receive Flow Off (XOFF was sent) */
#define CCSR_RXFLON	0x20	/* Receive Flow On (XON was sent) */
#define CCSR_TXEN	0x8	/* Transmitter Enabled */
#define CCSR_TXFLOFF	0x4	/* Transmit Flow Off (got XOFF) */
#define CCSR_TXFLON	0x2	/* Transmit Flow On (got XON) */

/*
 * Modem Change Option Register 1
 */
#define MCOR1_DSRZD	0x80	/* Detect 0->1 transition of DSR */
#define MCOR1_CDZD	0x40	/* Detect 0->1 transition of CD */
#define MCOR1_CTSZD	0x20	/* Detect 0->1 transition of CTS */
#define MCOR1_DTRTH	0xf	/* Automatic DTR flow control Threshold (1-8)*/
#define  MCOR1_NODTRFC	 0x0	 /* Automatic DTR flow control disabled */

/*
 * Modem Change Option Register 2
 */
#define MCOR2_DSROD	0x80	/* Detect 1->0 transition of DSR */
#define MCOR2_CDOD	0x40	/* Detect 1->0 transition of CD */
#define MCOR2_CTSOD	0x20	/* Detect 1->0 transition of CTS */

/*
 * Modem Change Register
 */
#define MCR_DSRCHG	0x80	/* DSR Changed */
#define MCR_CDCHG	0x40	/* CD Changed */
#define MCR_CTSCHG	0x20	/* CTS Changed */

/*
 * Modem Signal Value Register
 */
#define MSVR_DSR	0x80	/* Current state of DSR input */
#define MSVR_CD		0x40	/* Current state of DSR input */
#define MSVR_CTS	0x20	/* Current state of CTS input */
#define MSVR_DTR	0x2	/* Current state of DTR output */
#define MSVR_RTS	0x1	/* Current state of RTS output */

/*
 * Escape characters
 */
#define CD180_C_ESC	0x0	/* Escape character */
#define CD180_C_SBRK	0x81	/* Start sending BREAK */
#define CD180_C_DELAY	0x82	/* Delay output */
#define CD180_C_EBRK	0x83	/* Stop sending BREAK */
