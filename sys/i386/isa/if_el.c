/*
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI:	$Id: if_el.c,v 1.9 1993/12/19 04:22:26 karels Exp $
 */

/*
 * 3Com Corp. EtherLink 16 (3C507) Ethernet Card Driver.
 *
 * This card is based on i82586 Ethernet chip.
 *
 * Multicast address support added Apr 11 1993, by Steven McCanne,
 * mccanne@ee.lbl.gov.  Changes adapted from the Stanford diffs to
 * Sun's ie driver.
 */

#include "bpfilter.h"

/* #define ELDEBUG */

#include "param.h"
#include "mbuf.h"
#include "socket.h"
#include "device.h"
#include "ioctl.h"
#include "errno.h"
#include "syslog.h"

#include "net/if.h"
#include "net/netisr.h"

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

#include "ic/i82586.h"
#include "if_elreg.h"
#include "isa.h"
#include "isavar.h"
#include "icu.h"

#include "machine/cpu.h"

#if NBPFILTER > 0
#include "../net/bpf.h"
#include "../net/bpfdesc.h"
#endif

/* There cannot be more than 1 EtherLink card -- it's a hardware limitation */
#define MAX_EL 1

#define ETHER_MIN_LEN 64
#define ETHER_HDR_SIZE 14

#ifdef ELDEBUG
#define dprintf(x)	printf x
#else
#define dprintf(x)	
#endif
 
/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * qe_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 */
struct	el_softc {
	struct	device el_dev;		/* base device */
	struct	isadev el_id;		/* ISA device */
	struct	intrhand el_ih;		/* intrrupt vectoring */
	struct	arpcom el_ac;		/* Ethernet common part */
#define	el_if	el_ac.ac_if		/* network-visible interface */
#define	el_addr	el_ac.ac_enaddr		/* hardware Ethernet address */

	int     el_flags;               /* the device flags */
	short   el_irq;                 /* interrupt level */
	short	el_io;			/* i/o bus address */
	short	el_memconf;		/* memory configuartion byte */
	u_short	el_daddr;		/* card RAM chip's memory base */
	u_long	el_memsize;		/* card RAM bytes */
	caddr_t	el_vaddr;		/* card RAM virtual memory base */

	u_short	el_nthdrs;		/* Number of TX command headers */
	u_short el_tfreehdr;		/* List of available TX headers */
	u_short el_tqhead;		/* Head of TX queue */
	u_short el_tqtail;		/* Tail of TX queue */
	u_short el_tarea;		/* The beginning of TX area */
	u_short el_tend;		/* End of TX area */
	u_short el_tbusy;		/* Ptr to busy space in TX area */
	u_short el_tavail;		/* Ptr to the avail space in TX area */

	u_short el_nrhdrs;		/* Number of RX frame headers */
	u_short el_nrbufs;		/* Number of RX buffers */
	u_short el_rhdr;		/* The current RX frame hdr */
	u_short el_rbuf;		/* The current RX buffer */

#if NBPFILTER > 0
	caddr_t	el_bpf;			/* BPF filter */
#endif
	char	el_ipacket[ETHERMTU+ETHER_HDR_SIZE];	/* Received packet */
};

/* Device flags */
#define EL_CONF_BNC	01	/* configured for BNC connector */


/* Offset of a structure in the el_memory */
#define off(x)	((u_short)&(((struct el_memory *)0)->x))

#define ELTIMO	100000

#define ATTN(sc)	outb(el_cattn(sc->el_io), 0)
#define WAIT_I(sc, cnt)	for (cnt = ELTIMO; cnt && \
			(inb(el_ctrl(sc->el_io)) & EL_INT) == 0; cnt--) 
#define CLR_I(sc)	outb(el_iclr(sc->el_io), 0)

#define WAIT_SCBCLR(sc, em, cnt, unit) \
	for (;;) { \
		for (cnt = ELTIMO; cnt && em->Scb.cmd; cnt--) \
			; \
		if (cnt > 0) \
			break; \
		printf("el%d: timeout waiting SCB cmd clearing\n", unit); \
		ATTN(sc); \
	}

/* Tuneable parameters */
#define RXBSIZ	128	/* size of RX buffer */

#define TXSPACE	(ETHERMTU + 64)	/* Guaranteed TX buffer space per 8K segment */

#define AVGPLEN	(ETHERMTU / 2)	/* An average packet length */


#ifdef ELDEBUG
#define ELRESQUE(unit) \
	printf("rbuf=%d rbuf->nextbd=%d rhdr=%d rhdr->link=%d tqh=%d tqt=%d\n", \
	sc->el_rbuf, ((struct iel_rbd *)(sc->el_rbuf + sc->el_vaddr))->nextbd, \
	sc->el_rhdr, ((struct iel_rfd *)(sc->el_rhdr + sc->el_vaddr))->link, \
	sc->el_tqhead, sc->el_tqtail)
#else
#define ELRESQUE(unit)	elinit(unit)
#endif

/*
 * Exported functions
 */
int	elprobe __P((struct device *parent, struct cfdata *cf, void *aux));
void	elattach __P((struct device *parent, struct device *self, void *aux));
int	elstart __P((struct ifnet *ifp));
int	elintr __P((void *arg));
int	elinit __P((int unit));
int	elioctl __P((struct ifnet *ifp, int cmd, caddr_t data));
int	elwatchdog __P((int unit));

/*
 * Local procedures
 */
static	void elidgen();
static	int elprogram __P((int unit));
static	int eli82586init __P((int unit));
static	void elsetaddr __P((int unit));
static	void elrun __P((struct el_softc *sc));
static	void elread __P((struct el_softc *sc, int len));
static	struct mbuf *elget __P((int totlen, int off, struct el_softc *sc));
static	void slowcopy __P((caddr_t from, caddr_t to, int cnt));

int	ether_output();

struct cfdriver elcd = {
	NULL, "el", elprobe, elattach, sizeof(struct el_softc), NULL, 0
};

static struct el_rambase el_ramtab[] = EL_RAMTAB;

/*
 * Probe the EL8003 to see if it's there
 */
elprobe(parent, cf, aux)
        struct device *parent;
        struct cfdata *cf;
        void *aux;
{
        register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	register int i;
	struct el_rambase *rmbp;
	int irq;
	
	/*
	 * Check the validity of configuration parameters
	 */
	dprintf(("ELPROBE\n"));
	if (cf->cf_unit >= MAX_EL)
		return (0);
	if (ia->ia_iobase < 0x200 || (ia->ia_iobase & 0xf)) {
		printf("el%d: illegal base port number 0x%x\n",
			cf->cf_unit, ia->ia_iobase);
		return (0);
	}
	for (rmbp = el_ramtab; rmbp->elr_size; rmbp++) {
		if ((int)(ia->ia_maddr) == rmbp->elr_addr &&
		     ia->ia_msize == rmbp->elr_size)
			goto ramok;
	}
	printf("el%d: illegal RAM address/size combination (0x%x, %d)\n",
	    cf->cf_unit, ia->ia_maddr, ia->ia_msize);
	return (0);
ramok:
	/*
	 * First, load the ID Pattern twice to the ID register
	 */
	outb(EL_ID, 0);
	elidgen();
	elidgen();

	/*
	 * Now, we are able to load the base I/O port address to the ID
	 * This operation will bring contoller to CONFIG state.
	 */
	outb(EL_ID, (ia->ia_iobase - 0x200) >> 4);

	/*
	 * Read 3COM signature (asserting 82586 reset)
	 */
	outb(el_ctrl(ia->ia_iobase), EL_VB1|EL_LAE);
	for (i = 0; i < 5; i++)
		if (inb(el_nmd(ia->ia_iobase, i)) != EL_SIGN[i])
			return (0);

	if (ia->ia_irq == IRQUNK)
		if ((ia->ia_irq = isa_irqalloc(EL_IRQS)) == 0) {
			printf("el%d: no irq available\n", cf->cf_unit);
			return (0);
		}
	irq = ffs(ia->ia_irq) - 1;
	if (!EL_ILALLOWED(irq)) {
		printf("el%d: unsupported IRQ number %d\n", cf->cf_unit, irq);
		return (0);
	} 

	ia->ia_iosize = EL_NPORT;
	return (1);
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
void
elattach(parent, self, aux)
        struct device *parent, *self;
        void *aux;
{
        register struct el_softc *sc = (struct el_softc *) self;
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	register struct ifnet *ifp = &sc->el_if;
	struct el_rambase *rmbp;
	int i;
 
	/*
	 * Store hw parameters
	 */
	for (rmbp = el_ramtab; rmbp->elr_size; rmbp++) {
		if ((int)(ia->ia_maddr) == rmbp->elr_addr &&
		     ia->ia_msize == rmbp->elr_size)
			break;
	}
	sc->el_memconf = rmbp->elr_val;
	sc->el_io = ia->ia_iobase;
	sc->el_irq = ffs(ia->ia_irq)-1;
	sc->el_vaddr = (caddr_t) ISA_HOLE_VADDR(ia->ia_maddr);
	sc->el_memsize = ia->ia_msize;
	/* Memory always ends at 0xffff */
	sc->el_daddr = 0x10000 - ia->ia_msize;
	sc->el_if.if_flags = 0;

	/*
	 * Initialize the board
	 */
	if (elprogram(sc->el_dev.dv_unit) == 0) {
		printf(": init failed\n");
		return;
	}

	/*
	 * Read Ethernet address
	 */
	outb(el_ctrl(sc->el_io), EL_VB2|EL_LAE);
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		sc->el_addr[i] = inb(el_nmd(sc->el_io, i));

	/*
	 * Initialize ifnet structure
	 */
	ifp->if_unit = sc->el_dev.dv_unit;
	ifp->if_name = "el";
	ifp->if_mtu = ETHERMTU;
	ifp->if_flags = IFF_BROADCAST | IFF_MULTICAST | IFF_SIMPLEX |
	    IFF_NOTRAILERS;
	ifp->if_init = elinit;
	ifp->if_output = ether_output;
	ifp->if_start = elstart;
	ifp->if_ioctl = elioctl;
	ifp->if_watchdog = elwatchdog;
	if_attach(ifp);
	printf(": 3C507, address %s\n", ether_sprintf(sc->el_addr));
#if NBPFILTER > 0
	bpfattach(&sc->el_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif
        isa_establish(&sc->el_id, &sc->el_dev);
        sc->el_ih.ih_fun = elintr;
        sc->el_ih.ih_arg = (void *)sc;
        intr_establish(ia->ia_irq, &sc->el_ih, DV_NET);
}

/*
 * Do 16-bit memory copy
 */
static void
slowcopy(from, to, cnt)
	register caddr_t from;
	register caddr_t to;
	register int cnt;
{
	int i;

	if ((int)from & 1 || (int)to & 1) {
		while (cnt-- > 0)
			*to++ = *from++;
	} else {
		i = cnt;
		cnt >>= 1;
		while (cnt-- > 0)
			*((u_short *)to)++ = *((u_short *)from)++;
		if (i & 1)
			*to++ = *from++;
	}
}
 
/*
 * Initialize the board and i82586 LAN co-processor
 */
static int
elprogram(unit)
	int unit;
{
	register struct el_softc *sc = elcd.cd_devs[unit];
	register int i;
	caddr_t p;

	/*
	 * Load the irq and memory parameters
	 * 	(IFF_BNC switches between BNC and DIX)
	 */
	sc->el_flags = sc->el_if.if_flags;
	if (sc->el_if.if_flags & IFF_BNC)
		outb(el_romcnf(sc->el_io), EL_NOROM|EL_BNC);
	else
		outb(el_romcnf(sc->el_io), EL_NOROM);
	outb(el_ramcnf(sc->el_io), sc->el_memconf);
	dprintf(("EL - SET IRQ %d\n", sc->el_irq));
	outb(el_intcnf(sc->el_io), sc->el_irq);

	/*
	 * Configuration of I/O and RAM completed,
	 * bring the board to the RUN state
	 */
	outb(EL_ID, 0);
	elidgen();
	outb(EL_ID, 0);
	dprintf(("RAM conf=%x (should be %x)\n", inb(el_ramcnf(sc->el_io)), sc->el_memconf));

	/*
	 * Test RAM
	 */
	for (p = sc->el_vaddr + sc->el_memsize-1; p>=sc->el_vaddr; p -= 0x100) {
		*p = 0x5a;
		if (*(u_char *)p != 0x5a) {
		memerr:	printf("el%d: memory test failed\n", unit);
			return (0);
		}
		*p = 0xa5;
		if (*(u_char *)p != 0xa5)
			goto memerr;
	}

	/*
	 * Calculate parameters of RX and TX rings
	 */
	sc->el_nthdrs = ((sc->el_memsize / 8192) * TXSPACE + AVGPLEN - 1) /
	    AVGPLEN;

	i = sc->el_memsize - sizeof(struct iel_scp) - sizeof(struct el_memory);
	i -= sc->el_nthdrs * (sizeof(struct iel_transmit) +
	    sizeof(struct iel_tbd));
	i -= (sc->el_memsize / 8192) * TXSPACE;

	sc->el_nrhdrs = i / (AVGPLEN + sizeof(struct iel_rfd));
	i -= sc->el_nrhdrs * sizeof(struct iel_rfd);
	sc->el_nrbufs = i / (RXBSIZ + sizeof(struct iel_rbd));
	i -= sc->el_nrbufs * (RXBSIZ + sizeof(struct iel_rbd));

	sc->el_tarea = sizeof(struct el_memory);
	sc->el_tend = sc->el_tarea + (sc->el_memsize / 8192) * TXSPACE + i;

	dprintf(("TX: hdrs=%d ringsize=%d", sc->el_nthdrs, sc->el_tend-sc->el_tarea));
	dprintf((" mem=%d\n", sc->el_nthdrs*sizeof(struct iel_transmit)+
		sc->el_tend - sc->el_tarea));
	dprintf(("RX: hdrs=%d bufs=%d", sc->el_nrhdrs, sc->el_nrbufs));
	dprintf((" mem=%d\n", sc->el_nrhdrs * sizeof(struct iel_rfd)+
		sc->el_nrbufs * (sizeof(struct iel_rbd) + RXBSIZ)));

	return (eli82586init(unit));
}

/*
 * Intialize the i82586 chip
 */
static int
eli82586init(unit)
	int unit;
{
	register struct el_softc *sc = elcd.cd_devs[unit];
	int i, cnt;
	struct iel_scp *scp;
	register volatile struct el_memory *em;
	register caddr_t p;
	u_short offset;

	/*
	 * Initialize the TX avail list
	 */
	dprintf(("eli82586init\n"));
	sc->el_tavail = sc->el_tarea;
	sc->el_tbusy = sc->el_tend;

	offset = sc->el_tend;
	sc->el_tfreehdr = offset;
	p = offset + sc->el_vaddr;
#define SHIFT	(sizeof(struct iel_transmit) + sizeof(struct iel_tbd))
	for (i = 0; i < sc->el_nthdrs; i++) {
		offset += SHIFT;
		((struct iel_transmit *)p)->link = offset;
		((struct iel_transmit *)p)->tbd = offset -
		    sizeof(struct iel_tbd);
		p += SHIFT;
	}
	p -= SHIFT;
	((struct iel_transmit *)p)->link = 0;
	sc->el_tqhead = sc->el_tqtail = 0;
#undef SHIFT

	/*
	 * Intialize RX frames and buffers rings
	 */
	p = offset + sc->el_vaddr;
	sc->el_rhdr = offset;
	for (i = 1; i < sc->el_nrhdrs; i++) {
		offset += sizeof(struct iel_rfd);
		((struct iel_rfd *)p)->status = 0;
		((struct iel_rfd *)p)->cmd = 0;
		((struct iel_rfd *)p)->link = offset;
		((struct iel_rfd *)p)->rbd = 0xffff;
		p += sizeof(struct iel_rfd);
	}
	((struct iel_rfd *)p)->status = 0;
	((struct iel_rfd *)p)->cmd = 0;
	((struct iel_rfd *)p)->link = sc->el_rhdr;
	offset += sizeof(struct iel_rfd);

	p = offset + sc->el_vaddr;
	sc->el_rbuf = offset;
	for (i = 1; i < sc->el_nrbufs; i++) {
		offset += sizeof(struct iel_rbd) + RXBSIZ;
		((struct iel_rbd *)p)->actcnt = 0;
		((struct iel_rbd *)p)->nextbd = offset;
		((struct iel_rbd *)p)->buf = offset - RXBSIZ + sc->el_daddr;
		((struct iel_rbd *)p)->elsize = RXBSIZ;
		p += sizeof(struct iel_rbd) + RXBSIZ;
	}
	((struct iel_rbd *)p)->actcnt = 0;
	((struct iel_rbd *)p)->nextbd = sc->el_rbuf;
	((struct iel_rbd *)p)->buf = offset + sizeof(struct iel_rbd) +
	    sc->el_daddr;
	((struct iel_rbd *)p)->elsize = RXBSIZ;

 	/*
	 * Intialize the commands area
	 */
	scp = (struct iel_scp *)(sc->el_vaddr + IEL_SCPOFF - sc->el_daddr);
	bzero((caddr_t)scp, sizeof(struct iel_scp));
	scp->iscp_off = sc->el_daddr + off(Iscp);

	em = (struct el_memory *)(sc->el_vaddr);
	em->Iscp.busy = 1;
	em->Iscp.scboff = off(Scb);
	em->Iscp.base1 = sc->el_daddr;
	em->Iscp.base2 = 0;

	em->Config.cmd = IEL_CONFIG | IEL_EL;
	em->Config.status = 0;
	em->Config.link = 0;
	em->Config.bytecnt = 12;
	em->Config.fifolim = 8;
	em->Config.misc1 = IEL_PREAMLEN8 | IEL_ALLOC | IEL_ADDRLEN(ETHER_ADDR_LEN);
	em->Config.misc2 = 0;
	em->Config.ifspc = 96;
	em->Config.sltime = IEL_NRTRY(15) | IEL_SLOTTIM(512);
#if NBPFILTER > 0
	if (sc->el_if.if_flags & IFF_PROMISC)
		em->Config.misc3 = IEL_PRM;
	else
#endif
		em->Config.misc3 = 0;
	em->Config.minfrm = 60; /*ETHER_MIN_LEN*/
	em->Config.zero = 0;

	/*
	 * Run the initalization
	 */
	em->Scb.cmd = 0;
	em->Scb.cmdlist = off(Config);
	em->Scb.rfa = 0;
	/* Drop reset condition */
	outb(el_ctrl(sc->el_io), EL_NORST|EL_VB1|EL_LAE);
	ATTN(sc);
	
	/* Wait for Iscp.busy == 0 */
	for (cnt = ELTIMO; cnt && em->Iscp.busy; cnt--)
		;
	if (cnt == 0) {
		printf("el%d: timeout on init after reset\n", unit);
		return (0);
	}
	CLR_I(sc);

	/*
	 * Start excecution of the CONFIG instruction
	 */
	em->Scb.cmdlist = off(Config);
	em->Scb.cmd = IEL_CUC_START;
	ATTN(sc);
	for (cnt = ELTIMO; cnt && !(em->Config.status & IEL_C); cnt--)
		;
	if (cnt == 0) {
		printf("el%d: timeout on configure command\n", unit);
		return (0);
	}
	if ((cnt = em->Config.status) != (IEL_OK|IEL_C)) {
		printf("el%d: cannot configure i82586\n", unit);
		dprintf(("cmd status=%x\n", cnt));
		return (0);
	}

	/*
	 * Acknowledge condition bits
	 */
	em->Scb.cmd = em->Scb.status & IEL_ACK;
	ATTN(sc);
	for (cnt = ELTIMO; cnt && em->Scb.status; cnt--)
		;
	if (cnt == 0) {
		printf("el%d: timeout on status acknowledgement\n", unit);
		return (0);
	}
	CLR_I(sc);
	return (1);
}

/*
 * ID pattern generator
 */
static void
elidgen()
{
	register int i;
	register int c;

	c = 0xff;
	for (i = 255; i; i--) {
		outb(EL_ID, c);
		c <<= 1;
		if (c & 0x100)
			c ^= 0xe7;
	}
}

/*
 * Load the hardware interface address
 */
static void
elsetaddr(unit)
	int unit;
{
	register struct el_softc *sc = elcd.cd_devs[unit];
	int i, cnt;
	register volatile struct el_memory *em;

	dprintf(("elsetaddr\n"));
	em = (struct el_memory *)sc->el_vaddr;

	em->Iasetup.cmd = IEL_EL|IEL_I|IEL_IA_SETUP;
	em->Iasetup.status = 0;
	slowcopy((caddr_t)sc->el_addr, (caddr_t)em->Iasetup.eaddr, ETHER_ADDR_LEN);
	em->Scb.cmdlist = off(Iasetup);
	em->Scb.cmd = IEL_CUC_START;
	ATTN(sc);
	WAIT_I(sc, cnt);
	if (cnt == 0)
		printf("el%d: timeout on IA SETUP\n", unit);
	CLR_I(sc);
	if (em->Iasetup.status != (IEL_OK|IEL_C))
		printf("el%d: cannot setup hw address\n", unit);

	/*
	 * Acknowledge condition bits
	 */
	em->Scb.cmd = em->Scb.status & IEL_ACK;
	ATTN(sc);
	for (cnt = ELTIMO; cnt && em->Scb.status; cnt--)
		;
	if (cnt == 0)
		printf("el%d: timeout on status acknowledgement\n", unit);
}

#ifdef MULTICAST
/*
 * Initialize the multicast address filter.
 */
static void
elsetmcaf(unit)
	int unit;
{
	register struct el_softc *sc = elcd.cd_devs[unit];
	register volatile struct el_memory *em;
	register u_char *ep;
	int i, cnt;
	u_char eaddr[6];

	em = (struct el_memory *)sc->el_vaddr;

	em->Mcsetup.cmd = IEL_EL|IEL_I|IEL_MCSETUP;
	em->Mcsetup.status = 0;
	ep = (u_char *)em->Mcsetup.mcids;
	
	if (sc->el_if.if_flags & IFF_ALLMULTI) {
		/*
		 * Fill the multicast address vector with addresses which
		 * turn on all 64 hash filter bits.  It just so happens
		 * that starting with the address    01:00:00:00:00:00 and
		 * adding 2 to the first byte, up to 7f:00:00:00:00:00, hits
		 * all the hash bits.
		 */
allmulti:
		bzero(eaddr, sizeof(eaddr));
		for (i = 0; i < NMCADDR; i++) {
			eaddr[0] = (i << 1) + 1;
			slowcopy((caddr_t)eaddr, (caddr_t)ep, 6);
			ep += 6;
		}
	} else {
		struct ether_multi *enm;
		struct ether_multistep step;

		ETHER_FIRST_MULTI(step, &sc->el_ac, enm);
		for (i = 0; i < NMCADDR && enm != NULL; ++i) {
			if (bcmp(&enm->enm_addrlo, &enm->enm_addrhi, 6) != 0)
				/*
				 * We must listen to a range of multicast
				 * addresses.  For now, just accept all
				 * multicasts, rather than trying to set
				 * only those filter bits needed to match
				 * the range.  (At this time, the only use
				 * of address ranges is for IP multicast
				 * routing, for which the range is big
				 * enough to require all bits set.)
				 */
 				goto allmulti;

 			slowcopy((caddr_t)&enm->enm_addrlo, (caddr_t)ep, 6);
 			ETHER_NEXT_MULTI(step, enm);
			ep += 6;
 		}
 		if (enm != NULL) {
			/*
			 * Couldn't fit all our multicast addresses in the
			 * setup vector; turn on reception of all multicasts.
			 */
			goto allmulti;
		}
	}
	/* Hardware expects little-endian. */
	em->Mcsetup.mccnt = 6 * i;

	/*
	 * Fire off the synchronous command.
	 * XXX Seems like this machinery belongs in
	 * a central routine.
	 */
	em->Scb.cmdlist = off(Mcsetup);
	em->Scb.cmd = IEL_CUC_START;
	ATTN(sc);
	WAIT_I(sc, cnt);
	if (cnt == 0)
		printf("el%d: timeout on IA SETUP\n", unit);
	CLR_I(sc);
	if (em->Iasetup.status != (IEL_OK|IEL_C))
		printf("el%d: cannot setup multicast address filter\n", unit);

	/*
	 * Acknowledge condition bits
	 */
	em->Scb.cmd = em->Scb.status & IEL_ACK;
	ATTN(sc);
	for (cnt = ELTIMO; cnt && em->Scb.status; cnt--)
		;
	if (cnt == 0)
		printf("el%d: timeout on status acknowledgement\n", unit);
#ifdef notdef
	/* XXX port this code to verify address filter setup! */
 	/*
 	 * Dump the chip's internal registers and print the multicast
 	 * hash table (for dubugging).
 	 */
	{
 		struct iedump {
			struct iecb ied_cb;
			ieoff_t     ied_off;
			u_char      ied_dump[170];
		};
		register struct iedump *ied;
		
		ied = synccb_res(es, struct iedump *);
		bzero((caddr_t)ied, sizeof (struct iedump));
		ied->ied_cb.iec_cmd = IE_DUMP;
		ied->ied_off = to_ieoff(es, (caddr_t)ied->ied_dump);
		iesynccmd(es, &ied->ied_cb);
		
		printf("multicast hash table: %x %x %x %x %x %x %x %x\n",
		       ied->ied_dump[43],
		       ied->ied_dump[42],
		       ied->ied_dump[41],
		       ied->ied_dump[40],
		       ied->ied_dump[39],
		       ied->ied_dump[38],
		       ied->ied_dump[37],
		       ied->ied_dump[36]);
		
		synccb_rel(es);
	}
#endif
}
#endif

/*
 * Allow reception of packets on the card
 * (should not be called BEFORE the address is set!)
 */
static void
elrun(sc)
	register struct el_softc *sc;
{
	register volatile struct el_memory *em;
	struct iel_rfd *rx;
	struct iel_rbd *rbd;
	u_short p;
	int s;

	/*
	 * Set up initial RX pointers
	 */
	em = (struct el_memory *) sc->el_vaddr;
	dprintf(("elrun rhdr=%d rbuf=%d\n", sc->el_rhdr, sc->el_rbuf));
	s = splimp();
	rx = (struct iel_rfd *) (sc->el_rhdr + sc->el_vaddr);
	rx->cmd = IEL_EL;
	p = rx->link;
	rx = (struct iel_rfd *) (p + sc->el_vaddr);
	rx->status = 0;
	rx->cmd = 0;

	rbd = (struct iel_rbd *) (sc->el_rbuf + sc->el_vaddr);
	rbd->elsize = IEL_EL | RXBSIZ;
	rx->rbd = rbd->nextbd;
	em->Scb.rfa = p;
	
	/*
	 * Turn RU into active state and enable interrupts
	 */
	em->Scb.cmd = IEL_RUC_START;
	ATTN(sc);
	CLR_I(sc);
	outb(el_ctrl(sc->el_io), EL_NORST|EL_IEN|EL_LAE);
	splx(s);
}
 
/*
 * Timeout routine
 */
elwatchdog(unit)
	int unit;
{
	struct el_softc *sc = (struct el_softc *) elcd.cd_devs[unit];

	dprintf(("WATCHDOG!\n"));
	log(LOG_WARNING,"el%d: soft reset\n", unit);
	if (sc->el_if.if_flags & IFF_OACTIVE)
		elinit(unit);
}

/*
 * Initialization of interface
 */
elinit(unit)
	int unit;
{
	register struct el_softc *sc = elcd.cd_devs[unit];
	register struct ifnet *ifp = &sc->el_if;
	int s;
 
	/* address not known */
	if (ifp->if_addrlist == (struct ifaddr *)0)
		return;

	s = splimp();
	ifp->if_flags &= ~(IFF_RUNNING|IFF_OACTIVE);
	outb(el_ctrl(sc->el_io), 0);	/* Reset i82586 */
	DELAY(10);
	if ((sc->el_flags ^ ifp->if_flags) & IFF_BNC) {
		outb(EL_ID, 0);
		elidgen();
		elidgen();
		outb(EL_ID, (sc->el_io - 0x200) >> 4);
		if (!elprogram(unit)) {
			splx(s);
			return;
		}
	} else {
		if (!eli82586init(unit)) {
			splx(s);
			return;
		}
	}
	elsetaddr(unit);
#ifdef MULTICAST
	if ((ifp->if_flags & IFF_PROMISC) == 0)
		elsetmcaf(unit);
#endif
	elrun(sc);
	ifp->if_flags |= IFF_RUNNING;
	sc->el_flags = ifp->if_flags;
	splx(s);
	return;
}

/*
 * Start output on interface.
 */
elstart(ifp)
	struct ifnet *ifp;
{
	register struct el_softc *sc = elcd.cd_devs[ifp->if_unit];
	register struct mbuf *m;
	struct mbuf *m0;
	u_short buffer, xhdr;
	struct iel_transmit *hdr;
	struct iel_tbd *tbd;
	volatile struct el_memory *em;
	caddr_t bp;
	static char zeroes[ETHER_MIN_LEN];
	int len, s, tozero, cnt;
 
	/*
	 * Check if we have an available outgoing frame header and
	 * there is a sufficient space in TX output area.
	 */
	s = splimp();
	m = sc->el_if.if_snd.ifq_head;
	if (m == 0) {
		splx(s);
		return;
	}

	if (sc->el_tfreehdr == 0) {
		splx(s);
		return;
	}
	
	len = 0;
	while (m != 0) {
		len += m->m_len;
		m = m->m_next;
	}
	tozero = 0;
	if (len < ETHER_MIN_LEN) {
		tozero = ETHER_MIN_LEN - len;
		len = ETHER_MIN_LEN;
	}

	if (sc->el_tavail > sc->el_tbusy) {
		if (sc->el_tend - sc->el_tavail >= len)
			goto gotbuf;
		sc->el_tavail = sc->el_tarea;
	}
	if (sc->el_tbusy - sc->el_tavail < len) {
		splx(s);
		return;
	}

	/*
	 * Allocate resources
	 */
gotbuf:
	xhdr = sc->el_tfreehdr;
	hdr = (struct iel_transmit *)(xhdr + sc->el_vaddr);
	sc->el_tfreehdr = hdr->link;
	buffer = sc->el_tavail;
	sc->el_tavail += len | 1;	/* Keep the TX buffer word-aligned */
	IF_DEQUEUE(&sc->el_if.if_snd, m);
	splx(s);

	/*
	 * Fill in the buffer (it can be done at lower priority)
	 */
	bp = sc->el_vaddr + buffer;
	for (m0 = m; m != 0; m = m->m_next) {
		if (m->m_len == 0)
			continue;
		slowcopy(mtod(m, caddr_t), bp, m->m_len);
		bp += m->m_len;
	}
	m_freem(m0);
	if (tozero > 0)
		slowcopy(zeroes, bp, tozero);

#if NBPFILTER > 0
	/*
	 * If bpf is listening on this interface, let it
	 * see the packet before el commit it to the wire.
	 */
	if (sc->el_bpf)
		bpf_tap(sc->el_bpf, sc->el_vaddr + buffer, len - tozero);
#endif

	tbd = (struct iel_tbd *)(hdr->tbd + sc->el_vaddr);
	tbd->buf = sc->el_daddr + buffer;
	tbd->actcnt = len | IEL_EOF;

	hdr->cmd = IEL_TRANSMIT | IEL_EL | IEL_I;
	hdr->status = 0;
	hdr->txlen = len;

	dprintf(("TX: %s", ether_sprintf(sc->el_vaddr + buffer + 6)));
	dprintf(("--> %s", ether_sprintf(sc->el_vaddr + buffer)));
	dprintf((" (%x) len=%d\n", ntohs(*(u_short *)(sc->el_vaddr + buffer + 12)), len));

	/*
	 * Critical stuff -- add the buffer to TX chain and restart the CU
	 */
	em = (struct el_memory *)sc->el_vaddr;
	s = splimp();
	if (sc->el_tqtail != 0) {
		dprintf(("tqtail <- %d\n", xhdr));
		hdr = (struct iel_transmit *)(sc->el_tqtail + sc->el_vaddr);
		hdr->link = xhdr;
		hdr->cmd = IEL_TRANSMIT | IEL_I;	/* remove IEL_EL */
		sc->el_tqtail = xhdr;
		if ((em->Scb.status & IEL_CUS) == IEL_CU_IDLE) {
			WAIT_SCBCLR(sc, em, cnt, ifp->if_unit);
			em->Scb.cmdlist = sc->el_tqhead;
			em->Scb.cmd = IEL_CUC_START;
			ATTN(sc);
		}
			
	} else {
		dprintf(("tqhead <- %d\n", xhdr));
		sc->el_tqhead = sc->el_tqtail = xhdr;
		WAIT_SCBCLR(sc, em, cnt, ifp->if_unit);
		em->Scb.cmdlist = xhdr;
		em->Scb.cmd = IEL_CUC_START;
		ATTN(sc);
	}
	sc->el_if.if_flags |= IFF_OACTIVE;
	sc->el_if.if_timer = 5;
	splx(s);
}
		
/*
 * Ethernet interface interrupt processor
 */
elintr(arg)
	void *arg;
{
	register struct el_softc *sc = (struct el_softc *) arg;
	register volatile struct el_memory *em;
	struct iel_transmit *tx;
	struct iel_rfd *rx, *rx1;
	struct iel_rbd *rbd, *rbd1;
	u_short stat;
	u_short off;
	u_short l, totlen;
	caddr_t bp;
	int cnt;

	/*
	 * Acknowledge all status bits
	 */
	em = (struct el_memory *) sc->el_vaddr;
lookagain:
	stat = em->Scb.status;
	CLR_I(sc);
	em->Scb.cmd = stat & IEL_ACK;
	ATTN(sc);
	dprintf(("I<%s%s%s%sCU:%d,RU:%d>\n", (stat & IEL_CX)? "CX," : "",
		(stat & IEL_FR)? "FR," : "", (stat & IEL_CNA)? "CNA," : "",
		(stat & IEL_RNR)? "RNR," : "", (stat>>8) & 07, (stat>>4) & 07));
	
	/*
	 * Receiver overrun?
	 */
	if (stat & IEL_RNR) {
		log(LOG_ERR, "el%d: receiver buffer overflow\n",
		    sc->el_dev.dv_unit);
		elinit(sc->el_dev.dv_unit);
		return (1);
	}

	/*
	 * Process received frames
	 */
	if (stat & IEL_FR) {
		off = em->Scb.rfa;
		for (;;) {
			rx = (struct iel_rfd *) (off + sc->el_vaddr);
			if ((rx->status & IEL_C) == 0) {
				em->Scb.rfa = off;
				break;
			}

			if (rx->rbd == 0xffff) {
				dprintf(("NORBD\n"));
				sc->el_if.if_ierrors++;
				goto rls_frame;
			}
			rbd = (struct iel_rbd *)(rx->rbd + sc->el_vaddr);
			if (rx->status & IEL_OK) {
				/*
				 * Copy data to the buffer
				 */
				bp = sc->el_ipacket;
				totlen = 0;
				for (;;) {
					if ((rbd->actcnt & IEL_F) == 0)
						break;
					l = rbd->actcnt & IEL_CNT;
					if (l == 0)
						break;
					if (l + totlen > ETHERMTU+ETHER_HDR_SIZE) {
						printf("el%d: too large packet from %s\n", sc->el_dev.dv_unit,
							ether_sprintf(&sc->el_ipacket[ETHER_ADDR_LEN]));
						break;
					}
					bcopy(sc->el_vaddr - sc->el_daddr + rbd->buf, bp, l);
					totlen += l;
					bp += l;
					if (rbd->actcnt & IEL_EOF)
						break;
					rbd = (struct iel_rbd *)(rbd->nextbd + sc->el_vaddr);
				}
				elread(sc, totlen);
				dprintf(("RX: %s", ether_sprintf(sc->el_ipacket+6)));
				dprintf(("--> %s", ether_sprintf(sc->el_ipacket)));
				dprintf((" (%x) len=%d\n", ntohs(*(u_short *)(sc->el_ipacket+12)), totlen));
				sc->el_if.if_ipackets++;
			} else {
				dprintf(("RXERR <%x>\n", rx->status));
				sc->el_if.if_ierrors++;
			}

			/* Release consumed buffers */
			rbd = (struct iel_rbd *)(rx->rbd + sc->el_vaddr);
			rbd1 = (struct iel_rbd *)(sc->el_rbuf + sc->el_vaddr);
			if ((rbd1->elsize & IEL_EL) == 0) {
				printf("el%d: RX buffer ring botched (no EL)\n", sc->el_dev.dv_unit);
				ELRESQUE(sc->el_dev.dv_unit);
				return (1);
			}
			if (rbd1->nextbd != rx->rbd) {
				printf("el%d: RX buffer ring botched (links)\n", sc->el_dev.dv_unit);
				ELRESQUE(sc->el_dev.dv_unit);
				return (1);
			}
			l = ((struct iel_rfd *)(rx->link + sc->el_vaddr))->rbd;
			if (l == 0 || l == 0xffff) {
				printf("el%d: lost RX free buffer link\n", sc->el_dev.dv_unit);
				ELRESQUE(sc->el_dev.dv_unit);
				return (1);
			}
			if (rx->rbd != l) {
				cnt = 4000;
				while (rbd->nextbd != l) {
					rbd->actcnt = 0;
					rbd->elsize = RXBSIZ;
					if (--cnt == 0) {
						printf("el%d: RX buffer list corrupted\n", sc->el_dev.dv_unit); 
						ELRESQUE(sc->el_dev.dv_unit);
						return (1);
					}
					rbd = (struct iel_rbd *)(rbd->nextbd + sc->el_vaddr);
				}
				rbd->actcnt = 0;
				rbd->elsize = RXBSIZ|IEL_EL;
				rbd1->elsize = RXBSIZ;
				sc->el_rbuf = (caddr_t)rbd - sc->el_vaddr;
				dprintf(("rbuf <- %d; ", sc->el_rbuf));
			}

			/* Release the frame descriptor */
		rls_frame:
			rx->status = 0;
			rx->cmd = IEL_EL;
			rx->rbd = 0xffff;
			rx1 = (struct iel_rfd *)(sc->el_rhdr + sc->el_vaddr);
			if (rx1->cmd != IEL_EL) {
				printf("el%d: RX frame ring botched (no EL)\n", sc->el_dev.dv_unit);
				ELRESQUE(sc->el_dev.dv_unit);
				return (1);
			}
			if (rx1->link != off) {
				printf("el%d: RX frame ring botched (links)\n", sc->el_dev.dv_unit);
				ELRESQUE(sc->el_dev.dv_unit);
				return (1);
			}
			rx1->cmd = 0;
			sc->el_rhdr = off;
			dprintf(("rhdr <- %d\n", sc->el_rhdr));
			off = rx->link;
		}

		/*
		 * Restart receiver if necessary
		 */
		if ((em->Scb.status & IEL_RUS) != IEL_RU_RDY) {
			dprintf(("RU RST\n"));
			WAIT_SCBCLR(sc, em, cnt, sc->el_dev.dv_unit);
			em->Scb.cmd = IEL_RUC_START;
			ATTN(sc);
		}
	}

	/*
	 * Release transmitted buffers
	 */
	if (stat & (IEL_CX|IEL_CNA)) {
		dprintf(("TX!"));
		while (sc->el_tqhead != 0) {
			tx = (struct iel_transmit *)(sc->el_tqhead + sc->el_vaddr);
			dprintf(("<%d:%x>\n", sc->el_tqhead, tx->status));
			if ((tx->status & IEL_C) == 0)
				break;
		
			/* Update statistics */
			off = sc->el_tqhead;
			if (tx->status & IEL_OK) {
				dprintf(("TXOK\n"));
				sc->el_if.if_opackets++;
			} else {
				dprintf(("TXERR <%x>\n", tx->status));
				if (tx->status & IEL_S5)
					sc->el_if.if_collisions += 16;
				sc->el_if.if_oerrors++;
			}
			sc->el_if.if_collisions += tx->status & IEL_NCOL;

			/* Release frame header */
			off = tx->link;
			tx->link = sc->el_tfreehdr;
			sc->el_tfreehdr = sc->el_tqhead;
			if (sc->el_tqhead == sc->el_tqtail) {
				sc->el_tqhead = sc->el_tqtail = 0;
				if ((tx->cmd & IEL_EL) == 0) {
					printf("el%d: TX list botched (no EL)\n", sc->el_dev.dv_unit);
					ELRESQUE(sc->el_dev.dv_unit);
					return (1);
				}
				em->Scb.cmdlist = 0xffff;
				break;
			}
			em->Scb.cmdlist = sc->el_tqhead = off;
		}
		
		/* Release TX buffer space */
		if (sc->el_tqhead == 0) {
			sc->el_tavail = sc->el_tarea;
			sc->el_tbusy = sc->el_tend;
		} else
			sc->el_tbusy = ((struct iel_tbd *)(tx->tbd+sc->el_vaddr))->buf
					- sc->el_daddr;

		/*
		 * Restart transmit (if necessary)
		 */
		sc->el_if.if_timer = 0;
		if ((stat & IEL_CUS) == IEL_CU_IDLE) {
			if (sc->el_tqhead != 0) {
				WAIT_SCBCLR(sc, em, cnt, sc->el_dev.dv_unit);
				em->Scb.cmdlist = sc->el_tqhead;
				em->Scb.cmd = IEL_CUC_START;
				ATTN(sc);
				sc->el_if.if_timer = 5;
			} else
				sc->el_if.if_flags &= ~IFF_OACTIVE;
		}
		(void) elstart(&sc->el_if);
	}

	/*
	 * Recycle :-)
	 */
	if ((inb(el_ctrl(sc->el_io)) & EL_INT) ||
	    (em->Scb.status & (IEL_CX|IEL_FR|IEL_CNA|IEL_RNR)) )
		goto lookagain;
	return (1);
}
 
/*
 * Process an ioctl request.
 */
elioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	register struct ifaddr *ifa = (struct ifaddr *)data;
	struct el_softc *sc = elcd.cd_devs[ifp->if_unit];
	struct ifreq *ifr = (struct ifreq *)data;
	int s = splimp(), error = 0;


	switch (cmd) {

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			elinit(ifp->if_unit);	/* before arpwhohas */
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
				ina->x_host = *(union ns_host *)(sc->el_addr);
			elinit(ifp->if_unit);
			break;
		    }
#endif
		default:
			elinit(ifp->if_unit);
			break;
		}
		break;

	case SIOCSIFFLAGS:
		if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) == IFF_RUNNING) {
			ifp->if_flags &= ~IFF_RUNNING;
			sc->el_flags &= ~IFF_UP;
			outb(el_ctrl(sc->el_io), 0);	/* Reset i82586 and disable interrupts */

		} else if ((ifp->if_flags ^ sc->el_flags) & (IFF_UP|IFF_BNC|IFF_PROMISC))
			elinit(ifp->if_unit);
		break;

#ifdef MULTICAST
	/*
	 * Update our multicast list.
	 */
	case SIOCADDMULTI:
		error = ether_addmulti((struct ifreq *)data, &sc->el_ac);
		goto reset;

	case SIOCDELMULTI:
		error = ether_delmulti((struct ifreq *)data, &sc->el_ac);
	reset:
		if (error == ENETRESET) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
			elinit(ifp->if_unit);
			error = 0;
		}
		break;
#endif
#ifdef notdef
	case SIOCGHWADDR:
		bcopy((caddr_t)sc->sc_addr, (caddr_t) &ifr->ifr_data,
			sizeof(sc->sc_addr));
		break;
#endif

	default:
		error = EINVAL;
	}
	splx(s);
	return (error);
}
 
/*
 * Pass a packet to the higher levels.
 * We deal with the trailer protocol here.
 */
static void
elread(sc, len)
	register struct el_softc *sc;
	int len;
{
	register struct ether_header *eh;
    	struct mbuf *m;
	int off, resid;

	len -= sizeof(struct ether_header);

	/*
	 * Deal with trailer protocol: if type is trailer type
	 * get true type from first 16-bit word past data.
	 * Remember that type was trailer by setting off.
	 */
	eh = (struct ether_header *) sc->el_ipacket;
	eh->ether_type = ntohs((u_short)eh->ether_type);
	if (eh->ether_type >= ETHERTYPE_TRAIL &&
	    eh->ether_type < ETHERTYPE_TRAIL+ETHERTYPE_NTRAILER) {
		off = (eh->ether_type - ETHERTYPE_TRAIL) * 512;
		if (off >= ETHERMTU)
			return;		/* sanity */
		eh->ether_type = ntohs(*(u_short *)(sc->el_ipacket +
					sizeof(struct ether_header) + off));
		resid = ntohs(*(u_short *)(sc->el_ipacket +
					sizeof(struct ether_header) + off + 2));
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
	if (sc->el_bpf) {
		eh->ether_type = htons((u_short)eh->ether_type);
		bpf_tap(sc->el_bpf, sc->el_ipacket, len + sizeof(struct ether_header));
		eh->ether_type = ntohs((u_short)eh->ether_type);

		/*
		 * Note that the interface cannot be in promiscuous mode if
		 * there are no bpf listeners.  And if we are in promiscuous
		 * mode, we have to check if this packet is really ours.
		 *
		 * This test does not support multicasts.
		 */
		if ((sc->el_if.if_flags & IFF_PROMISC) &&
		    bcmp(eh->ether_dhost, sc->el_addr,
		    sizeof(eh->ether_dhost)) != 0 &&
		    bcmp(eh->ether_dhost, etherbroadcastaddr, 
		    sizeof(eh->ether_dhost)) != 0)
			return;
	}
#endif

	/*
	 * Pull packet off interface.  Off is nonzero if packet
	 * has trailing header; neget will then force this header
	 * information to be at the front, but el still have to drop
	 * the type and length which are at the front of any trailer data.
	 */
	m = elget(len, off, sc);
	if (m == 0)
		return;

	ether_input(&sc->el_if, eh, m);
}

/*
 * Pull read data off a interface.
 * Len is length of data, with local net header stripped.
 * Off is non-zero if a trailer protocol was used, and
 * gives the offset of the trailer information.
 * We copy the trailer information and then all the normal
 * data into mbufs.  When full cluster sized units are present
 * el copy into clusters.
 */
static struct mbuf *
elget(totlen, off, sc)
	int totlen, off;
	struct el_softc *sc;
{
	struct mbuf *top, **mp, *m, *p;
	int len;
	register caddr_t cp;
	char *epkt;

	cp = sc->el_ipacket + sizeof(struct ether_header);
	epkt = cp + totlen;

	if (off) {
		cp += off + 2 * sizeof(u_short);
		totlen -= 2 * sizeof(u_short);
	}

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == 0)
		return (0);
	m->m_pkthdr.rcvif = &sc->el_if;
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

		totlen -= len;
		bcopy(cp, mtod(m, caddr_t), (unsigned)len);
		cp += len;
		*mp = m;
		mp = &m->m_next;
	}
	return (top);
}
