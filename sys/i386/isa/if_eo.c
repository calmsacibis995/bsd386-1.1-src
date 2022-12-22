/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: if_eo.c,v 1.6 1994/01/30 04:20:37 karels Exp $
 */

/*
 * 3Com Corp. EtherLink (3C501) Ethernet Adapter Driver
 */

#include "bpfilter.h"

/* #define EODEBUG  */

/* #define DODMA      * Enable DMA code for REALLY ANCIENT 3c501s (ASSY 0345) */

#include "param.h"
#include "systm.h"
#include "mbuf.h"
#include "buf.h"
#include "protosw.h"
#include "socket.h"
#include "ioctl.h"
#include "errno.h"
#include "syslog.h"
#include "device.h"

#include "net/if.h"
#include "net/netisr.h"
#include "net/route.h"

#ifdef INET
#include "netinet/in.h"
#include "netinet/in_systm.h"
#include "netinet/in_var.h"
#include "netinet/ip.h"
#include "netinet/if_ether.h"
#endif

#ifdef NS
#include "netns/ns.h"
#include "netns/ns_if.h"
#endif

#include "isa.h"
#include "isavar.h"
#include "icu.h"
#include "machine/cpu.h"
#ifdef DODMA
#include "dma.h"
#endif

#if NBPFILTER > 0
#include "../net/bpf.h"
#include "../net/bpfdesc.h"
#endif

#include "if_eoreg.h"

#ifdef EODEBUG
#define dprintf(x)	printf x
#else
#define dprintf(x)
#endif

#define ETHER_MIN_LEN 64
#define ETHER_MAX_LEN 1536

/*
 * Ethernet software status per interface.
 */
struct eo_softc {
	struct	device eo_dev;		/* base device (must be first) */
	struct	isadev eo_id;		/* ISA device */
	struct	intrhand eo_ih;		/* interrupt vectoring */	
	struct	arpcom eo_ac;		/* Ethernet common part */
#define	eo_if	eo_ac.ac_if		/* network-visible interface */
#define	eo_addr	eo_ac.ac_enaddr		/* hardware Ethernet address */
	int	eo_base;		/* the board base address */
	int     eo_oflags;              /* old interface flags */
#ifdef DODMA
	int	eo_drq;			/* the dma channel */
#endif
	char	eo_pb[2048 /*ETHERMTU+sizeof(long)*/];
#if NBPFILTER > 0
	caddr_t	eo_bpf;			/* BPF filter */
#endif
};

/*
 * Prototypes
 */
int eoprobe __P((struct device *, struct cfdata *, void *));
void eoattach __P((struct device *, struct device *, void *));
int eointr __P((void *));
int eoinit __P((int));
int eostart __P((struct ifnet *));
int eoioctl __P((struct ifnet *, int, caddr_t));
int eortrequest __P((int, struct rtentry *, struct sockaddr *));

void eoread __P((struct eo_softc *, char *, int));
struct mbuf *eoget __P((caddr_t, int, int, struct ifnet *));

struct cfdriver eocd = {
	NULL, "eo", eoprobe, eoattach, sizeof(struct eo_softc)
};

/*
 * Probe routine
 */
int
eoprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	void eoforceintr(); 
	int flag, i;
	register base;
	u_char test[ETHER_ADDR_LEN];

#ifdef lint
	eointr(0);
#endif

	/*
	 * Check configuration parameters
	 */
	base = ia->ia_iobase;
	if (!EO_IOBASEVALID(base)) {
		printf("eo%d: invalid i/o base address %x\n", cf->cf_unit, base);
		return (0);
	}
#ifdef DODMA
	if (!EO_DRQVALID(ia->ia_drq)) {
		printf("eo%d: invalid DRQ %d\n", cf->cf_unit, ia->ia_drq);
		return (0);
	}
#endif
	if (ia->ia_irq != IRQUNK) {
		i = ffs(ia->ia_irq) - 1;
		if (!EO_IRQVALID(i)) {
			printf("eo%d: invalid IRQ number %d\n", cf->cf_unit, i);
			return (0);
		} 
	}

	/*
	 * Reset the board and check its existance
	 */
	outb(base+EO_ACR, EOAC_RESET);
	DELAY(5);
	outb(base+EO_ACR, 0);
	flag = 0;
	outb(base+EO_GPBPH, 0);
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		outb(base+EO_GPBPL, i);
		test[i] = inb(base+EO_EAPW);
		if (test[i] != test[0])
			flag++;
	}
	dprintf(("eo: READ ADDR OK %s\n", ether_sprintf(test)));
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		outb(base+EO_GPBPL, i);
		if (test[i] != inb(base+EO_EAPW))
			return (0);
	}
	dprintf(("eo: COMPARE OK\n"));
	if (flag == 0)
		return (0);

	/*
	 * Find out IRQ if unknown
	 */
	if (ia->ia_irq == IRQUNK) {
		ia->ia_irq = isa_discoverintr(eoforceintr, aux);
		outb(base+EO_ACR, 0);
		if (ffs(ia->ia_irq) - 1 == 0)
			return (0);
	}
	ia->ia_iosize = EO_NPORTS;
	return (1);
}

/*
 * force the card to interrupt (tell us where it is) for autoconfiguration
 */ 
void
eoforceintr(aux)
	void *aux;
{
	register base = ((struct isa_attach_args *) aux)->ia_iobase;

	dprintf(("eo: FORCEINT\n"));
	outb(base+EO_GPBPL, (EO_BUFSIZE-64));
	outb(base+EO_GPBPH, (EO_BUFSIZE-64)>>8);
	outb(base+EO_RBPCLR, 0);
	outb(base+EO_RCR, EORC_PROMISC | EORC_AGF);
#ifdef DODMA
	outb(base+EO_ACR, EOAC_RIDE | EOAC_LOOP | EOAC_TXBADFCS);
#else
	outb(base+EO_ACR, EOAC_IRE | EOAC_LOOP | EOAC_TXBADFCS);
#endif
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.  We get the ethernet address here.
 */
void
eoattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	register struct eo_softc *sc = (struct eo_softc *) self;
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	register struct ifnet *ifp = &sc->eo_if;
	int i;

	/*
	 * Reset the board
	 */
	sc->eo_base = ia->ia_iobase;
	outb(ia->ia_iobase+EO_ACR, EOAC_RESET);
	DELAY(5);
	outb(ia->ia_iobase+EO_ACR, 0);
#ifdef DODMA
	at_setup_dmachan(ia->ia_drq, EO_BUFSIZE);
	sc->eo_drq = ia->ia_drq;
#endif
	
	/*
	 * Extract board address
	 */
	dprintf(("eo: EXTRACT ADDR\n"));
	outb(ia->ia_iobase+EO_GPBPH, 0);
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		outb(ia->ia_iobase+EO_GPBPL, i);
		sc->eo_addr[i] = inb(ia->ia_iobase+EO_EAPW);
	}

	/*
	 * Initialize interface structure
	 */
	ifp->if_unit = sc->eo_dev.dv_unit;
	ifp->if_name = eocd.cd_name;
	ifp->if_mtu = ETHERMTU;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS;
	ifp->if_init = eoinit;
	ifp->if_output = ether_output;
	ifp->if_start = eostart;
	ifp->if_ioctl = eoioctl;
	ifp->if_watchdog = 0;
	if_attach(ifp);

	printf(" 3C501 %s\n", ether_sprintf(sc->eo_addr));

#if NBPFILTER > 0
	bpfattach(&sc->eo_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif

	isa_establish(&sc->eo_id, &sc->eo_dev, ia);
	sc->eo_ih.ih_fun = eointr;
	sc->eo_ih.ih_arg = (void *)sc;
	intr_establish(ia->ia_irq, &sc->eo_ih, DV_NET);
}

/*
 * Initialization of interface; set up initialization block
 * and transmit/receive descriptor rings.
 */
int
eoinit(unit)
	int unit;
{
	register struct eo_softc *sc = eocd.cd_devs[unit];
	struct ifnet *ifp = &sc->eo_if;
	int s;
	register base = sc->eo_base;
	register i;

 	if (ifp->if_addrlist == (struct ifaddr *) 0)
 		return (0);
	ifp->if_flags |= IFF_RUNNING;
	sc->eo_oflags = ifp->if_flags;

	s = splimp();
	/* Reset the board */
	outb(base+EO_ACR, EOAC_RESET);
	DELAY(5);
	outb(base+EO_ACR, 0);

	/* Load station address */
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		outb(base+EO_ADDR(i), sc->eo_addr[i]);

	/* Configure receive */
	if (ifp->if_flags & IFF_PROMISC)
		outb(base+EO_RCR, EORC_PROMISC|EORC_AGF|EORC_DSF|EORC_DDR|EORC_DOV);
	else
		outb(base+EO_RCR, EORC_AB|EORC_AGF|EORC_DSF|EORC_DDR|EORC_DOV);
	outb(base+EO_RBPCLR, 0);

	/* Configure transmitter */
	outb(base+EO_TCR, 0);

	/* Start receiption */
#ifdef DODMA
	outb(base+EO_ACR, EOAC_RIDE | EOAC_RCV);
#else
	outb(base+EO_ACR, EOAC_IRE | EOAC_RCV);
#endif
	
	eostart(ifp);
	splx(s);
}

/*
 * Setup output on interface.
 * Get another datagram to send off of the interface queue,
 * and map it to the interface before starting the output.
 * called only at splimp or interrupt level.
 */
int
eostart(ifp)
	struct ifnet *ifp;
{
	register struct eo_softc *sc = eocd.cd_devs[ifp->if_unit];
	register base = sc->eo_base;
	struct mbuf *m0, *m;
	int len, i, s, retrycnt;

	s = splimp();
	if (sc->eo_if.if_flags & IFF_OACTIVE)
		return;
	sc->eo_if.if_flags |= IFF_OACTIVE;
	for (;;) {
		IF_DEQUEUE(&sc->eo_if.if_snd, m0);
		if (m0 == 0) {
			sc->eo_if.if_flags &= ~IFF_OACTIVE;
			splx(s);
			return (0);
		}

		/* Disable receiver */
		outb(base+EO_ACR, EOAC_HOST);
		outb(base+EO_RBPCLR, 0);

		/*
		 * Copy packet to the buffer.
		 * XXX should use m->m_hdr.len.
		 * XXX XXX shouldn't copy!
		 */
		len = 0;
		for (m = m0; m != 0; m = m->m_next) {
			if (m->m_len == 0)
				continue;
			bcopy(mtod(m, caddr_t), sc->eo_pb + len, m->m_len);
			len += m->m_len;
		}
		m_freem(m0);
		if (len < ETHER_MIN_LEN) {
			bzero(sc->eo_pb + len, ETHER_MIN_LEN-len);
			len = ETHER_MIN_LEN;
		}

#if NBPFILTER > 0
		/*
		 * Feed outgoing packet to bpf
		 */
		if (sc->eo_bpf)
			bpf_tap(sc->eo_bpf, sc->eo_pb, len);
#endif

		/*
		 * Transfer data to the board
		 */
		dprintf(("SEND len=%d --", len));
		i = EO_BUFSIZE - len;
		outb(base+EO_GPBPL, i);
		outb(base+EO_GPBPH, i>>8);
#ifdef DODMA
		at_dma(0, sc->eo_pb, len, sc->eo_drq);
		outb(base+EO_ACR, EOAC_HOST | EOAC_RIDE | EOAC_DMAREQ);
		while ((inb(base+EO_ASR) & EOAS_DMADONE) == 0);
		outb(base+EO_ACR, EOAC_HOST);
		at_dma_terminate(sc->eo_drq);
#else
		outsb(base+EO_BUF, sc->eo_pb, len);
#endif

		/*
		 * Start transmission
		 */
		retrycnt = 0;
retry:
		dprintf(("GO! "));
		outb(base+EO_GPBPL, i);
		outb(base+EO_GPBPH, i>>8);
		outb(base+EO_ACR, EOAC_TFR);
		i = 20000;
		while ((inb(base+EO_ASR) & EOAS_TXBUSY) && --i)
			;
		if (i == 0) {
			dprintf(("NO TX RDY\n"));
			sc->eo_if.if_oerrors++;
			continue;
		}
		dprintf(("<%d cycles> ", 20000 - i));
	
		/*
		 * Analyze status
		 */
		i = inb(base+EO_TSR);
		if ((i & EOTS_RNF) == 0) {
			dprintf(("ERR tsr=%x", i));
			if (i & EOTS_UFLOW)
				sc->eo_if.if_oerrors++;
			else if (i & (EOTS_COLL|EOTS_C16)) {
				sc->eo_if.if_collisions++;
				if ((i & EOTC_DC16) == 0 && retrycnt++ < 15) {
					outb(base+EO_ACR, EOAC_HOST);
					goto retry;
				}
			} else
				sc->eo_if.if_oerrors++;
		}

		/*
		 * Give receiver a chance
		 */
		(void) inb(base+EO_ASR);
/*
		outb(base+EO_RBPCLR, 0);
		if (ifp->if_flags & IFF_PROMISC)
			outb(base+EO_RCR, EORC_PROMISC|EORC_AGF|EORC_DSF|EORC_DDR|EORC_DOV);
		else
			outb(base+EO_RCR, EORC_AB|EORC_AGF|EORC_DSF|EORC_DDR|EORC_DOV);
 */
#ifdef DODMA
		outb(base+EO_ACR, EOAC_RIDE | EOAC_RCV);
#else
		outb(base+EO_ACR, EOAC_IRE | EOAC_RCV);
#endif
		dprintf(("\n"));
		splx(s);
		/* an interrupt can happen here */
		(void) splimp();
	}
}

/*
 * Controller interrupt.
 */
int
eointr(arg)
	void *arg;
{
	register struct eo_softc *sc = arg;
	register base = sc->eo_base;
	int stat, rcvstat;
	int len;
	int len1;

	if ((sc->eo_if.if_flags & IFF_RUNNING) == 0) {
		dprintf(("eo: bogus interrupt\n"));
		return (1);
	}
	dprintf(("I "));

	/*
	 * Analyze the board status
	 */
	stat = inb(base+EO_ASR);
	if (stat & EOAS_RXBUSY) 
		goto out;

newpacket:
	rcvstat = inb(base+EO_RSR);
	
	if (rcvstat & EORS_STALE)
		goto out;

	/* Overflow -- reinitialize the board */
	if (!(rcvstat & EORS_NOVFLO)) {
		outb(base+EO_ACR, EOAC_RESET);
		DELAY(5);
		outb(base+EO_ACR, 0);

		/* Restore receive mode */
restore_rcv:	if (sc->eo_if.if_flags & IFF_PROMISC)
			outb(base+EO_RCR, EORC_PROMISC|EORC_AGF|EORC_DSF|EORC_DDR|EORC_DOV);
		else
			outb(base+EO_RCR, EORC_AB|EORC_AGF|EORC_DSF|EORC_DDR|EORC_DOV);
		(void) inb(base+EO_ASR);
		outb(base+EO_RBPCLR, 0);
		sc->eo_if.if_ierrors++;
		goto out;
	}

	/* Normal packet */
	len = inb(base+EO_RCVBPL);
	len |= inb(base+EO_RCVBPH) << 8;
	dprintf(("RCV len=%d rcvstat=%x ", len, rcvstat));

	outb(base+EO_ACR, EOAC_HOST);

	if (len <= sizeof(struct ether_header) || len > ETHER_MAX_LEN)
		goto restore_rcv;
	sc->eo_if.if_ipackets++;

	/*
	 * Copy data into buffer
	 */
	outb(base+EO_GPBPL, 0);
	outb(base+EO_GPBPH, 0);
	insb(base+EO_BUF, sc->eo_pb, len);
	outb(base+EO_RBPCLR, 0);
	outb(base+EO_ACR, EOAC_RCV);
	dprintf(("%s-->", ether_sprintf(sc->eo_pb+6)));
	dprintf(("%s (%x)\n", ether_sprintf(sc->eo_pb), *(u_short *)(sc->eo_pb+12)));

	/*
	 * Pass the packet to upper levels
	 */
	len -= sizeof(struct ether_header);
	eoread(sc, (caddr_t)(sc->eo_pb), len);

	/* Check if there is a new packet */
	stat = inb(base+EO_ASR);

	if ((stat & EOAS_RXBUSY) == 0) {
		dprintf(("<RESCAN INTR>\n"));
		goto newpacket;
	}
out:
	(void) inb(base+EO_RCR);
#ifdef DODMA
	outb(base+EO_ACR, EOAC_RIDE | EOAC_RCV);
#else
	outb(base+EO_ACR, EOAC_IRE | EOAC_RCV);
#endif
	return (1);
}

/*
 * Pass a packet to the higher levels.
 * We deal with the trailer protocol here.
 */
void
eoread(sc, buf, len)
	register struct eo_softc *sc;
	char *buf;
	int len;
{
	register struct ether_header *eh;
    	struct mbuf *m;
	int off, resid;

	/*
	 * Deal with trailer protocol: if type is trailer type
	 * get true type from first 16-bit word past data.
	 * Remember that type was trailer by setting off.
	 */
	eh = (struct ether_header *)buf;
	eh->ether_type = ntohs((u_short)eh->ether_type);
#define	eodataaddr(eh, off, type)	((type)(((caddr_t)((eh)+1)+(off))))
	if (eh->ether_type >= ETHERTYPE_TRAIL &&
	    eh->ether_type < ETHERTYPE_TRAIL+ETHERTYPE_NTRAILER) {
		off = (eh->ether_type - ETHERTYPE_TRAIL) * 512;
		if (off >= ETHERMTU)
			return;		/* sanity */
		eh->ether_type = ntohs(*eodataaddr(eh, off, u_short *));
		resid = ntohs(*(eodataaddr(eh, off + 2, u_short *)));
		if (off + resid > len)
			return;		/* sanity */
		len = off + resid;
	} else
		off = 0;

	if (len <= 0)
		return;

#if NBPFILTER > 0
        /*
         * Check if there's a bpf filter listening on this interface.
         * If so, hand off the raw packet to bpf, which must deal with
         * trailers in its own way.
         */
        if (sc->eo_bpf) {
                eh->ether_type = htons((u_short)eh->ether_type);
                bpf_tap(sc->eo_bpf, buf, len + sizeof(struct ether_header));
                eh->ether_type = ntohs((u_short)eh->ether_type);

                /*
                 * Note that the interface cannot be in promiscuous mode if
                 * there are no bpf listeners.  And if el are in promiscuous
                 * mode, el have to check if this packet is really ours.
                 *
                 * This test does not support multicasts.
                 */
                if ((sc->eo_if.if_flags & IFF_PROMISC)
                    && bcmp(eh->ether_dhost, sc->eo_addr,
                            sizeof(eh->ether_dhost)) != 0
                    && bcmp(eh->ether_dhost, etherbroadcastaddr, 
                            sizeof(eh->ether_dhost)) != 0)
		return;
        }
#endif

	/*
	 * Pull packet off interface.  Off is nonzero if packet
	 * has trailing header; neget will then force this header
	 * information to be at the front, but we still have to drop
	 * the type and length which are at the front of any trailer data.
	 */
	m = eoget(buf, len, off, &sc->eo_if);
	if (m == 0)
		return;

	ether_input(&sc->eo_if, eh, m);
}

/*
 * Pull read data off a interface.
 * Len is length of data, with local net header stripped.
 * Off is non-zero if a trailer protocol was used, and
 * gives the offset of the trailer information.
 * We copy the trailer information and then all the normal
 * data into mbufs.  When full cluster sized units are present
 * we copy into clusters.
 */
struct mbuf *
eoget(buf, totlen, off0, ifp)
	caddr_t buf;
	int totlen, off0;
	struct ifnet *ifp;
{
	struct mbuf *top, **mp, *m;
	int off = off0, len;
	register caddr_t cp = buf;
	char *epkt;

	buf += sizeof(struct ether_header);
	cp = buf;
	epkt = cp + totlen;


	if (off) {
		cp += off + 2 * sizeof(u_short);
		totlen -= 2 * sizeof(u_short);
	}

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == 0)
		return (0);
	m->m_pkthdr.rcvif = ifp;
	m->m_pkthdr.len = totlen;
	m->m_len = MHLEN;

	top = 0;
	mp = &top;
	while (totlen > 0) {
		if (top) {
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m == 0) {
				m_freem(top);
				return (0);
			}
			m->m_len = MLEN;
		}
		len = min(totlen, epkt - cp);
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
				if (top == 0 && len + max_linkhdr <= m->m_len)
					m->m_data += max_linkhdr;
				m->m_len = len;
			} else
				len = m->m_len;
		}
		bcopy(cp, mtod(m, caddr_t), (unsigned)len);
		cp += len;
		*mp = m;
		mp = &m->m_next;
		totlen -= len;
		if (cp == epkt)
			cp = buf;
	}
	return (top);
}

/*
 * Process an ioctl request.
 */
int
eoioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	register struct ifaddr *ifa = (struct ifaddr *)data;
	register struct eo_softc *sc = eocd.cd_devs[ifp->if_unit];
	int s;
	int error = 0;

	s = splimp();
	switch (cmd) {

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			eoinit(ifp->if_unit);	/* before arpwhohas */
			((struct arpcom *)ifp)->ac_ipaddr =
				IA_SIN(ifa)->sin_addr;
			arpwhohas((struct arpcom *)ifp, &IA_SIN(ifa)->sin_addr);
			break;
#endif
#ifdef NS
		case AF_NS:
		    {
			register struct ns_addr *ina = &(IA_SNS(ifa)->sns_addr);

			if (ns_nullhost(*ina))
				ina->x_host = *(union ns_host *)(sc->eo_addr);
			else {
				/* 
				 * The manual says we can't change the address 
				 * while the receiver is armed,
				 * so reset everything
				 */
				ifp->if_flags &= ~IFF_RUNNING; 
				bcopy((caddr_t)ina->x_host.c_host,
				    (caddr_t)sc->eo_addr, sizeof(sc->eo_addr));
			}
			eoinit(ifp->if_unit); /* does ne_setaddr() */
			break;
		    }
#endif
		default:
			eoinit(ifp->if_unit);
			break;
		}

		/*
		 * Associate a route patching routine to reduce TCP window size
		 * for routes going through this interface
		 */
		ifa->ifa_rtrequest = eortrequest;
		break;

	case SIOCSIFFLAGS:
		if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) == IFF_RUNNING) {
			ifp->if_flags &= ~(IFF_RUNNING|IFF_OACTIVE);
			sc->eo_oflags = ifp->if_flags;
			outb(sc->eo_base + EO_ACR, 0);
		} else if ((ifp->if_flags ^ sc->eo_oflags) & (IFF_UP|IFF_PROMISC))
			eoinit(ifp->if_unit);
		break;

	default:
		error = EINVAL;
	}
	splx(s);
	return (error);
}

/*
 * Reduce the default size of TCP receive window to avoid loosing
 * head-to-back packets.
 */
/*ARGSUSED*/
int eortrequest(req, rt, sa)
	int req;
	struct rtentry *rt;
	struct sockaddr *sa;
{
	dprintf(("eo: eortrequest %d\n", req));
	if ((req == RTM_ADD || req == RTM_RESOLVE) &&
	    (rt->rt_rmx.rmx_recvpipe == 0 || rt->rt_rmx.rmx_recvpipe > 1024))
		rt->rt_rmx.rmx_recvpipe = 1024;
	return (0);
}
