/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: if_rnreg.h,v 1.1 1992/07/29 03:10:40 karels Exp $

/*
 * Definitions for RISCom/N1 HDLC serial port card
 */

/*
 * History:
 *	6/12/92 [avg]	initial revision
 */

/* 
 * I/O ports (offsets from base adress)
 */
#define	RN_PCR		0x0		/* PC Control Register */
#define	RN_BAR		0x2		/* Mem Base Address Register */
#define	RN_PSR		0x4		/* Page Scan Register */
#define	RN_VPM		0x6		/* VPM Register */
#define	RN_NPORT	7		/* Number of contiguous I/O ports */

#define	RN_CRL		0x4000		/* Command Request Line */
#define	RN_CLI		0x4002		/* Clear INT Line */

#define	RN_CTL	(0x8000 + uPD_CTL * 2)		/* Control Register */
#define	RN_STAT	(0x8000 + uPD_STAT * 2)		/* Status Register */
#define	RN_MSET	(0x8000 + uPD_MSET_START * 2)	/* MSET Start Register */
#define	RN_MCMD (0x8000 + uPD_MSET_CMD * 2)	/* MSET Cmd Register */

#define	RN_BRDIV	0xc000	/* Baud Rate Generator Divisor */
#define	RN_BRCTL	0xc006	/* Baud Rate Generator Control Register */

/*
 * PC Control Register bits
 */
#define	RN_NCRES	0x01	/* 0 = reset uPD72103 */
#define	RN_ENVPM	0x02	/* 1 = enable VPM */
#define	RN_ENWIN	0x04	/* 1 = enable memory window */
#define	RN_EETC		0x08	/* 1 = enable external CLK out */
#define	RN_CLKM		0x30	/* Clock Mode */
#define	    RN_CLKINT	0x00	/* Internal Clock */
#define	    RN_CLKEXT	0x10	/* External Clock (RxC, TxC) */
#define	    RN_CLKPLL	0x20	/* 32MHz sync clock from RxC */
#define	RN_WINSIZ	0xc0	/* Window Size */
#define	    RN_WS16K	0x00	/* 16Kb */
#define	    RN_WS32K	0x40	/* 32Kb */
#define	    RN_WS64K	0x80	/* 64Kb */
#define	    RN_WS256K	0xc0	/* 256Kb */

/*
 * Virtual Protected Mode register bits
 */
#define	RN_XADR		0x0f	/* AD23-AD20 address extention */
#define	RN_ID		0x30	/* The board ID */
#define	RN_DSR		0x40	/* DSR Input */
#define	RN_PCINT	0x80	/* uPD72103 interrupt status (1=on) */

/*
 * Baud Rate Generator Control Register
 */
#define	RN_BR_INIT	0x36	/* don't ask me what does it mean */

/* On-board Clock Oscillator Frequency */
#define	RN_OSC_FREQ	8000000	/* 8MHz */

/*
 * Valid i/o base address settings
 */
#define	RN_VALIDBASE(a)	(((a) & ~0x3f0) == 0)

/*
 * Valid IRQs (2,3,4,5,9,10,11,12,15)
 */
#define	RN_VALIDIRQ(i)	((1<<(i)) & 0x9e3c)
