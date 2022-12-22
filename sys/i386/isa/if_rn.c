/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: if_rn.c,v 1.10 1994/01/30 07:46:56 karels Exp $
 */

/*
 * SDL Communications RISCom/N1 HDLC serial port driver
 *
 * Currently the external clock source and 16 bit HDLC checksum
 * are hardwired; i.e. there is no need to configure the driver
 * to support different line speeds.
 */

/*
 * History
 *	 6/15/92 [avg]	initial revision
 */

/*
 * TODO:
 * - add support for internal clock source and
 *   alternative HDLC checksum
 * - do something about carrier detect etc
 */

/*#define RNDEBUG 2 */

#include "bpfilter.h"

#include "param.h"
#include "systm.h"
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
#include "if_rnreg.h"
#include "ic/uPD72103.h"

#if NBPFILTER > 0
#include "net/bpf.h"
#include "net/bpfdesc.h"
#endif

#define RN_DEFAULT_RATE	56000	/* 56Kbps */

#undef dprintf
#ifdef RNDEBUG
int rndebug = RNDEBUG;
#define dprintf(x)	if (rndebug > 1) printf x
#define dprintf2(x)	if (rndebug) printf x
#else	/* !RNDEBUG */
#define dprintf(x)
#define dprintf2(x)
#endif

struct rn_softc {
	struct	device sc_dev;		/* base device */
	struct	isadev sc_id;		/* ISA device */
	struct 	intrhand sc_ih;		/* interrupt vectoring */
	struct  p2pcom sc_p2pcom;       /* point-to-point common stuff */
#define	sc_if	sc_p2pcom.p2p_if	/* network-visible interface */	

	u_short	sc_addr;		/* i/o port base address */
	u_char	sc_nlcw;		/* number of command/status words */
	u_char	sc_nrbuf;		/* number of rx buffers */
	caddr_t	sc_pmem;		/* the physical memory address */
	int	sc_memsize;		/* the memory window size */
	caddr_t	sc_vaddr;		/* virtual memory address */
	struct	upd_cmd *sc_cmdtab;	/* commands table */
	struct	upd_stat *sc_stattab;	/* transmit buffers table */
	struct	upd_rbw *sc_rbuftab;	/* receive buffers table */
	caddr_t	sc_rbufs;		/* receive buffers */
	caddr_t	sc_tstart;		/* start of transmit area */
	caddr_t	sc_tend;		/* end of transmit area */
	caddr_t	sc_tavail;		/* available buffer space */
	caddr_t	sc_tbusy;		/* busy buffer space */
	int	sc_rate;		/* current baudrate */
	struct	upd_dtsd *sc_tqueue;	/* queue of outgoing packets */
	struct	upd_dtsd *sc_tqtail;	/* tail of the queue */
	u_char	sc_statno;		/* status entry no */
	u_char	sc_cmdp;		/* current available command slot */
	struct	atshutdown sc_ats;	/* shutdown routine */
#if NBPFILTER > 0
	caddr_t	sc_bpf;			/* BPF filter */
#endif
};

int	rnprobe __P((struct device *, struct cfdata *, void *));
void	rnforceintr __P((void *));
void	rnattach __P((struct device *, struct device *, void *));
int	rnioctl __P((struct ifnet *, int, caddr_t));
int	rnintr __P((struct rn_softc *));
int	rnstart __P((struct ifnet *));
int     rnmdmctl __P((struct p2pcom *, int));
int     rnwatchdog __P((int));

void	rncopy __P((void *, void *, u_int));
int	rninit __P((struct rn_softc *));
void	rnsettimer __P((struct rn_softc *));
int	rnsendcmd __P((struct rn_softc *, caddr_t, int));
void	rnsetmodes __P((struct rn_softc *));
void	rncloseline __P((struct rn_softc *));
void	rnread __P((struct rn_softc *, caddr_t, int));
void	rnshutdown __P((void *));

extern int hz;

#define Kb	*1024

struct cfdriver rncd =
	{ NULL, "rn", rnprobe, rnattach, sizeof(struct rn_softc) };
 
/*
 * Probe routine
 */
rnprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	int i, v1, v2;
	struct rn_softc *sc;

	if (!RN_VALIDBASE(ia->ia_iobase)) {
		printf("rn%d: illegal base address %x\n", cf->cf_unit,
		    ia->ia_iobase);
		return (0);
	}
	if (ia->ia_msize != 16 Kb && ia->ia_msize != 32 Kb &&
	    ia->ia_msize != 64 Kb) {
		printf("rn%d: illegal memory windoiw size %d\n", cf->cf_unit,
		    ia->ia_msize);
		return (0);
	}
	if ((int) ia->ia_maddr & 0x3fff) {
		printf("rn%d: non-aligned memory window (at %x)\n",
		    cf->cf_unit, ia->ia_maddr);
		return (0);
	}
	i = ia->ia_iobase + RN_VPM;
	outb(i, 0xa);
	v1 = inb(i);
	if ((v1 & RN_XADR) != 0xa) {
		dprintf(("FAIL1\n"));
		return (0);
	}
	outb(i, 0xf5);
	v2 = inb(i);
	if ((v2 & RN_XADR) != 0x5) {
		dprintf(("FAIL2\n"));
		return (0);
	}
	if ((v2 ^ v1) & RN_ID) {
		dprintf(("FAIL3\n"));
		return (0);
	}
	if (ia->ia_irq == IRQUNK) {
		sc = (struct rn_softc *) malloc(sizeof(struct rn_softc),
						M_DEVBUF, M_WAITOK);
		if (sc == 0) {
			printf("rn%d: can't alloc memory\n", cf->cf_unit);
			return (0);
		}
		sc->sc_if.if_unit = cf->cf_unit;
		sc->sc_addr = ia->ia_iobase;
		sc->sc_pmem = ia->ia_maddr;
		sc->sc_memsize = ia->ia_msize;
		sc->sc_vaddr = ISA_HOLE_VADDR(ia->ia_maddr);
		sc->sc_rate = RN_DEFAULT_RATE;

		ia->ia_irq = isa_discoverintr(rnforceintr, sc);

		outb(ia->ia_iobase + RN_PCR, 0);
		outb(ia->ia_iobase + RN_CLI, 0);
		outb(ia->ia_iobase + RN_BAR, 0);
		free(sc, M_DEVBUF);

		if (ffs(ia->ia_irq) - 1 == 0)
			return (0);
	} else {
		i = ffs(ia->ia_irq) - 1;
		if (!RN_VALIDIRQ(i)) {
			printf("rn%d: illegal IRQ %d\n", cf->cf_unit, i);
			return (0);
		}
	}
	ia->ia_iosize = RN_NPORT;
	return (1);
}

static void
rnforceintr(aux)
	void *aux;
{
	struct rn_softc *sc = (struct rn_softc *) aux;

	if (rninit(sc) == 0) {
		printf("rn%d: init failed\n", sc->sc_if.if_unit);
		return;
	}
	rnsetmodes(sc);
	rncloseline(sc);
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
void
rnattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	register struct rn_softc *sc = (struct rn_softc *) self;
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	register struct ifnet *ifp = &sc->sc_if;
 
	sc->sc_addr = ia->ia_iobase;
	sc->sc_pmem = ia->ia_maddr;
	sc->sc_memsize = ia->ia_msize;
	sc->sc_vaddr = ISA_HOLE_VADDR(ia->ia_maddr);
	sc->sc_rate = RN_DEFAULT_RATE;

	if (rninit(sc) == 0) {
		printf(" init failed\n");
		return;
	}
	printf("\n");
	/*
	 * Initialize ifnet structure
	 */
	ifp->if_unit = sc->sc_dev.dv_unit;
	ifp->if_name = "rn";
	ifp->if_mtu = HDLCMTU;
	ifp->if_flags = IFF_POINTOPOINT | IFF_MULTICAST;
	ifp->if_type = IFT_PTPSERIAL;
	ifp->if_ioctl = p2p_ioctl;
	ifp->if_output = 0;             /* crash and burn on access */
	ifp->if_start = rnstart;
	ifp->if_watchdog = rnwatchdog;
	sc->sc_p2pcom.p2p_mdmctl = rnmdmctl;
	if_attach(ifp);
#if NBPFILTER > 0
	bpfattach(&sc->sc_bpf, ifp, DLT_PPP, 0);	/* XXX */
#endif

        isa_establish(&sc->sc_id, &sc->sc_dev);
        sc->sc_ih.ih_fun = rnintr;
        sc->sc_ih.ih_arg = (void *)sc;
        intr_establish(ia->ia_irq, &sc->sc_ih, DV_NET);

	sc->sc_ats.func = rnshutdown;
	sc->sc_ats.arg = (void *)sc;
	atshutdown(&sc->sc_ats, ATSH_ADD);
}

/*
 * Shutdown the interface (do things soft reset won't)
 */
void
rnshutdown(arg)
	void *arg;
{
	struct rn_softc *sc = (struct rn_softc *) arg;
	register base = sc->sc_addr;

	outb(base + RN_PCR, 0);	    /* reset everything and close the window */
	outb(base + RN_CLI, 0);	    /* clear pending interrupts (if any) */
	outb(base + RN_BAR, 0);
}

/*
 * Slow copy from/to rn on-board memory
 */
void
rncopy(src, dst, n)
	void *src;
	void *dst;
	register u_int n;
{
	register u_char *from = src;
	register u_char *to = dst;
	int odd;

	if ((int)from & 1 || (int)to & 1) {
		while (n--)
			*to++ = *from++;
		return;
	}
	odd = (n & 1);
	n >>= 1;
	while (n--) {
		*(u_short *) to = *(u_short *) from;
		to += sizeof(u_short);
		from += sizeof(u_short);
	}
	if (odd)
		*to++ = *from++;
}

/*
 * The watchdog routine
 */
int
rnwatchdog(unit)
	int unit;
{
	register struct rn_softc *sc = rncd.cd_devs[unit];
	int s;

	printf("rn%d: lost intr\n", unit);
	s = splimp();
	sc->sc_tqueue = 0;
	sc->sc_tavail = sc->sc_tstart;
	sc->sc_tbusy = sc->sc_tend;
	sc->sc_if.if_flags &= ~IFF_OACTIVE;
	(void) rnstart(&sc->sc_if);
	splx(s);
}

/*
 * Initialize the board
 */
rninit(sc)
	struct rn_softc *sc;
{
	register base = sc->sc_addr;
	int i, rboff; 
	register char *p;
	struct upd_mset *ms;
	u_char ctl;
	int s;

	/*
	 * Set up memory window
	 */
	s = splimp();
	dprintf(("rninit\n"));
	outb(base + RN_PSR, 0x0);
	outb(base + RN_VPM, 0x0);
	outb(base + RN_BAR, (int) sc->sc_pmem >> 12);
	switch (sc->sc_memsize) {
	case 16 Kb:
		ctl = RN_WS16K;
		break;
	case 32 Kb:
		ctl = RN_WS32K;
		break;
	case 64 Kb:
		ctl = RN_WS64K;
		break;
	}
	ctl |= RN_ENWIN | RN_CLKEXT | RN_EETC | RN_ENVPM;
	outb(base + RN_PCR, ctl);

	/*
	 * Test and zero memory
	 */
	for (i = 0; i < sc->sc_memsize; i += 256) {
		sc->sc_vaddr[i] = 0x55;
		if (sc->sc_vaddr[i] != 0x55) {
			printf("rn%d: memory test failed\n", sc->sc_if.if_unit);
			splx(s);
			return (0);
		}
		sc->sc_vaddr[i] = 0x2a;
		if (sc->sc_vaddr[i] != 0x2a) {
			printf("rn%d: memory test failed\n", sc->sc_if.if_unit);
			splx(s);
			return (0);
		}
	}
	bzero(sc->sc_vaddr, sc->sc_memsize);

	/*
	 * Calculate the memory layout parameters
	 */
	rboff = (sc->sc_memsize * 2)/3;		/* 2/3 memory is for rcv bufs */
	rboff /= HDLCMAX;
	dprintf(("rxb(max)=%d ", rboff));
	for (i = 0; (1 << i) <= rboff; i++)
		;
	if (i > 6)
		i = 6;		/* max 64 rbufs */
	sc->sc_nrbuf = 1 << (i - 1);
	sc->sc_nlcw = sc->sc_nrbuf * 2;
	dprintf(("NRBUF=%d NLCW=%d\n", sc->sc_nrbuf, sc->sc_nlcw));

	/*
	 * Initialize LCW/LSW/RBW tables
	 */
	p = sc->sc_vaddr;
	sc->sc_cmdtab = (struct upd_cmd *)p;
	for (i = 0; i < sc->sc_nlcw; i++) {
		((struct upd_mset *)p)->cmd = 0xff;
		((struct upd_mset *)p)->zero = 0xff;
		p += uPD_LCW_SIZE;
	}
	sc->sc_stattab = (struct upd_stat *)p;
	for (i = 0; i < sc->sc_nlcw; i++) {
		((struct upd_stat *)p)->stsn = 0xff;
		p += uPD_LSW_SIZE;
	}
	sc->sc_rbuftab = (struct upd_rbw *)p;
	rboff = sc->sc_nrbuf * uPD_LRBW_SIZE + 
		sc->sc_nlcw * (uPD_LCW_SIZE + uPD_LSW_SIZE);
	sc->sc_rbufs = sc->sc_vaddr + rboff;
	for (i = 0; i < sc->sc_nrbuf; i++) {
		((struct upd_rbw *)p)->brdy = 0;
		((struct upd_rbw *)p)->bufa0 = rboff;
		((struct upd_rbw *)p)->bufa1 = rboff >> 8;
		((struct upd_rbw *)p)->bufa2 = rboff >> 16;
		p += uPD_LRBW_SIZE;
		rboff += HDLCMAX;
	}
	sc->sc_tstart = sc->sc_tavail = sc->sc_vaddr + rboff;
	sc->sc_tend = sc->sc_tbusy = sc->sc_vaddr + sc->sc_memsize;
	dprintf(("RN MEMORY: LCW %x LSW %x RBW %x RBUF %x TBUF %x END %x\n",
		sc->sc_vaddr, sc->sc_stattab, sc->sc_rbuftab, sc->sc_rbufs,
		sc->sc_tstart, sc->sc_tend));

	/*
	 * Set up the default data speed
	 */
	rnsettimer(sc);

	/*
	 * Issue the MSET command
	 */
	outb(base + RN_PCR, ctl | RN_NCRES);
	ms = (struct upd_mset *)(sc->sc_vaddr);
	ms->cmd = uPD_MSET;
	ms->zero = 0;
	ms->addr0 = 0;
	ms->addr1 = 0;
	ms->addr2 = 0;
	ms->nlcw = sc->sc_nlcw;
	ms->nlsw = sc->sc_nlcw;
	ms->nlrbw = sc->sc_nrbuf;
	outb(base + RN_CTL, uPD_CTL_CRQ);
	i = 100000;
	while ((inb(base + RN_STAT) & uPD_STAT_CRQURDY) && --i)
		;
	if (i == 0) {
		printf("rn%d: no response\n", sc->sc_if.if_unit);
		splx(s);
		return (0);
	}

	sc->sc_statno = 0;
	sc->sc_cmdp = 0;
	sc->sc_tqueue = 0;
	sc->sc_tqtail = 0;
	ms->zero = 0xff;	/* wipe out the MSET command */
	
	/* Clear interrupts */
	outb(base + RN_CLI, 0);
	splx(s);
	return (1);
}

/*
 * Set data rate clock
 */
void
rnsettimer(sc)
	struct rn_softc *sc;
{
	register base = sc->sc_addr;
	int i;

	i = RN_OSC_FREQ / sc->sc_rate;
	outb(base + RN_BRCTL, RN_BR_INIT);
	outb(base + RN_BRDIV, i & 0xff);
	outb(base + RN_BRDIV, i >> 8);
}

/*
 * Start transmission
 */
rnstart(ifp)
	struct ifnet *ifp;
{
	register struct rn_softc *sc = rncd.cd_devs[ifp->if_unit];
	register struct mbuf *m;
	struct mbuf *m0;
	caddr_t buffer, bp;
	struct upd_dtsd Dtsd;
	struct ifqueue *ifq;
	int len;
	int s;

	dprintf(("<S>"));

	/*
	 * Check if we have an available outgoing frame header and
	 * there is a sufficient space in TX output area.
	 */
	ifq = &sc->sc_p2pcom.p2p_isnd;
	s = splimp();
	m = ifq->ifq_head;
	if (m == 0) {
		ifq = &sc->sc_if.if_snd;
		m = ifq->ifq_head;
	}
	if (m == 0) {
		splx(s);
		return (0);
	}

	if ((sc->sc_cmdtab[sc->sc_cmdp].zero & 0xfc) != 0xfc) {
		sc->sc_if.if_flags |= IFF_OACTIVE;
		splx(s);
		return (0);
	}

	len = 0;
	while (m != 0) {
		len += m->m_len;
		m = m->m_next;
	}

	if (sc->sc_tavail > sc->sc_tbusy) {
		if (sc->sc_tend - sc->sc_tavail >= len)
			goto gotbuf;
		sc->sc_tavail = sc->sc_tstart;
	}
	if (sc->sc_tbusy - sc->sc_tavail < len) {
		sc->sc_if.if_flags |= IFF_OACTIVE;
		splx(s);
		return (0);
	}

gotbuf:
	buffer = sc->sc_tavail;
	sc->sc_tavail += (len + 1) & ~1;	/* keep buffers halfword aligend for efficiency */

	IF_DEQUEUE(ifq, m);
	splx(s);

	/*
	 * Fill in the buffer (it can be done at lower priority)
	 */
	bp = buffer;
	for (m0 = m; m != 0; m = m->m_next) {
		if (m->m_len == 0)
			continue;
		rncopy(mtod(m, caddr_t), bp, m->m_len);
		bp += m->m_len;
	}
	m_freem(m0);

#if NBPFILTER > 0
	/*
	 * If bpf is listening on this interface, let it
	 * see the packet before we commit it to the wire.
	 */
	if (sc->sc_bpf)
		bpf_tap(sc->sc_bpf, buffer, len);
#endif

	/*
	 * Issue the command
	 */
	Dtsd.cmd = uPD_DTSD;
	Dtsd.xbc = 0;
	Dtsd.bc0 = len;
	Dtsd.bc1 = len >> 8;
	len = buffer - sc->sc_vaddr;
	Dtsd.bufa0 = len;
	Dtsd.bufa1 = len >> 8;
	Dtsd.bufa2 = len >> 16;

	(void) rnsendcmd(sc, (caddr_t) &Dtsd, sizeof(Dtsd));
	return (0);
}

/*
 * Interrupt routine
 */
rnintr(sc)
	register struct rn_softc *sc;
{
	caddr_t addr;
	int len, i;
	register struct upd_stat *st;
#define dtrv ((struct upd_dtrv *)st)
#define txed ((struct upd_txed *)st)

	/* Clear interrupt status */
	outb(sc->sc_addr + RN_CLI, 0);

	/* Scan the status reports */
	for (;;) {
		st = &sc->sc_stattab[sc->sc_statno];
		dprintf(("[%d->%x] ", sc->sc_statno, st->stsn & 0xff));
		switch (st->stsn) {
		case 0xfc:	/* no more status words */
		case 0xfd:
		case 0xfe:
		case 0xff:
			dprintf(("\n"));
			return (1);

		default:
			printf("rn%d: invalid status %x\n", sc->sc_if.if_unit,
			    st->stsn);
			break;

		case uPD_DTRV:		/* Data Received */
			len = dtrv->bc0 + (dtrv->bc1 << 8);
			addr = sc->sc_vaddr + dtrv->bufa0 + (dtrv->bufa1 << 8) +
				(dtrv->bufa2 << 16);
			dprintf(("RCV: len=%d addr=%x\n", len, addr));

#ifdef RNDEBUG
			if (rndebug > 1) {
				char *p;

				i = len;
				if (i >= (79/3))
					i = 79/3;
				p = addr;
				while (i--)
					printf("%02x ", *p++ & 0xff);
				printf("\n");
				if ((addr - sc->sc_rbufs) % HDLCMAX)
					printf("rn%d: non-aligned buffer?\n",
					    sc->sc_if.if_unit);
			}
#endif /* RNDEBUG */
			sc->sc_if.if_ipackets++;
			rnread(sc, addr, len);

			/*
			 * Release the buffer
			 */
			sc->sc_rbuftab[(addr - sc->sc_rbufs) / HDLCMAX].brdy = 0;
			break;


		case uPD_TXUR:	/* Data Transmit Interrupt */
			dprintf(("TXUR cmds=%d\n", txed->txen));
			sc->sc_if.if_oerrors += txed->txen;
			goto rls_tx_buf;

		case uPD_TXED:	/* Data Transmission End */
			dprintf(("TXED cmds=%d\n", txed->txen));
			sc->sc_if.if_opackets += txed->txen;
		
			/*
			 * Release transmit buffers
			 */
		rls_tx_buf:
			for (i = txed->txen; i-- > 0; ) {
				if (sc->sc_tqueue == 0)
					break;
				if (sc->sc_tqueue <  (struct upd_dtsd *)(sc->sc_cmdtab) ||
				    sc->sc_tqueue >= (struct upd_dtsd *)(sc->sc_stattab)) {
					dprintf(("sc->sc_tqueue == %x\n", sc->sc_tqueue));
					printf("rn%d: tx queue botched (0x%x).\n",
					    sc->sc_if.if_unit, sc->sc_tqueue);
					sc->sc_tqueue = 0;
					break;
				}
				rncopy(sc->sc_tqueue->txdt, &sc->sc_tqueue,
						    sizeof(struct upd_dtsd *));
				dprintf(("NEW tqueue = %x\n", sc->sc_tqueue));
			}
			if (sc->sc_tqueue == 0) {
				sc->sc_tqtail = 0;
				sc->sc_tavail = sc->sc_tstart;
				sc->sc_tbusy = sc->sc_tend;
			} else if (sc->sc_tqueue <  (struct upd_dtsd *)(sc->sc_cmdtab) ||
				   sc->sc_tqueue >= (struct upd_dtsd *)(sc->sc_stattab)) {
				dprintf(("sc->sc_tqueue == %x\n", sc->sc_tqueue));
				printf("rn%d: tx queue botched (0x%x)\n",
					sc->sc_if.if_unit, sc->sc_tqueue);
				sc->sc_tqueue = 0;
				sc->sc_tqtail = 0;
				sc->sc_tavail = sc->sc_tstart;
				sc->sc_tbusy = sc->sc_tend;
			} else {
				sc->sc_tbusy = sc->sc_tqueue->bufa0 +
				    (sc->sc_tqueue->bufa1 << 8) +
				    (sc->sc_tqueue->bufa2 << 16) +
				    sc->sc_vaddr;
				sc->sc_tbusy += sc->sc_tqueue->bc0 +
				    (sc->sc_tqueue->bc1 << 8);
			}
			sc->sc_if.if_flags &= ~IFF_OACTIVE;
			sc->sc_if.if_timer = 0;
			(void) rnstart(&sc->sc_if);
			break;

		case uPD_TOUT:	/* Idle Watchdog Timer Timeout */
			dprintf(("TOUT\n"));
			break;

		case uPD_LOAK:	/* Line Open End */
			dprintf2(("LOAK\n"));
			break;

		case uPD_LCAK:	/* Line Close End */
			dprintf2(("LCAK\n"));
			break;

		case uPD_CILG:	/* Command Illegal */
			dprintf(("CILG\n"));
			break;

		case uPD_GI1C:	/* General-purpose Input Pin Change Detection I */ 
		case uPD_GI2C:	/* General-purpose Input Pin Change Detection II */
		case uPD_GPAK:	/* General-purpose I/O Pin Read Response */
		case uPD_GPAE:	/* General-purpose I/O Pin Read Response II */
			dprintf2(("GI CHG/READ - %x\n", st->stsn));
			break;

		case uPD_OLSW:	/* Status Table Overflow */
			printf("rn%d: status table overflow\n",
			    sc->sc_if.if_unit);
			break;
		}
		st->stsn = 0xff;
		sc->sc_statno++;
		if (sc->sc_statno == sc->sc_nlcw)
			sc->sc_statno = 0;
	}
	/* NOTREACHED */
#undef dtrv
#undef txed
}

/*
 * Send a command to uPD72103
 * returns 0 if everything is ok and 1 if not
 */
rnsendcmd(sc, cmdp, cmdlen)
	register struct rn_softc *sc;
	register caddr_t cmdp;
	register cmdlen;
{
	struct upd_cmd *cp;
	u_char *p;
	int i, s;

	s = splimp();
	dprintf(("CMD=%x SLOT %d\n", *cmdp & 0xff, sc->sc_cmdp));

	/*
	 * Choose a new command slot
	 */
	cp = &sc->sc_cmdtab[sc->sc_cmdp];
	if ((cp->zero & 0xfc) != 0xfc)
		return (1);
	sc->sc_cmdp++;
	if (sc->sc_cmdp == sc->sc_nlcw)
		sc->sc_cmdp = 0;

	/*
	 * Copy command to the slot
	 */
	cp->cmd = *cmdp++;
	cmdp++;		/* skip zero */
	cmdlen -= 2;
	p = cp->args;
	for (i = sizeof(cp->args); i--; )
		*p++ = (cmdlen-- > 0)? *cmdp++ : 0;
	
	/*
	 * Link the data send command(s)
	 */
	if (cp->cmd == uPD_DTSD) {
		p = ((struct upd_dtsd *)cp)->txdt;
		*p++ = 0; *p++ = 0; *p++ = 0; *p = 0;
		if (sc->sc_tqtail != 0)
			rncopy(&cp, sc->sc_tqtail->txdt, sizeof cp);
		else
			sc->sc_tqueue = (struct upd_dtsd *)cp;
		sc->sc_tqtail = (struct upd_dtsd *)cp;
		sc->sc_if.if_timer = 2;
	}

	/*
	 * Activate the command
	 */
	cp->zero = 0;
	outb(sc->sc_addr + RN_CRL, 1);
	splx(s);
	return (0);
}

/*
 * Set operation modes and open the line
 */
void
rnsetmodes(sc)
	register struct rn_softc *sc;
{
	int i;
	static struct upd_mdst Mdst = {
		uPD_MDST,			/* cmd */
		0,				/* zero */
		uPD_M0_DMAW0,			/* m0 */
		uPD_M1_NOADR|uPD_M1_SHORT2|uPD_M1_OCT|uPD_M1_NOLOOP,
		uPD_M2_TXED|uPD_M2_GICS2,	/* m2 */
		0,				/* stbc */
		0,				/* time */
		HDLCMAX & 0xff,                 /* rxbs0 */
		HDLCMAX >> 8,                   /* rxbs1 */
		HDLCMAX & 0xff,                 /* maxd0 */
		HDLCMAX >> 8,                   /* maxd1 */
		0,				/* hold */
		0				/* retn */	
	};
	static struct upd_lopn Lopn = {
		uPD_LOPN,			/* cmd */
		0				/* zero */
	};
	static struct upd_gowr Gowr = {
		uPD_GOWR,			/* cmd */
		0,				/* zero */
		0 /* !RTS == 0, !DTR == 0 */	/* go */
	};
		
	/* set modes */
	i = 10000;
	while (rnsendcmd(sc, (caddr_t) &Mdst, sizeof(Mdst)) && --i)
		;
	if (i == 0)
		goto err;
	
	/* open line */
	i = 10000;
	while (rnsendcmd(sc, (caddr_t) &Lopn, sizeof(Lopn)) && --i)
		;
	if (i == 0)
		goto err;

	/* assert RTS and DTR */
	i = 10000;
	while (rnsendcmd(sc, (caddr_t) &Gowr, sizeof(Gowr)) && --i)
		;
	if (i == 0)
		goto err;
	return;
err:
	printf("rn%d: cannot set modes\n", sc->sc_if.if_unit);
}

/*
 * Close line
 */
void
rncloseline(sc)
	struct rn_softc *sc;
{
	static struct upd_lcls Lcls = {
		uPD_LCLS,	/* cmd */
		0		/* zero */
	};
	static struct upd_gowr Gowr = {
		uPD_GOWR,			/* cmd */
		0,				/* zero */
		uPD_GOWR_1 |	/* !RTS */	/* go */
		uPD_GOWR_2	/* !DTR */
	};
	
	/* Drop RTS and DTR */
	(void) rnsendcmd(sc, (caddr_t)&Gowr, sizeof(Gowr));

	/* Close line */
	(void) rnsendcmd(sc, (caddr_t)&Lcls, sizeof(Lcls));
}

/*
 * Modem control
 * flag = 0 -- disable line; flag = 1 -- enable line
 */
int
rnmdmctl(pp, flag)
	struct p2pcom *pp;
	int flag;
{
	struct rn_softc *sc = rncd.cd_devs[pp->p2p_if.if_unit];

	if (flag)
		rnsetmodes(sc);
	else
		rncloseline(sc);

	/* XXX Report "carrier" change -- should be something more intelligent */
	if (sc->sc_p2pcom.p2p_modem)
		(*sc->sc_p2pcom.p2p_modem)(pp, flag);
}

/*
 * Pass the accepted IP packet to upper levels
 */
void
rnread(sc, cp, totlen)
	register struct rn_softc *sc;
	register caddr_t cp;
	int totlen;
{
	struct mbuf *m, *top, **mp;
	int len;
	extern struct timeval time;

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
		rncopy(cp, mtod(m, caddr_t), (unsigned)len);
		cp += len;
		*mp = m;
		mp = &m->m_next;
	}

	/* pass the packet upstream */
	(*sc->sc_p2pcom.p2p_input)(&sc->sc_p2pcom, top);
}
