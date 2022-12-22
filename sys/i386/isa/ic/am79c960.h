/*	BSDI $Id: am79c960.h,v 1.1 1994/01/06 02:14:09 karels Exp $	*/

/*
 * i386/isa/ic/am79c960.h by South Coast Computing Services, Inc.
 * Released without restrictions.
 */


/*
 * Am79C960 registers.
 *
 * The I/O space is 24 bytes long.  The first 16 bytes are
 * the address PROM, followed by registers comprising windows
 * into the logical csrN register set of the modified LANCE core.
 *
 *	TNIO_* macros are offsets from the base address of
 *	the card in ISA bus I/O space.
 */

#define TNIO_APROM	0	/* base of address PROM */
#define TNIO_RDP	16	/* CSR register r/w window */
#define TNIO_RAP	18	/* reg addr ptr for RDP and IDP */
#define TNIO_RESET	20	/* READING this resets board! */
#define TNIO_IDP	22	/* ISA config register r/w window */

/*
 *	The VSW is documented by AMD but not implemented on the TNIC1500.
 *	The TNIC card do not respond to addresses beyond base+23, so
 *	you should read 0xffff if you try it.
 */

#define TNIO_VSW	24	/* Vendor-Specific Word */

/*
 *	There are two logical address spaces *within* the chip.
 *	They share the "address pointer" register RAP, with
 *	read/write access through IDP and RDP.  Strange, but
 *	you get used to it.  The IDP set isn't used very much.
 */

/*
 * ISA config registers -- select with RAP and access thru IDP
 *
 *	The TNIRA_* macros are RAP chip-internal address space
 *	selector values -- write them to base+TNIO_RAP to enable
 *	read/write access to the corresponding register through
 *	base+TNIO_IDP.
 */

#define TNIRA_MSRDA	0	/* MEMR signal timing, 50 ns units, 5 dflt */
#define TNIRA_MSWRA	1	/* MEMW signal timing, 50 ns units, 5 dflt */
#define TNIRA_MAU	2	/* MAU config, see below */
#define TNIRA_LED(n)	(4+n)	/* config for LED 1-3, see below */

/*
 * MAU configuration register bits
 */

#define TNMAU_EADISEL	0x0008	/* Enable EADI (ext add match) port (NOT!) */
#define TNMAU_AWAKE	0x0004	/* Auto-Wake -- unsleep on link pulse */
#define TNMAU_ASEL	0x0002	/* Auto Select the active media */
#define TNMAU_XMAUSEL	0x0001	/* H/W selection of media -- TNIC1500 */

/*
 * LED activity mask bits -- the five conditions are masked (ANDed
 * with the corresponding mask word bit) then ORed together to form
 * the TNLED_OUT bit.  The optional pulse stretcher adds 50-75 ms if
 * enabled.  That output lights the LED.
 */

#define TNLED_OUT	0x8000	/* read-only monitor instantaneous state */
#define TNLED_PSE	0x0080	/* enable pulse stretcher */
#define TNLED_XMT	0x0010	/* indicate transmitter activity */
#define TNLED_RXPOL	0x0008	/* indicate Rx polarity reversal */
#define TNLED_RCV	0x0004	/* indicate receiver (network) activity */
#define TNLED_JAB	0x0002	/* indicate jabber detected */
#define TNLED_COL	0x0001	/* indicate collision on network */



/*
 * LANCE CSR registers -- select with RAP and access thru RDP
 *
 *	The TNRA_* macros are RAP chip-internal address space
 *	selector values -- write them to base+TNIO_RAP to enable
 *	read/write access to the corresponding register through
 *	base+TNIO_RDP.
 */


/*
 * Control/status register (CSR0).
 *
 * Latched status bits are acked (cleared) individually by writing 1s;
 * writing zeros to them is a no-op, except as noted.
 */

#define TNRA_CSR	0

#define TNCSR_ERR	0x8000	/* r/o internal OR of error conditions */
#define TNCSR_BABL	0x4000	/* Babble error */
#define TNCSR_CERR	0x2000	/* Collision error (tx pkt discarded) */
#define TNCSR_MISS	0x1000	/* Missed frame (ring empty) */
#define TNCSR_MERR	0x0800	/* Memory error (no DACK) */
#define TNCSR_RINT	0x0400	/* Rx interrupt pending (pre mask) */
#define TNCSR_TINT	0x0200	/* Tx interrupt pending (pre mask) */
#define TNCSR_IDON	0x0100	/* Initialization done interrupt */
#define TNCSR_INTR	0x0080	/* OR of irq sources after mask */
#define TNCSR_IENA	0x0040	/* master irq enable - read/write */
#define TNCSR_RXON	0x0020	/* Rxr enabled (read only) */
#define TNCSR_TXON	0x0010	/* Txr enabled (read only) */
#define TNCSR_TDMD	0x0008	/* write 1 to force poll of Tx ring */
#define TNCSR_STOP	0x0004	/* write 1 to disable the board */
#define TNCSR_STRT	0x0002	/* write 1 to clear stop and reenable */
#define TNCSR_INIT	0x0001	/* write 1 to init from RAM */

/*
 * IADR -- initialization block (physical!) address register
 *
 * A 24 bit value, zero bits 24-31 (HI half of IAHI).
 */

#define TNRA_IALOW	1
#define TNRA_IAHI	2

struct tn_iblock
{
	u_short	mode;		/* load val for mode reg, CSR15 */
	u_char	enaddr[6];	/* ethernet address */
	u_short	laddr[4];	/* multicast filter - not used */

	/* Note: rings must be 8-byte aligned */

	u_long	rdra : 24;	/* physical address of Rx ring base */
	u_long	padr : 5;	/* C structure padding */
	u_long	rlen : 3;	/* coded Rx ring length */

	u_long	tdra : 24;	/* physical address of Tx ring base */
	u_long	padt : 5;	/* C structure padding */
	u_long	tlen : 3;	/* coded Tx ring length */
};

/*
 *	The rlen and tlen fields of the iblock encode a power-of-2
 *	ring size.  You can override that by writing to some of the
 *	CSRs after initialization, but we don't support that.
 */

#define TNIB_LEN1	0
#define TNIB_LEN2	1
#define TNIB_LEN4	2
#define TNIB_LEN8	3
#define TNIB_LEN16	4
#define TNIB_LEN32	5
#define TNIB_LEN64	6
#define TNIB_LEN128	7
#define TNIB_LEN(n)	(TNIB_LEN##n)	/* used in tn_ifreg.h */

/*
 * IM - interrupt masks and medium arbitration control (CSR3)
 *
 * Generally writable only when TNSR_STOP is set.  1 masks
 * the interrupt source, 0 enables it.
 */

#define TNRA_IM		3

/* interrupt masks correspond to interrupt status bits */
#define TNIM_BABL	TNCSR_BABL	/* babble mask */
#define TNIM_MISS	TNCSR_MISS	/* missed frame mask */
#define TNIM_MERR	TNCSR_MERR	/* memory error mask */
#define	TNIM_RINT	TNCSR_RINT	/* Rx interrupt mask */
#define TNIM_TINT	TNCSR_TINT	/* Tx interrupr mask */
#define TNIM_IDON	TNCSR_IDON	/* init done mask */

/*
 * deferral bits control control various flavors of CSMA/CD
 * collision deferral and backoff algorithms.  See the datasheet.
 */

#define TNIM_DXMT2PD	0x0010	/* disable "2 part" deferral */
#define TNIM_EMBA	0x0008	/* enable modified backoff algorithm */

/*
 * TEST and Features control register (CSR4)
 */

#define TNRA_TF		4

#define TNTF_ENTST	0x8000	/* enable test mode.  let's not. */
#define TNTF_DMAPLUS	0x4000	/* disable DMA transaction counter */
#define TNTF_TIMER	0x2000	/* enable TIMER (CSR82) */
#define TNTF_DPOLL	0x1000	/* disable Tx ring polling */
#define TNTF_APADX	0x0800	/* enable automatic padding */
#define TNTF_ASTRP	0x0400	/* enable auto stripping of padding */
#define TNTF_MPCO	0x0200	/* miss counter overflow (w 1 to clr) */
#define TNTF_MPCOM	0x0100	/* MCO interrupt mask bit */
#define TNTF_RCVCCO	0x0020	/* Rx collision counter overflow (w1 clr) */
#define TNTF_RCVCCOM	0x0010	/* CCO interrupt mask bit */
#define TNTF_TXSTRT	0x0008	/* set when xmission starts */
#define TNTF_TXSTRTM	0x0004	/* irq mask bit for TXSTRT */
#define TNTF_JAB	0x0002	/* Jabber det by TMAU (w1 clr) */
#define TNTF_JABM	0x0001	/* irq mask bit for JAB */

/*
 * MODE register (CSR15)
 * 
 * Mostly writable only when STOP is set.  Note that the mode
 * register is loaded from the iblock at INIT time.
 */

#define TNRA_MODE	15

#define TNMD_PROM	0x8000	/* enable promiscuous mode */
#define TNMD_DRCVBC	0x4000	/* disable reception of broadcasts */
#define TNMD_DRCVPA	0x2000	/* disable rcv physical address detect */
#define TNMD_DLNKTST	0x1000	/* disable link status monitoring */
#define TNMD_DAPC	0x0800	/* disable auto polarity correction */
#define TNMD_MENDECL	0x0400	/* loopback at manchester enc/dec */
#define TNMD_LRT	0x0200	/* UTP reduced Rx threshold enable */
#define TNMD_TSEL	0x0200	/* AUI only -- set for Ethernet 1 xcvr */
#define TNMD_SELAUI	0x0000	/* S/W select AUI port (see XMAUSEL) */
#define TNMD_SELUTP	0x0080	/* S/W select UTP port (see XMAUSEL) */
#define TNMD_INTL	0x0040	/* loopback at the "internal" point */
#define TNMD_DRTY	0x0020	/* disable retry on collision */
#define TNMD_FCOLL	0x0010	/* force coll (internal loopback only) */
#define TNMD_DXMTFCS	0x0008	/* disable Tx of CRC (for testing) */
#define TNMD_LOOP	0x0004	/* loopback enable (see loop pt selects) */
#define TNMD_DTX	0x0002	/* disable transmitter section */
#define RNMD_DRX	0x0001	/* disable receiver section */

/*
 * BURST and FIFO control register (CSR80)
 *
 * The bitfields of this word control the high- and low-water marks
 * and the burst length for the fifos.  Tuning these in relation to
 * the burst parameters of the Adaptec and leaving room for the floppy
 * to work is a delicate job.
 *
 * The low-order eight bits of this register specify the number of
 * memory cycles allowed in a dma burst.  Too low a setting can lead
 * to slow starvation in the presence of other high-rate DMA clients.
 */

#define TNRA_FIFO	80

/* number of Rx bytes before starting dma */
#define TNFI_RX16	0x0000	/* recommended */
#define TNFI_RX32	0x1000
#define TNFI_RX64	0x2000	/* default */

/* number of Tx bytes in fifo before starting xmiter */
#define TNFI_TXS4	0x0000
#define TNFI_TXS16	0x0400
#define TNFI_TXS64	0x0800	/* default */
#define TNFI_TXS112	0x0c00

/* number of Tx fifo free bytes required to initiate a burst */
#define TNFI_TXW8	0x0000	/* default */
#define TNFI_TXW16	0x0100	/* recommended */
#define TNFI_TXW32	0x0200

/*
 * DMA TIMER register (CSR82)
 *
 * This value, in 100 ns (.1 us) units, controls the DMA burst
 * maximum duration when TNTF_TIMER is set.
 */

#define TNRA_TIMER	82

/*
 *	Receive (rx) Ring descriptor blocks
 */

struct tn_rblock
{
	u_long	addr  : 24;	/* 24-bit physical address of buffer */
	u_long	flags :  8;	/* see below: TNRB_*  */
	short	nsize;		/* 0 - buffer size (i.e. 2's compl) */
	short	bcnt;		/* valid bytes in buffer */
};

/*
 *	rblock.flags values
 */

#define TNRB_OWN	0x80	/* 0=host, 1=card owns buffer */
#define TNRB_ERR	0x40	/* set by card on any problem */
#define TNRB_FRAM	0x20	/* framing error */
#define TNRB_OFLO	0x10	/* overflow (not enough buffers avail) */
#define TNRB_CRC	0x08	/* CRC error on frame */
#define TNRB_BUFF	0x04	/* buffer exchange protocol problem */
#define TNRB_STP	0x02	/* this buffer is the start of a frame */
#define TNRB_ENP	0x01	/* this buffer is the end of a frame */

/*
 *	Transmit (tx) ring descriptor blocks
 */

struct tn_tblock
{
	u_long	addr  : 24;	/* 24-bit physical address of buffer */
	u_long	flags :  8;	/* see below: TNTXF_*  */
	short	nbcnt;		/* 0 - valid bytes (i.e. 2's compl) */
	u_short	tdr   : 10;	/* reflectometry counter - not used */
	u_short	err   :  6;	/* more error bits: TNTXE_*  */
};

#define TNTXF_OWN	0x80	/* 0=host, 1=card owns buffer */
#define TNTXF_ERR	0x40	/* some error occured */
#define TNTXF_ADDFCS	0x20	/* override DXMTFCS */
#define TNTXF_MORE	0x10	/* more than one retry occured */
#define TNTXF_ONE	0x08	/* exactly one retry occured */
#define TNTXF_DEF	0x04	/* deferred when attempting xmit */
#define TNTXF_STP	0x02	/* start of packet */
#define TNTXF_ENP	0x01	/* end of packet */

#define TNTXE_BUFF	0x20	/* buffer exchange protocol problem */
#define TNTXE_UFLO	0x10	/* DMA failed to keep up with xmitter */
#define TNTXE_LCOL	0x04	/* Late collision (no retry) */
#define TNTXE_LCAR	0x02	/* loss of carrier (no retry) */
#define TNTXE_RTRY	0x01	/* retry error; 16 strikes and you're out */

