/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: if_rh.c,v 1.6 1994/01/30 04:19:21 karels Exp $
 */

/*
 * SDL Communications RISCom/H2 dual sync/async serial port driver
 *
 * Currently the external clock source and 16 bit HDLC checksum
 * are hardwired; i.e. there is no need to configure the driver
 * to support different line speeds.
 */

/*
 * TODO:
 * - add support for internal clock source and
 *   alternative HDLC checksum
 * - do something about carrier detect etc
 */

#undef  RHDEBUG
#undef  RHDEBUG_X

/* #define RH_PAD  32      * Pad too short packets to 32 bytes */

#include "bpfilter.h"

#include "param.h"
#include "mbuf.h"
#include "socket.h"
#include "ioctl.h"
#include "errno.h"
#include "syslog.h"
#include "device.h"

#if INET
#include "netinet/in.h"
#include "netinet/in_systm.h"
#include "netinet/ip.h"		/* XXX for slcompress/if_p2p.h */
#endif

#include "net/if.h"
#include "net/netisr.h"
#include "net/if_types.h"
#include "net/if_p2p.h"

#include "icu.h"
#include "isa.h"
#include "isavar.h"
#include "if_rhreg.h"
#include "ic/hd64570.h"

#if NBPFILTER > 0
#include "net/bpf.h"
#include "net/bpfdesc.h"
#endif

#define RH_DEFAULT_RATE 56000   /* 56Kbps */

#undef dprintf
#ifdef RHDEBUG
#define dprintf(x)      printf x
#ifdef RHDEBUG_X
int _rinb(b, r, s)
{
	int a = RH_SCA(r) + b + 0x8000;
	int v = inb(a);
	printf(" {%x<-%s %x}", v, s, a);
	return(v);
}

void _routb(b, r, v, s)
{
	int a = RH_SCA(r) + b + 0x8000;
	printf(" {%s %x<-%x}", s, a, v & 0xff);
	outb(a, v);
}

#undef rinb
#undef routb
#define rinb(b, r) _rinb(b, r, #r)
#define routb(b, r, v) _routb(b, r, v, #r)
#endif
#else
#define dprintf(x)
#endif

/*
 * Tuneable parameters
 */
#define NTXFRAG 16              /* the max number of TX fragments */

struct rh_softc {
	struct	device sc_dev;		/* base device */
	struct	isadev sc_id;		/* ISA device */
	struct 	intrhand sc_ih;		/* interrupt vectoring */
	struct  p2pcom sc_p2pcom;       /* point-to-point common stuff */
#define	sc_if	sc_p2pcom.p2p_if	/* network-visible interface */	

	struct rh_softc *sc_nextchan;   /* next channel's rh_softc */

	u_short sc_addr;                /* base i/o port */
	u_short sc_msci;                /* channel-adjusted MSCI address */
	u_short sc_dma;                 /* channel-adjusted DMAC address */
	u_short sc_drq;                 /* AT DMA channel */
	int     sc_rate;                /* baud rate on channel */

	int     sc_txphys;              /* physical address of sc_txd */
	int     sc_txbphys;             /* physical address of sc_txbuf */
	struct  mbuf *sc_txtop;         /* the header of the transmitted buffer */
	struct  hd_cbd sc_txd[NTXFRAG]; /* TX buffer descriptors */

	int     sc_rxphys;              /* physical address of sc_rxd */
	int     sc_rxbphys;             /* physical address of sc_rxbuf */
	struct  hd_cbd sc_rxd;          /* RX buffer descriptor */

	char    sc_rxbuf[HDLCMTU + 8];  /* RX buffer space */
	char    sc_txbuf[HDLCMTU + 8];  /* TX DMA bounce buffer */

#if NBPFILTER > 0
	caddr_t	sc_bpf;			/* BPF filter */
#endif
};

int     rhprobe __P((struct device *, struct cfdata *, void *));
void    rhforceintr __P((void *));
void    rhattach __P((struct device *, struct device *, void *));
int     rhioctl __P((struct ifnet *, int, caddr_t));
int     rhintr __P((struct rh_softc *));
int     rhstart __P((struct ifnet *));
int     rhmdmctl __P((struct p2pcom *, int));

static void rhinit __P((struct rh_softc *));
static void rheanble __P((struct rh_softc *));
static void rhdisable __P((struct rh_softc *));
static void rhread __P((struct rh_softc *, char *, int));

struct cfdriver rhcd =
	{ NULL, "rh", rhprobe, rhattach, sizeof(struct rh_softc) };
 
/*
 * Probe routine
 */
rhprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	int i, v1, v2;

	/*
	 * Check configuration parameters
	 */
	if (!RH_IOBASEVALID(ia->ia_iobase)) {
		printf("rh%d: illegal base address %x\n", cf->cf_unit,
		    ia->ia_iobase);
		return (0);
	}
	if (!RH_DRQVALID(ia->ia_drq)) {
		printf("rh%d: illegal drq %d\n", cf->cf_unit, ia->ia_drq);
		return (0);
	}
	if (ia->ia_irq != IRQUNK) {
		i = ffs(ia->ia_irq) - 1;
		if (!RH_IRQVALID(i)) {
			printf("rh%d: illegal IRQ %d\n", cf->cf_unit, i);
			return (0);
		}
	}

	/*
	 * Check if card is present
	 */
	outb(ia->ia_iobase, RHC_RUN);
	if (inb(ia->ia_iobase) != RHC_RUN)
		return (0);
	routb(ia->ia_iobase, TEPR, 0xff);
	if (rinb(ia->ia_iobase, TEPR) != 0x7)
		return (0);
	routb(ia->ia_iobase, TEPR, 0);
	if (rinb(ia->ia_iobase, TEPR) != 0)
		return (0);

	/*
	 * Automagic irq detection
	 */
	if (ia->ia_irq == IRQUNK)
		ia->ia_irq = isa_discoverintr(rhforceintr, (void *) ia);

	/* Reset SCA */
	outb(ia->ia_iobase, 0);

	if (ffs(ia->ia_irq) - 1 == 0)
		return (0);
	ia->ia_iosize = RH_NPORT;
	return (1);
}

static void
rhforceintr(aux)
	void *aux;
{
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;

	/* Enable interrupt on tx buffer empty (it sure is) */
	outb(ia->ia_iobase, RHC_RUN | RHC_DMAEN);
	routb(ia->ia_iobase, IE0, ST0_TXRDY);
	routb(ia->ia_iobase, IER0, ISR0_TXRDY0);
	routb(ia->ia_iobase, TRC0, 32);
	routb(ia->ia_iobase, CMD, CMD_TXENB);
}

/*
 * Interface exists: make available by filling in network interface
 * records.  System will initialize the interface when it is ready
 * to accept packets.
 */
void
rhattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct rh_softc *sc = (struct rh_softc *) self;
	struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	struct ifnet *ifp;
	struct rh_softc *csc;
	int i;

	/*
	 * Allow SCA operation
	 */
	DELAY(5);
	outb(ia->ia_iobase, RHC_RUN);
 
	/*
	 * Load the wait controller registers
	 */
	routb(ia->ia_iobase, WCRL, 0);
	routb(ia->ia_iobase, WCRM, 0);
	routb(ia->ia_iobase, WCRH, 0);

	/*
	 * Initialize DMA
	 */
	at_dma_cascade(ia->ia_drq);
	routb(ia->ia_iobase, PCR, PCR_BRC | PCR_ROT);
	routb(ia->ia_iobase, DMER, DMER_DME);
	outb(ia->ia_iobase, RHC_RUN | RHC_DMAEN);

	/*
	 * Initialize interrupts
	 */
	routb(ia->ia_iobase, IER0, ISR0_TXINT0 | ISR0_TXINT1);
	routb(ia->ia_iobase, IER1, ISR1_DMIA0R | ISR1_DMIB0R |
				   ISR1_DMIA1R | ISR1_DMIB1R);

	/*
	 * Initialize interface data
	 */
	printf("\n");
	for (i = 0, csc = sc; i < RH_NCHANS; i++) {
		if (i) {
			csc = (struct rh_softc *) malloc(
				sizeof(struct rh_softc), M_DEVBUF, M_NOWAIT);
			if (csc == 0) {
				printf("rh%d: no memory for the channel 1 data",
					sc->sc_dev.dv_unit);
				return;
			}
			bzero((caddr_t) csc, sizeof(struct rh_softc));
			sc->sc_nextchan = csc;
		}

		csc->sc_nextchan = 0;
		csc->sc_addr = ia->ia_iobase;
		csc->sc_msci = ia->ia_iobase + RH_SCA(MCHAN * i);
		csc->sc_dma = ia->ia_iobase + RH_SCA(CHAN * i);
		csc->sc_drq = ia->ia_drq;
		csc->sc_rate = RH_DEFAULT_RATE;
		rhinit(csc);

		ifp = &csc->sc_if;
		ifp->if_unit = (sc->sc_dev.dv_unit * RH_NCHANS) + i;
		ifp->if_name = "rh";
		ifp->if_mtu = HDLCMTU;
		ifp->if_flags = IFF_POINTOPOINT | IFF_MULTICAST;
		ifp->if_type = IFT_PTPSERIAL;
		ifp->if_ioctl = p2p_ioctl;
		ifp->if_output = 0;             /* crash and burn on access */
		ifp->if_start = rhstart;
		csc->sc_p2pcom.p2p_mdmctl = rhmdmctl;
		if_attach(ifp);
#if NBPFILTER > 0
		bpfattach(&sc->sc_bpf, ifp, DLT_PPP, 0);	/* XXX */
#endif
	}

	/* Register the interrupt handler and isa device */
        isa_establish(&sc->sc_id, &sc->sc_dev);
	sc->sc_ih.ih_fun = rhintr;
        sc->sc_ih.ih_arg = (void *)sc;
        intr_establish(ia->ia_irq, &sc->sc_ih, DV_NET);
}


/*
 * Initialize the channel
 */
static void
rhinit(sc)
	register struct rh_softc *sc;
{
	register dma  = sc->sc_dma;
	struct hd_cbd *d;
	int i, j;

	sc->sc_txphys = kvtop((caddr_t) sc->sc_txd);
	sc->sc_txbphys = kvtop((caddr_t) sc->sc_txbuf);
	sc->sc_rxphys = kvtop((caddr_t) &sc->sc_rxd);
	sc->sc_rxbphys = kvtop((caddr_t) sc->sc_rxbuf);
	dprintf(("txphys=%x rxphys=%x\n", sc->sc_txphys, sc->sc_rxphys));

	j = sc->sc_txphys + sizeof(struct hd_cbd);
	for (d = sc->sc_txd, i = 0; i < NTXFRAG ; i++, d++) {
		d->cbd_next = j;
		d->cbd_stat = ST2_EOT;
		j += sizeof(struct hd_cbd);
	}

	/* Disable DMA interrupts */
	routb(dma, DIR, 0);
	routb(dma, DIR+TX, 0);
}

/*
 * Set transmission parameters and enable the channel
 */
static void
rhenable(sc)
	register struct rh_softc *sc;
{
	register msci = sc->sc_msci;
	register dma  = sc->sc_dma;
	int br, div;
	int chan =  sc->sc_if.if_unit % RH_NCHANS;

	dprintf(("enable unit %d:", sc->sc_if.if_unit));

	/* Reset the channel */
	routb(msci, CMD, CMD_CHRST);

	/* Idle fill pattern -- flag character */
	routb(msci, IDL, 0x7e);

	/* Clear DMA channels */
	routb(dma, DMR, 0);
	routb(dma, DCR, DCR_ABORT);
	routb(dma, DMR+TX, 0);
	routb(dma, DCR+TX, DCR_ABORT);

	/* Initialize channel DMA mode registers */
	routb(dma, DMR, DMR_TMOD);
	routb(dma, DMR+TX, DMR_TMOD | DMR_NF);

	/* HDLC, standard CRC */
	routb(msci, MD0, MD0_HDLC | MD0_CRCENB | MD0_CRCINIT | MD0_CRCCCITT);

	/* No address field checks */
	routb(msci, MD1, MD1_NOADR);

	/* NRZ, full duplex */
	routb(msci, MD2, MD2_NRZ | MD2_DUPLEX);

	/* Reset RX to validate new modes */
	routb(msci, CMD, CMD_RXRST);

	/* Set FIFO depths */
	routb(msci, RRC, 0);    /* start dma immediately */
	routb(msci, TRC0, 32);  /* lowat */
	routb(msci, TRC1, 63);  /* hiwat */

	/* Load the baud rate generator registers */
	div = RH_OSC_FREQ / sc->sc_rate;
	if (div == 0)
		div = 1;
	br = 0;
	if (div > 10 || !(div & 01)) {  /* avoid br=0 -- it yields duty ratio != 0.5 */
		do {
			br++;
			div /= 2;
		} while (div > 127);
	}
	dprintf((" baud rate=%d -> div=%d br=%d\n", sc->sc_rate, div, br));
	routb(msci, TMC, div);

	/* External clock (shouldn't be hardwired...) */
	routb(msci, RXS, RXS_LINE | br);
	routb(msci, TXS, TXS_LINE | br);

	/* Enable RX & TX interrupts from the channel */
	routb(dma, DIR, DSR_EOT | DSR_COF | DSR_BOF | DSR_EOM);
	routb(msci, IE0, ST0_TXINT);

	/* Set the RX DMA address & length */
	sc->sc_rxd.cbd_bp0 = sc->sc_rxbphys;
	sc->sc_rxd.cbd_bp1 = sc->sc_rxbphys >> 16;
	sc->sc_rxd.cbd_stat = 0;
	sc->sc_rxd.cbd_len = 0;
	routb(dma, BFLL, sizeof sc->sc_rxbuf);
	routb(dma, BFLH, (sizeof sc->sc_rxbuf) >> 8);
	routb(dma, EDAL, sc->sc_rxphys + sizeof sc->sc_rxd);
	routb(dma, EDAH, (sc->sc_rxphys + sizeof sc->sc_rxd) >> 8);
	routb(dma, CDAL, sc->sc_rxphys);
	routb(dma, CDAH, sc->sc_rxphys >> 8);
	routb(dma, CPB, sc->sc_rxphys >> 16);

	/* Enable SCA transmitter and receiver */
	routb(msci, CMD, CMD_TXENB);
	routb(msci, CMD, CMD_RXENB);

	/* Start RX DMA */
	routb(dma, DSR, DSR_DE | DSR_EOT);

	/* Enable on-board trasmitter and raise DTR */
	br = inb(sc->sc_addr);
	if (sc->sc_if.if_unit % RH_NCHANS)
		br |= RHC_DTR1 | RHC_TE1;
	else
		br |= RHC_DTR0 | RHC_TE0;
	outb(sc->sc_addr, br);

	/* Raise RTS and start transmission of the idle pattern */
	routb(msci, CTL, CTL_RTS | CTL_IDLC | CTL_UNDRC);

	/* Kickstart output */
	sc->sc_txtop = 0;
	sc->sc_if.if_flags &= ~IFF_OACTIVE;
	(void) rhstart(&sc->sc_if);
}

/*
 * Start output
 */
int
rhstart(ifp)
	register struct ifnet *ifp;
{
	struct rh_softc *sc0 = rhcd.cd_devs[ifp->if_unit / RH_NCHANS];
	struct rh_softc *sc = (ifp->if_unit % RH_NCHANS)?
					sc0->sc_nextchan : sc0;
	int cnt, phys, bflag;
	int bounce, totlen;
	register struct mbuf *m;
	register struct hd_cbd *d;

	/*
	 * Check if we have an available outgoing frame header and
	 * there is a sufficient space in TX output area.
	 */
	IF_DEQUEUE(&sc->sc_p2pcom.p2p_isnd, m);
	if (m == 0)
		IF_DEQUEUE(&sc->sc_if.if_snd, m);
	if (m == 0) {
		dprintf((" EMPTY TX Q\n"));
		return (0);
	}
	ifp->if_flags |= IFF_OACTIVE;
	dprintf(("TX%d: buf=%x ", ifp->if_unit, m));

#if NBPFILTER > 0
	/*
	 * If bpf is listening on this interface, let it
	 * see the packet before we commit it to the wire.
	 */
	if (sc->sc_bpf)
		bpf_mtap(sc->sc_bpf, m);
#endif

	/*
	 * Fill in the descriptors
	 */
	sc->sc_txtop = m;
	bounce = 0;
	bflag = 0;
	totlen = 0;
	cnt = 0;
	d = sc->sc_txd;
scan:   for (; m != 0; cnt++, m = m->m_next) {
		if (m->m_len == 0)
			continue;
		totlen += m->m_len;

		phys = ~0;      /* all 1s */
		if (cnt < NTXFRAG-1)
			phys = kvtop(m->m_data);

		/*
		 * Can't do DMA directly from mbuf?
		 */
		if (phys >= ISA_MAXADDR) {
			dprintf((" %x(%d)->", phys, m->m_len));
			phys = sc->sc_txbphys + bounce;
			if (bounce + m->m_len > sizeof sc->sc_txbuf)
				break;  /* shouldn't happen */
			bcopy(mtod(m, caddr_t), &sc->sc_txbuf[bounce], m->m_len);
			bounce += m->m_len;
			if (bflag) {
				d[-1].cbd_len += m->m_len;
				continue;
			}
			bflag = 2;
		}
		bflag >>= 1;

		/*
		 * Set up the TX buffer descriptor
		 */
		dprintf((" [%d:%d@%x]", cnt, m->m_len, phys));
		d->cbd_bp0 = phys;
		d->cbd_bp1 = phys >> 16;
		d->cbd_len = m->m_len;
		d->cbd_stat = 0;
		d++;
	}
#ifdef RH_PAD
	if (totlen < RH_PAD) {
		static struct mbuf pad;

		pad.m_data = pad.m_dat;
		pad.m_len = RH_PAD - totlen;
		m = &pad;       /* pad.m_next == 0 */
		goto scan;
	}
#endif /* RH_PAD */

	d[-1].cbd_stat = ST2_EOM;       /* End of frame */
	dprintf(("\n"));

	/*
	 * Start output
	 */
	phys = sc->sc_txphys + (char *)d - (char *)(sc->sc_txd);
	routb(sc->sc_dma, EDAL+TX, phys);
	routb(sc->sc_dma, EDAH+TX, phys >> 8);
	routb(sc->sc_dma, CDAL+TX, sc->sc_txphys);
	routb(sc->sc_dma, CDAH+TX, sc->sc_txphys >> 8);
	routb(sc->sc_dma, CPB+TX, sc->sc_txphys >> 16);
	routb(sc->sc_dma, DSR+TX, DSR_DE);

	/*
	 * Enable interrupt on entering the TX idle state
	 * The SCA manual says we should be able to start the
	 * new transmission immediately on completion of
	 * the previous DMA. In practice, it does not allow
	 * enough time for the final flag recognition
	 * by stupid (for example cisco's) HDLC controllers --
	 * they think the first packet was corrupted.
	 *
	 * Apparently we always need to send two flags between
	 * HDLC frames.
	 */
	DELAY(1);
	routb(sc->sc_msci, IE1, ST1_IDL);
	return (0);
}

/*
 * Interrupt routine
 */
rhintr(sc0)
	register struct rh_softc *sc0;
{
	register struct rh_softc *sc;
	u_char st;
	int len;

	(void) inb(sc0->sc_addr + 4);   /* For REV A cards */
	dprintf(("I "));
loop:
	/*
	 * Packet received?
	 */
	if (st = rinb(sc0->sc_addr, ISR1)) {
		sc = sc0;
		if ((st & (ISR1_DMIA0R | ISR1_DMIB0R)) == 0)
			sc = sc->sc_nextchan;
		dprintf(("RX%d ", sc->sc_if.if_unit));

		/*  Analyze DMA status */
		st = rinb(sc->sc_dma, DSR);
		st &= DSR_EOT | DSR_EOM | DSR_BOF | DSR_COF;
		if (st & (DSR_BOF | DSR_COF)) {
			sc->sc_if.if_ierrors++;
			dprintf(("-dmaerr %x", st));
			routb(sc->sc_dma, DSR, st | DSR_DWD);
		}

		/* Analyse frame status */
		else if ((sc->sc_rxd.cbd_stat & ST2_EOM) == 0) {
			dprintf(("-no status"));
			sc->sc_if.if_ierrors++;
		} else if (sc->sc_rxd.cbd_stat &
			   (ST2_SHRT|ST2_ABT|ST2_FRME|ST2_OVRN|ST2_CRCE)) {
			dprintf(("-frmerr %x", st));
			sc->sc_if.if_ierrors++;
		} else {
			len = sc->sc_rxd.cbd_len;
			dprintf(("-len=%d\n", len));
#ifdef RHDEBUG_X
			{
				char *p;
				int i;

				i = len;
				if (i >= (79/3))
					i = 79/3;
				p = sc->sc_rxbuf;
				while (i--)
					printf("%02x ", *p++ & 0xff);
				printf("\n");
			}
#endif /* RHDEBUG_X */
			rhread(sc, sc->sc_rxbuf, len);
		}

		/* Clear DMA status */
		routb(sc->sc_dma, DSR, st | DSR_DWD);

		/* Start new RX DMA */
		sc->sc_rxd.cbd_bp0 = sc->sc_rxbphys;
		sc->sc_rxd.cbd_bp1 = sc->sc_rxbphys >> 16;
		sc->sc_rxd.cbd_stat = 0;
		sc->sc_rxd.cbd_len = 0;
		routb(sc->sc_dma, CDAL, sc->sc_rxphys);
		routb(sc->sc_dma, CDAH, sc->sc_rxphys >> 8);
		routb(sc->sc_dma, CPB, sc->sc_rxphys >> 16);
		routb(sc->sc_dma, DSR, DSR_DE);
	} else

	/*
	 * Packet transmitted?
	 */
	if (st = rinb(sc0->sc_addr, ISR0)) {
		sc = sc0;
		if ((st & ISR0_TXINT0) == 0)
			sc = sc->sc_nextchan;

		dprintf((" TX%d", sc->sc_if.if_unit));

		/* Clear DMA status */
		st = rinb(sc->sc_dma, DSR+TX);
		st &= DSR_EOT | DSR_EOM | DSR_BOF | DSR_COF;
		if (st == 0) {
			dprintf(("-false"));
			goto loop;
		}
		routb(sc->sc_dma, DSR+TX, st | DSR_DWD);
		sc->sc_if.if_opackets++;

		/* Disable TX IDLE intr */
		routb(sc->sc_msci, IE1, 0);

		/* Release buffer */
		if (sc->sc_txtop)
			m_freem(sc->sc_txtop);
		sc->sc_txtop = 0;

		/* Restart transmission */
		sc->sc_if.if_flags &= ~IFF_OACTIVE;
		(void) rhstart(&sc->sc_if);
	} else {

		/* All sources served */
		dprintf(("\n"));
		return (1);
	}
	goto loop;      /* rescan status registers */
}

/*
 * Modem control
 * flag = 0 -- disable line; flag = 1 -- enable line
 */
int
rhmdmctl(pp, flag)
	struct p2pcom *pp;
	int flag;
{
	struct rh_softc *sc0 = rhcd.cd_devs[pp->p2p_if.if_unit / RH_NCHANS];
	struct rh_softc *sc = (pp->p2p_if.if_unit % RH_NCHANS) ?
				sc0->sc_nextchan : sc0;
	int br;
	int s;

	s = splimp();
	if (flag)
		rhenable(sc);
	else {
		dprintf(("disable unit %d:", sc->sc_if.if_unit));

		/* disable TX & RX interrupts */
		routb(sc->sc_dma, DIR, 0);
		routb(sc->sc_msci, IE1, 0);

		/* Halt dma in progress */
		routb(sc->sc_dma, DSR, 0);
		routb(sc->sc_dma, DSR+TX, 0);
		routb(sc->sc_dma, DCR, DCR_ABORT);
		routb(sc->sc_dma, DCR+TX, DCR_ABORT);

		/* disable transmitter and receiver */
		routb(sc->sc_msci, CMD, CMD_RXDIS);
		routb(sc->sc_msci, CMD, CMD_TXDIS);
		routb(sc->sc_msci, CTL, 0);	       /* drop RTS */
		br = inb(sc->sc_addr);                  /* drop DTR and disable transmitter */
		if (sc->sc_if.if_unit % RH_NCHANS)
			br |= RHC_DTR1 | RHC_TE1;
		else
			br |= RHC_DTR0 | RHC_TE0;
		outb(sc->sc_addr, br);

		/* Release tx buffers */
		dprintf(("release txtop %x --", sc->sc_txtop));
		if (sc->sc_txtop)
			m_freem(sc->sc_txtop);
		sc->sc_txtop = 0;
		dprintf(("done\n"));
	}

	/* XXX Report "carrier change" -- should be something more intelligent */
	if (sc->sc_p2pcom.p2p_modem)
		(*sc->sc_p2pcom.p2p_modem)(pp, flag);
	splx(s);
}

/*
 * Pass the accepted IP packet to upper levels
 */
static void
rhread(sc, cp, totlen)
	register struct rh_softc *sc;
	register caddr_t cp;
	int totlen;
{
	struct mbuf *m, *top, **mp;
	int len;

	if (totlen == 0)
		return;
	
#if NBPFILTER > 0
	/*
	 * Feed BPF
	 */
	if (sc->sc_bpf)
		bpf_tap(sc->sc_bpf, cp, totlen);
#endif

	/*
	 * Copy data into mbuf chain
	 */
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == 0)
		return;
	m->m_pkthdr.rcvif = &sc->sc_if;
	m->m_pkthdr.len = totlen;
	m->m_len = MHLEN;

	top = 0;
	mp = &top;
	while (totlen > 0) {
		if (top) {
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m == 0) {
				m_freem(top);
				return;
			}
			m->m_len = MLEN;
		}
		len = totlen;
		if (len >= MINCLSIZE) {
			MCLGET(m, M_DONTWAIT);
			if (m->m_flags & M_EXT)
				m->m_len = len = min(len, MCLBYTES);
			else
				len = m->m_len;
		} else {
			/*
			 * Place initial small packet/header at end of mbuf.
			 */
			if (len < m->m_len) {
				if (top == 0)
					MH_ALIGN(m, len);
				m->m_len = len;
			} else
				len = m->m_len;
		}

		totlen -= len;
		bcopy(cp, mtod(m, caddr_t), (unsigned)len);
		cp += len;
		*mp = m;
		mp = &m->m_next;
	}
	(*sc->sc_p2pcom.p2p_input)(&sc->sc_p2pcom, top);
}
