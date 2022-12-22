/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: if_ep.c,v 1.5 1994/01/30 04:20:15 karels Exp $
 */

/*
 * 3Com Corp. EtherLink Plus (3C505) Ethernet Adapter Driver
 *
 * History:
 *	8/16/92	[avg]	- initial revision
 */

#include "bpfilter.h"

/* #define EPDEBUG */

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

#if NBPFILTER > 0
#include "../net/bpf.h"
#include "../net/bpfdesc.h"
#endif

#include "if_epreg.h"

#ifdef EPDEBUG
#define dprintf(x)	printf x
#else
#define dprintf(x)
#endif

#define ETHER_MIN_LEN 64
#define ETHER_MAX_LEN 1536

/*
 * Ethernet software status per interface.
 */
struct ep_softc {
	struct	device ep_dev;		/* base device (must be first) */
	struct	isadev ep_id;		/* ISA device */
	struct	intrhand ep_ih;		/* interrupt vectoring */	
	struct	arpcom ep_ac;		/* Ethernet common part */
#define	ep_if	ep_ac.ac_if		/* network-visible interface */
#define	ep_addr	ep_ac.ac_enaddr		/* hardware Ethernet address */
	int	ep_base;		/* the board base address */
	int     ep_oflags;              /* the old interface flags */
	char	ep_pb[1600];		/* Input packet buffer */
#if NBPFILTER > 0
	caddr_t	ep_bpf;			/* BPF filter */
#endif
};

#define EP_TIMO		100000	/* Number of wait loops */

/*
 * Prototypes
 */
int epprobe __P((struct device *, struct cfdata *, void *));
void epattach __P((struct device *, struct device *, void *));
int epintr __P((void *));
int epinit __P((int));
int epstart __P((struct ifnet *));
int epioctl __P((struct ifnet *, int, caddr_t));

void epread __P((struct ep_softc *, char *, int));
struct mbuf *epget __P((caddr_t, int, int, struct ifnet *));

struct cfdriver epcd = {
	NULL, "ep", epprobe, epattach, sizeof(struct ep_softc)
};

/*
 * Probe routine
 */
int
epprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	void epforceintr(); 
	int i;
	register base;
	union ep_pcb pcb;

#ifdef lint
	epintr(0);
#endif

	/*
	 * Check configuration parameters
	 */
	base = ia->ia_iobase;
	if (!EP_IOBASEVALID(base)) {
		printf("ep%d: invalid i/o base address %x\n", cf->cf_unit, base);
		return (0);
	}
	if (ia->ia_irq != IRQUNK) {
		i = ffs(ia->ia_irq)-1;
		if (!EP_IRQVALID(i)) {
			printf("ep%d: invalid IRQ number %d\n", cf->cf_unit, i);
			return (0);
		} 
	}

	/* Reset the board (do a soft reset) */
	dprintf(("ep: SOFT RESET\n"));
	outb(base+EP_CTL, EPC_ATTN);
	DELAY(5);
	outb(base+EP_CTL, 0);
	DELAY(500);

	/*
	 * Ask the adapter info
	 */
	pcb.c82586.cmd = EPHC_C82586;
	pcb.c82586.len = 0;
	pcb.c82586.mode = EPMO_ILOOP;
	if (epsendpcb(base, &pcb))
		return (0);

	/*
	 * Wait for response
	 */
	if (epwait(base)) {
		dprintf(("ep: no ACRF\n"));
		return (0);
	}

	/*
	 * Extract adapter pcb
	 */
	if (eprcvpcb(base, &pcb, 0))
		return (0);
	if (pcb.g.cmd != EPAC_C82586) {
		printf("ep%d: received pcb has wrong type 0x%x\n", cf->cf_unit, pcb.g.cmd);
		return (0);
	}
	if (pcb.c82586.len != PCBARGSIZ(c82586)) {
		printf("ep%d: received pcb has bad length %d\n", cf->cf_unit, pcb.ai.len);
		return (0);
	}

	/*
	 * Find out IRQ if unknown
	 */
	if (ia->ia_irq == IRQUNK) {
		ia->ia_irq = isa_discoverintr(epforceintr, aux);
		dprintf(("got irq=%d\n", ffs(ia->ia_irq) - 1));
		outb(base+EP_CTL, 0);			/* disable interrupts */
		(void) eprcvpcb(base, &pcb, 0);		/* clean up the stuff */
		if (ffs(ia->ia_irq) - 1 == 0)
			return (0);
	}
	ia->ia_iosize = EP_NPORTS;
	return (1);
}

/*
 * force the card to interrupt (tell us where it is) for autoconfiguration
 */ 
void
epforceintr(aux)
	void *aux;
{
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	register base = ia->ia_iobase;
	union ep_pcb pcb;

	dprintf(("ep: FORCEINT\n"));
	
	/* Enable interrupts */
	outb(base+EP_CTL, EPC_CMDE);

	/* Send a command */
	pcb.g.cmd = EPHC_GADDR;
	pcb.g.len = 0;
	(void) epsendpcb(base, &pcb);
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.  We get the ethernet address here.
 */
void
epattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	register struct ep_softc *sc = (struct ep_softc *) self;
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	register struct ifnet *ifp = &sc->ep_if;
	int i;
	union ep_pcb pcb;

	/*
	 * Reset the board
	 */
	sc->ep_base = ia->ia_iobase;
	outb(ia->ia_iobase+EP_CTL, EPC_ATTN);
	DELAY(5);
	outb(ia->ia_iobase+EP_CTL, 0);
	DELAY(500);

	/*
	 * Extract board address
	 */
	dprintf(("ep: EXTRACT ADDR\n"));
	pcb.g.cmd = EPHC_GADDR;
	pcb.g.len = 0;
	if (epsendpcb(ia->ia_iobase, &pcb) || epwait(ia->ia_iobase) ||
	    eprcvpcb(ia->ia_iobase, &pcb, 0) ||
	    pcb.g.cmd != EPAC_GADDR || pcb.saddr.len != PCBARGSIZ(saddr)) {
		printf(": cannot get station address\n");
		return;
	}
	bcopy(pcb.saddr.addr, sc->ep_addr, sizeof sc->ep_addr);

	/*
	 * Initialize interface structure
	 */
	ifp->if_unit = sc->ep_dev.dv_unit;
	ifp->if_name = epcd.cd_name;
	ifp->if_mtu = ETHERMTU;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS;
	ifp->if_init = epinit;
	ifp->if_output = ether_output;
	ifp->if_start = epstart;
	ifp->if_ioctl = epioctl;
	ifp->if_watchdog = 0;
	if_attach(ifp);

	printf("\nep%d: 3C505 address %s\n", ifp->if_unit,
	    ether_sprintf(sc->ep_addr));

#if NBPFILTER > 0
	bpfattach(&sc->ep_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif

	isa_establish(&sc->ep_id, &sc->ep_dev);
	sc->ep_ih.ih_fun = epintr;
	sc->ep_ih.ih_arg = (void *)sc;
	intr_establish(ia->ia_irq, &sc->ep_ih, DV_NET);
}

/*
 * Send PCB to the board
 */
int
epsendpcb(base, pcb)
	register int base;
	union ep_pcb *pcb;
{
	register caddr_t p = (caddr_t) pcb;
	int i;
	int cnt;
	int lastctl;

	dprintf(("SEND PCB: cmd=%x len=%x\n", pcb->g.cmd, pcb->g.len));
	
	/* Clear HSF */
	lastctl = inb(base + EP_CTL) & ~EPSF_MASK;
	outb(base+EP_CTL, lastctl);

	i = pcb->g.len + 2;
	while (i--) {
		/* Send the command byte */
		outb(base+EP_CMD, *p++);
	
		/* Wait for the acceptance of the command */
		cnt = EP_TIMO;
		while ((inb(base+EP_STAT) & EPS_HCRE) == 0 && --cnt);
		if (cnt == 0) {
			dprintf(("PCB load failed on byte %d\n", pcb->g.len + 1 - cnt));
			return (1);
		}
	}

	/* Signal the end of PCB */
	outb(base+EP_CTL, lastctl | EPSF_EOPCB);
	outb(base+EP_CMD, pcb->g.len + 2);

	/* Wait for acceptance */
	cnt = EP_TIMO;
	while (--cnt) {
		i = inb(base+EP_STAT) & EPSF_MASK;
		if (i != (inb(base+EP_STAT) & EPSF_MASK))
			continue;
		if (i == EPSF_ACCEPT || i == EPSF_REJECT)
			break;
	}
	outb(base+EP_CTL, lastctl);
	if (cnt == 0) {
		dprintf(("PCB acceptance timeout\n"));
		return (1);
	}
	if (i == EPSF_REJECT) {
		dprintf(("PCB rejected\n"));
		return (1);
	}
	return (0);
}

/*
 * Receive adapter PCB
 */
int
eprcvpcb(base, pcb, noaccept)
	register int base;
	union ep_pcb *pcb;
	int noaccept;
{
	register caddr_t p = (caddr_t) pcb;
	caddr_t q = p + sizeof(union ep_pcb);
	int i;
	int cnt;

	for (;;) {
		/* Wait for ACRF go up */
		cnt = EP_TIMO;
		while (!((i = inb(base+EP_STAT)) & EPS_ACRF) && --cnt);
		if (cnt == 0) {
			dprintf(("ep: ACRF timeout\n"));
			return (1);
		}

		/* End of PCB? */
		if ((i & EPSF_MASK) == EPSF_EOPCB)
			break;
	
		/* Get the next byte */
		if (p < q)
			*p++ = inb(base+EP_CMD);
		else
			(void) inb(base+EP_CMD);
	}
	
	/* Discard the length indicator */
	(void) inb(base+EP_CMD);
	i = inb(base+EP_CTL) & ~EPSF_MASK;

	dprintf(("ep: got PCB cmd=%x len=%d\n", pcb->g.cmd, pcb->g.len));

	/* Sanity check */
	if (p - (caddr_t)pcb != pcb->g.len + 2) {
		outb(base+EP_CTL, i | EPSF_REJECT);
		printf("ep: bad pcb length\n");
		return (1);
	}

	/* Accept the PCB */
	if (!noaccept)
		outb(base+EP_CTL, i | EPSF_ACCEPT);
	return (0);
}

/*
 * Wait for response PCB
 */
int
epwait(base)
	register int base;
{
	int i;

	i = EP_TIMO;
	while ((inb(base+EP_STAT) & EPS_ACRF) == 0 && --i)
		;
#ifdef EPDEBUG
	if (i == 0)
		printf("epwait: timeout\n");
#endif
	return (i == 0);
}

/*
 * Flush the PCB queue and disable interrupts
 */
epstop(base)
	register int base;
{
	int s;
	union ep_pcb pcb;

	s = splimp();
	pcb.c82586.cmd = EPHC_C82586;
	pcb.c82586.len = PCBARGSIZ(c82586);
	pcb.c82586.mode = EPMO_ILOOP;
	(void) epsendpcb(base, &pcb);
	splx(s);
}

/*
 * Initialization of interface
 */
int
epinit(unit)
	int unit;
{
	register struct ep_softc *sc = epcd.cd_devs[unit];
	struct ifnet *ifp = &sc->ep_if;
	int s;
	register base = sc->ep_base;
	register i;
	union ep_pcb pcb;

 	if (ifp->if_addrlist == (struct ifaddr *) 0)
 		return (0);
	if (ifp->if_flags & IFF_RUNNING)
		return (0);

	/* Reset the board */
	s = splimp();
	outb(base+EP_CTL, EPC_ATTN);
	DELAY(5);
	outb(base+EP_CTL, 0);
	DELAY(500);

	/*
	 * Load the station address
	 */
	pcb.saddr.cmd = EPHC_SADDR;
	pcb.saddr.len = PCBARGSIZ(saddr);
	bcopy(sc->ep_addr, pcb.saddr.addr, sizeof pcb.saddr.addr);
	if (epsendpcb(base, &pcb) || epwait(base) || eprcvpcb(base, &pcb, 0) ||
	    pcb.g.cmd != EPAC_SADDR || pcb.resp.len != PCBARGSIZ(resp) ||
	    pcb.resp.res != 0) {
		printf("ep%d: cannot load station address\n", unit);
		return (0);
	}

	/*
	 * Configure 82586
	 */
	pcb.c82586.cmd = EPHC_C82586;
	pcb.c82586.len = PCBARGSIZ(cam);
	pcb.c82586.mode = ((ifp->if_flags & IFF_PROMISC) ? EPMO_PROMISC : 0) | EPMO_AB;
	if (epsendpcb(base, &pcb) || epwait(base) || eprcvpcb(base, &pcb, 0) ||
	    pcb.g.cmd != EPAC_C82586 || pcb.resp.len != PCBARGSIZ(resp) ||
	    pcb.resp.res != 0) {
		printf("ep%d: cannot configure adapter memory\n", unit);
		return (0);
	}

	/* Enable interrupts */
	outb(base+EP_CTL, EPC_CMDE);

	/*
	 * Start receiving packets
	 */
	pcb.rcv.cmd = EPHC_RCV;
	pcb.rcv.len = PCBARGSIZ(rcv);
	pcb.rcv.off = 0;
	pcb.rcv.segm = 0;
	pcb.rcv.blen = sizeof sc->ep_pb;
	pcb.rcv.timo = 0;
	if (epsendpcb(base, &pcb))
		printf("ep%d: cannot start receiving\n", unit);

	/* Start transmitting */
	ifp->if_flags |= IFF_RUNNING;
	sc->ep_oflags = ifp->if_flags;
	epstart(ifp);
	splx(s);
}

/*
 * Setup output on interface.
 * Get another datagram to send off of the interface queue,
 * and map it to the interface before starting the output.
 * called only at splimp or interrupt level.
 */
int
epstart(ifp)
	struct ifnet *ifp;
{
	struct ep_softc *sc = epcd.cd_devs[ifp->if_unit];
	register base = sc->ep_base;
	struct mbuf *m0, *m;
	int len, s;
	u_short oddword;
	register u_short *bp;
	register i;
	union ep_pcb pcb;

	s = splimp();
	if (sc->ep_if.if_flags & IFF_OACTIVE)
		return;
	sc->ep_if.if_flags |= IFF_OACTIVE;
	IF_DEQUEUE(&sc->ep_if.if_snd, m0);
	if (m0 == 0) {
		sc->ep_if.if_flags &= ~IFF_OACTIVE;
		splx(s);
		return (0);
	}

	/*
	 * Calculate the packet size.
	 * XXX should use m->m_hdr.len.
	 */
	len = 0;
	for (m = m0; m != 0; m = m->m_next)
		len += m->m_len;
	if (len < ETHER_MIN_LEN)
		len = ETHER_MIN_LEN;
	len = (len + 1) & ~01;	/* make the length even */

#if NBPFILTER > 0
	/*
	 * Feed outgoing packet to bpf
	 */
	if (sc->ep_bpf)
		bpf_mtap(sc->ep_bpf, m0);
#endif

	/*
	 * Issue a transmit PCB
	 */
	dprintf(("SEND len=%d --", len));
	outb(base+EP_CTL, EPC_CMDE);	/* clear direction flag */
	pcb.trans.cmd = EPHC_TRANS;
	pcb.trans.len = PCBARGSIZ(trans);
	pcb.trans.off = 0;
	pcb.trans.segm = 0;
	pcb.trans.plen = len;
	if (epsendpcb(base, &pcb)) {
		printf("ep%d: transmit PCB rejected\n", sc->ep_if.if_unit);
		m_freem(m0);
		return (0);
	}

	/*
	 * Transfer data to the board
	 */
	len >>= 1;
	for (m = m0; m != 0; ) {
		if (m->m_len >= 2) {
			i = m->m_len / 2;
			len -= i;
			bp = (u_short *) (m->m_data);
			while (i--) {
				outw(base+EP_DATA, *bp++);
				while ((inb(base+EP_STAT) & EPS_HRDY) == 0) ;
			}
		}
		if (m->m_len & 1) {
			oddword = *(mtod(m, caddr_t) + m->m_len - 1) & 0xff;
			if (m = m->m_next) {
				oddword |= *(mtod(m, caddr_t)) << 8;
				m->m_data++;
				m->m_len--;
			}
			len--;
			outw(base+EP_DATA, oddword);
			while ((inb(base+EP_STAT) & EPS_HRDY) == 0) ;
		} else
			m = m->m_next;
	}
	while (len-- > 0) {	/* Pad the packet */
		outw(base+EP_DATA, 0);
		while ((inb(base+EP_STAT) & EPS_HRDY) == 0) ;
	}
	m_freem(m0);
}

/*
 * Controller interrupt.
 */
int
epintr(arg)
	void *arg;
{
	struct ep_softc *sc = arg;
	register base = sc->ep_base;
	register int len;
	register u_short *bp;
	union ep_pcb pcb;

	dprintf(("I "));

	/*
	 * Extract PCB from the board
	 */
nextpcb:
	if (eprcvpcb(base, &pcb, 1)) {
		printf("ep%d: cannot extract PCB on interrupt\n", sc->ep_if.if_unit);
		return (1);
	}

	/*
	 * Transmission completed?
	 */
	if (pcb.g.cmd == EPAC_TRANS) {
		outb(base+EP_CTL, EPC_CMDE | EPSF_ACCEPT);
		if (pcb.rtrans.len != PCBARGSIZ(rtrans))
			printf("ep%d: corrupted transmit complete PCB\n", sc->ep_if.if_unit);
		if (pcb.rtrans.res == 0) {
			dprintf(("TX OK\n"));
			sc->ep_if.if_opackets++;
			sc->ep_if.if_collisions += pcb.rtrans.txstat & 0xf;
		} else {
			dprintf(("TX ERR, res=%x, txstat=%x\n", pcb.rtrans.res, pcb.rtrans.txstat));
			sc->ep_if.if_oerrors++;
		}
		sc->ep_if.if_flags &= ~IFF_OACTIVE;
		if (sc->ep_if.if_flags & IFF_RUNNING)
			epstart(&sc->ep_if);
		goto rescan;
	}

	/*
	 * Receive completed?
	 */
	if (pcb.g.cmd != EPAC_RCV) {
		outb(base+EP_CTL, EPC_CMDE | EPSF_ACCEPT);
		if (pcb.g.cmd != EPAC_C82586 || sc->ep_if.if_flags & IFF_RUNNING)
			printf("ep%d: got strange PCB 0x%x\n", sc->ep_if.if_unit, pcb.g.cmd);
		goto rescan;
	}
	if (pcb.rrcv.len != PCBARGSIZ(rrcv)) {
		printf("ep%d: corrupted receive complete PCB\n", sc->ep_if.if_unit);
		pcb.rrcv.res = -1;
	}
	dprintf(("RCV len=%d (dmalen=%d)", pcb.rrcv.pklen, pcb.rrcv.dmalen));
	len = (pcb.rrcv.pklen + 1) / 2;
	if (pcb.rrcv.res != 0 || len > (sizeof sc->ep_pb)*2) {
		dprintf(("REJECT\n"));
		sc->ep_if.if_ierrors++;
		outb(base+EP_CTL, EPC_CMDE | EPSF_REJECT);
		goto rescan;
	}

	/*
	 * Extract data from the board
	 */
	outb(base+EP_CTL, EPC_CMDE | EPC_DIR | EPSF_ACCEPT);
	bp = (u_short *) (sc->ep_pb);
	while (len--) {
		while ((inb(base+EP_STAT) & EPS_HRDY) == 0) ;
		*bp++ = inw(base+EP_DATA);
	}
	outb(base+EP_CTL, EPC_CMDE);
	sc->ep_if.if_ipackets++;
	dprintf(("%s -->", ether_sprintf(&sc->ep_pb[6]));
	dprintf(("%s (%x)\n", ether_sprintf(sc->ep_pb), ntohs(*(short *)&sc->ep_pb[12]))));

	/*
	 * Pass the packet to upper levels
	 */
	epread(sc, (caddr_t)(sc->ep_pb), pcb.rrcv.pklen - sizeof(struct ether_header));

	/*
	 * Start the next receive command
	 */
	pcb.rcv.cmd = EPHC_RCV;
	pcb.rcv.len = PCBARGSIZ(rcv);
	pcb.rcv.off = 0;
	pcb.rcv.segm = 0;
	pcb.rcv.blen = sizeof sc->ep_pb;
	pcb.rcv.timo = 0;
	if (epsendpcb(base, &pcb))
		printf("ep%d: cannot restart receiving\n", sc->ep_if.if_unit);

	/* Did we receive any new PCBs? */
rescan:
	if (inb(base+EP_STAT) & EPS_ACRF) {
		dprintf(("R "));
		goto nextpcb;
	}
	return (1);
}

/*
 * Pass a packet to the higher levels.
 * We deal with the trailer protocol here.
 */
void
epread(sc, buf, len)
	register struct ep_softc *sc;
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
#define	epdataaddr(eh, off, type)	((type)(((caddr_t)((eh)+1)+(off))))
	if (eh->ether_type >= ETHERTYPE_TRAIL &&
	    eh->ether_type < ETHERTYPE_TRAIL+ETHERTYPE_NTRAILER) {
		off = (eh->ether_type - ETHERTYPE_TRAIL) * 512;
		if (off >= ETHERMTU)
			return;		/* sanity */
		eh->ether_type = ntohs(*epdataaddr(eh, off, u_short *));
		resid = ntohs(*(epdataaddr(eh, off + 2, u_short *)));
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
        if (sc->ep_bpf) {
                eh->ether_type = htons((u_short)eh->ether_type);
                bpf_tap(sc->ep_bpf, buf, len + sizeof(struct ether_header));
                eh->ether_type = ntohs((u_short)eh->ether_type);

                /*
                 * Note that the interface cannot be in promiscuous mode if
                 * there are no bpf listeners.  And if el are in promiscuous
                 * mode, el have to check if this packet is really ours.
                 *
                 * This test does not support multicasts.
                 */
                if ((sc->ep_if.if_flags & IFF_PROMISC) &&
                    bcmp(eh->ether_dhost, sc->ep_addr,
                    sizeof(eh->ether_dhost)) != 0 &&
                    bcmp(eh->ether_dhost, etherbroadcastaddr, 
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
	m = epget(buf, len, off, &sc->ep_if);
	if (m)
		ether_input(&sc->ep_if, eh, m);
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
epget(buf, totlen, off0, ifp)
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
epioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	register struct ifaddr *ifa = (struct ifaddr *)data;
	register struct ep_softc *sc = epcd.cd_devs[ifp->if_unit];
	int s;
	int error = 0;

	s = splimp();
	switch (cmd) {

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			epinit(ifp->if_unit);	/* before arpwhohas */
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
				ina->x_host = *(union ns_host *)(sc->ep_addr);
			else {
				/* 
				 * The manual says we can't change the address 
				 * while the receiver is armed,
				 * so reset everything
				 */
				ifp->if_flags &= ~IFF_RUNNING; 
				bcopy((caddr_t)ina->x_host.c_host,
				    (caddr_t)sc->ep_addr, sizeof(sc->ep_addr));
			}
			epinit(ifp->if_unit); /* does ne_setaddr() */
			break;
		    }
#endif
		default:
			epinit(ifp->if_unit);
			break;
		}
		break;

	case SIOCSIFFLAGS:
		if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) == IFF_RUNNING) {
			ifp->if_flags &= ~IFF_RUNNING;
			sc->ep_oflags = ifp->if_flags;
			epstop(sc->ep_base);
		} else if ((ifp->if_flags ^ sc->ep_oflags) & (IFF_UP|IFF_PROMISC))
			epinit(ifp->if_unit);
		break;

	default:
		error = EINVAL;
	}
	splx(s);
	return (error);
}
