/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI $Id: if_epreg.h,v 1.1 1992/09/29 00:31:34 karels Exp $
 */

/*
 * Definitions for 3Com Corp. Etherlink Plus (3C505) Ethernet Adapter
 */

/*
 * Checks for validity of i/o ports
 */
#define EP_IOBASEVALID(b)	(((b) & ~0x3f0) == 0)
#define EP_IRQVALID(i)		(0xdef8 & (1<<(i)))
#define EP_DRQVALID(d)		(0xea & (1<<(d)))

#define EP_NPORTS		8

/* Host-accessible registers */
#define EP_CMD		0	/* Command Register */
#define EP_STAT		2	/* Status Register (R) */
#define EP_AUXDMA	2	/* Aux DMA Register (W) */
#define EP_DATA		4	/* Data Register */
#define EP_CTL		6	/* Control Register */

/*
 * Host Status Register (EP_STAT) bits
 */
#define EPS_ASF1	0x01	/* Adapter Status Flag 1 */
#define EPS_ASF2	0x02	/* Adapter Status Flag 2 */
#define EPS_ASF3	0x04	/* Adapter Status Flag 3 */
#define EPS_DONE	0x08	/* DMA Done */
#define EPS_DIR		0x10	/* Data Transfer Direction (1 to the host) */
#define EPS_ACRF	0x20	/* Adapter Command Register Full */
#define EPS_HCRE	0x40	/* Host Command Register Empty */
#define EPS_HRDY	0x80	/* Data Register Ready */

/*
 * Host AUX DMA Register (EP_AUXDMA) bits
 */
#define EPS_BRST	0x01	/* DMA Burst Mode */

/*
 * Host Control Register (EP_CTL) bits
 */
#define EPC_HSF1	0x01	/* Host Status Flag 1 */
#define EPC_HSF2	0x02	/* Host Status FLag 2 */
#define EPC_CMDE	0x04	/* Command Register Interrupt Enable */
#define EPC_TCEN	0x08	/* Terminal Count Interrupt Enable */
#define EPC_DIR		0x10	/* Data Transfer Direction (1 to the host) */
#define EPC_DMAE	0x20	/* DMA Enable */
#define EPC_FLUSH	0x40	/* Flush Data Register */
#define EPC_ATTN	0x80	/* Attention */
#define EPC_RESET	0xc0	/* Hard Reset */

/*
 * Host to Adapter Commands
 */
#define EPHC_CAM	0x01	/* Configure Adater Memory */
#define EPHC_C82586	0x02	/* Configure 82586 */
#define EPHC_GADDR	0x03	/* Get Station Address */
#define EPHC_DDMA	0x04	/* Download Data to Adapter using DMA */
#define EPHC_UDMA	0x05	/* Upload Data to Host using DMA */
#define EPHC_DPIO	0x06	/* Download Data to Adapter using PIO */
#define EPHC_UPIO	0x07	/* Upload Data to Host using PIO */
#define EPHC_RCV	0x08	/* Receive a packet */
#define EPHC_TRANS	0x09	/* Transmit a packet */
#define EPHC_NETSTAT	0x0a	/* Get Netwosk Statistics */
#define EPHC_LML	0x0b	/* Load Multicast List */
#define EPHC_CDP	0x0c	/* Clear Downloaded Programs */
#define EPHC_DPROG	0x0d	/* Download Program */
#define EPHC_EXEC	0x0e	/* Execute Program */
#define EPHC_TEST	0x0f	/* Self-Test */
#define EPHC_SADDR	0x10	/* Set Station Address */
#define EPHC_AI		0x11	/* Get Adapter Information */

/*
 * Adapter to Host Commands
 */
#define EPAC_CAM	0x31	/* Configure Adapter Memory responce */
#define EPAC_C82586	0x32	/* Configure 82586 responce */
#define EPAC_GADDR	0x33	/* Get Station Address responce */
#define EPAC_DDR	0x34	/* DMA Download Data (to the adapter) Request */
#define EPAC_UDR	0x35	/* DMA Upload Data (to the host) Request */
#define EPAC_RCV	0x38	/* Receive Packet Complete */
#define	EPAC_TRANS	0x39	/* Transmit Packet Complete */
#define EPAC_NETSTAT	0x3a	/* Get Network Statistic responce */
#define EPAC_LML	0x3b	/* Load Multicast List responce */
#define EPAC_CDP	0x3c	/* Clear Downloaded Programs responce */
#define EPAC_DPROG	0x3d	/* Download Program responce */
#define EPAC_EXEC	0x3e	/* Execute Program responce */
#define EPAC_TEST	0x3f	/* Self-Test responce */
#define EPAC_SADDR	0x40	/* Set Station Address responce */
#define EPAC_AI		0x41	/* Adapter Info responce */

/*
 * Status Flags Usage Conventions
 */
#define EPSF_ZERO	0	/* No status pending */
#define EPSF_ACCEPT	1	/* PCB accepted */
#define EPSF_REJECT	2	/* PCB rejected */
#define EPSF_EOPCB	3	/* End Of PCB */
#define EPSF_MASK	3	/* Bit mask for status flags */

/*
 * Receive/loopback modes 
 */
#define EPMO_SONLY	0x00	/* Accept station address only */
#define EPMO_AB		0x01	/* Accept broadcasts */
#define EPMO_AM		0x02	/* Accept multicasts */
#define EPMO_PROMISC	0x04	/* Promiscuous mode */
#define EPMO_NOLOOP	0x00	/* No loopback */
#define EPMO_ILOOP	0x08	/* 82586 internal loopback */
#define EPMO_ELOOP	0x10	/* 82586 external loopback */

/*
 * PCB formats
 */
union ep_pcb {
	/* Generic */
	struct {
		u_char	cmd;	/* Command Code */
		u_char	len;	/* Lengths of Data Portion of PCB */
		u_char	data[62]; /* Arbitrary data */
	} g;
	
	/* EPHC_CAM */
	struct {
		u_char	cmd;	/* Command Code */
		u_char	len;	/* Lengths of Data Portion of PCB */
		u_short	ncpcb;	/* command PCB queue entries */
		u_short nrpcb;	/* receive PCB queue entries */
		u_short nma;	/* number of multicast addresses */
		u_short nfdes;	/* number of frame descriptors */
		u_short nrcvb;	/* number of receive buffers */
		u_short	ndprog;	/* number of download programs */
	} cam;

	/* EPHC_C82586 */
	struct {
		u_char	cmd;	/* Command Code */
		u_char	len;	/* Lengths of Data Portion of PCB */
		u_short mode;	/* receive/loopback mode (see before) */
	} c82586;

	/* EPHC_DDMA, EPHC_UDMA, EPHC_DPIO, EPHC_UPIO, EPAC_DDR, EPAC_UDR */
	struct {
		u_char	cmd;	/* Command Code */
		u_char	len;	/* Lengths of Data Portion of PCB */
		u_short blen;	/* data block length (must be even) */
		u_short off;	/* adapter source/dest offset */
		u_short segm;	/* adapter source/dest segment */
	} data;

	/* EPHC_RCV */
	struct {
		u_char	cmd;	/* Command Code */
		u_char	len;	/* Lengths of Data Portion of PCB */
		u_short off;	/* offset of the host receive buffer */
		u_short segm;	/* segment of the host receive buffer */
		u_short blen;	/* length of the host receive buffer */
		u_short timo;	/* timeout in 10ms increments */
	} rcv;

	/* EPHC_TRANS */
	struct {
		u_char	cmd;	/* Command Code */
		u_char	len;	/* Lengths of Data Portion of PCB */
		u_short off;	/* offset of the host transmit buffer */
		u_short segm;	/* segment of the host transmit buffer */
		u_short plen;	/* length of the packet (must be even) */
	} trans;

	/* EPHC_LML */
	struct {
		u_char	cmd;	/* Command Code */
		u_char	len;	/* Lengths of Data Portion of PCB */
		u_char	ma[1][6]; /* the flex array of multicast addresses */
	} lml;

	/* EPHC_DPROG */
	struct {
		u_char	cmd;	/* Command Code */
		u_char	len;	/* Lengths of Data Portion of PCB */
		u_short	plen;	/* length of program in bytes */
	} dprog;

	/* EPHC_EXEC */
	struct {
		u_char	cmd;	/* Command Code */
		u_char	len;	/* Lengths of Data Portion of PCB */
		u_short	pid;	/* program id */
		u_short args[1]; /* the flex array of program parameters */
	} exec;

	/* EPHC_SADDR, EPAC_GADDR */
	struct {
		u_char	cmd;	/* Command Code */
		u_char	len;	/* Lengths of Data Portion of PCB */
		u_char	addr[6]; /* station address */
	} saddr;

	/* EPAC_CAM, EPAC_C82586, EPAC_LML, EPAC_SADDR */
	struct {
		u_char	cmd;	/* Command Code */
		u_char	len;	/* Lengths of Data Portion of PCB */
		u_short	res;	/* 0 if success */
	} resp;

	/* EPAC_RCV */
	struct {
		u_char	cmd;	/* Command Code */
		u_char	len;	/* Lengths of Data Portion of PCB */
		u_short	off;	/* offset of host receive buffer */
		u_short segm;	/* segment of host receive buffer */
		u_short dmalen;	/* number of bytes to be DMA'ed */
		u_short pklen;	/* actual packet length */
		u_short res;	/* 0 if success (-1 if timeout) */
		u_short	rxstat;	/* 82586 receive status */
		u_short	time0;	/* time tag in 15ms ticks */
		u_short time1;	/* high bits of the time tag */
	} rrcv;

	/* EPAC_TRANS */
	struct {
		u_char	cmd;	/* Command Code */
		u_char	len;	/* Lengths of Data Portion of PCB */
		u_short off;	/* offset of host transmission buffer */
		u_short segm;	/* segment of host transmission buffer */
		u_short res;	/* 0 if success, -1 Xmit fail, -2 DMA timo */
		u_short txstat;	/* 82586 transmit status */
	} rtrans;

	/* EPAC_NETSTAT */
	struct {
		u_char	cmd;	/* Command Code */
		u_char	len;	/* Lengths of Data Portion of PCB */
		u_short	icnt0;	/* total received packets (low) */
		u_short	icnt1;	/* total received packets (high) */
		u_short	ocnt0;	/* total transmitted packets (low) */
		u_short	ocnt1;	/* total transmitted packets (high) */
		u_short ecrc;	/* CRC errors */
		u_short	ealign;	/* alignment errors */
		u_short eres;	/* no resources errors */
		u_short eover;	/* overruns */ 
	} netstat;

	/* EPAC_CDP */
	struct {
		u_char	cmd;	/* Command Code */
		u_char	len;	/* Lengths of Data Portion of PCB */
		u_short	avail;	/* amount of available memory in paragraphs */
	} cdp;

	/* EPAC_DPROG */
	struct {
		u_char	cmd;	/* Command Code */
		u_char	len;	/* Lengths of Data Portion of PCB */
		u_short	pid;	/* program id (>0 if allocated) */
		u_short off;	/* programm offset in adapter memory */
		u_short	segm;	/* program segment ib adapter memory */
		u_short avail;	/* remaining memory in paragraphs */
	} rdprog;

	/* EPAC_EXEC */
	struct {
		u_char	cmd;	/* Command Code */
		u_char	len;	/* Lengths of Data Portion of PCB */
		u_short pid;	/* program id (-1 if bad pid in request) */
		u_short ret[1];	/* returned values */
	} rexec;

	/* EPAC_TEST */
	struct {
		u_char	cmd;	/* Command Code */
		u_char	len;	/* Lengths of Data Portion of PCB */
		u_short	res;	/* test status */
		u_short diag[1]; /* diagnostic data */
	} test;

	/* EPAC_AI */
	struct {
		u_char	cmd;	/* Command Code */
		u_char	len;	/* Lengths of Data Portion of PCB */
		u_short rev;	/* ROM revision */
		u_short chksum;	/* checksum value */
		u_short memsiz;	/* memory size in Kb */
		u_short avoff;	/* free memory offset */
		u_short avsegm;	/* free memory segment */
	} ai;
};

/*
 * PCB Data field length
 */
#define PCBARGSIZ(type)	(sizeof ((union ep_pcb *)0)->type - 2)
