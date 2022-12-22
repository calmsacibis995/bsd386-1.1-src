/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: if_ne.c,v 1.12 1994/01/30 07:46:16 karels Exp $
 */

/*-
 * Copyright (c) 1990, 1991 William F. Jolitz.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)if_ne.c	7.4 (Berkeley) 5/21/91
 */

/*
 * NE-1000/NE-2000 Ethernet driver
 *
 * Parts inspired from Tim Tucker's if_wd driver for the wd8003,
 * insight on the ne2000 gained from Robert Clements PC/FTP driver.
 */

#include "bpfilter.h"

/* #define NEDEBUG */

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

#include "isavar.h"

#if NBPFILTER > 0
#include "net/bpf.h"
#include "net/bpfdesc.h"
#endif

#include "if_nereg.h"
#include "icu.h"
#include "machine/cpu.h"

#ifdef NEDEBUG
#define dprintf(x)	printf x
#else
#define dprintf(x)
#endif

#define ETHER_MIN_LEN 64
#define ETHER_MAX_LEN 1536

/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * ns_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 */
struct	ne_softc {
	struct	device ne_dev;		/* base device (must be first) */
	struct	isadev ne_id;		/* ISA device */
	struct	intrhand ne_ih;		/* interrupt vectoring */	
	struct	arpcom ns_ac;		/* Ethernet common part */
#define	ns_if	ns_ac.ac_if		/* network-visible interface */
#define	sc_addr	ns_ac.ac_enaddr		/* hardware Ethernet address */
	int	ns_base;		/* the board base address */
	int	ns_ba;			/* byte addr in buffer ram of inc pkt */
	int	ns_cur;			/* current page being filled */
	int	ns_ifoldflags;		/* previous if_flags */
	u_char	ns_ne1000;		/* true if the board is NE1000 */
	u_char	ns_tbuf;		/* start of TX buffer (pages) */
	u_char	ns_rbuf;		/* begin of RX ring (pages) */
	u_char	ns_rbufend;		/* end of RX ring (pages) */
	struct	prhdr	ns_ph;		/* hardware header of incoming packet*/
	struct	ether_header ns_eh;	/* header of incoming packet */
	char	ns_pb[2048 /*ETHERMTU+sizeof(long)*/];
#if NBPFILTER > 0
	caddr_t	ns_bpf;			/* BPF filter */
#endif
};

/*
 * Prototypes
 */
int neprobe __P((struct device *, struct cfdata *, void *));
void neattach __P((struct device *, struct device *, void *));
int neintr __P((struct ne_softc *));
int neinit __P((int));
int nestart __P((struct ifnet *));
int neioctl __P((struct ifnet *, int, caddr_t));

static nememtest __P((int, int, int));
void nefetch __P((int, int, caddr_t, int, int));
void neput __P((int, int, caddr_t, int, int));
void nerecv __P((struct ne_softc *));
void neread __P((struct ne_softc *, char *, int));
struct mbuf *neget __P((caddr_t, int, int, struct ifnet *));

struct cfdriver necd =
	{ NULL, "ne", neprobe, neattach, sizeof(struct ne_softc) };

extern int autodebug;

/*
 * Probe routine
 */
/* ARGSUSED */
neprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	void neforceintr(); 
	int val, i;
	register nec;

#ifdef lint
	neintr(0);
#endif

	nec = ia->ia_iobase;
	if (isa_portcheck(nec, NE_NPORT) == 0) {
	 	if (autodebug)
			printf("port range overlaps existing, ");
		return (0);
	}
	/*
	 * Check configuration parameters
	 */
	if (!NE_IOBASEVALID(nec)) {
		printf("ne%d: invalid i/o base address %x\n", cf->cf_unit, nec);
		return (0);
	}

	/*
	 * Reset the board and check its existence
	 */
	dprintf(("ne%d: RESET\n", cf->cf_unit));
	val = inb(nec+ne_reset);
	DELAY(10000);
	outb(nec+ne_reset, val);

	outb(nec+ds_cmd, DSCM_STOP|DSCM_NODMA);
	
	dprintf(("ne%d: WAIT FOR RES", cf->cf_unit));
	i = 100000;
	while ((inb(nec+ds0_isr)&DSIS_RESET) == 0 && i-- > 0)
		;
	dprintf(("-- res=%d\n", i));
	if (i < 0)
		return (0);

	/* Reset interrupts */
	outb(nec+ds0_isr, 0xff);

	/* 
	 * Select NE1000 params -- byte transfers, burst mode, FIFO at 8 bytes.
	 */
	outb(nec+ds0_dcr, DSDC_BMS|DSDC_FT1);

	outb(nec+ds_cmd, DSCM_NODMA|DSCM_PG0|DSCM_STOP);
	DELAY(100);
	if (inb(nec+ds_cmd) != (DSCM_NODMA|DSCM_PG0|DSCM_STOP))
		return (0);

	/*
	 * Try to touch NE-2000 memory.  Use NE-1000 (byte-mode) transfer,
	 * as word mode wedges an NE-1000.
	 */
	ia->ia_aux = (void *) 0;	/* NE-2000 by default */
	dprintf(("TRY NE2000"));
	outb(nec+ds0_dcr, DSDC_BMS|DSDC_FT1);
	if (nememtest(nec, 1, NE2000_TBUF)) {
		dprintf((" TRY NE1000"));
		if (nememtest(nec, 1, NE1000_TBUF)) {
			/*
			 * Last chance: try NE-2000 with word transfers.
			 * Some "compatibles" don't do byte transfers.
			 */
			dprintf(("TRY NE2000 word-mode"));
			outb(nec+ds0_dcr, DSDC_WTS|DSDC_BMS|DSDC_FT1);
			if (nememtest(nec, 0, NE2000_TBUF)) {
			if (autodebug)
				printf("mem test failed, ");
				dprintf(("OOPS\n"));
				return (0);
			}
		} else
			/* It's NE-1000 */
			ia->ia_aux = (void *)1;
	}
	dprintf(("OK\n"));

	/*
	 * Find out IRQ if unknown
	 */
	if (ia->ia_irq == IRQUNK) {
		ia->ia_irq = isa_discoverintr(neforceintr, aux);
		outb(nec+ds0_imr, 0);
		if (ffs(ia->ia_irq) - 1 == 0)
			return (0);
	}
	ia->ia_iosize = NE_NPORT;
	return (1);
}

/*
 * Test on-board memory at the specified address, returning 0 if good.
 */
static
nememtest(nec, ne1000, addr)
	register int nec;
	int ne1000;
	int addr;
{
	static char test_pattern[32] = "*** This is a test pattern ***";
	char testbuf[32];

	dprintf((" -- PUT --"));
	neput(nec, ne1000, test_pattern, addr, sizeof(test_pattern));
	dprintf((" FETCH -- "));
	nefetch(nec, ne1000, testbuf, addr, sizeof(test_pattern));
	return (bcmp(testbuf, test_pattern, sizeof(test_pattern)));
}

/*
 * force the card to interrupt (tell us where it is) for autoconfiguration
 */ 
void
neforceintr(arg)
	void *arg;
{
	int s;
	register nec = ((struct isa_attach_args *) arg)->ia_iobase;

	s = splhigh();

	/* init chip regs */
	outb(nec+ds_cmd, DSCM_NODMA|DSCM_PG0|DSCM_STOP);
	outb(nec+ds0_rbcr0, 0);
	outb(nec+ds0_rbcr1, 0);
	outb(nec+ds0_imr, 0);
	outb(nec+ds0_isr, 0xff);
	outb(nec+ds0_dcr, DSDC_BMS|DSDC_FT1);
	outb(nec+ds0_tcr, 0);
	outb(nec+ds0_rcr, DSRC_MON);
	outb(nec+ds0_tpsr, 0);
	outb(nec+ds0_pstart, NE1000_RBUF / DS_PGSIZE);
	outb(nec+ds0_pstop, NE1000_RBUFEND / DS_PGSIZE);
	outb(nec+ds0_bnry, NE1000_RBUF / DS_PGSIZE);
	outb(nec+ds_cmd, DSCM_NODMA|DSCM_PG1|DSCM_STOP);
	outb(nec+ds1_curr,  NE1000_RBUF / DS_PGSIZE);
	outb(nec+ds_cmd, DSCM_NODMA|DSCM_PG0|DSCM_START);
	outb(nec+ds0_rcr, DSRC_AB);
	outb(nec+ds0_imr, 0xff);

	outb(nec+ds0_tbcr0, ETHER_MIN_LEN);
	outb(nec+ds0_tbcr1, 0);
	outb(nec+ds0_tpsr, NE1000_TBUF / DS_PGSIZE);
	outb(nec+ds_cmd, DSCM_TRANS|DSCM_NODMA|DSCM_START);
	splx(s);
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.  We get the ethernet address here.
 */
/* ARGSUSED */
void
neattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	register struct ne_softc *ns = (struct ne_softc *) self;
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	int unit = ns->ne_dev.dv_unit;
	register struct ifnet *ifp = &ns->ns_if;
	register nec;
	int i;

	/*
	 * Set up NE1000/NE2000 stuff
	 */
	nec = ns->ns_base = ia->ia_iobase;
	outb(nec+ds_cmd, DSCM_NODMA|DSCM_PG0|DSCM_STOP);
	if ((int)(ia->ia_aux)) {
		ns->ns_ne1000 = 1;
		ns->ns_tbuf = NE1000_TBUF / DS_PGSIZE;
		ns->ns_rbuf = NE1000_RBUF / DS_PGSIZE;
		ns->ns_rbufend = NE1000_RBUFEND / DS_PGSIZE;
		outb(nec+ds0_dcr, DSDC_BMS|DSDC_FT1);
	} else {
		/* Setup NE2000 params */
		ns->ns_ne1000 = 0;
		ns->ns_tbuf = NE2000_TBUF / DS_PGSIZE;
		ns->ns_rbuf = NE2000_RBUF / DS_PGSIZE;
		ns->ns_rbufend = NE2000_RBUFEND / DS_PGSIZE;
		outb(nec+ds0_dcr, DSDC_WTS|DSDC_BMS|DSDC_FT1);
	}

	/*
	 * Make sure the board is silent and
	 * prepare for extracting the PROM address
	 */
	outb(nec+ds0_rcr, DSRC_MON);
	outb(nec+ds0_imr, 0);
	outb(nec+ds0_isr, 0);
	outb(nec+ds_cmd, DSCM_NODMA|DSCM_PG0|DSCM_STOP);

	/*
	 * Extract board address
	 */
	dprintf(("ne%d: EXTRACT ADDR\n", unit));
	nefetch(nec, ns->ns_ne1000, ns->ns_pb, 0, 2 * ETHER_ADDR_LEN);
	if (ns->ns_ne1000)
		for (i = 0; i < ETHER_ADDR_LEN; i++)
			ns->sc_addr[i] = ns->ns_pb[i];
	else
		for (i = 0; i < ETHER_ADDR_LEN; i++)
			ns->sc_addr[i] = ((u_short *)(ns->ns_pb))[i];

	/*
	 * Initialize interface structure
	 */
	ifp->if_unit = unit;
	ifp->if_name = necd.cd_name;
	ifp->if_mtu = ETHERMTU;
	ifp->if_flags = IFF_BROADCAST | IFF_MULTICAST | IFF_SIMPLEX |
	    IFF_NOTRAILERS;
	ifp->if_init = neinit;
	ifp->if_output = ether_output;
	ifp->if_start = nestart;
	ifp->if_ioctl = neioctl;
	ifp->if_watchdog = 0;
	if_attach(ifp);

	printf(": NE-%c000, address %s\n", ns->ns_ne1000 ? '1' : '2',
	    ether_sprintf(ns->sc_addr));

#if NBPFILTER > 0
	bpfattach(&ns->ns_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif

	isa_establish(&ns->ne_id, &ns->ne_dev);
	ns->ne_ih.ih_fun = neintr;
	ns->ne_ih.ih_arg = (void *)ns;
	intr_establish(ia->ia_irq, &ns->ne_ih, DV_NET);
}

/*
 * Initialization of interface; set up initialization block
 * and transmit/receive descriptor rings.
 */
neinit(unit)
	int unit;
{
	register struct ne_softc *ns = necd.cd_devs[unit];
	struct ifnet *ifp = &ns->ns_if;
	int s;
	register nec = ns->ns_base;
	register i;

 	if (ifp->if_addrlist == (struct ifaddr *) 0)
 		return (0);
	if ((ifp->if_flags & IFF_RUNNING) && ifp->if_flags == ns->ns_ifoldflags)
		return (0);

	s = splimp();

	/* set physical address on ethernet */
	outb(nec+ds_cmd, DSCM_NODMA|DSCM_PG1|DSCM_STOP);
	for (i = 0; i < 6; i++)
		outb(nec+ds1_par0+i, ns->sc_addr[i]);

	/* clr logical address hash filter for now */
	for (i = 0; i < 8; i++)
		outb(nec+ds1_mar0+i, 0xff);

	/*
	 * Init DS8390
	 */
	outb(nec+ds_cmd, DSCM_NODMA|DSCM_PG0|DSCM_STOP);
	outb(nec+ds0_rbcr0, 0);
	outb(nec+ds0_rbcr1, 0);
	outb(nec+ds0_imr, 0);
	outb(nec+ds0_isr, 0xff);
	outb(nec+ds0_dcr, ns->ns_ne1000 ?
	    DSDC_BMS|DSDC_FT1 : DSDC_WTS|DSDC_BMS|DSDC_FT1);
	outb(nec+ds0_tcr, 0);
	outb(nec+ds0_rcr, DSRC_MON);
	outb(nec+ds0_tpsr, 0);
	outb(nec+ds0_pstart, ns->ns_rbuf);
	outb(nec+ds0_pstop, ns->ns_rbufend);
	outb(nec+ds0_bnry, ns->ns_rbuf);
	outb(nec+ds_cmd, DSCM_NODMA|DSCM_PG1|DSCM_STOP);
	outb(nec+ds1_curr, ns->ns_rbuf);
	outb(nec+ds_cmd, DSCM_NODMA|DSCM_PG0|DSCM_START);
	if (ifp->if_flags & IFF_PROMISC)
		outb(nec+ds0_rcr, DSRC_AB|DSRC_AM|DSRC_PRO);
	else if (ns->ns_ac.ac_multiaddrs != 0 || ifp->if_flags & IFF_ALLMULTI)
		outb(nec+ds0_rcr, DSRC_AB|DSRC_AM);
	else
		outb(nec+ds0_rcr, DSRC_AB);
	outb(nec+ds0_imr, 0xff);

	ns->ns_cur = ns->ns_rbuf;

	ns->ns_if.if_flags &= ~IFF_OACTIVE;
	ns->ns_if.if_flags |= IFF_RUNNING;
	nestart(ifp);
	splx(s);
}

/*
 * Setup output on interface.
 * Get another datagram to send off of the interface queue,
 * and copy it to the interface before starting the output.
 * Called only at splimp or interrupt level.
 */
nestart(ifp)
	struct ifnet *ifp;
{
	register struct ne_softc *ns = necd.cd_devs[ifp->if_unit];
	register nec = ns->ns_base;
	struct mbuf *m0, *m;
	u_char cmd, oddword[2];
	int len;

	if (ns->ns_if.if_flags & IFF_OACTIVE)
		panic("nestart reentered");

	IF_DEQUEUE(&ns->ns_if.if_snd, m0);
	if (m0 == 0)
		return (0);

	ns->ns_if.if_flags |= IFF_OACTIVE;	/* prevent entering nestart */
	
#if NBPFILTER > 0
	/*
	 * Feed outgoing packet to bpf
	 */
	if (ns->ns_bpf)
		bpf_mtap(ns->ns_bpf, m0);
#endif

	/*
	 * Calculate the length of a packet.
	 * XXX should use m->m_hdr.len.
	 */
	for (len = 0, m = m0; m != 0; m = m->m_next)
		len += m->m_len;
	if ((len & 01) && !ns->ns_ne1000)
		len++;

	/* Setup for remote dma */
	cmd = inb(nec+ds_cmd);
	outb(nec+ds_cmd, DSCM_NODMA|DSCM_PG0|DSCM_START);

	outb(nec+ds0_isr, DSIS_RDC);
	outb(nec+ds0_rbcr0, len);
	outb(nec+ds0_rbcr1, len >> 8);
	outb(nec+ds0_rsar0, 0);
	outb(nec+ds0_rsar1, ns->ns_tbuf);

	outb(nec+ds_cmd, DSCM_RWRITE|DSCM_PG0|DSCM_START);
	
	/*
	 * Push out data from each mbuf, watching out
	 * for 0 length mbufs and odd-length mbufs (on ne2000).
	 */
	if (ns->ns_ne1000) {
		for (m = m0; m != 0; m = m->m_next) {
			if (m->m_len == 0)
				continue;
			outsb(nec+ne_data, m->m_data, m->m_len);
		}
	} else {
		for (m = m0; m != 0; ) {
			if (m->m_len >= 2)
				outsw(nec+ne_data, m->m_data, m->m_len / 2);
			if (m->m_len & 1) {
				oddword[0] = *(mtod(m, caddr_t) + m->m_len - 1);
				if (m = m->m_next) {
					oddword[1] = *(mtod(m, caddr_t));
					m->m_data++;
					m->m_len--;
				} else
					oddword[1] = 0;
				outsw(nec+ne_data, oddword, 1);		/* "outw" */
			} else
				m = m->m_next;
		}
	}

	/* Wait till done, then shutdown feature */
	while ((inb(nec+ds0_isr) & DSIS_RDC) == 0)
		;
	outb(nec+ds0_isr, DSIS_RDC);
	outb(nec+ds_cmd, cmd);		/* Clear remote DMA */

	/*
	 * Init transmit length registers, and set transmit start flag.
	 */
	if (len < ETHER_MIN_LEN)
		len = ETHER_MIN_LEN;
	outb(nec+ds0_tbcr0, len);
	outb(nec+ds0_tbcr1, len >> 8);
	outb(nec+ds0_tpsr, ns->ns_tbuf);
	outb(nec+ds_cmd, DSCM_TRANS|DSCM_NODMA|DSCM_START);

	m_freem(m0);
}

/*
 * Controller interrupt.
 */
neintr(ns)
	register struct ne_softc *ns;
{
	register nec = ns->ns_base;
	u_char cmd, isr;

	/* Save cmd, clear interrupt */
	cmd = inb(nec+ds_cmd);
loop:
	isr = inb(nec+ds0_isr);
	outb(nec+ds_cmd,DSCM_NODMA|DSCM_START);
	outb(nec+ds0_isr, isr);

	/* Receiver error */
	if (isr & DSIS_RXE) {
		/* need to read these registers to clear status */
		(void) inb(nec + ds0_rsr);
		(void) inb(nec + ds0_rcvalctr);
		(void) inb(nec + ds0_rcvcrcctr);
		(void) inb(nec + ds0_rcvfrmctr);
		ns->ns_if.if_ierrors++;
	}

	/* We received something; rummage thru tiny ring buffer */
	if (isr & (DSIS_RX|DSIS_RXE|DSIS_ROVRN)) {
		u_char pend, lastfree;

		outb(nec+ds_cmd, DSCM_START|DSCM_NODMA|DSCM_PG1);
		pend = inb(nec+ds1_curr);
		outb(nec+ds_cmd, DSCM_START|DSCM_NODMA|DSCM_PG0);
		lastfree = inb(nec+ds0_bnry);

		/* Have we wrapped? */
		if (lastfree >= ns->ns_rbufend)
			lastfree = ns->ns_rbuf;
		if (pend < lastfree && ns->ns_cur < pend)
			lastfree = ns->ns_cur;
		else	if (ns->ns_cur > lastfree)
			lastfree = ns->ns_cur;

		/* Something in the buffer? */
		while (pend != lastfree) {
			u_char nxt;

			/* Extract header from microcephalic board */
			nefetch(nec, ns->ns_ne1000, (caddr_t)&ns->ns_ph,
			    lastfree * DS_PGSIZE, sizeof(ns->ns_ph));
			ns->ns_ba = lastfree * DS_PGSIZE + sizeof(ns->ns_ph);

			/* Incipient paranoia */
			if (ns->ns_ph.pr_status == DSRS_RPC ||
				/* for dequna's */
				ns->ns_ph.pr_status == 0x21)
				nerecv(ns);
#ifdef NEDEBUG
			else  {
				printf("cur %x pnd %x lfr %x ",
					ns->ns_cur, pend, lastfree);
				printf("nxt %x len %x ", ns->ns_ph.pr_nxtpg,
				    (ns->ns_ph.pr_sz1 << 8) + ns->ns_ph.pr_sz0);
				printf("Bogus Sts %x\n", ns->ns_ph.pr_status);	
			}
#endif

			nxt = ns->ns_ph.pr_nxtpg;

			/* Sanity check */
			if (nxt >= ns->ns_rbuf && nxt <= ns->ns_rbufend &&
			    nxt <= pend)
				ns->ns_cur = nxt;
			else
				ns->ns_cur = nxt = pend;

			/* Set the boundaries */
			lastfree = nxt;
			if (--nxt < ns->ns_rbuf)
				nxt = ns->ns_rbufend - 1;
			outb(nec+ds0_bnry, nxt);
			outb(nec+ds_cmd, DSCM_START|DSCM_NODMA|DSCM_PG1);
			pend = inb(nec+ds1_curr);
			outb(nec+ds_cmd, DSCM_START|DSCM_NODMA|DSCM_PG0);
		}
		outb(nec+ds_cmd, DSCM_START|DSCM_NODMA);
	}

	/* Packet Transmitted or Transmit error */
	if (isr & (DSIS_TX|DSIS_TXE)) {
		ns->ns_if.if_flags &= ~IFF_OACTIVE;
		/* Need to read these registers to clear status */
		++ns->ns_if.if_opackets;
		ns->ns_if.if_collisions += inb(nec+ds0_tbcr0);
		if (isr & DSIS_TXE)
			ns->ns_if.if_oerrors++;
	}

	/* Receiver ovverun? */
	if (isr & DSIS_ROVRN) {
		log(LOG_ERR, "ne%d: error: isr %x\n", ns->ne_dev.dv_unit,
		    isr /*, DSIS_BITS*/);
		outb(nec+ds0_rbcr0, 0);
		outb(nec+ds0_rbcr1, 0);
		outb(nec+ds0_tcr, DSTC_LB0);
		outb(nec+ds0_rcr, DSRC_MON);
		outb(nec+ds_cmd, DSCM_START|DSCM_NODMA);
		outb(nec+ds0_rcr, DSRC_AB);
		outb(nec+ds0_tcr, 0);
	}

	/* Any more to send? */
	outb(nec+ds_cmd, DSCM_NODMA|DSCM_PG0|DSCM_START);
	if ((ns->ns_if.if_flags & IFF_OACTIVE) == 0)
		nestart(&ns->ns_if);
	outb(nec+ds_cmd, cmd);
	outb(nec+ds0_imr, 0xff);

	/* Still more to do? */
	isr = inb(nec+ds0_isr);
	if (isr)
		goto loop;
	return (1);
}

/*
 * Ethernet interface receiver interface.
 * If input error just drop packet.
 * Otherwise examine packet to determine type.  If can't determine length
 * from type, then have to drop packet.  Othewise decapsulate
 * packet based on type and pass to type specific higher-level
 * input routine.
 */
void
nerecv(ns)
	register struct ne_softc *ns;
{
	int len;
	int b;
	register int l;
	register caddr_t p;
	register nec = ns->ns_base;

	ns->ns_if.if_ipackets++;
	len = ns->ns_ph.pr_sz0 + (ns->ns_ph.pr_sz1 << 8);
	if(len < ETHER_MIN_LEN || len > ETHER_MAX_LEN)
		return;

	/* this need not be so torturous - one/two bcopys at most into mbufs */
	nefetch(nec, ns->ns_ne1000, ns->ns_pb, ns->ns_ba,
	    min(len, DS_PGSIZE - sizeof(ns->ns_ph)));
	if (len > DS_PGSIZE - sizeof(ns->ns_ph)) {
		l = len - (DS_PGSIZE - sizeof(ns->ns_ph));
		p = ns->ns_pb + (DS_PGSIZE - sizeof(ns->ns_ph));

		if(++ns->ns_cur >= ns->ns_rbufend)
			ns->ns_cur = ns->ns_rbuf;
		b = ns->ns_cur*DS_PGSIZE;
		
		while (l >= DS_PGSIZE) {
			nefetch(nec, ns->ns_ne1000, p, b, DS_PGSIZE);
			p += DS_PGSIZE;
			l -= DS_PGSIZE;
			if(++ns->ns_cur >= ns->ns_rbufend)
				ns->ns_cur = ns->ns_rbuf;
			b = ns->ns_cur*DS_PGSIZE;
		}
		if (l > 0)
			nefetch(nec, ns->ns_ne1000, p, b, l);
	}
	/* don't forget checksum! */
	len -= sizeof(struct ether_header) + sizeof(long);
			
	neread(ns, (caddr_t)(ns->ns_pb), len);
}

/*
 * Fetch from onboard ROM/RAM
 */
void
nefetch(nec, ne1000, up, ad, len)
	register nec;
	int ne1000;
	caddr_t up;
	int ad, len;
{
	u_char cmd;
	int cnt;

	if (!ne1000 && len & 01)
		len++;

	cmd = inb(nec+ds_cmd);
	outb(nec+ds_cmd, DSCM_NODMA|DSCM_PG0|DSCM_START);

	/* Setup remote dma */
	outb(nec+ds0_isr, DSIS_RDC);
	outb(nec+ds0_rbcr0, len);
	outb(nec+ds0_rbcr1, len >> 8);
	outb(nec+ds0_rsar0, ad);
	outb(nec+ds0_rsar1, ad >> 8);

	/* Execute & extract from card */
	outb(nec+ds_cmd, DSCM_RREAD|DSCM_PG0|DSCM_START);
	if (ne1000)
		insb(nec+ne_data, up, len);
	else
		insw(nec+ne_data, up, len / 2);

	/* Wait till done, then shutdown feature */
	cnt = 10000;
	while ((inb(nec+ds0_isr) & DSIS_RDC) == 0 && --cnt)
		;
	outb(nec+ds0_isr, DSIS_RDC);
	outb(nec+ds_cmd, cmd);
}

/*
 * Put to onboard RAM.
 */
void
neput(nec, ne1000, up, ad, len)
	register int nec;
	int ne1000;
	caddr_t up;
	int ad, len;
{
	u_char cmd;
	int cnt;
	
	if (!ne1000 && len & 01)
		len++;

	cmd = inb(nec+ds_cmd);
	outb(nec+ds_cmd, DSCM_NODMA|DSCM_PG0|DSCM_START);

	/* Setup for remote dma */
	outb(nec+ds0_isr, DSIS_RDC);
	outb(nec+ds0_rbcr0, len);
	outb(nec+ds0_rbcr1, len >> 8);
	outb(nec+ds0_rsar0, ad);
	outb(nec+ds0_rsar1, ad >> 8);

	/* Execute & stuff to card */
	outb(nec+ds_cmd, DSCM_RWRITE|DSCM_PG0|DSCM_START);
	if (ne1000)
		outsb(nec+ne_data, up, len);
	else
		outsw(nec+ne_data, up, len / 2);
	
	/* Wait till done, then shutdown feature */
	cnt = 10000;
	while ((inb(nec+ds0_isr) & DSIS_RDC) == 0 && --cnt)
		;
	outb(nec+ds0_isr, DSIS_RDC);
	outb(nec+ds_cmd, cmd);
}

/*
 * Pass a packet to the higher levels.
 * We deal with the trailer protocol here.
 */
void
neread(ns, buf, len)
	register struct ne_softc *ns;
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
#define	nedataaddr(eh, off, type)	((type)(((caddr_t)((eh)+1)+(off))))
	if (eh->ether_type >= ETHERTYPE_TRAIL &&
	    eh->ether_type < ETHERTYPE_TRAIL+ETHERTYPE_NTRAILER) {
		off = (eh->ether_type - ETHERTYPE_TRAIL) * 512;
		if (off >= ETHERMTU)
			return;		/* sanity */
		eh->ether_type = ntohs(*nedataaddr(eh, off, u_short *));
		resid = ntohs(*(nedataaddr(eh, off + 2, u_short *)));
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
        if (ns->ns_bpf) {
                eh->ether_type = htons((u_short)eh->ether_type);
                bpf_tap(ns->ns_bpf, buf, len + sizeof(struct ether_header));
                eh->ether_type = ntohs((u_short)eh->ether_type);

                /*
                 * Note that the interface cannot be in promiscuous mode if
                 * there are no bpf listeners.  And if we are in promiscuous
                 * mode, we have to check if this packet is really ours.
                 */
                if ((ns->ns_if.if_flags & IFF_PROMISC) &&
                    bcmp(eh->ether_dhost, ns->sc_addr,
                    sizeof(eh->ether_dhost)) != 0 &&
#ifdef MULTICAST
		    /* non-multicast (also non-broadcast) */
		    !ETHER_IS_MULTICAST(eh->ether_dhost)
#else
                    bcmp(eh->ether_dhost, etherbroadcastaddr, 
			 sizeof(eh->ether_dhost)) != 0
#endif
		    )
			return;
        }
#endif

#if 0
	/*
	 * Promiscuous multicast hack will cause us to receive
	 * packets we don't want.  Be paranoid and always filter.
	 */
	if (!ETHER_IS_MULTICAST(eh->ether_dhost) &&
	    bcmp(eh->ether_dhost, ns->sc_addr, sizeof(eh->ether_dhost)) != 0)
		return;
#endif
	/*
	 * Pull packet off interface.  Off is nonzero if packet
	 * has trailing header; neget will then force this header
	 * information to be at the front, but we still have to drop
	 * the type and length which are at the front of any trailer data.
	 */
	m = neget(buf, len, off, &ns->ns_if);
	if (m == 0)
		return;

	ether_input(&ns->ns_if, eh, m);
}

/*
 * Supporting routines
 */

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
neget(buf, totlen, off0, ifp)
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
neioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	register struct ifaddr *ifa = (struct ifaddr *)data;
	struct ne_softc *ns = (struct ne_softc *) necd.cd_devs[ifp->if_unit];
	register nec = ns->ns_base;
	int s;
	int error = 0;

	s = splimp();
	switch (cmd) {

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			neinit(ifp->if_unit);	/* before arpwhohas */
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
				ina->x_host = *(union ns_host *)(ns->sc_addr);
			else {
				/* 
				 * The manual says we can't change the address 
				 * while the receiver is armed,
				 * so reset everything
				 */
				ifp->if_flags &= ~IFF_RUNNING; 
				bcopy((caddr_t)ina->x_host.c_host,
				    (caddr_t)ns->sc_addr, sizeof(ns->sc_addr));
			}
			neinit(ifp->if_unit); /* does ne_setaddr() */
			break;
		    }
#endif
		default:
			neinit(ifp->if_unit);
			break;
		}
		break;

	case SIOCSIFFLAGS:
		if ((ifp->if_flags & IFF_UP) == 0 &&
		    ifp->if_flags & IFF_RUNNING) {
			ifp->if_flags &= ~(IFF_RUNNING|IFF_OACTIVE);
			outb(nec+ds_cmd, DSCM_STOP|DSCM_NODMA);
		} else
			neinit(ifp->if_unit);
		break;

#ifdef MULTICAST
	/*
	 * Update our multicast list.
	 */
	case SIOCADDMULTI:
		error = ether_addmulti((struct ifreq *)data, &ns->ns_ac);
		goto reset;

	case SIOCDELMULTI:
		error = ether_delmulti((struct ifreq *)data, &ns->ns_ac);
	reset:
		if (error == ENETRESET) {
			neinit(ifp->if_unit);
			error = 0;
		}
		break;
#endif
#ifdef notdef
	case SIOCGHWADDR:
		bcopy((caddr_t)ns->sc_addr, (caddr_t) &ifr->ifr_data,
			sizeof(ns->sc_addr));
		break;
#endif

	default:
		error = EINVAL;
	}
	ns->ns_ifoldflags = ifp->if_flags;
	splx(s);
	return (error);
}
