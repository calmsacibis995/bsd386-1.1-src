/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: if_ef.c,v 1.15 1994/01/05 20:57:38 karels Exp $
 */

/*
 * 3Com Corp. EtherLink III (3C509/3C579) Ethernet Adapter Driver
 */

#include "bpfilter.h"

/* #define EFDEBUG 1 */

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

#ifdef EISA
#include "i386/eisa/eisa.h"
#endif

#if NBPFILTER > 0
#include "../net/bpf.h"
#include "../net/bpfdesc.h"
#endif

#include "if_efreg.h"

#ifdef EFDEBUG
#define dprintf(x)	printf x
#if EFDEBUG > 1
#define dprintf1(x)	printf x
#else
#define dprintf1(x)
#endif
#else
#define dprintf(x)
#define dprintf1(x)
#endif

#define ETHER_MIN_LEN 64
#define ETHER_MAX_LEN 1536

/*
 * Ethernet software status per interface.
 */
struct ef_softc {
	struct	device ef_dev;		/* base device (must be first) */
	struct	isadev ef_id;		/* ISA device */
	struct	intrhand ef_ih;		/* interrupt vectoring */	
	struct	arpcom ef_ac;		/* Ethernet common part */
#define	ef_if	ef_ac.ac_if		/* network-visible interface */
#define	ef_addr	ef_ac.ac_enaddr		/* hardware Ethernet address */
	struct  atshutdown ef_ats;      /* shutdown routine */
	short	ef_base;		/* the board base address */
	short	ef_irq;			/* irq */
	short	ef_conf;		/* POR configuraion */
	short   ef_oldflags;		/* Old interface flags */
	struct	mbuf *ef_mb;		/* local mbuf cache for speed */
	int	ef_flags;
#define	EF_EISA		0x01
#ifdef EFWATCH
#define	EF_TXPEND	0x02
#define	EF_RXPEND	0x04
#define	EF_RXOK		0x08
#define	EF_WATCHFLAGS	(EF_TXPEND|EF_RXPEND|EF_RXOK)
#endif
#if NBPFILTER > 0
	caddr_t	ef_bpf;			/* BPF filter */
#endif
};

#define EF_TIMO		100000	/* Number of wait loops */

/*
 * Prototypes
 */
int efprobe __P((struct device *, struct cfdata *, void *));
void efattach __P((struct device *, struct device *, void *));
int efintr __P((void *));
int efinit __P((int));
int efstart __P((struct ifnet *));
int efioctl __P((struct ifnet *, int, caddr_t));
int efwatchdog __P((int));
void efshutdown __P((void *));

#ifdef EISA
static char *ef_ids[] = {
	"TCM5092",		/* 3C579-TP */
	"TCM5093",		/* 3C579 */
	0
};
#else
#define ef_ids NULL
#endif

struct cfdriver efcd = {
	NULL, "ef", efprobe, efattach, sizeof(struct ef_softc), ef_ids
};
 
/* RX Early threshold */
int	ef_rxearly = MHLEN + sizeof(struct ether_header);
int	ef_txstart = 16;			/* TX Start threshold */
int	efwatch = 1;

/*
 * Probe routine
 */
int
efprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	int i, c, slot, eisa = 0;
	register base;

#ifdef lint
	efintr(0);
#endif

#ifdef EISA
	if ((slot = eisa_match(cf, ia)) != 0) {
		ia->ia_iobase = slot << 12;
		ia->ia_iosize = EISA_NPORT;
		eisa_slotalloc(slot);
		eisa++;
	}
#endif

	/*
	 * Check configuration parameters
	 */
	base = ia->ia_iobase;
	if (!eisa && !EF_IOBASEVALID(base)) {
		printf("ef%d: invalid i/o base address %x\n",
		    cf->cf_unit, base);
		return (0);
	}
	if (!eisa) {
		/*
		 * Now activate the adapter 
		 */
		/* Generate ID sequence */
		outb(EF_ID, 0);
		outb(EF_ID, 0);
		c = 0xff;
		for (i = 255; i; i--) {
			outb(EF_ID, c);
			c <<= 1;
			if (c & 0x100)
				c ^= 0xcf;
		}

		/* Test the tag (turn all activated cards
		 * to the ID_WAIT state)
		 */
		outb(EF_ID, EFID_TESTTAG(0));

		/* Run contention test against 3Com station address */
		for (c = 0; c < (ETHER_ADDR_LEN/2); c++) {
			outb(EF_ID, EFID_RDROM(EFROM_ADDR(c)));
			DELAY(1000);
			for (i = 0; i < 16; i++)
				(void) inb(EF_ID);
		}

		/* Tag the adapter */
		outb(EF_ID, EFID_TAG(cf->cf_unit + 1));

		/* Activate the passed adapter with the given i/o address */
		outb(EF_ID, EFID_ACT(base));
	}

	/*
	 * If we were rebooting then adapters were already configured;
	 * in this case the ID sequences were ignored
	 * Select window 0
	 */
	i = 10000;
	while (inw(base+EF_STAT) & EFS_CIP && --i)
		;
	outw(base+EF_CMD, EFC_WINDOW | 0);

	/*
	 * Check for the presence of the card
	 */
	i = inw(base+EF0_MID);
	if (i != EF_MFG_ID) {
		dprintf(("ef%d: wrong manufacturer id (%x)\n",
			 cf->cf_unit, i));
		return (0);
	}
	if (!eisa) {
		outw(base+EF0_MID, 0);
		outw(base+EF0_PID, 0x1234);
		if (inw(base+EF0_MID) != EF_MFG_ID ||
		    inw(base+EF0_PID) != 0x1234) {
			dprintf(("ef%d: persistence test failed\n",
				 cf->cf_unit));
			return (0);
		}
		ia->ia_iosize = EF_NPORT;
	}

	/*
	 * Disable boot ROM and interrupts even if configured
	 * in EEPROM defaults
	 */
	outw(base+EF0_CC, 0);
	outw(base+EF0_ACNF, EFAC_BNC | EFAC_NOROM |
	     (eisa ?EFAC_EISASLOT :EFAC_BASE(base)));
	outw(base+EF0_RCNF, EFRC_IRQ(0));

	if (ia->ia_irq == IRQUNK) {
		if ((ia->ia_irq = isa_irqalloc(EF_IRQS)) == 0) {
			printf("ef%d: no irq available\n", cf->cf_unit);
			return (0);
		}
	}
	i = ffs(ia->ia_irq) - 1;
	if (!EF_IRQVALID(i)) {
		printf("ef%d: invalid IRQ number %d\n", cf->cf_unit, i);
		return (0);
	} 

	return (1);
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.  We get the ethernet address here.
 */
void
efattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	register struct ef_softc *sc = (struct ef_softc *) self;
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	register struct ifnet *ifp = &sc->ef_if;
	int i;
	int cnt;

	sc->ef_base = ia->ia_iobase;
	sc->ef_irq = ffs(ia->ia_irq) - 1;

#ifdef EISA
	/*
	 * this is a lousy heuristic but since the bus type isn't part of
	 * the device, we can't just look at our parent to see what we are.
	 */
	if (sc->ef_base & EISA_SLOTMASK)
		sc->ef_flags |= EF_EISA;
#endif

	/*
	 * Extract the board address
	 * (Use OEM version)
	 */
	dprintf(("ef: EXTRACT ADDR\n"));
	for (i = 0; i < ETHER_ADDR_LEN/2; i++) {
		outw(sc->ef_base+EF0_PROMCMD, EFPC_RREG(EFROM_ADDR(i)));
		for (cnt = 1000; cnt > 0; cnt--) {
			if (!(inw(sc->ef_base+EF0_PROMCMD) & EFPC_EBY))
				break;
			DELAY(10);
		}
		((u_short *)(sc->ef_addr))[i] = ntohs(inw(sc->ef_base+EF0_PROMDATA));
	}
	
	/* Read the available interfaces */
	sc->ef_conf = inw(sc->ef_base+EF0_CC);
	dprintf(("ef: CCR=%x\n", sc->ef_conf));

	/*
	 * Initialize interface structure
	 */
	ifp->if_unit = sc->ef_dev.dv_unit;
	ifp->if_name = efcd.cd_name;
	ifp->if_mtu = ETHERMTU;
	ifp->if_flags = IFF_BROADCAST | IFF_MULTICAST | IFF_SIMPLEX |
	    IFF_NOTRAILERS;
	ifp->if_init = efinit;
	ifp->if_output = ether_output;
	ifp->if_start = efstart;
	ifp->if_ioctl = efioctl;
	ifp->if_watchdog = efwatchdog;
	if_attach(ifp);

	printf("\nef%d: 3C5%c9 address %s interfaces:",
	       ifp->if_unit, (sc->ef_flags & EF_EISA) ? '7' : '0',
	       ether_sprintf(sc->ef_addr));
	i = 0;
	if (sc->ef_conf & EFCC_AUI)
		printf(" AUI"); i++;
	if (sc->ef_conf & EFCC_BNC)
		printf("%s 10BASE2 (BNC)", i++? "," : "");
	if (sc->ef_conf & EFCC_TP)
		printf("%s 10BASE-T (TP)", i? "," : "");
	printf("\n");

#if NBPFILTER > 0
	bpfattach(&sc->ef_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif

	isa_establish(&sc->ef_id, &sc->ef_dev);
	sc->ef_ih.ih_fun = efintr;
	sc->ef_ih.ih_arg = (void *)sc;
	intr_establish(ia->ia_irq, &sc->ef_ih, DV_NET);

	sc->ef_ats.func = efshutdown;
	sc->ef_ats.arg = (void *)sc;
	atshutdown(&sc->ef_ats, ATSH_ADD);
}

/*
 * Shutdown before reboot
 */
void
efshutdown(arg)
	void *arg;
{
	struct ef_softc *sc = (struct ef_softc *) arg;

	/* Issue global reset -- it takes some long time */
	outw(sc->ef_base+EF_CMD, EFC_RESET);
}

/*
 * Initialization of interface
 */
int
efinit(unit)
	int unit;
{
	register struct ef_softc *sc = efcd.cd_devs[unit];
	struct ifnet *ifp = &sc->ef_if;
	int s;
	int in;
	register base = sc->ef_base;
	register i;

	s = splimp();

	/* Load window 0 configuration stuff */
	i = EF_TIMO;
	while (inw(base+EF_STAT) & EFS_CIP && --i)
		;
	outw(base+EF_CMD, EFC_WINDOW | 0);
	outw(base+EF0_CC, 0);	/* disable adapter */

	in = -1;
	if (ifp->if_flags & IFF_LINK0) {
		if (sc->ef_conf & EFCC_BNC)
			in = EFAC_BNC;
		else
			printf("ef%d: no BNC connector\n", ifp->if_unit);
	} else if (ifp->if_flags & IFF_LINK1) {
		if (sc->ef_conf & EFCC_TP)
			in = EFAC_TP;
		else
			printf("ef%d: no TP connector\n", ifp->if_unit);
	}
	if (in == -1) {
		if (sc->ef_conf & EFCC_AUI)
			in = EFAC_AUI;
		else if (sc->ef_conf & EFCC_TP)
			in = EFAC_TP;
		else
			in = EFAC_BNC;
	}

	outw(base+EF0_ACNF, in | EFAC_NOROM |
	     ((sc->ef_flags & EF_EISA) ? EFAC_EISASLOT : EFAC_BASE(base)));
	outw(base+EF0_RCNF, EFRC_IRQ(sc->ef_irq));
	outw(base+EF0_CC, EFCC_ENA);

	/* Load station address */
	outw(base+EF_CMD, EFC_WINDOW | 2);
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		outb(base+EF2_ADDR(i), sc->ef_addr[i]);

	/* Reset RX and TX */
	outw(base+EF_CMD, EFC_WINDOW | 1);
	outw(base+EF_CMD, EFC_RXRST);
	outw(base+EF_CMD, EFC_TXRST);

	/* Clean up TX status */
	for (i = 0; i < 32; i++)
		(void) inb(base+EF1_TXSTAT);

	/* Clean up interrupts */
	outw(base+EF_CMD, EFC_ACK | 0xff);

	/* Load Interrupt masks */
	outw(base+EF_CMD, EFC_SIM | EFS_AF | EFS_TXC | EFS_TXA | EFS_RXE | EFS_RXC);
	outw(base+EF_CMD, EFC_SZM | EFS_IL | EFS_AF | EFS_TXC | EFS_TXA |
				    EFS_RXE | EFS_RXC | EFS_IREQ);

	/* Set RX filter */
	outw(base+EF_CMD, EFC_SRXF | EFRXF_ADDR | EFRXF_AB |
	    ((sc->ef_ac.ac_multiaddrs ||
	    ifp->if_flags & IFF_ALLMULTI) ? EFRXF_AM : 0) |
	    ((ifp->if_flags & IFF_PROMISC) ? EFRXF_PROM : 0));

	/* Set RX Early threshold */
	outw(base+EF_CMD, EFC_RXEARLY | ef_rxearly);

	/* Set TX Start threshold */
	outw(base+EF_CMD, EFC_TXSTART | ef_txstart); 

	/* For BNC turn on the power converter */
	if (in == EFAC_BNC)
		outw(base+EF_CMD, EFC_COAXSTA);

	/* For TP enable link beat detector and polarity reversal */
	else if (in == EFAC_TP) {
		outw(base+EF_CMD, EFC_WINDOW | 4);
		outw(base+EF4_MTS, EFMTS_JABE | EFMTS_LBE);
		outw(base+EF_CMD, EFC_WINDOW | 1);
	}

	/* enable statistics gathering */
	outw(sc->ef_base+EF_CMD, EFC_STATENB);

	/* Finally enable receiver and transmitter */
	outw(base+EF_CMD, EFC_TXENB);
	outw(base+EF_CMD, EFC_RXENB);

	/* Start transmitting */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;
#ifdef EFWATCH
	sc->ef_flags &= ~EF_WATCHFLAGS;
#endif
	efstart(ifp);
	sc->ef_if.if_timer = efwatch;
	splx(s);
}

/*
 * Stop the interface
 */
void
efstop(base)
	register base;
{
	outw(base+EF_CMD, EFC_RXDIS);
	outw(base+EF_CMD, EFC_TXDIS);
	outw(base+EF_CMD, EFC_COAXSTP);
}

#ifdef EFDEBUG
void
eftrace(int unit, char *label, int len, register struct mbuf *m0)
{
	register int clen = 0;

	printf("ef%d(%s#%d):", unit, label, len);
	for ( ;  m0;  m0 = m0->m_next) {
		printf(" %d@%08lx[%04x]", m0->m_len, m0->m_data, m0->m_flags);
		clen += m0->m_len;
	}
	printf(" [#%d]\n", clen);
}
#endif

/*
 * Setup output on interface.
 * Get another datagram to send off of the interface queue,
 * and map it to the interface before starting the output.
 * called only at splimp or interrupt level.
 */
int
efstart(ifp)
	struct ifnet *ifp;
{
	struct ef_softc *sc = efcd.cd_devs[ifp->if_unit];
	register base = sc->ef_base;
	register eisa = (sc->ef_flags & EF_EISA);
	struct mbuf *m0, *m;
	int len, exactlen;
	u_short oddword;
	register i;
	static char zeropad[4];

	if (ifp->if_flags & IFF_OACTIVE) {
		dprintf(("OACT!!!\n"));
		return; 
	}
nextpacket:
	m = ifp->if_snd.ifq_head;
	if (m == 0)
		return (0);

	/*
	 * Calculate the packet size.
	 * XXX should use m->m_pkthdr.len.
	 */
	len = 0;
	for (; m != 0; m = m->m_next)
		len += m->m_len;
	if (len < ETHER_MIN_LEN)
		len = ETHER_MIN_LEN;
	exactlen = len;
	len = (len + 3) & ~03;	/* make the length dword-aligned */

	/*
	 * Check if there is enough free space in TX buffer
	 */
	i = inw(base+EF1_TXFREE) - 4;	/* exclude preamble */
	if (i < len) {	/* not enough */
		outw(base+EF_CMD, EFC_TXAVAIL | (len + 4));
		ifp->if_flags |= IFF_OACTIVE;
		dprintf1(("TXWAIT-"));
		return (0);	
	}

	IF_DEQUEUE(&ifp->if_snd, m0);
#ifdef EFDEBUG
	if (ifp->if_flags & IFF_DEBUG)
		eftrace(ifp->if_unit, "send", exactlen, m0);
#endif
#if NBPFILTER > 0
	/*
	 * Feed outgoing packet to bpf
	 */
	if (sc->ef_bpf)
		bpf_mtap(sc->ef_bpf, m0);
#endif

	dprintf1(("SEND len=%d --", exactlen));

	/*
	 * Send preamble
	 * (no interrupts on completion requested)
	 */
	if (eisa)
		outl(base+EF1_DATA, exactlen);
	else {
		outw(base+EF1_DATA, exactlen);
		outw(base+EF1_DATA, 0);
	}

	/*
	 * Transfer data to the board
	 * This should be done with interrupts closed
	 * to avoid underruns
	 */
	len >>= 1;
	for (m = m0; m != 0; ) {
#ifdef EISA
		/*
		 * This is ifdef'd to save code space on non-EISA machines,
		 * and it has a "continue" to save indenting subsequent code.
		 */
		if (eisa) {
			if (i = m->m_len >> 2) {
				len -= i << 1;
				outsl(base+EF1_DATA, (u_char*)m->m_data, i);
			}

			if (i = m->m_len & 3) {
				register u_char *dp = ((u_char*)m->m_data) +
				    m->m_len - i;
				register u_long l = *dp++;
				register s = 8;

				/* unrolled loop. */
				if (i > 1) {
					l |= (*dp++ << 8);
					s = 16;
					if (i > 2) {
						l |= (*dp++ << 16);
						s = 24;
					}
				}

				i = 4 - i;
				while (i && (m = m->m_next)) {
					while (i && (m->m_len > 0)) {
						l |= *(mtod(m, u_char*)) << s;
						s += 8;
						m->m_data++;
						m->m_len--;
						i--;
					}
				}
				outl(base+EF1_DATA, l);
				len -= 2;
			} else
				m = m->m_next;
			continue;
		}
#endif /* EISA */
		if (m->m_len >= 2) {
			i = m->m_len / 2;
			len -= i;
			outsw(base+EF1_DATA, m->m_data, i);
		}
		if (m->m_len & 1) {
			oddword = *(mtod(m, caddr_t) + m->m_len - 1) & 0xff;
			if (m = m->m_next) {
				oddword |= *(mtod(m, caddr_t)) << 8;
				m->m_data++;
				m->m_len--;
			}
			len--;
			outw(base+EF1_DATA, oddword);
		} else
			m = m->m_next;
	}

	/* Pad the packet (note: never happens on EISA boards; see above) */
	if (len > 0)
		outsw(base+EF1_DATA, zeropad, len);
	m_freem(m0);
	ifp->if_opackets++;
	goto nextpacket;
}

struct mbuf *
ef_newm(size, needheader)
	int *size;
	int needheader;
{
	register struct mbuf *n;
	register int s;

	if (needheader) {
		MGETHDR(n, M_DONTWAIT, MT_DATA);
		s = MHLEN;
	} else {
		MGET(n, M_DONTWAIT, MT_DATA);
		s = MLEN;
	}
	if (n && *size > s) {
		MCLGET(n, M_DONTWAIT);
		if (n->m_flags & M_EXT)
			s = MCLBYTES;
		else {
			void *t;
			MFREE(n, t);
			n = NULL;
		}
	}
	if (n)
		*size = s;
	return (n);
}

/*
 * Controller interrupt.
 */
int
efintr(arg)
	void *arg;
{
	struct ef_softc *sc = arg;
	struct ifnet *ifp = &sc->ef_if;
	register int base = sc->ef_base;
	register int ts = (sc->ef_flags & EF_EISA) ? 4 : 2;
	register int i, rem, tlen, rxs;
	int stat, txs, waitloops;
	register char *bp;
	struct mbuf *m0;
	register struct mbuf *m;
	struct ether_header *eh;

	dprintf1(("I"));

	/*
	 * Acknowledge interrupt sources
	 * (but not interrupt latch!)
	 */
	stat = 0;
rescan: i = inw(base+EF_STAT);
	dprintf1(("<%x> ", i));
	i &= EFS_AF | EFS_TXC | EFS_TXA | EFS_RXC | EFS_RXE | EFS_UPDS | EFS_IREQ;
	stat |= i;
	if (stat == 0) {	/* NOTHING */
		/* Finally, clear the interrupt latch */
		dprintf1(("ACK\n"));
		outw(base+EF_CMD, EFC_ACK | EFS_IL);
		/*
		 * On some machines, if an interrupt is again pending,
		 * we run a risk of losing the next interrupt when we
		 * ack the interrupt latch here.  If there is another
		 * interrupt now pending, process it now.
		 */
		if (inw(base+EF_STAT) & (EFS_AF | EFS_TXC | EFS_TXA | EFS_RXC | EFS_RXE | EFS_UPDS | EFS_IREQ))
			goto rescan;
		return (1);
	}
	outw(base+EF_CMD, EFC_ACK | i);
	
	/*
 	 * Adapter Failure
	 */
	if (stat & EFS_AF) {
		printf("ef%d: adapter failure\n", ifp->if_unit);
		outw(base+EF_CMD, EFC_ACK | EFS_IL);
		efinit(ifp->if_unit);
		return (1);
	}
		
	/*
	 * Packet reception?
	 */
	if (stat & (EFS_RXE|EFS_RXC)) {
#ifdef EFWATCH
		sc->ef_flags &= ~EF_RXPEND;
#endif
#ifdef EFDEBUG
		if (ifp->if_flags & IFF_DEBUG) {
			rxs = inw(base+EF1_RXSTAT);
			printf("ef%d: [%c%c%c] (%d)\n",
			       ifp->if_unit,
			       (stat&EFS_RXE) ?'E' :' ',
			       (stat&EFS_RXC) ?'C' :' ',
			       (rxs&EFRS_INCOMPL) ?'I' :' ',
			       rxs & EFRS_BYTES);
		}
#endif
		stat &= ~(EFS_RXE|EFS_RXC);

/*-- begin macro --*/
#define EF_NEWM { \
		if (rem == 0) { \
			int _size = (rxs & EFRS_INCOMPL) ? MCLBYTES : bytes; \
			if (!(m->m_next = ef_newm(&_size, (m == m0)))) { \
				m_freem(m0); \
				goto mbuferr; \
			} \
			m = m->m_next; \
			m->m_len = 0; \
			rem = _size; \
			bp = mtod(m, char*); \
		} \
	}
/*-- end macro --*/

		if (m0 = sc->ef_mb)
			sc->ef_mb = NULL;
		else {
			MGETHDR(m0, M_DONTWAIT, MT_DATA);
			if (!m0) {
mbuferr:			log(LOG_ERR, "ef%d: mbuf problem",
				    ifp->if_unit);
				goto rxerror;
			}
		}

		eh = mtod(m0, struct ether_header *);
		m = m0;
		m->m_len = 0;
		bp = mtod(m, char*);
		rem = sizeof(struct ether_header);
		tlen = 0;
		waitloops = 0;
		txs = 0;
		for (;;) {
			register int bytes;

			rxs = inw(base+EF1_RXSTAT);
			bytes = (rxs & EFRS_BYTES);

			/* 
			 * Since C's "&" operator doesn't sign-extend, the
			 * "read past end of FIFO" condition looks like a
			 * high positive number.  this occurs because we use
			 * inl/inw to get the last dregs of a packet, which
			 * can fetch pad bytes, which causes the EFRS_BYTES
			 * field to go what the 3Com manual calls "negative".
			 */
			if (!(rxs & EFRS_INCOMPL) &&
			    (bytes == 0 || bytes > ETHERMTU + sizeof(*eh)))
				break;

			if (bytes == 0) {
rxwait:				if (waitloops++ > EF_TIMO) {
					log(LOG_ERR, "ef%d: slow receive\n",
					    ifp->if_unit);
					m_freem(m0);
					goto rxerror;
				}
				continue;
			}
			txs++;

			/*
			 * Get full words, if any, using string-IO.
			 */
			if (bytes >= ts) {
				EF_NEWM;
				i = min(bytes, rem) & ~(ts - 1);
				if (i) {
					if (ts == 4)
					    insl(base+EF1_DATA, bp, i >> 2);
					else
					    insw(base+EF1_DATA, bp, i >> 1);
					bp += i;
					rem -= i;
					m->m_len += i;
					tlen += i;
					bytes -= i;
				}
				if (rem == 0)
					continue;
			}

			/*
			 * We can't read in the rest of our mbuf until there
			 * is a full ts in the fifo or a complete packet.
			 */
			if (bytes < ts && rxs & EFRS_INCOMPL)
				goto rxwait;

			/*
			 * now read partial bytes, if any.
			 */
			if (bytes > 0) {
				register u_long l;

				if (ts == 4)
					l = inl(base+EF1_DATA);
				else
					l = inw(base+EF1_DATA);
				for (i = min(bytes, ts);  i > 0;  i--) {
					EF_NEWM;
					*bp++ = l;
					l >>= 8;
					m->m_len++;
					tlen++;
					rem--;
					bytes--;
				}
			}
		}

		if (m0->m_next == NULL) {
			/*
			 * There is no such thing as a useful ethernet
			 * packet which contains only a 14-byte header.
			 */
			/* log(LOG_ERR, "ef%d: runt rcvd\n", ifp->if_unit); */
			m_freem(m0);
			goto rxerror;
		}

		if (rxs & EFRS_ERR) {
			dprintf(("RXERR: rxs=%x\n", rxs));
			switch (rxs & EFRS_ETYPE) {
			case EFET_DRIB:
			case EFET_OVRSIZ:
				break;	/* should be treated as "normal" */

			default:
				m_freem(m0);
rxerror:
				outw(base+EF_CMD, EFC_RXDTP);
				ifp->if_ierrors++;
				goto rxdone;
			}
		}

		/* Start discarding the packet */
		dprintf1(("RX %d/%d WL ", waitloops, txs));
		outw(base+EF_CMD, EFC_RXDTP);
#ifdef EFWATCH
		sc->ef_flags |= EF_RXOK;
#endif

		/*
		 * Pass the packet to upper levels
		 */
		ifp->if_ipackets++;

		dprintf1(("%s -->", ether_sprintf(eh->ether_shost)));
		dprintf1(("%s (%x)", ether_sprintf(eh->ether_dhost),
			  ntohs(eh->ether_type)));
		dprintf1((" len=%d\n", m0->m_pkthdr.len));

#ifdef EFDEBUG
		if (ifp->if_flags & IFF_DEBUG)
			eftrace(ifp->if_unit, "recv0", tlen, m0);
#endif

#if NBPFILTER > 0
		/*
		 * Check if there's a bpf filter listening on this interface.
		 * If so, hand off the raw packet to bpf, which must deal with
		 * trailers in its own way.
		 */
		if (sc->ef_bpf) {
			m0->m_pkthdr.rcvif = ifp;
			m0->m_pkthdr.len = tlen;
			bpf_mtap(sc->ef_bpf, m0);

			/*
			 * Note that the interface cannot be in promiscuous
			 * mode if there are no bpf listeners.  And if we are
			 * in promiscuous mode, we have to check if this
			 * packet is really ours.
			 */
			if ((ifp->if_flags & IFF_PROMISC) &&
			    bcmp(eh->ether_dhost, sc->ef_addr,
			    sizeof(eh->ether_dhost)) != 0 &&
#ifdef MULTICAST
			    /* non-multicast (also non-broadcast) */
			    !ETHER_IS_MULTICAST(eh->ether_dhost)
#else
			    bcmp(eh->ether_dhost, etherbroadcastaddr, 
			    sizeof(eh->ether_dhost)) != 0
#endif
			    ) {
				m_freem(m0);
				goto rxdone;
			}
		}
#endif /* BPF */

		/*
		 * XXX the last mbuf on the chain can be a cluster whose
		 * m_len<=MLEN if INCOMPL didn't shut off soon enough.
		 * we could copy it down to a nonclustered mbuf and
		 * cache the cluster for next time, but since we take
		 * care of whole packets whose len<=MHLEN, this is
		 * probably not worth doing here.  slow machines will
		 * tend to see INCOMPL before they can allocate the
		 * oversized mbuf, and fast ones generally don't care
		 * if they make a suboptimal decision once in a while.
		 */

		tlen -= sizeof(struct ether_header);
		m = m0->m_next;
		m->m_pkthdr.rcvif = ifp;
		m->m_pkthdr.len = tlen;
		m0->m_next = NULL;		/* not strictly nec'y */

#ifdef EFDEBUG
		if (ifp->if_flags & IFF_DEBUG)
			eftrace(ifp->if_unit, "recv1", tlen, m);
#endif

		eh->ether_type = ntohs(eh->ether_type);
		ether_input(ifp, eh, m);
		sc->ef_mb = m0;			/* save it for next time */
rxdone:
		/* Wait for CIP to go down */
		i = EF_TIMO;
		while ((inw(base+EF_STAT) & EFS_CIP) && i--)
			;
		goto rescan;
	}

	/*
	 * Transmission completed -- check for errors
	 */
	if (stat & EFS_TXC) {
#ifdef EFWATCH
		sc->ef_flags &= ~EF_TXPEND;
#endif
		stat &= ~EFS_TXC;
		while ((txs = inb(base+EF1_TXSTAT)) & EFTS_COMPL) {
			outb(base+EF1_TXSTAT, 0);	/* pop up the stack */
			dprintf(("TXERR txs=%x\n", txs));
			if (txs & (EFTS_UNDR | EFTS_JAB)) {
				/* Reset transmitter */
				outw(base+EF_CMD, EFC_TXRST);
				ifp->if_opackets--;
				ifp->if_oerrors++;
			} else if(txs & EFTS_MAXCOL) {
				ifp->if_opackets--;
				ifp->if_collisions += 16;
			}
			/* Enable transmitter */
			outw(base+EF_CMD, EFC_TXENB);
			goto txrestart;
		}
		goto rescan;
	}

	/*
	 * TX buffer space available
	 */
	if (stat & EFS_TXA) {
#ifdef EFWATCH
		sc->ef_flags &= ~EF_TXPEND;
#endif
		stat &= ~EFS_TXA;
		dprintf1(("TXA-"));
txrestart:
		ifp->if_flags &= ~IFF_OACTIVE;
		(void) inw(base+EF1_TXFREE);	/* doc requires it dunno why */
		efstart(ifp);
		goto rescan;
	}

	if (stat & (EFS_UPDS | EFS_IREQ)) {
		printf("ef%d: bad status %x\n", ifp->if_unit, stat);
		stat = 0;
	}
	goto rescan;
}

int
efwatchdog(unit)
	int unit;
{
	register struct ef_softc *sc = efcd.cd_devs[unit];
#ifdef EFWATCH
	struct ifnet *ifp = &sc->ef_if;
	int stat, rxs, needintr = 0;

	stat = inw(sc->ef_base+EF_STAT);
	if (stat & EFS_RXC) {
		if (sc->ef_flags & EF_RXPEND) {
			printf("ef%d: lost rx intr, stat %x, rstat %x: ",
			    unit, stat, (rxs = inw(sc->ef_base+EF1_RXSTAT)));
			if ((rxs&EFRS_INCOMPL) == 0 && sc->ef_flags & EF_RXOK)
				needintr = 1;
			else {
				printf("reset\n");
				ifp->if_flags &= ~(IFF_RUNNING|IFF_OACTIVE);
				efstop(sc->ef_base);
				sc->ef_flags &= ~EF_RXOK;
				efinit(ifp->if_unit);
			}
		} else
			sc->ef_flags |= EF_RXPEND;
	}
	if (ifp->if_flags & IFF_OACTIVE) {
		if (sc->ef_flags & EF_TXPEND) {
			printf("ef%d: lost tx intr, stat %x\n", unit, stat);
			needintr = 1;
		} else
			sc->ef_flags |= EF_TXPEND;
	}
	if (needintr)
		efintr(sc);
#endif

	/*
	 * Collect collision statistics.
	 */
	outw(sc->ef_base+EF_CMD, EFC_WINDOW | 6);
	outw(sc->ef_base+EF_CMD, EFC_STATDIS);
	sc->ef_if.if_collisions += inb(sc->ef_base+EF6_OCOL) +
	     2 * inb(sc->ef_base+EF6_MCOL);	/* minimum number */
	outw(sc->ef_base+EF_CMD, EFC_STATENB);
	outw(sc->ef_base+EF_CMD, EFC_WINDOW | 1);

	sc->ef_if.if_timer = efwatch;
}

/*
 * Process an ioctl request.
 */
int
efioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	register struct ifaddr *ifa = (struct ifaddr *)data;
	register struct ef_softc *sc = efcd.cd_devs[ifp->if_unit];
	int s;
	int error = 0;

	s = splimp();
	switch (cmd) {

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			efinit(ifp->if_unit);	/* before arpwhohas */
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
				ina->x_host = *(union ns_host *)(sc->ef_addr);
			else {
				/* 
				 * The manual says we can't change the address 
				 * while the receiver is armed,
				 * so reset everything
				 */
				ifp->if_flags &= ~IFF_RUNNING; 
				bcopy((caddr_t)ina->x_host.c_host,
				    (caddr_t)sc->ef_addr, sizeof(sc->ef_addr));
			}
		    }
			/*FALL THROUGH*/
#endif
		default:
			efinit(ifp->if_unit);
			sc->ef_oldflags = ifp->if_flags;
			break;
		}
		break;

	case SIOCSIFFLAGS:
		if ((ifp->if_flags ^ sc->ef_oldflags) &
		    (IFF_UP|IFF_PROMISC|IFF_LINK0|IFF_LINK1)) {
			if (ifp->if_flags & IFF_UP)
				efinit(ifp->if_unit);
			else {
				ifp->if_flags &= ~(IFF_RUNNING|IFF_OACTIVE);
				efstop(sc->ef_base);
			}
		}
		sc->ef_oldflags = ifp->if_flags;
		break;

#ifdef MULTICAST
	/*
	 * Update our multicast list.
	 */
	case SIOCADDMULTI:
		error = ether_addmulti((struct ifreq *)data, &sc->ef_ac);
		goto reset;

	case SIOCDELMULTI:
		error = ether_delmulti((struct ifreq *)data, &sc->ef_ac);
	reset:
		if (error == ENETRESET) {
			efinit(ifp->if_unit);
			error = 0;
		}
		break;
#endif

	default:
		error = EINVAL;
	}
	splx(s);
	return (error);
}
