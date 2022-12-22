/*	BSDI $Id: if_hpplus.c,v 1.3 1994/02/02 23:13:14 karels Exp $	*/

/*-
 * Copyright (c) 1993 The Regents of the University of California.
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
 *	California, Berkeley and the University of California, San Diego
 *	and its contributors.
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
 */

/*
 * Driver for HP EtherTwist PC LAN Adapter/16 Plus
 *
 *	Models:
 *		HP 27247B PC LAN Adapter/16 TP Plus [AUI/UTP]
 *		HP 27252A PC LAN Adapter/16 TL Plus [AUI/BNC]
 *		
 * Based on the hppclanp Clarkson packet driver
 *	[R. Nelson, Clarkson University]
 * and the ne1000 BSD/386 driver
 *	[W. Jolitz, UC Berkeley]
 *
 * Written for use in BSD/386 by Kevin Fall (kfall@cs.ucsd.edu)
 * University of California, San Diego
 * December, 1993
 *
 * $Log: if_hpplus.c,v $
 * Revision 1.3  1994/02/02  23:13:14  karels
 * fix NS bug
 *
 * Revision 1.2  1994/01/30  05:27:05  karels
 * remove unneeded/wrong Nxxx dependencies
 *
 * Revision 1.1  1994/01/29  05:22:31  karels
 * driver for HP EtherTwist cards from Kevin Fall, kfall@cs.ucsd.edu
 *
 * Revision 1.5  1994/01/22  20:56:46  kfall
 * Add support for BPF and proper handling of PROMISC mode
 * (including ifoldflags field)
 *
 * Revision 1.4  1994/01/22  18:15:59  kfall
 * Added $Log: if_hpplus.c,v $
 * Revision 1.3  1994/02/02  23:13:14  karels
 * fix NS bug
 *
 * Revision 1.2  1994/01/30  05:27:05  karels
 * remove unneeded/wrong Nxxx dependencies
 *
 * Revision 1.1  1994/01/29  05:22:31  karels
 * driver for HP EtherTwist cards from Kevin Fall, kfall@cs.ucsd.edu
 *
 * Revision 1.5  1994/01/22  20:56:46  kfall
 * Add support for BPF and proper handling of PROMISC mode
 * (including ifoldflags field)
 * entry to header
 *
 */

/* #define	HPP_DEBUG */

#include "bpfilter.h"

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
#include "ic/ds8390.h"

#if NBPFILTER > 0
#include "../net/bpf.h"
#include "../net/bpfdesc.h"
#endif

#include "if_hpplusreg.h"

#define ETHER_MIN_LEN 64
#define ETHER_MAX_LEN 1536

/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * ns_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 */
struct hpp_softc {
	struct device hpp_dev;
	struct isadev hpp_id;
	struct intrhand hpp_ih;

	struct arpcom ns_ac;		/* Ethernet common part */
#define	hpp_if ns_ac.ac_if		/* network-visible interface */
#define	hpp_addr ns_ac.ac_enaddr	/* hardware Ethernet address */

	u_short hpp_base;	/* base I/O port address */
	u_short hpp_irq;	/* interrupt vector */
	caddr_t hpp_maddr;	/* base of mapped memory from cfg */
	u_int hpp_msize;	/* size of mapped memory */
	caddr_t hpp_vaddr;	/* virtual location of mapped memory */

	u_int hpp_cur;		/* current page being filled */
	u_char hpp_cflags;	/* configuration flags from board */
	u_char hpp_rev;		/* revision ID from board */
	u_char hpp_smn;		/* software model # from board */
	u_char hpp_aui;		/* aui indication bit, see header file */
	struct prhdr hpp_ph;	/* received packet descriptor (see ds8390.h) */
	short hpp_ifoldflags;	/* old IF flags (detect PROMISC on) */
#if NBPFILTER > 0
	caddr_t ns_bpf;
#endif
};

/* table of acceptable IRQ's, used if IRQ not specified in config file */
static int hpp_irqs[] = {
	IRQUNK, IRQUNK, IRQUNK,
	IRQ3,
	IRQ4,
	IRQ5,
	IRQ6,
	IRQ7,
	IRQUNK,
	IRQ9,
	IRQ10,
	IRQ11,
	IRQ12,
	IRQUNK,
	IRQUNK,
	IRQ15
};


/*
 * Prototypes -- first public, then private
 */

int  hppprobe __P((struct device *, struct cfdata *, void *));
void hppattach __P((struct device *, struct device *, void *));
int  hppinit __P((int));
int  hppstart __P((struct ifnet *));
int  hppintr __P((void *));
int  hppioctl __P((struct ifnet *, int, caddr_t));
int  hppreset __P((int, int));

static void hppstop __P((u_int));
static void hpprint __P((struct hpp_softc *));
static void hpprecv __P((struct hpp_softc *, u_int));
static int  hpp_putchain __P((struct hpp_softc *, struct mbuf *));
static struct mbuf *hppget __P((u_int, int, struct hpp_softc *, struct ether_header **));
static void hpp_memin __P((struct hpp_softc *, u_int, caddr_t, int));
static void hpp_memout __P((struct hpp_softc *, u_int, caddr_t, int));
static void reset_board __P((u_int));
static void terminate_board __P((u_int));

#ifdef HPP_DEBUG
static void dumpbuf __P((u_char *, int));
#endif

/*
 * driver autoconfig structure
 */
struct cfdriver hppcd = 
	{ NULL, HPP_DEVNAME, hppprobe, hppattach, sizeof(struct hpp_softc) };

/*
 * HP PC Lan+ probe routine:
 *	get base address from config file, check it
 *	verify existence of board at base address
 *	establish IRQ settings
 *	establish memory on card (2K "window" used)
 *	[board actually has 32KB buffering]
 */

int
hppprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct isa_attach_args	*ia = (struct isa_attach_args *)aux;
	u_short			base = ia->ia_iobase;
	caddr_t			lowaddr;
	int			irq;

	/*
	 * Check configuration parameters
	 */
	if (!HPP_IOBASEVALID(base)) {
		printf("%s%d: config failure: invalid I/O base address 0x%x\n",
			HPP_DEVNAME, cf->cf_unit, base);
		return(0);
	}

	/* search for magic HP signature */
	if ((inb(base + HPP_ID + 0) != HPP_IDBYTE0) ||
	    (inb(base + HPP_ID + 1) != HPP_IDBYTE1) ||
	   ((inb(base + HPP_ID + 2) & HPP_PAGEMASK) != HPP_IDBYTE2) ||
	    (inb(base + HPP_ID + 3) != HPP_IDBYTE3)) {
    		/* board not found , be silent */
    		return(0);
	}

	/*
	 * if there's no IRQ given in the kernel configuration
	 * file, we'll try and get it from the board.
	 *
	 * if there is one given, then just make sure the
	 * board agrees
	 */

	if (ia->ia_irq != IRQUNK) {

		/* IRQ was set in config file... is it valid? */

		int board_irq;

		irq = ffs(ia->ia_irq) - 1;
		if (!HPP_IRQVALID(irq)) {
			printf("%s%d: config failure: illegal IRQ %d in kernel config file\n",
				HPP_DEVNAME, cf->cf_unit, irq);
			return(0);
		}
		outw(base + HPP_PAGING, HPP_PAGE_HW);
		board_irq = inb(base + HPP_PAGE5);
		if (board_irq != irq) {
			printf("%s%d: config failure: IRQ %d from config file doesn't match IRQ %d from board\n",
				HPP_DEVNAME, cf->cf_unit, irq, board_irq);
			return(0);
		}

	} else {
		/* IRQ not set in config file, get from board */
		outw(base + HPP_PAGING, HPP_PAGE_HW);
		irq = inb(base + HPP_PAGE5);
		ia->ia_irq = 0;
		if ((irq >= 0) && (irq < 16)) {
			if (hpp_irqs[irq] != IRQUNK)
				ia->ia_irq = isa_irqalloc(hpp_irqs[irq]);
		}

		if ((ia->ia_irq == 0) || ((ffs(ia->ia_irq)-1) == 0)) {
			printf("%s%d: config failure: illegal IRQ %d on board\n",
				HPP_DEVNAME, cf->cf_unit, irq);
			return(0);
		}

		/* will always be valid due to array above */
	}

	if (ia->ia_msize != HPP_MEM_SIZE) {
		printf("%s%d: config failure: illegal memory size in config file %d, should be %d\n",
			HPP_DEVNAME, cf->cf_unit, ia->ia_msize, HPP_MEM_SIZE);
		return(0);
	}

	if (!HPP_MEMVALID((u_int)ia->ia_maddr)) {
		printf("%s%d: config failure: illegal memory start address 0x%x\n",
			HPP_DEVNAME, cf->cf_unit, ia->ia_maddr);
		return(0);
	}

	if (!HPP_MEMALIGNED((u_int)ia->ia_maddr)) {
		printf("%s%d: config failure: illegal memory alignment, address 0x%x\n",
			HPP_DEVNAME, cf->cf_unit, ia->ia_maddr);
		return(0);
	}

	/* make sure board isn't in I/O mapped mode */

	outw(base + HPP_PAGING, HPP_PAGE_PERF);
	if (!(inb(base + HPP_OPTION) & HPP_OPT_MEM_ON)) {
		printf("%s%d: config failure: board has not been set into memory-mapped mode\n",
			HPP_DEVNAME, cf->cf_unit);
		printf("%s%d: run hplanset utility\n",
			HPP_DEVNAME, cf->cf_unit);
		return(0);
	}

	/* low maddr from board */
	outw(base + HPP_PAGING, HPP_PAGE_HW);	/* set hardware pg */
	lowaddr = (caddr_t)(inw(base + HPP_PAGE1) << 4);

	if (((u_int)lowaddr << 4) != (u_int)ia->ia_maddr) {
		printf("%s%d: config failure: board shmem addr (0x%x) doesn't match config file address (0x%x)\n",
			HPP_DEVNAME, cf->cf_unit, ((u_int)lowaddr << 4), ia->ia_maddr);
		printf("%s%d: run hplanset utility\n",
			HPP_DEVNAME, cf->cf_unit);
		return(0);
	}

	ia->ia_iosize = HPP_NPORTS;
	return(1);

}

/*
 * Attach Routine
 *
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.  We get the ethernet address here.
 */
void
hppattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct hpp_softc	*sc  = (struct hpp_softc *) self;
	struct isa_attach_args	*ia  = (struct isa_attach_args *)aux;
	register struct ifnet	*ifp = &sc->hpp_if;
	u_short			base = ia->ia_iobase;
	int s, i;
	u_short tmp;

	/* set up softc structure */

	sc->hpp_base  = base ;
	sc->hpp_irq   = ffs(ia->ia_irq) - 1;
	sc->hpp_maddr = ia->ia_maddr;
	sc->hpp_msize = ia->ia_msize;
	sc->hpp_vaddr = ISA_HOLE_VADDR(ia->ia_maddr);

	/* Save ether address to softc structure */
	outb(base + HPP_PAGING, HPP_PAGE_MAC);	/* set correct page */
	for (i = 0; i < 6; i++)
		sc->hpp_addr[i] = inb(base + HPP_PAGE0 + i);

	/* examine software model number and config flags */
	/* will put it into the controller at init time */
	outw(base + HPP_PAGING, HPP_PAGE_ID);
	tmp = inw(base + HPP_PAGE4);
	sc->hpp_cflags = (tmp & 0xff);	/* low byte only */
	tmp >>= 8;			/* now want high byte only */
	sc->hpp_smn = tmp & ~(0x03);	/* 6 bits of model number */
	sc->hpp_rev = tmp & 0x03;	/* 2 bits of revision */

	/* detect from the config flags if this is twisted pair or thinlan */
	if ((sc->hpp_cflags & HPP_AUI_TL) != 0)
		sc->hpp_aui = HPP_AUI_TL;
	else
		sc->hpp_aui = HPP_AUI_TP;

	/* initialize the interface structures */
	ifp->if_unit = sc->hpp_dev.dv_unit;
	ifp->if_name = hppcd.cd_name;
	ifp->if_mtu = ETHERMTU;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS;
	sc->hpp_ifoldflags = ifp->if_flags;
	ifp->if_init = hppinit;
	ifp->if_output = ether_output;
	ifp->if_start = hppstart;
	ifp->if_ioctl = hppioctl;
	ifp->if_reset = hppreset;
#if 0
	ifp->if_watchdog = (int (*)()) 0;
#endif
	if_attach(ifp);

#if NBPFILTER > 0
	bpfattach (&sc->ns_bpf, ifp, DLT_EN10MB,
		sizeof(struct ether_header));
#endif

	isa_establish(&sc->hpp_id, &sc->hpp_dev);
	sc->hpp_ih.ih_fun = hppintr;
	sc->hpp_ih.ih_arg = (void *)sc;
	intr_establish(ia->ia_irq, &sc->hpp_ih, DV_NET);

	/* print informational message */
	printf("\n%s%d: %s [%d,%d] address %s\n", HPP_DEVNAME, ifp->if_unit,
		((sc->hpp_aui == HPP_AUI_TP) ?
			"HP 27247B PC LAN TP+" :
			"HP 27252A PC LAN TL+"),
		sc->hpp_smn, sc->hpp_rev, ether_sprintf(sc->hpp_addr));

	return;
}

/*
 * Reset of interface.
 *
 * Not called in the normal course of things
 */
int
hppreset(unit, uban)
	int unit;
	int uban;
{
	register struct hpp_softc *sc = hppcd.cd_devs[unit];

	printf("%s%d: reset\n", HPP_DEVNAME, unit);

	/* calls to reset_board are done by hppinit */
	hppinit(unit);
	return(0);
}

/*
 * Initialization of interface
 *
 * bring us up from just about nowhere
 */
int
hppinit(unit)
	int unit;
{
	struct hpp_softc	*sc = hppcd.cd_devs[unit];
	struct ifnet		*ifp = &sc->hpp_if;
	u_short			base = sc->hpp_base;
	u_short			cmd, tmp;
	int s, i;

	/* address not known */
	if (ifp->if_addrlist == (struct ifaddr *) 0) {
		return(0);
	}

	ifp->if_flags &= ~(IFF_RUNNING); /* not running now */
	/* quiet everything down */
	terminate_board(base);
	reset_board(base);

	s = splimp();

	/*
	 * turn the board on
	 *
	 * turn both memory bits on, when we actually do a memory
	 * operation (hpp_mem...) we just write a zero to the MMAP_DIS bit
	 * also turn on interrupt enables, and the reset bits
	 */
	cmd = HPP_OPT_MEM_ON | HPP_OPT_MMAP_DIS |
		HPP_OPT_ZWAIT_ON | HPP_OPT_INEA | HPP_OPT_HWRST |
		HPP_OPT_NICRST;

	outw(base + HPP_OPTION, cmd);

	/* write memory-mapped page info to page 6 */
	/* this represents the receive buffers */
	/* WHAT's THIS REALLY DO? */
	outw(base + HPP_PAGING, HPP_PAGE_HW);
	/* looks like: high byte->stop_pg-1, low byte->start page */
	outw(base + HPP_PAGE6,
		HPP_RCV_START_PG + ((HPP_RCV_STOP_PG - 1) << 8));

	/* set to LAN control page , set the AUI thing */
	outw(base + HPP_PAGING, HPP_PAGE_LAN);
	tmp = inw(base + HPP_PAGE0);
	tmp |= sc->hpp_aui;	/* turn on aui select bit */
	outw(base + HPP_PAGE0, tmp);
	DELAY(150000);	/* delay 150ms, per packet driver */

	/* set back to the expected page */
	outw(base + HPP_PAGING, HPP_PAGE_PERF);

	/*
	 * the following sub-part is DS8390, and mimics
	 * what is to be found in if_wel.c:ds_chipinit()
	 */

	/* set command stop, page 0 */
	outb(base + HPP_ENBASE + ds_cmd,
		DSCM_STOP | DSCM_NODMA | DSCM_PG0);

	/* data conguration: set burst mode, fifo threshold select */
	outb(base + HPP_ENBASE + ds0_dcr, DSDC_FT1 | DSDC_BMS);

	/* clear remote byte count registers */
	outb(base + HPP_ENBASE + ds0_rbcr0, 0); /* low byte */
	outb(base + HPP_ENBASE + ds0_rbcr1, 0); /* high byte */

	/* set monitor mode on receiver (no packets) */
	outb(base + HPP_ENBASE + ds0_rcr, DSRC_MON);

	/* this line from if_we driver encoded loopback */
	outb(base + HPP_ENBASE + ds0_tcr, DSTC_LB0);

	/* set up boundary ptr in chip */
	outb(base + HPP_ENBASE + ds0_bnry, HPP_RCV_START_PG);
	/* set Transmit Page */
	outb(base + HPP_ENBASE + ds0_tpsr, HPP_TX_START_PG);
	/* set up start and stop receive pages */
	outb(base + HPP_ENBASE + ds0_pstart, HPP_RCV_START_PG);
	outb(base + HPP_ENBASE + ds0_pstop, HPP_RCV_STOP_PG);

	/* clear any pending interrupts (write all-1's to isr) */
	outb(base + HPP_ENBASE + ds0_isr, 0xff);

	/* enable all interupts */
	outb(base + HPP_ENBASE + ds0_imr, 0xff);

	/* program command stop, page 1 */
	outb(base + HPP_ENBASE + ds_cmd,
		DSCM_STOP | DSCM_NODMA | DSCM_PG1);

	/* set ethernet (physical) address into 8390 */
	for (i = 0; i < 6; i++)
		outb(base + HPP_ENBASE + ds1_par0 + i, sc->hpp_addr[i]);

	/* clear logical address (multicast) has filter for now */
	/* this will accept all multicasts */
	for (i = 0; i < 8; i++)
		outb(base + HPP_ENBASE + ds1_mar0 + i, 0xff);

	/* start current point in recv ring at beginning */
	outb(base + HPP_ENBASE + ds1_curr, HPP_RCV_START_PG);

	/* program command register start, page 0 */
	outb(base + HPP_ENBASE + ds_cmd,
		DSCM_START | DSCM_NODMA | DSCM_PG0);

	outb(base + HPP_ENBASE + ds0_isr, 0xff); /* clear int status */
	outb(base + HPP_ENBASE + ds0_tcr, 0x00); /* xmit config */

	/*
	 * if we're in promisc. mode we accept:
	 *	broadcast, multicast, and everything else
	 * otherwise we accept
	 *	broadcast
	 */
	if (ifp->if_flags & IFF_PROMISC) {
		outb(base + HPP_ENBASE + ds0_rcr,
			DSRC_AB | DSRC_AM | DSRC_PRO |
			DSRC_AR | DSRC_SEP);
	} else {

		outb(base + HPP_ENBASE + ds0_rcr, DSRC_AB);
	}


	sc->hpp_cur = HPP_RCV_START_PG; 
		/* force NOTRAILERS */
	sc->hpp_if.if_flags |= (IFF_RUNNING | IFF_NOTRAILERS);
	sc->hpp_if.if_flags &= ~(IFF_OACTIVE);


	/*
	 * now check out the shared buffers.
	 */
	{
		u_char buf[DS_PGSIZE];
		u_char cbuf[DS_PGSIZE];
		register i;
		register u_int bufnum;

		for (i = 0; i < sizeof(buf); i++)
			buf[i] = (u_char) 0xee;

		for (bufnum = 0; bufnum < HPP_RCV_STOP_PG; bufnum++) {
			*buf = bufnum;	/* put pg# as first byte */
			hpp_memout(sc, (bufnum << 8), (caddr_t)buf, DS_PGSIZE);
			bzero(cbuf, sizeof(cbuf));
			hpp_memin(sc, (bufnum << 8), (caddr_t)cbuf, DS_PGSIZE);
			if (bcmp(cbuf+1, buf+1, sizeof(buf) - 1) != 0) {
				printf("%s%d: trouble with shared buffers!\n",
					HPP_DEVNAME, sc->hpp_if.if_unit);
				printf("%s%d: continuing anyhow..\n",
					HPP_DEVNAME, sc->hpp_if.if_unit);
			}
		}
	}

	/* kick transmitter to send anything in intf send queue */
	hppstart(ifp);
	splx(s);

	return(0);
}

/*
 * Start Routine
 *
 * Setup output on interface.
 * Get another datagram to send off of the interface queue,
 * and map it to the interface before starting the output.
 *
 * called only at splnet or interrupt level, during 'ifconfig up',
 * attach time, or once an xmit interrupt has gone off
 *
 */
int
hppstart(ifp)
	struct ifnet *ifp;
{
	struct hpp_softc	*sc = hppcd.cd_devs[ifp->if_unit];
	u_short			base = sc->hpp_base;
	struct mbuf		*m;
	u_short			tmp;
	int			totlen = 0;
	u_char			cmd;

	/* if interface isn't running or is busy, forget it */
	if ((sc->hpp_if.if_flags & IFF_RUNNING) == 0) {
#ifdef HPP_DEBUG
printf("hppstart: aborting (IFF_RUNNING not on)\n");
#endif
		return(0);
	}

	/* set back to the expected page */
	outw(base + HPP_PAGING, HPP_PAGE_PERF);

	/* detect if transmitter is running */
	if ((cmd = inb(base + HPP_ENBASE + ds_cmd)) & DSCM_TRANS) {
#ifdef HPP_DEBUG
printf("hppstart: xmitter already running\n");
#endif
		return(0);
	}

#ifdef HPP_DEBUG
printf("hppstart: chip cmd reg: 0x%x\n", cmd);
printf("hppstart: isr = 0x%x\n", inb(base + HPP_ENBASE + ds0_isr));
printf("hppstart: tsr = 0x%x\n", inb(base + HPP_ENBASE + ds0_tsr));
printf("hppstart: rsr = 0x%x\n", inb(base + HPP_ENBASE + ds0_rsr));
printf("hppstart: rcvalctr = 0x%x\n", inb(base + HPP_ENBASE + ds0_rcvalctr));
#endif

	/* get packet from send queue */
	IF_DEQUEUE (&sc->hpp_if.if_snd, m);
	if (m == (struct mbuf *) 0) {
#ifdef HPP_DEBUG
printf("hppstart: EMPTY interface send queue\n");
#endif
		return(0);
	}
	sc->hpp_if.if_flags |= IFF_OACTIVE;


#if NBPFILTER > 0
	/*
	 * give bpf a looksie
	 */

	if (sc->ns_bpf)
		bpf_mtap(sc->ns_bpf, m);
#endif

	/* copy mbuf chain to interface buffers */
	/* will always return at least ETHERMIN+ether_hdr size */
	totlen = hpp_putchain(sc, m);

 	/* save what's in cmd register */
 	cmd = inb(base + HPP_ENBASE + ds_cmd);
 	/* page 0, can't set STOP here or it will not work :( */
 	outb(base + HPP_ENBASE + ds_cmd,
 		DSCM_START | DSCM_NODMA | DSCM_PG0);

 	/* set packet length hi and low bytes */
 	outb(base + HPP_ENBASE + ds0_tbcr0, (totlen & 0xff));	/* lo */
 	outb(base + HPP_ENBASE + ds0_tbcr1, ((totlen >> 8) & 0xff)); /* hi */

 	/* send it */
 	cmd |= (DSCM_TRANS|DSCM_START);
 	cmd &= ~DSCM_STOP;
 	outb(base + HPP_ENBASE + ds_cmd, cmd);

	return(0);
}

/*
 * Controller interrupt handler.
 */

int
hppintr(arg)
	void *arg;	/* ptr to softc structure */
{
	struct hpp_softc	*sc = (struct hpp_softc *)arg;
	struct ifnet		*ifp = &sc->hpp_if;
	u_short			base = sc->hpp_base;
	volatile u_char		intr;

	/* set to zero page */
	outb(base + HPP_ENBASE + ds_cmd,
		DSCM_START | DSCM_NODMA | DSCM_PG0);
	/* get interrupt type */
	intr = inb(base + HPP_ENBASE + ds0_isr);
	/* disable interupts for now */
	outb(base + HPP_ENBASE + ds0_imr, 0x00);

	/* main interrupt processing loop */
	/* do it as long as the isr is nonzero */

#ifdef HPP_DEBUG
printf("!!hppintr!!, intr:%x, opt:0x%x \n", intr,
inw(base + HPP_OPTION));
#endif

	do {
		/* ack's everything */
		outb(base + HPP_ENBASE + ds0_isr, intr);

		/* some kind of receive situation */
		if (intr & (DSIS_RX|DSIS_RXE)) {
			if (intr & DSIS_RXE) {
				(void) inb(base + HPP_ENBASE + ds0_rcvalctr);
				(void) inb(base + HPP_ENBASE + ds0_rcvcrcctr);
				(void) inb(base + HPP_ENBASE + ds0_rcvfrmctr);
				sc->hpp_if.if_ierrors++;
#ifdef HPP_DEBUG
printf("hpp: rxerror, intr = 0x%x\n", intr);
#endif
			}
			hpprint(sc);	/* packet received */
		}

		/* some kind of xmit situation */
		if (intr & (DSIS_TX|DSIS_TXE)) {
			if (intr & DSIS_TXE) {
				sc->hpp_if.if_oerrors++;
#ifdef HPP_DEBUG
printf("hpp: txerror, intr = 0x%x\n", intr);
#endif
			}
			/* ok xmit if we get here */
			sc->hpp_if.if_opackets++;
			sc->hpp_if.if_collisions +=
				inb(base + HPP_ENBASE + ds0_tbcr0);
			sc->hpp_if.if_flags &= ~IFF_OACTIVE;
			hppstart(&sc->hpp_if);
		}

		/*  Receiver overrun in the ring */
		if (intr & DSIS_ROVRN) {
#ifdef HPP_DEBUG
printf("hpp: rxoverrun, intr = 0x%x\n", intr);
#endif
		}

		if (intr & DSIS_RESET) {
#ifdef HPP_DEBUG
printf("hpp: reset_complete, intr = 0x%x\n", intr);
#endif
			/*  Reset Complete */
		}

		/*  Diagnostic counters need attn */
		if (intr & DSIS_CTRS) {
#ifdef HPP_DEBUG
printf("hpp: diag ctrs require attn, intr = 0x%x\n", intr);
#endif
		}

		if (intr & DSIS_RDC) {
			/* remote DMA complete */
			/* we're not using it, so this shouldn't happen */
			log(LOG_WARNING, "%s%d: RDC interrupt unexpected\n",
				HPP_DEVNAME, ifp->if_unit);
		}

		/* ALL DONE w/Interrupts, try again while here */

		/* set to zero page */
		outb(base + HPP_ENBASE + ds_cmd,
			DSCM_START | DSCM_NODMA | DSCM_PG0);

		/* re-enable interrupts */
		outb(base + HPP_ENBASE + ds0_imr, 0xff);
	} while (intr = inb(base + HPP_ENBASE + ds0_isr));
	return (1);
}

/*
 * hpprecv
 *
 * called by lower layer function hpprint()
 * the packet descriptor (from 8390) will be in sc->hpp_ph
 * hpprecv encompasses what would ordinarily be
 * the recv and read routines in other drivers
 */
static void
hpprecv(sc, bufnum)
	register struct hpp_softc *sc;
	u_int bufnum;
{
	register u_int base = sc->hpp_base;
	struct ether_header *eh;
	struct mbuf *m;
	int len;

	sc->hpp_if.if_ipackets++;
	len = sc->hpp_ph.pr_sz0 + (sc->hpp_ph.pr_sz1 << 8);

	if (len < ETHER_MIN_LEN || len > ETHER_MAX_LEN) {
		return;
	}

	/* hppget will set up the eh pointer */
	m = hppget(bufnum, len, sc, &eh);
	if (m == (struct mbuf *) 0)
		return;

	/* this byte swap must happen before ether_input */
	eh->ether_type = ntohs((u_short)eh->ether_type);

	/* detect trailers and die if it's there */
	if (eh->ether_type >= ETHERTYPE_TRAIL &&
		eh->ether_type < ETHERTYPE_TRAIL + ETHERTYPE_NTRAILER) {
		m_freem(m);
		return;
	}

	/* give up to the ethernet generic layer */
	ether_input(&sc->hpp_if, eh, m);

	return;
}

/*
 * hppget - get packet from board
 *
 * bufnum: page# to find packet data in
 * totlen: total length including 4-byte descriptor and ether_header
 * sc: ptr to softc structure
 * ec: filled in to point to ether_header
 *	(should end up in 1st mbuf
 *
 * Doesn't handle trailer protocols at all..
 */

static struct mbuf *
hppget(bufnum, packet_len, sc, ec)
	u_int bufnum;
	int packet_len;
	struct hpp_softc *sc;
	struct ether_header **ec;
{
	struct mbuf *m, *top, **mp = &top;
	u_int base = sc->hpp_base;
	u_int cp, tmp;
	int len;
	int totlen = packet_len;

	bufnum <<= 8;	/* get page to high byte, offset 0 */
	cp = bufnum;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return (0);

	m->m_pkthdr.rcvif = &sc->hpp_if;
	m->m_len = MHLEN; /* len of mbuf with a header inside */

#ifdef HPP_DEBUG
printf("hppget: packet_len = %d\n", packet_len);
#endif

	top = (struct mbuf *) NULL;

	while (totlen > 0) {
		if (top != NULL) {
			/* link a new mbuf on the chain */
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m == (struct mbuf *) NULL) {
				m_freem(top);
				return(NULL);
			}
			m->m_len = MLEN; /* normal len (not pkthdr) */
		}

		len = totlen;

		/*
		 * len is now how much to actually fetch
		 * from the board this time around
		 */

		if (len >= MINCLSIZE) {
			/* big enough for a cluster, get one */
			MCLGET(m, M_DONTWAIT);
			if (m->m_flags & M_EXT) {
				/* MCLGET succeeded */
				m->m_len = MCLBYTES;
			}
		}


#ifdef notdef
	 	/* force mbuf address to be longword aligned */
	 	if (tmp = (mtod(m, u_int) & 3)) {
	 		m->m_data += (sizeof(u_long) - tmp);
	 		m->m_len -= (sizeof(u_long) - tmp);
		}
#endif

		/*
		 * m->m_len is how much mbuf room we have
		 * len is how much to grab from board
		 * round len down to longword, but make
		 * sure we don't overwrite the mbuf
		 */

	 	if (len < sizeof(u_long)) {
			hpp_memin(sc, cp, mtod(m, caddr_t), sizeof(u_long));
		} else {
			len &= ~3; /* round down to nearest longword */
			m->m_len &= ~3; /* ditto for mbuf */
			len = min(len, m->m_len);
			hpp_memin(sc, cp, mtod(m, caddr_t), len);
		}
		m->m_len = len;

		cp += len;
		*mp = m;
		mp = &m->m_next;
		totlen -= len;

		/* make us wrap */
		if (cp >= (HPP_RCV_STOP_PG << 8))
			cp = (HPP_RCV_START_PG << 8) +
				(cp - (HPP_RCV_STOP_PG << 8));
	}

	/*
	 * ok, so now we have an mbuf chain, but it contains
	 * a 4-byte packet descriptor (struct prhdr),
	 * an ethernet header, and
	 * the type field of the ether_header may be byte-
	 * reversed.  Deal with the 1st two things here...
	 * the byte-swapped problem is taken care of in
	 * hpprecv
	 */

 	m = top; /* convenience */

 	if (m->m_len >= (sizeof(struct ether_header)
 	    + sizeof(struct prhdr))) {
		struct prhdr *ph = mtod(m, struct prhdr *);
		ph++;
 		*ec = (struct ether_header *) ph;
	} else {
		log(LOG_ERR, "%s%d: no ether header in 1st mbuf!",
			HPP_DEVNAME, sc->hpp_if.if_unit);
		m_freem(m);
		*ec = NULL;
		return(NULL);
	}

#ifdef HPP_DEBUG
printf("hppget: current_len:%d\n", m->m_len);
#endif

	/* chops out ether header + descriptor from front */
	len = sizeof(struct ether_header) + sizeof(struct prhdr);

#if NBPFILTER > 0

	/*
	 * if there's a filter hanging on, make sure to leave
	 * the ethernet header in the beginning, but we must still
	 * trim off the prhdr at the beginning
	 * the filter wants to see the header type field in
	 * network order, so just leave it alone here
	 *
	 * if there isn't one (or NBPFILTER isn't on), then
	 * we will trim off both the prhdr and ether header
	 * below
	 */

	if (sc->ns_bpf) {
		struct ether_header *eh;
		/* trim off prhdr from beginning */
		len = sizeof(struct prhdr);
		if (m->m_len > len) {
			m->m_data += len;
			m->m_len -= len;
		} else
			m_adj(m, len);
		m->m_pkthdr.len = packet_len - sizeof(struct prhdr);
		eh = mtod(top, struct ether_header *);
		bpf_mtap(sc->ns_bpf, top);

		/* don't pass up if not for us */
		if ((sc->hpp_if.if_flags & IFF_PROMISC) &&
		    bcmp(eh->ether_dhost, sc->hpp_addr,
		      sizeof(eh->ether_dhost)) != 0 &&
		    bcmp(eh->ether_dhost, etherbroadcastaddr,
		      sizeof(eh->ether_dhost)) != 0) {
		        m_freem(top);
			return ((struct mbuf *) NULL);
		}
		len = sizeof(struct ether_header);
	}
#endif

 	/* trim off ether+pr if no bpf, trip only ether if bpf */
 	/* perf hack, don't call m_adj if we don't have to */
 	if (m->m_len > len) {
 		m->m_data += len;
 		m->m_len -= len;
	} else
 		m_adj(m, len);

	m->m_pkthdr.len = packet_len - sizeof(struct ether_header)
		- sizeof(struct prhdr);

	return(top);
}

/*
 * Process an ioctl request.
 */
int
hppioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	register struct ifaddr *ifa = (struct ifaddr *) data;
	struct hpp_softc *sc = hppcd.cd_devs[ifp->if_unit];
	struct ifreq *ifr = (struct ifreq *) data;
	int s, error = 0;

	s = splimp();

	switch (cmd) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		switch (ifa->ifa_addr->sa_family) {

#ifdef INET
		case AF_INET:
			hppinit(ifp->if_unit); /* before arpwhohas */

			((struct arpcom *) ifp)->ac_ipaddr =
				IA_SIN (ifa)->sin_addr;
			arpwhohas ((struct arpcom *) ifp,
				&IA_SIN (ifa)->sin_addr);
			break;
#endif

#ifdef NS
		case AF_NS:
		{
			register struct ns_addr *ina =
				&(IA_SNS(ifa)->sns_addr);

			if (ns_nullhost(*ina))
				ina->x_host = *(union ns_host *) (sc->hpp_addr);
			else {
				/*
				 * The manual says we can't change the address
				 * while the receiver is armed,
				 * so reset everything
				 */
				ifp->if_flags &= ~IFF_RUNNING;
				bcopy ((caddr_t) ina->x_host.c_host,
				       (caddr_t) sc->hpp_addr,
				       sizeof(sc->hpp_addr));
			}
			hppinit (ifp->if_unit);	/* does hp_setaddr() */
			break;
		}
#endif
		default:
		  hppinit (ifp->if_unit);
		  break;
		} /* 2nd layer switch */
		break;

	case SIOCSIFFLAGS:

#ifdef HPP_DEBUG
      printf ("hp: setting flags, up: %s, running: %s\n",
	      ifp->if_flags & IFF_UP ? "yes" : "no",
	      ifp->if_flags & IFF_RUNNING ? "yes" : "no");
#endif

	if ((ifp->if_flags & IFF_UP) == 0 &&
	    ifp->if_flags & IFF_RUNNING) {
	    	/* not up, but running... means to shut down */
		ifp->if_flags &= ~IFF_RUNNING;
		hppstop(sc->hpp_base);
	} else if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) == IFF_UP ||
	       ((ifp->if_flags ^ sc->hpp_ifoldflags) & IFF_PROMISC))
		hppinit (ifp->if_unit);
	sc->hpp_ifoldflags = ifp->if_flags;
	break;

#ifdef notdef
    case SIOCGHWADDR:
      bcopy ((caddr_t) sc->hpp_addr, (caddr_t) & ifr->ifr_data,
	     sizeof(sc->hpp_addr));
      break;
#endif

	default:
		error = EINVAL;
	}

	splx (s);
	return(error);
}

/*
 * reset_board--
 *	consists of telling the hp control registers to
 *	halt, plus letting the 8390 part know, too
 */
static void
reset_board(base)
	u_int	base;
{
	int s;
	u_short tmp;

	s = splimp();

	/* reset 8390 */
	tmp = inw(base + HPP_OPTION);
	tmp &= ~(HPP_OPT_HWRST|HPP_OPT_NICRST);

	/* turn off the hardware */
	outw(base + HPP_OPTION, tmp);
	DELAY(150000);	/* 150ms delay */

	/* turn it back on */
	tmp |= (HPP_OPT_HWRST|HPP_OPT_NICRST|HPP_OPT_INEA);
	outb(base + HPP_OPTION, (tmp & 0xff));

	/* now working with constants from ds8390.h */
	/* stop the ds8390 */
	tmp = inb(base + HPP_ENBASE + ds_cmd) & 0xff;
	tmp |= (DSCM_STOP|DSCM_NODMA);
	tmp &= ~(DSCM_START|DSCM_TRANS);
	outb(base + HPP_ENBASE + ds_cmd, tmp);

	DELAY(10000);	/* wait for nic to stop xmit or receive */
	splx(s);
}

static void
terminate_board(base)
	u_int base;
{
	int s = splimp();
	u_short sav = inw(base + HPP_OPTION);

	/* turn on mmap_disable, nic reset, hardware reset */
	sav |= (HPP_OPT_MMAP_DIS|HPP_OPT_NICRST|HPP_OPT_HWRST);
	/* turn off interrupts */
	sav &= ~(HPP_OPT_INEA);

	outw(base + HPP_OPTION, sav);
	splx(s);
}

static void
hppstop(base)
	u_int base;
{
	reset_board(base);
	terminate_board(base);
}

/*
 * hpprint
 *
 * what to do on a received packet interrupt
 * borrowed heavily from the neintr() code in if_ne.c
 */

static void
hpprint(sc)
	register struct hpp_softc *sc;
{
	register u_int base = sc->hpp_base;
	u_char pend, lastfree, nxt;

	/* switch to page 1, get curr */
	outb(base + HPP_ENBASE + ds_cmd,
		DSCM_START|DSCM_NODMA|DSCM_PG1);
	pend = inb(base + HPP_ENBASE + ds1_curr);

	/* switch to page 0, get bnry/lastfree */
	outb(base + HPP_ENBASE + ds_cmd, DSCM_START | DSCM_NODMA | DSCM_PG0);
	lastfree = inb(base + HPP_ENBASE + ds0_bnry);

#ifdef HPP_DEBUG
printf("hpprint start [fromchip]: pend=0x%x,lastfree=0x%x\n",
pend, lastfree);
#endif

	/*
	 * check for receive ring wraparound
	 */

 	if (lastfree >= HPP_RCV_STOP_PG) {
 		/* reset to beginning */
 		lastfree = HPP_RCV_START_PG;
	}

	if (pend < lastfree && sc->hpp_cur < pend)
		lastfree = sc->hpp_cur;
	else if (sc->hpp_cur > lastfree)
		lastfree = sc->hpp_cur;

#ifdef HPP_DEBUG
printf("hpprint start: pend=0x%x,lastfree=0x%x,hpp_cur=0x%x\n",
pend, lastfree, sc->hpp_cur);
#endif

	/*
	 * look for something in the buffer
	 */

	while (pend != lastfree) {

#ifdef HPP_DEBUG
printf("hpprint toploop: pend=0x%x,lastfree=0x%x,hpp_cur=0x%x\n",
pend, lastfree, sc->hpp_cur);
#endif

		/* copy packet descriptor from board */
		hpp_memin(sc, (lastfree << 8),
			(caddr_t)&sc->hpp_ph, sizeof(struct prhdr));

#ifdef HPP_DEBUG
printf("hpprint PRHDR::stat=0x%x,nxtpg=0x%x,pr_sz0=0x%x,pr_sz1=0x%x\n",
sc->hpp_ph.pr_status, sc->hpp_ph.pr_nxtpg, sc->hpp_ph.pr_sz0,
sc->hpp_ph.pr_sz1);
#endif

		/*
		 * there's actually more bits than defined
		 * in ds8390.h, and we need to be aware of
		 * not only DSRS_RPC (ok packet) but
		 * also 0x21.
		 *
		 * here's what the bits mean:
		 *	0x01	ok packet received
		 *	0x02	CRC error
		 *	0x04	frame alignment error
		 *	0x08	FIFO overrun
		 *	0x10	missed packet
		 *	0x20	physical/multicast address enabled
		 *	0x40	receiver disabled (monitor mode)
		 *	0x80	deferring
		 */
		if (sc->hpp_ph.pr_status == 0x21 ||
		    sc->hpp_ph.pr_status == DSRS_RPC) {
	    		hpprecv(sc, lastfree);
		} else {
			log(LOG_ERR, "%s%d: bad pkt recv desc %d stat: 0x%x\n",
				HPP_DEVNAME, sc->hpp_if.if_unit,
				lastfree, sc->hpp_ph.pr_status);
			sc->hpp_if.if_ierrors++;
		}

		nxt = sc->hpp_ph.pr_nxtpg;

		/* sanity check nxt */
		if (nxt >= HPP_RCV_START_PG &&
		    nxt <= HPP_RCV_STOP_PG && nxt <= pend) {

	    		sc->hpp_cur = nxt;
		} else {
			sc->hpp_cur = nxt = pend;
		}

		/* set boundaries */
		lastfree = nxt;

		if (--nxt < HPP_RCV_START_PG)
			nxt = HPP_RCV_STOP_PG - 1;

		/* set page 0 */
		outb(base + HPP_ENBASE + ds_cmd,
			DSCM_START | DSCM_NODMA | DSCM_PG0);
		outb(base + HPP_ENBASE + ds0_bnry, nxt);

		/* set page 1 */
		outb(base + HPP_ENBASE + ds_cmd,
			DSCM_START | DSCM_NODMA | DSCM_PG1);
		pend = inb(base + HPP_ENBASE + ds1_curr);

		/* set page 0 */
		outb(base + HPP_ENBASE + ds_cmd,
			DSCM_START | DSCM_NODMA | DSCM_PG0);
	} /* while (pend != lastfree) */
}

/*
 * hpp_putchain: copy data from mbuf chain into the appropriate
 *	board memory buffer
 *
 * return: the total # of bytes copied to the board
 * side effect: frees mbuf chain
 */

static int
hpp_putchain(sc, m0)
	struct hpp_softc *sc;
	struct mbuf *m0;
{
	struct mbuf *m;
	int len = 0;
	caddr_t vaddr = sc->hpp_vaddr;
	int s, wasodd = 0;
	u_char oddword[sizeof(u_short)];
	u_int baddr;

	baddr = (HPP_TX_START_PG << 8);
	/* copy mbuf chain into interface */
	for (m = m0; m != (struct mbuf *)  0; m = m->m_next) {
		wasodd = 0;
		if (m->m_len == 0)
			continue;

		/* be careful about odd mbufs */
		if (m->m_len & 0x01) {
			wasodd++;
			/* get last byte of this mbuf */
			oddword[0] = *(mtod(m, caddr_t) +
				m->m_len - 1);
			m->m_len--;	/* now it's even */
			/* get 1st byte of next mbuf */
			if (m->m_next) {
				oddword[1] = *(mtod(m->m_next, caddr_t));
				m->m_next->m_data++;
				m->m_next->m_len--;
			} else
				oddword[1] = 0;
		}
		hpp_memout(sc, baddr, mtod(m, caddr_t), m->m_len);
		baddr += m->m_len;
		if (wasodd) {
			hpp_memout(sc, baddr, (caddr_t)oddword, sizeof(oddword));
			len += sizeof(oddword);
			baddr += sizeof(oddword);
		}
		len += m->m_len;
	}

	/* make sure it's long enough for the function return */
	if (len < ETHERMIN + sizeof(struct ether_header))
		len = ETHERMIN + sizeof(struct ether_header);

	m_freem(m0);
	return(len);
}

/*
 * memin: copy data from board to main memory
 *
 *
 *	sc: softc descriptor
 *	baddr: board address (page number in high byte, offset in lo)
 *		[note: board truncates odd addresses!]
 *		[thus must be short-word aligned to work]
 *	maddr: address in host memory of data sink
 *	cnt: byte count
 */

static void
hpp_memin(sc, baddr, maddr, cnt)
	struct hpp_softc *sc;
	u_int baddr;		/* buffer number (in card) */
	caddr_t	maddr;		/* memory address (in host) */
	int cnt;
{
	u_short	tmp, sav;
	u_short base = sc->hpp_base;
	int s;

	if (cnt > sc->hpp_msize) {
		panic("hpp_memin: memin too big");
	}

#ifdef HPP_DEBUG
printf("hpp_memin: copy %d bytes from buffer 0x%x to vaddr 0x%x\n",
cnt, baddr, maddr);
#endif

	/* turn odd to even */
	if (baddr & 0x01) {
		printf("%s%d bad ODD board access!\n",
			HPP_DEVNAME, sc->hpp_if.if_unit);
		baddr++;	/* XXX */
	}

	/* set perf page */
	outw(base + HPP_PAGING, HPP_PAGE_PERF);

	/* set read address in page2 register (read pointer) */
	outw(base + HPP_PAGE2, (baddr & 0xffff));

	/* mem on, bootrom off */
	tmp = sav = inw(base + HPP_OPTION);
	tmp &= ~(HPP_OPT_MMAP_DIS | HPP_OPT_BOOTROM_ON);
	outw(base + HPP_OPTION, tmp);

	s = splimp();

#ifdef HPP_DEBUG
printf("hpp_memin: saved OPTION reg: 0x%x, wrote reg 0x%x\n",
sav, tmp);
#endif

	{
		register i = 0;
		volatile u_long *src, *dest;

		src = (u_long *) sc->hpp_vaddr;
		dest = (u_long *) maddr;

		for (i = 0; i < cnt; i += sizeof(u_long))
			*dest++ = *src; /* src not advanced! */
	}

	/* restore what was in option register */
	outw(base + HPP_OPTION, sav);
	splx(s);
}


/*
 * hpp_memout: copy data from arbitrary buffer
 *	to board memory buffer
 *
 */

static void
hpp_memout(sc, pagenum, maddr, len)
	register struct hpp_softc *sc;
	u_int pagenum;
	volatile caddr_t maddr;
	int len;
{
	u_short	tmp, sav;
	u_short base = sc->hpp_base;
	volatile caddr_t vaddr = sc->hpp_vaddr;

	/* set perf page */
	outw(base + HPP_PAGING, HPP_PAGE_PERF);

	/* set write address in page register */
	outw(base + HPP_PAGE0, pagenum & 0xffff);

	/* mem on, bootrom off */
	tmp = sav = inw(base + HPP_OPTION);
	tmp &= ~(HPP_OPT_MMAP_DIS | HPP_OPT_BOOTROM_ON);
	outw(base + HPP_OPTION, tmp);

	bcopy(maddr, vaddr, len);

#ifdef notdef
	{
		register i = 0;
		volatile u_long *src, *dest;

		src = (u_long *) maddr;
		dest = (u_long *) vaddr;

		for (i = 0; i < len; i += sizeof(u_long))
			*dest = *src++;
	}
#endif


	/* restore what was in option register */
	outw(base + HPP_OPTION, sav);
	return;
}

#ifdef HPP_DEBUG
static void
dumpbuf(buf, len)
	u_char *buf; 
	int len;
{
        int i;

	printf("dump (%d bytes):\n", len);
        printf("--\n");
        for (i = 0; i < len; i++) {
                printf("0x%x ", buf[i]);
                if (((i+1) % 6) == 0)
                        printf("\n");
        }
        printf("\n");
        printf("--\n");
        return;
}
#endif
