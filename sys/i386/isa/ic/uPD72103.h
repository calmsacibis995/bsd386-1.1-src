/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: uPD72103.h,v 1.1 1992/07/28 03:20:15 karels Exp $
 */

/*
 * Definitions for uPD-72103 HDLC controller chip
 */

/*
 * History:
 *	6/12/92 [avg]	initial revision
 */

/*
 * Sizes of command/status/receive blocks (bytes)
 */
#define	uPD_LCW_SIZE	sizeof(struct upd_cmd)
#define	uPD_LSW_SIZE	sizeof(struct upd_stat)
#define	uPD_LRBW_SIZE	sizeof(struct upd_rbw)

/*
 * 72103 registers
 */
#define	uPD_CTL		0	/* Control Register (write) */
#define	uPD_STAT	0	/* Status Register (read) */
#define	uPD_MSET_START	1	/* MSET Start Register (write) */
#define	uPD_MSET_CMD	3	/* MSET Command Register (write) */

/* 
 * Control Register Bits
 */
#define	uPD_CTL_CRQ	0x01	/* Command Request */
#define	uPD_CTL_RST	0x02	/* Reset */
#define	uPD_CTL_CLRINT	0x08	/* Clear INT */

/*
 * Status Register Bits
 */
#define	uPD_STAT_FRDY	0x02	/* FIFO Ready */
#define	uPD_STAT_INTS	0x10	/* INT Status */
#define	uPD_STAT_CRQURDY 0x20	/* Command Request Unready */

/*
 * MSET Start Register
 */
#define	uPD_MSET_MAGIC	5	/* Magic value to write in MSET_START */

/*
 * uPD72103 Commands
 */
#define	uPD_DTSD	0x31	/* Data Send */
#define	uPD_MSET	0x34	/* Memory Area Set */
#define	uPD_MDST	0x35	/* Operation Mode Set */
#define	uPD_AFST	0x36	/* Receive Addres Field Set */
#define	uPD_LOPN	0x37	/* Line Open */
#define	uPD_LCLS	0x38	/* Line Close */
#define	uPD_MARD	0x3a	/* Memory Area Read */
#define	uPD_MDRD	0x3b	/* Operation Mode Read */
#define	uPD_AFRD	0x3c	/* Receive Address Field Read */
#define	uPD_SIRD	0x3d	/* Statistical Information Read */
#define	uPD_GOWR	0x41	/* General-purpose Output Pin Write */
#define	uPD_GPRD	0x42 	/* General-purpose I/O Pin Read */
#define	uPD_SIRE	0x44	/* Statistical Information Read II */
#define	uPD_GPRE	0x45	/* General-purpose I/O Pin Read */
#define	uPD_MDSE	0x46	/* Operation Mode Set II */
#define	uPD_AFSE	0x47	/* Receive Address Field Set II */

/*
 * Generic command structure
 */
struct upd_cmd {
	u_char	cmd;	/* command code */
	u_char	zero;	/* 0 for command, 0xff for available */
	u_char	args[14];	/* command arguments */
};

/*
 * Data Send
 */
struct upd_dtsd {
	u_char	cmd;	/* uPD_DTSD = 0x31 */
	u_char	zero;	/* should be 0 */
	u_char	xbc;	/* extra byte counters */
	u_char	bc0;	/* byte counter L */
	u_char	bc1;	/* byte counter H */
	u_char	bufa0;	/* buffer address L */
	u_char	bufa1;	/* buffer address M */
	u_char	bufa2;	/* buffer address H */
	u_char	txdt[8]; /* in-command transmit data */
};

/* upd_dtsd.xbc bits */
#define	uPD_DTSD_TXBC(n) (n)		/* number of in-command transmit bytes */
#define	uPD_DTSD_FRBC(n) ((n)<<4)	/* number of fraction bits */
#define	uPD_DTSD_CB	0x80		/* chain transmit buffer */

/*
 * Memory Area Set
 */
struct upd_mset {
	u_char	cmd;	/* uPD_MSET = 0x34 */
	u_char	zero;	/* should be 0 */
	u_char	addr0;	/* table base address L */
	u_char	addr1;	/* table base address M */
	u_char	addr2;	/* table base address H */
	u_char	nlcw;	/* number of command blocks */
	u_char	nlsw;	/* number of status blocks */
	u_char	nlrbw;	/* number of receive buffers */
};

/*
 * Operation Mode Set
 */
struct upd_mdst {
	u_char	cmd;	/* uPD_MDST = 0x35 */
	u_char	zero;	/* should be 0 */
	u_char	m0;	/* mode bits 0 */
	u_char 	m1;	/* mode bits 1 */
	u_char	m2;	/* mode bits 2 */
	u_char	stbc;	/* number of received bytes to store in status */
	u_char	time;	/* number of idle time detections */
	u_char	rxbs0;	/* receive buffer size L */
	u_char	rxbs1;	/* receive buffer size H */
	u_char	maxd0;	/* max number of receive data bytes L */
	u_char	maxd1;	/* max number of receive data bytes H */
	u_char	hold;	/* time to hold data in FIFO (x 4usec) */
	u_char	retn;	/* number of retransmission on underrun */
};

/* upd_mdst.m0 bits */
#define	uPD_M0_CPU	0x01	/* address type (1=big-endian) */
#define	uPD_M0_DMAB	0x02	/* DMA block size (0=4/1=8 bytes) */
#define	uPD_M0_DMAW	0x0c	/* DMA wait cycles */
#define	    uPD_M0_DMAW0	0x00	/* no wait cycles */
#define	    uPD_M0_DMAW1	0x04	/* 1 wait cycle */
#define	    uPD_M0_DMAW2	0x08	/* 2 wait cycles */
#define	    uPD_M0_DMAW3	0x0c	/* 3 wait cycles */
#define	uPD_M0_CLK	0x10	/* tx/rx clock source select */
#define	uPD_M0_CODE	0x20	/* data format code (0=NRZ, 1=NRZI) */
#define	uPD_M0_TFIL	0x40	/* flag transmission (0)/mark transmission (1) */
#define	uPD_M0_FCS	0x80	/* frame checksum size (0=16bit/1=32bit) */

/* upd_mdst.m1 bits */
#define	uPD_M1_AUTO	0x03	/* automatic recognition of frame address */
#define	    uPD_M1_NOADR	0x00	/* no address recognition */
#define	    uPD_M1_ADR1_1	0x01	/* compare 1st byte only */
#define	    uPD_M1_ADR1_2	0x02	/* compare 2nd byte only */
#define	    uPD_M1_ADR2		0x03	/* 2-byte address */
#define	uPD_M1_SHORT	0x0c	/* "short" frame length */
#define	    uPD_M1_SHORT2	0x00	/* under 2 bytes */
#define	    uPD_M1_SHORT3	0x04	/* under 3 bytes */
#define	    uPD_M1_SHORT4	0x08	/* under 4 bytes */
#define	uPD_M1_OCT	0x10	/* prohibit receiption fraction byte frames */
#define	uPD_M1_STEP	0x20	/* watchdog timer step (0=100ms/1=8ms) */
#define	uPD_M1_LOOP	0xc0	/* loopback mode */
#define	    uPD_M1_NOLOOP	0x00	/* no loopback */
#define	    uPD_M1_LOOP1	0x40	/* internal loopback, transmit outside */
#define	    uPD_M1_LOOP2	0x80	/* internal loopback, transmit 1s */
#define	    uPD_M1_LOOP3	0xc0	/* RXD->TXD + isolated internal loopback */

/* upd_mdst.m2 bits */
#define	uPD_M2_LOAK	0x01	/* line open ACK status report timing */
#define	uPD_M2_TXED	0x02	/* report data transmission end */
#define	uPD_M2_GICS	0x0c	/* report genereal-purpose input pin change: */
#define	    uPD_M2_NOGICS	0x00	/* do not report */
#define	    uPD_M2_GICS1	0x04	/* report changes on pin 1 */
#define	    uPD_M2_GICS2	0x08	/* report changes on both pins */
#define	uPD_M2_BUFC	0x10	/* do receive buffer chaining */
#define	uPD_M2_BUFE	0x20	/* do transmit buffer chaining */

/* upd_mdst.retn bits */
#define	uPD_MDST_RETN	0x7f	/* number of retransmissions on underrun */
#define	uPD_MDST_TXUR	0x80	/* report the "data transmission suspension" status */

/*
 * Address Field Set
 */
struct upd_afst {
	u_char	cmd;	/* uPD_AFST = 0x36 */
	u_char	zero;	/* should be 0 */
	u_char 	bc;	/* number of bytes in addr info */
	u_char	af[12];	/* addresses */
};

/*
 * Line Open
 */
struct upd_lopn {
	u_char	cmd;	/* uPD_LOPN = 0x37 */
	u_char	zero;	/* should be 0 */
};

/*
 * Line Close
 */
struct upd_lcls {
	u_char	cmd;	/* uPD_LCLS = 0x38 */
	u_char	zero;	/* should be 0 */
};

/*
 * Memory Area Read
 */
struct upd_mard {
	u_char	cmd;	/* uPD_MARD = 0x3a */
	u_char	zero;	/* should be 0 */
};

/*
 * Operation Mode Read
 */
struct upd_mdrd {
	u_char	cmd;	/* uPD_MDRD = 0x3b */
	u_char	zero;	/* should be 0 */
};

/*
 * Receive Address Field Read
 */
struct upd_afrd {
	u_char	cmd;	/* uPD_AFRD = 0x3c */
	u_char	zero;	/* should be 0 */
};

/*
 * Statistical Information Read
 */
struct upd_sird {
	u_char	cmd;	/* uPD_SIRD = 0x3d */
	u_char	zero;	/* should be 0 */
};

/*
 * General-purpose Output Pin Write
 */
struct upd_gowr {
	u_char 	cmd;	/* uPD_GOWR = 0x41 */
	u_char	zero;	/* should be 0 */
	u_char	go;	/* value to write */
};

/* upd_gowr.go bits */
#define	uPD_GOWR_1	0x01	/* pin 1 (0=low/1=high) */
#define	uPD_GOWR_2	0x02	/* pin 2 (0=low/1=high) */

/*
 * General-purpose I/O Pin Read
 * (read value holding for 8ms)
 */
struct upd_gprd {
	u_char	cmd;	/* uPD_GPRD = 0x42 */
	u_char	zero;	/* should be 0 */
};

/*
 * Statistical Information Read II
 */
struct upd_sire {
	u_char	cmd;	/* uPD_SIRE = 0x44 */
	u_char 	zero;	/* should be 0 */
	u_char	sin0;	/* statistical info number 0 */
	u_char	sin1;	/* statistical info number 1 */
};

/* upd_sire.sin0 and upd_sire.sin1 values */
				/* Numbers of: */
#define	uPD_SIRE_OVERRUN 	0	/* overruns generated */
#define	uPD_SIRE_UNDERRUN 	1	/* underruns generated */
#define	uPD_SIRE_IDLE		2	/* idles received */
#define	uPD_SIRE_SHORT		3	/* short frames received */
#define	uPD_SIRE_AFERR		4	/* address filed error frames received */
#define	uPD_SIRE_LONG		5	/* long frames received */
#define	uPD_SIRE_ABORT		6	/* abort frames received */
#define	uPD_SIRE_FCS		7	/* FCS error frames received */
#define	uPD_SIRE_FRACTION	8	/* fraction bit frames received */
#define	uPD_SIRE_STATOVFL	9	/* status table overflows generated */
#define	uPD_SIRE_RCVOVFL	10	/* rx buffer table overflows */

/*
 * General-purpose I/O Pin Read II
 * (read the current value)
 */
struct upd_gpre {
	u_char	cmd;	/* uPD_GPRE = 0x45 */
	u_char	zero;	/* should be 0 */
};

/*
 * Operation Mode Set II
 */
struct upd_mdse {
	u_char	cmd;	/* uPD_MDSE = 0x46 */
	u_char 	zero;	/* should be 0 */
	u_char	mode;	/* mode bits */
};

/* upd_mdse.mode bits */
#define	uPD_ME_AUTO	0x03	/* automatic recognition of frame address */
#define	    uPD_ME_NOADR	0x00	/* no address recognition */
#define	    uPD_ME_ADR1_1	0x01	/* compare 1st byte only */
#define	    uPD_ME_ADR1_2	0x02	/* compare 2nd byte only */
#define	    uPD_ME_ADR2		0x03	/* 2-byte address */
#define	uPD_ME_SHORT	0x0c	/* "short" frame length */
#define	    uPD_ME_SHORT2	0x00	/* under 2 bytes */
#define	    uPD_ME_SHORT3	0x04	/* under 3 bytes */
#define	    uPD_ME_SHORT4	0x08	/* under 4 bytes */
#define	uPD_ME_GICS	0x30	/* report genereal-purpose input pin change: */
#define	    uPD_ME_NOGICS	0x00	/* do not report */
#define	    uPD_ME_GICS1	0x10	/* report changes on pin 1 */
#define	    uPD_ME_GICS2	0x20	/* report changes on both pins */

/*
 * Receive Address Field Set II
 * (should be used after Operation Mode Set II)
 */
struct upd_afse {
	u_char	cmd;	/* uPD_AFSE = 0x47 */
	u_char	zero;	/* should be 0 */
	u_char 	bc;	/* number of bytes in addr info */
	u_char	af[12];	/* addresses */
};


/*
 * uPD72103 Status Words
 */
#define	uPD_DTRV	0x31	/* Data Receive */
#define	uPD_TXUR	0x32	/* Data Transmit Interrupt */
#define	uPD_TOUT	0x33	/* Idle Watchdog Timer Timeout */
#define	uPD_LOAK	0x37	/* Line Open End */
#define	uPD_LCAK	0x38	/* Line Close End */
#define	uPD_TXED	0x39	/* Data Transmission End */
#define	uPD_MAAK	0x3a	/* Memory Area Address Read Response */
#define	uPD_MDAK	0x3b	/* Operation Mode Read Read Response */
#define	uPD_AFAK	0x3c	/* Receive Address Field Read Response */
#define	uPD_SIAK	0x3d	/* Statistical Information Read Response */
#define	uPD_CILG	0x3f	/* Command Illegal */
#define	uPD_GI1C	0x40	/* General-purpose Input Pin Change Detection I */
#define	uPD_GI2C	0x41	/* General-purpose Input Pin Change Detection II */
#define	uPD_GPAK	0x42	/* General-purpose I/O Pin Read Response */
#define	uPD_OLSW	0x43	/* Status Table Overflow */
#define	uPD_SIAF	0x44	/* Statistical Information Read Response II */
#define	uPD_GPAE	0x45	/* General-purpose I/O Pin Read Response II */

/*
 * General status
 */
struct upd_stat {
	u_char	stsn;	/* status number */
	u_char	res[15]; /* result bits */
};

/*
 * Data Receive
 */
struct upd_dtrv {
	u_char	stsn;	/* uPD_DTRV = 0x31 */
	u_char	unused;
	u_char	xbc;	/* extra byte counters */
	u_char	bc0;	/* byte count L */
	u_char	bc1;	/* byte count H */
	u_char	bufa0;	/* buffer address L */
	u_char	bufa1;	/* buffer address H */
	u_char	bufa2;	/* buffer address M */
	u_char	rxdt[8]; /* in-status received data */
};

/* upd_dtrv.xbc bits */
#define	uPD_DTRV_RXBC(n) ((n) & 0xf)	/* number of in-status received bytes */
#define	uPD_DTRV_FRBC(n) (((n)>>4)&07)	/* number of fraction bits */
#define	uPD_DTRV_CB	0x80		/* receive buffer chained */

/*
 * Data Transmission Terminated
 */
struct upd_txur {
	u_char	stsn;	/* uPD_TXUR = 0x32 */
	u_char	txur;	/* number of terminated transmit cmds */
};

/*
 * Data Transmission End
 */
struct upd_txed {
	u_char	stsn;	/* uPD_TXED = 0x39 */
	u_char	txen;	/* number of completed transmit cmds */
};

/*
 * Memory Area Read Response
 */
struct upd_maak {
	u_char	stsn;	/* uPD_MAAK = 0x3a */
	u_char	unused;
	u_char	addr0;	/* base address L */
	u_char	addr1;	/* base address M */
	u_char	addr2;	/* base address H */
	u_char	nlcw;	/* number of command words */
	u_char	nlsw;	/* number of status words */
	u_char	nlrbw;	/* number of receive buffers */
	u_char	vlsiver; /* chip version */
	u_char	romver;	/* ROM version */
	u_char	mon;	/* month of release */
	u_char	year;	/* year of release */
};

/*
 * Operation Mode Read Response
 */
struct upd_mdak {
	u_char	stsn;	/* uPD_MDAK = 0x3b */
	u_char	unused;
	u_char	m0;	/* mode bits 0 */
	u_char 	m1;	/* mode bits 1 */
	u_char	m2;	/* mode bits 2 */
	u_char	stbc;	/* number of received bytes to store in status */
	u_char	time;	/* number of idle time detections */
	u_char	rxbs0;	/* receive buffer size L */
	u_char	rxbs1;	/* receive buffer size H */
	u_char	maxd0;	/* max number of receive data bytes L */
	u_char	maxd1;	/* max number of receive data bytes H */
	u_char	hold;	/* time to hold data in FIFO (x 4usec) */
	u_char	retn;	/* number of retransmission on underrun */
};

/* upd_mdak.m0 m1 m2 retn bits are the same as in MDST */

#define	uPD_MDAK_STBC	0xf	/* STBC mask */

/*
 * Receive Address Field Read Response
 */
struct upd_afak {
	u_char	stsn;	/* uPD_AFAK = 0x3c */
	u_char	unused;
	u_char 	bc;	/* number of bytes in addr info */
	u_char	af[12];	/* addresses */
};

#define	uPD_AFAK_BC	0xf	/* Mask for BC */

/*
 * Statistical Information Read Response
 */
struct upd_siak {
	u_char	stsn;	/* uPD_SIAK = 0x3d */
	u_char	count;	/* nr of errors in consequent fields */
	u_char	ovrn;	/* number of rx overruns */
	u_char	unrn;	/* number of tx underruns */
	u_char	idle;	/* number of idle states detected */
	u_char	short;	/* number of short packets recd */
	u_char	addr;	/* number of address mismatches */
	u_char	long;	/* number of too long packets recd */
	u_char	abort;	/* number of abort patterns received */
	u_char	fcs;	/* number of frames with FCS errs */
	u_char	frac;	/* number of fractal-byte frames recd */
	u_char	flsw;	/* receive status table overflows */
	u_char	flrbw;	/* rx frames discarded due no space */
};

/*
 * Command Illegal
 */
struct upd_cilg {
	u_char	stsn;	/* uPD_CILG = 0x3f */
	u_char	ilst;	/* why command is illegal */
	u_char	lstn;	/* when command became illegal */
	u_char	lcwn;	/* command block # */
	u_char	cmdn;	/* command number */
};

/* upd_cilg.ilst values */
#define	uPD_CILG_UNDEF	0	/* Undefined command number */
#define	uPD_CILG_PARM	1	/* Parameter error */
#define	uPD_CILG_STAT	2	/* Status mismatch */

/* upd_cilg.lstn values */
#define	uPD_CILG_IDLE	0	/* Idle state */
#define	uPD_CILG_FLAG	1	/* FLag receive wait state */
#define	uPD_CILG_DATA	2	/* Data transission state */

/*
 * General-purpose Input/Output Pin Status:
 *	common for GI1C, GI2C, GPAK, GPAE
 */
struct upd_gpio {
	u_char	stsn;	/* uPD_GI1C = 0x40 */
			/* uPD_GI2C = 0x41 */
			/* uPD_GPAK = 0x42 */
			/* uPD_GPAE = 0x45 */
	u_char	pins;	/* pin states */
};

/* upd_gpio.pins bits */
#define	uPD_GPIO_O1	0x01	/* Output 1 (0=low/1=high) */
#define	uPD_GPIO_O2	0x02	/* Output 2 (0=low/1=high) */
#define	uPD_GPIO_I1	0x04	/* Input 1 (0=low/1=high) */
#define	uPD_GPIO_I2	0x08	/* Input 2 (0=low/1=high) */
#define	uPD_GPIO_IDLE	0x10	/* 1=idle detected on RxD */
#define	uPD_GPIO_SYNC	0x20	/* 1=receiving flag */

/*
 * Status Table Overflow
 */
struct upd_olsw {
	u_char	stsn;	/* uPD_OLSW = 0x43 */
	u_char	pins;	/* GO/GI pin states, same as in GPIO */
	u_char	lstn;	/* when the table overflowed, same as in CILG */
};

/*
 * Statistical Information Read Response II
 */
struct upd_siaf {
	u_char	stsn;	/* uPD_SIAF = 0x44 */
	u_char	sin0;	/* stat info 0 */
	u_char	sin1;	/* stat info 1 */
};


/*
 * Receive buffer table entry
 */
struct upd_rbw {
	u_char	brdy;	/* status field */
	u_char	bufa0;	/* buffer address L */
	u_char	bufa1;	/* buffer address M */
	u_char	bufa2;	/* buffer address H */
};
