/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: wtreg.h,v 1.6 1992/07/28 03:14:26 karels Exp $
 */

/*
 * Wangtek QIC-02/QIC-36 / Archive 402 ISA QIC-02  streamer tape driver
 */

#define WTBLKSIZE	512	/* hardware constant */

/*----------------Wangtek definitions-----------------------*/

/*
 * Tape controller ports
 */
#define	wt_statport(base)	(base)		/* read, status */
#define wt_ctlport(base)	(base)		/* write, control */
#define wt_dataport(base) 	((base) + 1)	/* read, data */
#define wt_cmdport(base)	((base) + 1)	/* write, command */
#define WT_NPORT		2		/* 4 I/O ports */

/*
 * Status bits
 */
#define WT_READY	01	/* INVERTED controller is ready */
#define WT_EXCEP	02	/* INVERTED exception occured */
#define WT_STATMASK	03	/* mask for extracting stat bits */
#define WT_DIRECT	04	/* data xfer direction */
#define WT_RESETMASK	07
#define WT_RESETVAL	05	/* expected value of 3 low bits after reset */

/*
 * Control bits
 */
#define WT_ONLINE	01	/* select drive ONLINE */
#define WT_RESET	02	/* reset drive */
#define WT_REQUEST	04	/* request to enter a new command and start */
				/* it on reset */
/* Interrupt & DMA enable */
#define WT_IENB(dmachan) (((dmachan) < 3) ? 010 : 020)


/*-----------------Archive definitions---------------------*/

/*
 * Tape controller ports
 */
#define arc_dataport(base)	(base)		/* read, data */
#define arc_cmdport(base)	(base)		/* write, commands */
#define arc_statport(base)	((base) + 1)	/* read, status */
#define arc_ctlport(base)	((base) + 1)	/* write, control */
#define arc_sdmaport(base)	((base) + 2)	/* write, start DMA */
#define arc_rdmaport(base)	((base) + 3)	/* write, reset DMA */
#define ARC_NPORT		4		/* 4 I/O ports */

/*
 * Status bits
 */
#define ARC_DIRECT	010	/* data xfer direction */
#define ARC_DONE	020	/* INVERTED DMA done */
#define ARC_EXCEP	040	/* INVERTED excecption occured */
#define ARC_READY	0100	/* INVERTED controller is ready */
#define ARC_IRQF	0200	/* Interrupt request flag */
#define ARC_STATMASK	0140	/* ARC_EXCEP|ARC_READY */
#define ARC_RESETMASK	0370
#define ARC_RESETVAL	0120

/*
 * Control bits
 */
#define ARC_DNIEN	020	/* Done interupt enable */
#define ARC_IEN		040	/* Interrupt enable */
#define ARC_REQUEST	0100	/* Request */
#define ARC_RESET	0200	/* Reset drive */


/*
 * Generic QIC-02/QIC-36 commands
 */
#define WT_SELECT	0x01	/* select the drive */
#define WT_REWIND	0x21	/* rewind tape */
#define WT_ERASE	0x22	/* erase tape */
#define WT_QIC11	0x26	/* select QIC-11 format */
#define WT_QIC24	0x27	/* select QIC-24 format */
#define WT_QIC120	0x28	/* select QIC-120 format */
#define WT_QIC150	0x29	/* select QIC-150 format */
#define WT_WRDATA	0x40	/* write data */
#define WT_WRMARK	0x60	/* write file mark */
#define WT_RDDATA	0x80	/* read data */
#define WT_RDMARK	0xa0	/* seek forward for data mark */
#define WT_RDSTAT	0xc0	/* read status */

/*
 * The number of blocks from the BOT for enough 
 * to get WRITE FILE MARK to work as expected.
 */
#define WTBOTFF	30		/* A real bug in this stupid device */

/*
 * Error/status bits
 */
#define WT_ERRFLAG	0x80	/* if present shows that info in byte
				 * is valid */
#define WT_ERRMASK	0x7f

#define WTE_FIL		0x1	/* file mark detected */
#define WTE_BNL		0x2	/* block not located */
#define WTE_UDA		0x4	/* unrecoverable data error */
#define WTE_EOM		0x8	/* end of media */
#define WTE_WRP		0x10	/* write protected */
#define WTE_USL		0x20	/* unselected drive */
#define WTE_CNI		0x40	/* cartridge not in place */
#define WTE_DEAD	0x80	/* cannot get status from controller */
#define WTE_POR		0x100	/* power-on/reset occured */
#define WTE_ERM		0x200	/* end of recorded media */
#define WTE_BOM		0x800	/* beginning of media */
#define WTE_MBD		0x1000	/* marginal block detected */
#define WTE_NDT		0x2000	/* no data detected */
#define WTE_ILL		0x4000	/* illegal command */

/* hard errors */
#define WTE_HARD  (WTE_DEAD|WTE_CNI|WTE_USL|WTE_ERM|WTE_UDA|WTE_BNL|WTE_NDT|WTE_ILL)
