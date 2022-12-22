/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI $Id: rcreg.h,v 1.5 1993/11/07 22:58:43 karels Exp $
 */

/*
 * Definitions for RISCom/8 Async Mux card by SDL Communications, Inc.
 */


/*
 * Address mapping between Cirrus Logic CD180 chip internal registers
 * and ISA port addresses:
 *
 *	CL-CD180	        A6  A5  A4  A3                    A2 A1 A0
 *      ISA	 	A15 A14 A13 A12.A11 A10 A9 A8.A7 A6 A5 A4.A3 A2 A1 A0
 */
#define RC_ADDR(reg)	((((reg)&7)<<1) | (((reg)&~7)<<7))

/* Input Byte from CL CD180 register */
#define rinb(base, reg)		inb((base) + RC_ADDR(reg))

/* Output Byte to CL CD180 register */
#define routb(base, reg, val)	outb((base) + RC_ADDR(reg), (val))

/*
 * The configurable board base addresss bits and minimal BA
 */
#define RC_MINBASE	0x200
#define RC_BASEBITS	0x1f0

/*
 * The list of configurable IRQs
 * (3,4,5,6,7,9(2),10,11,12,15)
 */
#define RC_VALIDIRQ(i)	((3<=(i) && (i)<=7) || (9<=(i) && (i)<=12) || (i)==15)

/*
 * RISCom/8 On-Board Registers (assuming address translation)
 */
#define RC_RI		0x100	/* Ring Indicator Register (r) */
#define RC_DTR		0x100	/* DTR Register (w) */
#define RC_BSR		0x101	/* Board Status Register (r) */
#define RC_CTOUT	0x101	/* Clear Timeout (w) */

/*
 * Board Status Register
 */
#define	RC_BSR_TOUT	0x8	/* Hardware Timeout */
#define RC_BSR_RINT	0x4	/* Receiver Interrupt */
#define RC_BSR_TINT	0x2	/* Transmitter Interrupt */
#define RC_BSR_MINT	0x1	/* Modem Ctl Interrupt */

/*
 * On-board oscillator frequency (in Hz)
 */
#define RC_OSCFREQ	9830400

/*
 * Values of choice for Interrupt ACKs
 */
#define RC_ACK_MINT	0x81	/* goes to PILR1 */
#define RC_ACK_RINT	0x82	/* goes to PILR3 */
#define RC_ACK_TINT	0x84	/* goes to PILR2 */

/* Chip ID (sorry, only one chip now) */
#define RC_ID		0x10

/* The number of I/O ports on the board */
#define RC_NPORT	16	/* decoder recognizes 16 addresses... */
