/*-
 * Copyright (c) 1992, 1993, 1994 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI $Id: if_we.c,v 1.22 1994/01/30 00:35:17 karels Exp $
 */

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Tim L. Tucker.
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
 *	@(#)if_we.c	7.3 (Berkeley) 5/21/91
 */

/*
 * Generic driver for DS8390 + shared memory-based Ethernet/Starlan adapters.
 * Currently supports Western Digital/SMC WD8003E,  WD8003EBT, WD8003S,
 * WD8003SBT, and WD8013EBT; and 3Com Corp. 3C503 (Etherlink II), SMC Ultra.
 *
 * This code is derived from the Berkeley if_we.c for Western Digital only.
 * Part of 3c503 initialization code is borrowed from the driver by Herb Peyerl
 * (hpeyerl@novatel.cuc.ab.ca) who in turn borrowed it from a Clarkson packet
 * driver.
 *
 * Includes modifications for 16-bit operation from Yuval Yarom.
 *
 * If the shared memory is > 8192 bytes, 2 tx buffers is allocated to be
 * used for double buffering.  The double buffering idea was found in
 * David Greenman's 386bsd ed driver.  Hans Nasten.  (nasten@Everyware.SE).
 *
 * support for SMC ULTRA added by Bill Webb (webb@telebit.com)
 */

/* #define WEDEBUG */

/*
 * Uncomment the following #define to allow automatic
 * IRQ selection on Western Digital cards.
 */
/* #define WDSETIRQ */

#include "bpfilter.h"

#include "param.h"
#include "mbuf.h"
#include "socket.h"
#include "device.h"
#include "ioctl.h"
#include "errno.h"
#include "syslog.h"
#include "systm.h"

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

#include "ic/ds8390.h"
#include "if_wereg.h"
#include "isa.h"
#include "isavar.h"
#include "icu.h"

#include "machine/cpu.h"
#if _BSDI_VERSION >= 199312
#include "machine/inline.h"
#else
#define	ETHER_ADDR_LEN	6
#endif

#if NBPFILTER > 0
#include "net/bpf.h"
#include "net/bpfdesc.h"
#endif

#ifdef WEDEBUG
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
struct	we_softc {
	struct  device we_dev;		/* base device */
	struct  isadev we_id;		/* ISA device */
	struct  intrhand we_ih;		/* interrupt vectoring */

	struct	arpcom we_ac;		/* Ethernet common part */
#define	we_if	we_ac.ac_if		/* network-visible interface */
#define	we_addr	we_ac.ac_enaddr		/* hardware Ethernet address */

	u_char	we_xmit_busy;		/* Tx operation active. */
	u_char	we_max_bufs;		/* Total number of Tx bufs. */
	u_char	we_next_buf;		/* Next buffer to be sent. */
	u_char	we_n_free_bufs;		/* Number of free Tx bufs. */
	u_short	we_next_buf_len;	/* Size of next buffer to be sent. */

	u_char	we_bdry;		/* software BDRY register */
	u_char	we_vmem_off;		/* offset of on-chip memory from 0 (pages) */
	u_char	we_rx_off;		/* receive ring offset (pages) */
	u_char	we_rx_end;		/* end of the ring (pages) */

	u_short	we_type;		/* interface type code */
	u_short	we_base;		/* NIC i/o ports base address */

	caddr_t	we_vmem_addr;		/* card RAM virtual memory base */
	u_long	we_vmem_size;		/* card RAM bytes */
	caddr_t	we_vmem_ring;		/* receive ring RAM vaddress */
	caddr_t	we_vmem_end;		/* receive ring RAM end	*/
	int	we_ifoldflags;		/* old interface flags */
	int	we_lan16;		/* 8390 in 16-bit mode */
	struct	atshutdown we_ats;	/* shutdown routine */
#if NBPFILTER > 0
	caddr_t	we_bpf;			/* packet filter */
#endif
	u_char	we_bpf_bounce[ETHERMTU+100];/* bpf bounce buffer at mem wrap */
};

/*
 * Switch DS8390 register pages
 */
#define PAGE0(base)  outb((base) + ds_cmd, DSCM_NODMA|DSCM_PG0)
#define PAGE1(base)  outb((base) + ds_cmd, DSCM_NODMA|DSCM_PG1)

int	weprobe __P((struct device *, struct cfdata *, void *));
void	weattach __P((struct device *, struct device *, void *));
int	weintr __P((struct we_softc *));
int	wewatchdog __P((int));
int	weinit __P((int));
void	ds_chipinit __P((int, u_char *, int, int, int, struct arpcom *, int));
int	westart __P((struct ifnet *));
int	weioctl __P((struct ifnet *, int, caddr_t));

void	weinitring __P((struct we_softc *, int));
void	weshutdown __P((void *));
void	westop __P((int));
void	werint __P((struct we_softc *));
void	weread __P((struct we_softc *, caddr_t, int));
struct	mbuf *weget __P((caddr_t, int, int, struct we_softc *));
void	ds_getmcaf __P((struct arpcom *, u_long *));

struct cfdriver wecd =
	{ NULL, "we", weprobe, weattach, sizeof(struct we_softc) };

struct we_aux {
	int	cardid;
	int	wd16;
	int	is_790;
} we_aux;

extern int autodebug;
 
/*
 * Probe the WD8003 to see if it's there
 */
weprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	register int i;
	u_char sum;
#ifndef WDSETIRQ
	void weforceintr();
#endif
	register base = ia->ia_iobase;
	caddr_t p, b;
	int irq;

	ia->ia_aux = (void *) &we_aux;

	/*
	 * First, try WD8003
	 */
	if (ia->ia_maddr == 0 || !WD_BASEVALID(base))
		goto try3c503; 
	if (isa_portcheck(base, WD_NPORT) == 0) {
	 	if (autodebug)
			printf("SMC port range overlaps existing, ");
		goto try3c503; 
	}

	/*
	 * First check if a card seems to be present by looking at some
	 * higher ports for 0xff (nothing present), then check for SMC 790
	 * (which may require writing ports).  If a 790 appears to be present,
	 * is_smc_790 will leave the LAN address registers visible.
	 * Then we check the card ROM, if the checksum passes, and the
	 * type code and Ethernet address check out, then we know we have
	 * a wd80x3 or ultra card.
	 */
	if (inb(base + WD_ROMADDR(0x6)) == 0xff &&
	    inb(base + WD_ROMADDR(0x7)) == 0xff)
		goto try3c503;
	we_aux.is_790 = is_smc_790(base);
	if (autodebug && we_aux.is_790)
		printf("card at %x is smc 790\n", base);
	for (sum = 0, i = 0; i < 8; i++)
		sum += inb(base + WD_ROMADDR(i));
	if (sum != WD_CHECKSUM)
		goto try3c503;
	if (we_aux.is_790)
		we_aux.cardid = SMC_ULTRA;
	else
		we_aux.cardid = inb(base + WD_CARDID);

	/*
	 * Check parameters
	 */
#define	KB	*1024
	if (ia->ia_msize != 8 KB && ia->ia_msize != 16 KB &&
	    ia->ia_msize != 32 KB && ia->ia_msize != 64 KB) {
		printf("we%d: illegal memory size %d\n", cf->cf_unit,
		    ia->ia_msize);
		return (0);
	}
	if ((int)(ia->ia_maddr) & 0x1fff) {
		printf("we%d: illegal memory alignment\n", cf->cf_unit);
		return (0);
	}

	/*
	 * Check for large memory (32K/64K),
	 * and test for 16-bit card in 16-bit slot.
	 */
	i = inb(base + WD_ICR);
	if ((i & WDICR_MSZ) == 0 && ia->ia_msize >= 32 KB)
		ia->ia_msize = 16 KB;
	we_aux.wd16 = 0;
	if (we_aux.is_790) {
		if (inb(base+SMC_HWR) & SMCHWR_HOST16)
			we_aux.wd16 = 1;
		dprintf((" %dbit slot\n", we_aux.wd16 ? 16 : 8));
		we_ram_790(base, we_aux.wd16, ia->ia_maddr, ia->ia_msize);
		outb(base+SMC_ICR, SMCICR_EIL);	/* enable interrupts */
	} else {
		/* try to clear WDICR_BIT16 */
		outb(base + WD_ICR, i &~ WDICR_BIT16);
		if (inb(base + WD_ICR) & WDICR_BIT16) {
			dprintf(("16bit slot\n"));
			if (ia->ia_msize > 8 KB) {
				if (autodebug > 1 &&
				    cnquery("use 16-bit access") == 0)
					goto wd8;
				outb(base + WD_LAAR,
				    WDLAAR_M16EN | WDLAAR_L16EN | WD_ADDR19);
				we_aux.wd16 = 1;
			}
		} else {
			outb(base + WD_ICR, i);
		wd8:
			if (ia->ia_msize == 16 KB || ia->ia_msize == 64 KB)
				ia->ia_msize /= 2;
		}

		/*
		 * Map interface memory, setup memory select register
		 */
		outb(base + WD_MSR, WDMSR_MENB |
		    ((((int)(ia->ia_maddr)) >> 13) & WDMSR_ADDR));
	}

	/*
	 * Clear interface memory, then sum to make sure its valid.
	 * (Should probably do 16-bit test if flags indicate 16-bit use.)
	 */
	b = ISA_HOLE_VADDR(ia->ia_maddr);
	for (i = 0, p = b; i < ia->ia_msize; i++, p++)
		*p = 0x0;
	for (i = 0, p = b; i < ia->ia_msize; i++, p++)
	    if (*p != 0) {
	    	if (i >= 16 KB)
	    		ia->ia_msize = 16 KB;
	    	else if (i >= 8 KB) {
	    		ia->ia_msize = 8 KB;
			we_aux.wd16 = 0;
		} else {
			printf("we%d: dual port memory error at 0x%x\n",
			    cf->cf_unit, ia->ia_maddr + i);
			we_disable_ram(base, ia);
			return (0);
		}
		break;
	}

	if (ia->ia_irq == IRQUNK) {
#ifdef WDSETIRQ
		if ((ia->ia_irq = isa_irqalloc(WD_IRQS)) == 0) {
			printf("we%d: no irq available for WD8013\n",
			    cf->cf_unit);
			we_disable_ram(base, ia);
			return (0);
		}
#else
		ia->ia_irq = isa_discoverintr(weforceintr, aux);
		westop(base + WD_NIC_OFFSET);
		if (ffs(ia->ia_irq) - 1 == 0) {
			we_disable_ram(base, ia);
			return (0);
		}
#endif /* WDSETIRQ */
	}

	we_disable_ram(base, ia);
	ia->ia_iosize = WD_NPORT;
	return (1);

	/*
	 * Try to check if it is a 3c503 board
	 */ 
try3c503:
	if (isa_portcheck(base, EC_NPORT) == 0) {
	 	if (autodebug)
			printf("3C503 port range overlaps existing, ");
		goto next_type; 
	}
	switch (base) {
	default:
		goto next_type;
	case 0x300:	i = 0x80; break;
	case 0x310:	i = 0x40; break;
	case 0x330:	i = 0x20; break;
	case 0x350:	i = 0x10; break;
	case 0x250:	i = 0x08; break;
	case 0x280:	i = 0x04; break;
	case 0x2a0:	i = 0x02; break;
	case 0x2e0:	i = 0x01; break;
	}
	if (inb(base+E33G_IOBASE) != i)
		goto next_type;

	outb(base + E33G_STARTPG, 0x5a);
	if (inb(base + E33G_STARTPG) != 0x5a)
		goto next_type;

	/*
	 * Read the memory window location from jumpers
	 * (if not explicitly configured)
	 */
	if (ia->ia_maddr == 0) {
		if (ia->ia_msize == 0)
			ia->ia_msize = EC_MEM_SIZE;
		i = inb(base + E33G_ROMBASE) & EROM_MASK;
		switch (i) {
		case EROM_C8000:
			ia->ia_maddr = (caddr_t) 0xc8000;
			break;
		case EROM_CC000:
			ia->ia_maddr = (caddr_t) 0xcc000;
			break;
		case EROM_D8000:
			ia->ia_maddr = (caddr_t) 0xd8000;
			break;
		case EROM_DC000:
			ia->ia_maddr = (caddr_t) 0xdc000;
			break;
		}
	}

	/*
	 * Check parameters
	 */
	if (ia->ia_msize != EC_MEM_SIZE)
		goto next_type;
	switch ((int) ia->ia_maddr) {
	case 0xc8000:
		i = EROM_C8000;
		break;
	case 0xcc000:
		i = EROM_CC000;
		break;
	case 0xd8000:
		i = EROM_D8000;
		break;
	case 0xdc000:
		i = EROM_DC000;
		break;
	default:
		goto next_type;
	}
	if ((inb(base + E33G_ROMBASE) & EROM_MASK) != i)
		printf("we%d: configured iomem %x doesn't match 3C503 jumpers\n",
		    cf->cf_unit, ia->ia_maddr);

	/* Select RAM */
	outb(base + E33G_GACFR, EGACFR_IRQOFF);

	/*
	 * Clear interface memory, then sum to make sure it is valid
	 */
	b = ISA_HOLE_VADDR(ia->ia_maddr);
	for (i = 0, p = b; i < ia->ia_msize; i++, p++)
		*p = 0x0;
	for (sum = 0, i = 0, p = b; i < ia->ia_msize; i++, p++)
		sum += *p;
	if (sum != 0) {
		printf("we%d: 3C503 dual port RAM error\n", cf->cf_unit);
		return (0);
	}

	if (ia->ia_irq == IRQUNK) {
		if ((ia->ia_irq = isa_irqalloc(EC_IRQS)) == 0) {
			printf("we%d: no irq available for 3C503\n",
			    cf->cf_unit);
			return (0);
		}
	} 
	irq = ffs(ia->ia_irq) - 1;
	if (!EC_IRQVALID(irq)) {
		printf("we%d: illegal IRQ %d for 3C503\n", cf->cf_unit, irq);
		return (0);
	}
	we_aux.cardid = WD_3C503;
	ia->ia_iosize = EC_NPORT;
	return (1);

	/*
	 * Add the new DS8390 + shared-memory card types here
	 */
next_type:
	return (0);
}

we_disable_ram(base, ia)
	int base;
	struct isa_attach_args *ia;
{

	if (we_aux.is_790)
		we_ram_790(base, we_aux.wd16, ia->ia_maddr, 0);
	else
		outb(base + WD_MSR, 0);		/* disable memory */
}

#ifndef WDSETIRQ
/*
 * force a WD8003 card to interrupt for autoconfiguration
 */ 
void
weforceintr(aux)
	void *aux;
{
	struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	int base = ia->ia_iobase + WD_NIC_OFFSET;
	int cmd;
	static u_char testaddr[ETHER_ADDR_LEN];

	ds_chipinit(base, testaddr, WD_VMEM_OFFSET,
	    WD_VMEM_OFFSET + (WE_TXBUF_SIZE / DS_PGSIZE),
	    WD_VMEM_OFFSET + (8 KB / DS_PGSIZE), NULL, 0);

	if (we_aux.is_790) {
		/* attempt to force interrupt */
		outb(base + SMC_ICR, SMCICR_EIL | SMCICR_SINT);
	}

	/*
	 * Init transmit length registers, and set transmit start flag.
	 */
	cmd = inb(base + ds_cmd);
	PAGE0(base);
	outb(base + ds0_tbcr0, ETHERMIN + sizeof(struct ether_header));
	outb(base + ds0_tbcr1, (ETHERMIN + sizeof(struct ether_header)) >> 8);
	outb(base + ds_cmd, cmd|DSCM_TRANS);
}
#endif /* WDSETIRQ */
 
/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
void
weattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	register struct we_softc *sc = (struct we_softc *) self;
	struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	register struct ifnet *ifp = &sc->we_if;
	register base;
	int irq;
	u_char sum;
	register int i;
#ifdef WDSETIRQ
	static u_char wd_irq_irr[] = {
		0, 0, WDIRR_IRQ2_10, WDIRR_IRQ3_11, WDIRR_IRQ7_4,
		WDIRR_IRQ5_15, 0, WDIRR_IRQ7_4, 0, WDIRR_IRQ2_10,
		WDIRR_IRQ2_10, WDIRR_IRQ3_11, 0, 0, 0,
		WDIRR_IRQ5_15
	};
	static u_char wd_irq_icr[] = {
		0, 0, 0, 0, WDICR_IR2,
		0, 0, 0, 0, 0,
		WDICR_IR2, WDICR_IR2, 0, 0, 0,
		WDICR_IR2
	};
#endif /* WDSETIRQ */
	static u_char ec_irq_tab[] = {
		0, 0, EIDCFR_IRQ2, EIDCFR_IRQ3, EIDCFR_IRQ4,
		EIDCFR_IRQ5, 0, 0, 0, EIDCFR_IRQ2
	};

	/*
	 * Initialize the board
	 */
	base = ia->ia_iobase;
	irq = ffs(ia->ia_irq) - 1;
	sc->we_type = ((struct we_aux *) ia->ia_aux)->cardid;
	switch (WD_MFR(sc->we_type)) {
	case WD_SMC:
		(void) hwr_swh(base, 0);  /* to look at WD compatible regs */
		for (i = 0; i < ETHER_ADDR_LEN; i++)
			sc->we_addr[i] = inb(base + WD_ROMADDR(i));
		sc->we_lan16 = ((struct we_aux *) ia->ia_aux)->wd16;
		we_ram_790(base, sc->we_lan16, ia->ia_maddr, ia->ia_msize);
		outb(base + SMC_ICR, SMCICR_EIL);	/* enable interrupts */
		goto common;

	case WD_WD:
		/*
		 * Read board ROM station address
		 */
		for (i = 0; i < ETHER_ADDR_LEN; i++)
			sc->we_addr[i] = inb(base + WD_ROMADDR(i));

		/*
		 * Map interface memory, setup memory select register
		 */
		outb(base + WD_MSR, WDMSR_MENB |
		    ((((int)(ia->ia_maddr)) >> 13) & WDMSR_ADDR));
		if (((struct we_aux *) ia->ia_aux)->wd16)
			sc->we_lan16 = 1;

	common:
		/*
		 * Setup card RAM area and i/o addresses
		 */
		sc->we_base = base + WD_NIC_OFFSET;
		sc->we_vmem_addr = ISA_HOLE_VADDR(ia->ia_maddr);
		sc->we_vmem_size = ia->ia_msize;
		sc->we_vmem_end = sc->we_vmem_addr + ia->ia_msize;
		sc->we_vmem_off = WD_VMEM_OFFSET;

		weinitring(sc, ia->ia_msize);

#ifdef WDSETIRQ
		/*
		 * Set up IRQs
		 */
		if (WD_MFR(sc->we_type) == WD_WD) {
			sum = inb(base + WD_IRR) & ~WDIRR_IR;
			outb(base + WD_IRR, sum | wd_irq_irr[irq]);
			outb(base + WD_ICR, wd_irq_icr[irq]);
		}
#endif /* WDSETIRQ */
		break;


	case WD_3Com:
		/*
		 * Setup card RAM area and i/o addresses
		 */
		sc->we_base = base;
		sc->we_vmem_addr = ISA_HOLE_VADDR(ia->ia_maddr);
		sc->we_vmem_size = ia->ia_msize;
		sc->we_vmem_ring = sc->we_vmem_addr + WE_TXBUF_SIZE;
		sc->we_vmem_end = sc->we_vmem_addr + ia->ia_msize;
		sc->we_vmem_off = EC_VMEM_OFFSET;
		sc->we_rx_off = EC_VMEM_OFFSET + (WE_TXBUF_SIZE / DS_PGSIZE);
		sc->we_rx_end = EC_VMEM_OFFSET + (EC_MEM_SIZE / DS_PGSIZE);
		sc->we_max_bufs = 1;	/* No double buffering on 3C503. */

		/* 
		 * Toggle reset bit on/off
		 */
		outb(base + E33G_CNTRL, ECNTRL_RESET|ECNTRL_THIN);
		DELAY(30);
		outb(base + E33G_CNTRL, 0);

		/* Extract Ethernet address */
		outb(base + E33G_CNTRL, ECNTRL_SAPROM); 
		for (i = 0; i < ETHER_ADDR_LEN; i++)
			sc->we_addr[i] = inb(base + i);
		outb(base + E33G_CNTRL, 0); 

		/* tcm, rsel, mbs0, nim */
		outb(base + E33G_GACFR, EGACFR_IRQOFF);

		/* Stop 8390 */
		PAGE0(base);
		outb(base + ds_cmd, DSCM_NODMA|DSCM_STOP);

		/* Point vector pointer registers to nowhere */
		outb(base + E33G_VP2, 0xff);
		outb(base + E33G_VP1, 0xff);
		outb(base + E33G_VP0, 0x0);

		/* Set up control of shared memory, buffer ring, etc.  */
		outb(base + E33G_STARTPG, sc->we_rx_off);
		outb(base + E33G_STOPPG, sc->we_rx_end);

		/*
		 * Set up the IRQ and NBURST on the board.
		 *  (Not sure why we set up NBURST since we don't use DMA) 
		 */
		outb(base + E33G_IDCFR, ec_irq_tab[irq]);
		outb(base + E33G_NBURST, 0x08);     /* Set Burst to 8 */
		outb(base + E33G_DMAAH, sc->we_vmem_off);
		outb(base + E33G_DMAAL, 0x0);
		break;

	default:
		panic("weattach bad mfr");
		break;
	}
 
	/*
	 * Initialize ifnet structure
	 */
	ifp->if_unit = sc->we_dev.dv_unit;
	ifp->if_name = "we";
	ifp->if_mtu = ETHERMTU;
#ifdef MULTICAST
	ifp->if_flags = IFF_BROADCAST|IFF_MULTICAST|IFF_SIMPLEX|IFF_NOTRAILERS;
#else
	ifp->if_flags = IFF_BROADCAST|IFF_SIMPLEX|IFF_NOTRAILERS;
#endif
	ifp->if_init = weinit;
	ifp->if_output = ether_output;
	ifp->if_start = westart;
	ifp->if_ioctl = weioctl;
	ifp->if_watchdog = wewatchdog;
	if_attach(ifp);
 
	/*
	 * Banner...
	 */
	printf("\nwe%d: ", sc->we_dev.dv_unit);
	switch (sc->we_type) {
	case WD_STARLAN:
		printf("WD8003S Starlan"); 
		break;
	case WD_ETHER:
		printf("WD8003E"); 
		break;
	case WD_ETHER2:
		printf("WD8013EBT"); 
		break;
	case WD_ETHER3:
		printf("WD8013EB"); 
		break;
	case WD_ETHER4:
		printf("WD8013EBP");
		break;
	case WD_ETHER5:
		printf("WD8013EPC");
		break;
	case CNET_ETHER:
		printf("CNET-850E");
		break;
	case WD_3C503:
		printf("3C503");
		break;
	case SMC_ULTRA:
		printf("SMC-ULTRA");
		break;
	default:
		printf("type 0x%x", sc->we_type);
		break;
	}
	if (sc->we_lan16)
		printf(" 16bit");
	printf(" address %s\n", ether_sprintf(sc->we_addr));

#if NBPFILTER > 0
	bpfattach(&sc->we_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif

	isa_establish(&sc->we_id, &sc->we_dev);
	sc->we_ih.ih_fun = weintr;
	sc->we_ih.ih_arg = (void *)sc;
	intr_establish(ia->ia_irq, &sc->we_ih, DV_NET);

	sc->we_ats.func = weshutdown;
	sc->we_ats.arg = (void *)sc;
	atshutdown(&sc->we_ats, ATSH_ADD);
}
 
/*
 * Set up transmit/receive buffers for WD_WD or SMC
 * depending on amount of memory available for use.
 */
void
weinitring(sc, msize)
	struct we_softc *sc;
	int msize;
{

	if (msize > WD_MIN_MEMSIZE) {
		sc->we_max_bufs = 2;
		sc->we_vmem_ring = sc->we_vmem_addr + 2 * WE_TXBUF_SIZE;
		sc->we_rx_off = WD_VMEM_OFFSET +
		    (2 * WE_TXBUF_SIZE / DS_PGSIZE);
	} else {
		sc->we_max_bufs = 1;
		sc->we_vmem_ring = sc->we_vmem_addr + WE_TXBUF_SIZE;
		sc->we_rx_off = WD_VMEM_OFFSET +
		    (WE_TXBUF_SIZE / DS_PGSIZE);
	}
	sc->we_rx_end = WD_VMEM_OFFSET + (msize / DS_PGSIZE);
}

/*
 * Disable interface at shutdown
 */
void
weshutdown(arg)
	void *arg;
{
	struct we_softc *sc = (struct we_softc *) arg;
	register base = sc->we_base;
	int s;

	s = splimp();
	westop(base);
	switch (WD_MFR(sc->we_type)) {
	case WD_SMC:
		base -= WD_NIC_OFFSET;
		outb(base + SMC_ICR, 0);	/* disable interrupts */

		/* Disable memory window and reset LAN controller */
		outb(base + SMC_CR, SMCCR_RST);
		DELAY(5);
		outb(base + SMC_CR, 0);

		/* reset everything */
		outb(base+SMC_HWR, inb(base+SMC_HWR) | SMCHWR_NUKE);
		delay(100);
		break;

	case WD_WD:
		base -= WD_NIC_OFFSET;
		/* Disable interrupts */
		outb(base + WD_IRR, 0);
		
		/* Disable memory window and reset LAN controller */
		outb(base + WD_MSR, WDMSR_RST);
		DELAY(5);
		outb(base + WD_MSR, 0);

		/*
		 * Recall power-on parameters from EEPROM;
		 * this resets LAAR (and most of the other registers).
		 */
		outb(base + WD_ICR, WDICR_RLA | WDICR_RIO | WDICR_RX7);
		break;

	case WD_3Com:
		/* Do soft reset sequence */
		outb(base + E33G_CNTRL, ECNTRL_RESET | ECNTRL_THIN);
		DELAY(5);
		outb(base + E33G_CNTRL, 0); /* leaves default value in CNTRL */
		break;
	}
	splx(s);
}
 
/*
 * Take interface offline.
 */
void
westop(base)
	int base;
{
	u_char cmd;
	int s;
 
	/*
	 * Shutdown DS8390
	 */
	s = splimp();
	cmd = inb(base + ds_cmd);
	cmd |= DSCM_STOP;
	cmd &= ~(DSCM_START|DSCM_TRANS);
	outb(base + ds_cmd, cmd);
	splx(s);
}

/*
 * Interface was silent for a long time...
 */
wewatchdog(unit)
	int unit;
{

	log(LOG_WARNING, "we%d: timed out, resetting\n", unit);
	westop(((struct we_softc *) wecd.cd_devs[unit])->we_base);
	((struct we_softc *) wecd.cd_devs[unit])->we_if.if_oerrors++;
	weinit(unit);
}

/*
 * Initialization of interface (really just DS8390). 
 */
weinit(unit)
	int unit;
{
	register struct we_softc *sc = wecd.cd_devs[unit];
	struct ifnet *ifp = &sc->we_if;
	register base = sc->we_base;
	int s, lan16;
 
	/* address not known */
	if (ifp->if_addrlist == (struct ifaddr *)0)
		return;

	/*
	 * Select interface type for 3C503
	 */
	s = splimp();

	/*
	 * For 3C503, LINK0 selects BNC.
	 */
	if (sc->we_type == WD_3C503) {
		outb(base + E33G_GACFR, EGACFR_NORM);
		/* select type of interface... */
		outb(base + E33G_CNTRL,
		    (ifp->if_flags & IFF_LINK0) ? ECNTRL_THIN : 0);
	}

	/*
	 * For WD/SMC Elite, LINK1 forces 8-bit mode;
	 * LINK2 switches to 16-bit when accessing device memory.
	 */
	lan16 = sc->we_lan16;
	if (WD_MFR(sc->we_type) == WD_WD && lan16) {
		int msize = sc->we_vmem_size;

		if (sc->we_if.if_flags & IFF_LINK1) {
			outb(base-WD_NIC_OFFSET+WD_LAAR, WD_ADDR19);
			lan16 = 0;
			/* need to redo mem layout with one transmit buffer */
		} else if (sc->we_if.if_flags & IFF_LINK2)
			outb(base-WD_NIC_OFFSET+WD_LAAR,
			    WD_ADDR19 | WDLAAR_L16EN);
		else
			outb(base-WD_NIC_OFFSET+WD_LAAR,
			     WD_ADDR19 | WDLAAR_M16EN | WDLAAR_L16EN);

		if (lan16 == 0 && (msize == 16 KB || msize == 64 KB))
			msize /= 2;
		weinitring(sc, msize);
	}

	/*
	 * For SMC Ultra, LINK0 selects BNC.
	 */
	if (sc->we_type == SMC_ULTRA) {
		/* select type of interface... */
		int save = hwr_swh(base-WD_NIC_OFFSET, SMCHWR_SWH);
		int gpr = inb(base-WD_NIC_OFFSET + SMC_GCR);

		if (ifp->if_flags & IFF_LINK0)
			gpr |= GPR_BNC;
		else
			gpr &= ~GPR_BNC;
		outb(base-WD_NIC_OFFSET+SMC_GCR, gpr | SMCGCR_LIT);
		outb(base-WD_NIC_OFFSET+SMC_HWR, save);
	}

	sc->we_bdry = 0;
	ds_chipinit(base, sc->we_addr, sc->we_vmem_off, sc->we_rx_off,
	    sc->we_rx_end, &sc->we_ac, lan16);

	/*
	 * Take the interface out of reset, program the vector, 
	 * enable interrupts, and tell the world we are up.
	 */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;
	sc->we_xmit_busy = 0;
	sc->we_next_buf = 0;
	sc->we_n_free_bufs = sc->we_max_bufs;
	westart(ifp);
	splx(s);
}

/*
 * Initialize DS8390 in order given in NSC NIC manual.
 */
void
ds_chipinit(base, addr, vmem_off, rx_off, rx_end, ac, bit16)
	int base;
	u_char *addr;
	int vmem_off, rx_off, rx_end, bit16;
	struct arpcom *ac;
{
	int i;

	outb(base + ds_cmd, DSCM_NODMA|DSCM_PG0|DSCM_STOP);
	if (bit16)
		outb(base + ds0_dcr, DSDC_FT1|DSDC_BMS|DSDC_LAS|DSDC_WTS);
	else
		outb(base + ds0_dcr, DSDC_FT1|DSDC_BMS);
	outb(base + ds0_rbcr0, 0x0);
	outb(base + ds0_rbcr1, 0x0);
	outb(base + ds0_rcr, DSRC_MON);
	outb(base + ds0_tcr, DSTC_LB0);
	outb(base + ds0_bnry, rx_off);
	outb(base + ds0_tpsr, vmem_off);
	outb(base + ds0_pstart, rx_off);
	outb(base + ds0_pstop, rx_end);
	outb(base + ds0_isr, 0xff);
	outb(base + ds0_imr, 0xff);

	/* 
	 * Load unicast address into comparator
	 */
	PAGE1(base);
	for (i = 0; i < ETHER_ADDR_LEN; i++, addr++)
		outb(base + ds1_par0 + i, *addr);

#ifdef MULTICAST
	/* set up multicast addresses and filtering modes */
	if (ac && ac->ac_if.if_flags & IFF_MULTICAST) {
		u_long mcaf[2];

		if (ac->ac_if.if_flags & (IFF_PROMISC | IFF_ALLMULTI)) {
			mcaf[0] = 0xffffffff;
			mcaf[1] = 0xffffffff;
		} else
			ds_getmcaf(ac, mcaf);
		/*
		 * Set multicast filter on chip.
		 */
		for (i = 0; i < 8; i++)
			outb(base + ds1_mar0 + i, ((u_char *)mcaf)[i]);
	}
#else
	/*
	 * Set multicast filter mask bits to accept all multicasts
	 */
	for (i = 0; i < 8; i++)
		outb(base + ds1_mar0 + i, 0xff);
#endif

	/* Set current shared page for RX to work on */
	outb(base + ds1_curr, rx_off);

	/*
	 * Start the 8390
	 */
	outb(base + ds_cmd, DSCM_START|DSCM_PG0|DSCM_NODMA);
	outb(base + ds0_isr, 0xff);
	outb(base + ds0_tcr, 0x0);
	i = DSRC_AB;
	if (ac) {
		if (ac->ac_if.if_flags & IFF_PROMISC)
			/* set promiscuous mode */
			i |= DSRC_PRO|DSRC_AM|DSRC_AR|DSRC_SEP;
#ifdef MULTICAST
		if (ac->ac_multiaddrs || ac->ac_if.if_flags & IFF_ALLMULTI)
			i |= DSRC_AM;
#endif
	}
	outb(base + ds0_rcr, i);
}
 
/*
 * Start output on interface.
 * Must be called at splimp.
 */
westart(ifp)
	struct ifnet *ifp;
{
	register struct we_softc *sc = wecd.cd_devs[ifp->if_unit];
	struct mbuf *m0, *m;
	register caddr_t buffer;
#if NBPFILTER > 0
	caddr_t b2;
#endif
	int len;
	register base = sc->we_base;
	u_char cmd;
 
	/*
	 * Loop until all available Tx buffers are filled or until
         * no more data is available.
	 * The IFF_OACTIVE flag is used to indicate that all Tx buffers
	 * are filled, _not_ to indicate that the transmitter is active.
	 * The sc->we_xmit_busy flag keeps track of transmitter status.
         */
again:
	if (sc->we_n_free_bufs == 0) {
		ifp->if_flags |= IFF_OACTIVE;
		return (0);
	}
	IF_DEQUEUE(&sc->we_if.if_snd, m);
	if (m == 0)
		return (0);

	/*
	 * Copy the mbuf chain into the transmit buffer.  If LINK2 is set,
	 * switch the card into 16-bit memory access mode before moving
	 * the packet onto the card, and switch back to 8-bit mode afterwards.
	 */
	buffer = 
#if NBPFILTER > 0
		 b2 = 
#endif
		      sc->we_vmem_addr + (sc->we_next_buf * WE_TXBUF_SIZE);
	len = 0;
	if (sc->we_if.if_flags & IFF_LINK2)
		outb(base - WD_NIC_OFFSET+WD_LAAR,
		     WD_ADDR19 | WDLAAR_M16EN | WDLAAR_L16EN);
	for (m0 = m; m != 0; m = m->m_next) {
		if (m->m_len == 0)
			continue;
		bcopy(mtod(m, caddr_t), buffer, m->m_len);
		buffer += m->m_len;
		len += m->m_len;
	}

	m_freem(m0);

#if NBPFILTER > 0
	/*
	 * If bpf is listening on this interface, let it
	 * see the packet before we commit it to the wire.
	 */
	if (sc->we_bpf)
		bpf_tap(sc->we_bpf, b2, len);
#endif
	if (sc->we_if.if_flags & IFF_LINK2)
		outb(base-WD_NIC_OFFSET+WD_LAAR, WD_ADDR19 | WDLAAR_L16EN);

	/*
	 * Set packet min length and send the packet if the
	 * transmitter is free.
	 */
	if (len < ETHERMIN + sizeof(struct ether_header))
		len = ETHERMIN + sizeof(struct ether_header);
	if (!sc->we_xmit_busy) {
		cmd = inb(base + ds_cmd);
		PAGE0(base);
		outb(base + ds0_tpsr,
		     (sc->we_next_buf ? WE_TXBUF1 : WE_TXBUF0));
		outb(base + ds0_tbcr0, len);
		outb(base + ds0_tbcr1, len >> 8);
		outb(base + ds_cmd, cmd|DSCM_TRANS);
		sc->we_xmit_busy = 1;
		sc->we_if.if_timer = 2;	/* shouldn't take over 2 sec */
		sc->we_next_buf = (sc->we_next_buf || sc->we_max_bufs == 1 ?
			0 : 1 );
	} else
		/*
		 * The transmitter was busy, remember packet length
		 * until the packet is sent.
		 * It's not possible to reach this code with
		 * only 1 txbuf.
		 */
		sc->we_next_buf_len = len; /* Tx busy, save length. */

	sc->we_n_free_bufs--;		   /* One more buffer filled. */
	goto again;
}
 
/*
 * Ethernet interface interrupt processor
 */
weintr(sc)
	struct we_softc *sc;
{
	register base = sc->we_base;
	u_char sts, cmd;

	/* Get current interrupt status */
	PAGE0(base);
	sts = inb(base + ds0_isr);
	outb(base + ds0_imr, 0x0);

	do {
		outb(base + ds0_isr, sts);

		/* Receive notification */
		if (sts & (DSIS_RX|DSIS_RXE)) {
			/* Receive Error? */
			if (sts & DSIS_RXE) { 
				if (sc->we_if.if_flags & IFF_DEBUG)
					log(LOG_ERR, "we%d: recv error (%x)\n",
					    sc->we_if.if_unit,
					    inb(base + ds0_rsr));
				(void) inb(base + ds0_rcvalctr);
				(void) inb(base + ds0_rcvcrcctr);
				(void) inb(base + ds0_rcvfrmctr);
				sc->we_if.if_ierrors++;
			}
			werint(sc);
		}

		/* Transmit complete */
		if (sts & (DSIS_TX|DSIS_TXE)) {

			/* Transmit error */
			if (sts & DSIS_TXE) {
				int tsr = inb(base + ds0_tsr);

				sc->we_if.if_oerrors++;
				if (tsr & DSTS_COLL16)
					sc->we_if.if_collisions += 16;
				if (sc->we_if.if_flags & IFF_DEBUG)
					log(LOG_ERR, "we%d: xmit error (%x)\n",
					    sc->we_if.if_unit, tsr);
			}
			sc->we_if.if_opackets++;
			sc->we_if.if_collisions += inb(base + ds0_tbcr0) & 0x0f;
			sc->we_if.if_timer = 0;
			sc->we_if.if_flags &= ~IFF_OACTIVE;
			sc->we_xmit_busy = 0;
			if (sc->we_n_free_bufs < sc->we_max_bufs) /* ??? */
				sc->we_n_free_bufs++;

			/*
			 * Any bufferd data ready to transmit ?
			 * With only 1 tx buffer, sc->we_n_free_bufs
			 * is never < sc->we_max_bufs when !sc->we_xmit_busy.
			 * We won't execute this code with only 1 txbuf.
			 */
			if (sc->we_n_free_bufs < sc->we_max_bufs) {
				/*
				 * Init transmit length registers
				 * and set transmit start flag.
				 */
				cmd = inb(base + ds_cmd);
				PAGE0(base);
				outb(base + ds0_tpsr,
				    (sc->we_next_buf ? WE_TXBUF1 : WE_TXBUF0));
				outb(base + ds0_tbcr0, sc->we_next_buf_len);
				outb(base + ds0_tbcr1,
				     sc->we_next_buf_len >> 8);
				outb(base + ds_cmd, cmd|DSCM_TRANS);
				sc->we_xmit_busy = 1;
				sc->we_if.if_timer = 2;
				sc->we_next_buf = (sc->we_next_buf ? 0 : 1);
			}
			westart(&sc->we_if);
		}
		if ((sts & (DSIS_RX|DSIS_RXE|DSIS_TX|DSIS_TXE)) == 0 &&
		    sc->we_if.if_flags & IFF_DEBUG)
			log(LOG_ERR, "we%d: intr status %x\n",
			    sc->we_if.if_unit, sts);

		/* Reenable onboard interrupts */
		PAGE0(base);
		outb(base + ds0_imr, 0xff);
	} while (sts = inb(base + ds0_isr));
	return (1);	
}

/*
 * Receive interrupt.
 */
void
werint(sc)
	register struct we_softc *sc;
{
	u_char bnry, curr;
	int len;
	struct prhdr *ecr;
	register base = sc->we_base;
 
	/*
	 * Traverse the receive ring looking for packets to pass back.
	 * The search is complete when we find a descriptor not in use.
	 */
	if (sc->we_bdry)
		bnry = sc->we_bdry;
	else {
		PAGE0(base);
		bnry = inb(base + ds0_bnry);
	}
	PAGE1(base);
	curr = inb(base + ds1_curr);

	while (bnry != curr) {
		if (sc->we_if.if_flags & IFF_LINK2)
			outb(base - WD_NIC_OFFSET+WD_LAAR,
			     WD_ADDR19 | WDLAAR_M16EN | WDLAAR_L16EN);
		/* get pointer to this buffer header structure */
		ecr = (struct prhdr *) (sc->we_vmem_addr +
		    (bnry - sc->we_vmem_off) * DS_PGSIZE);

		sc->we_if.if_ipackets++;

		len = ecr->pr_sz0 | (ecr->pr_sz1 << 8);
		len -= 4;       /* throw out checksum */

		if (len > 30 && len <= ETHERMTU+100) {
			weread(sc, (caddr_t)(ecr + 1), len);
		} else {
			log(LOG_ERR, "we%d: bad input packet length %d\n",
			    sc->we_if.if_unit, len);
			if (sc->we_if.if_flags & IFF_DEBUG)
				log(LOG_ERR, " (bnry:%x curr:%x)\n",
				    bnry, curr);
		}

		PAGE0(base);

		/* advance on-chip Boundary register */
		if ((caddr_t) ecr + DS_PGSIZE - 1 > sc->we_vmem_end) {
			outb(base + ds0_bnry, sc->we_rx_end - 1);
			bnry = sc->we_rx_off;
		} else {
			if (len > 30 && len <= ETHERMTU+100)
				bnry = ecr->pr_nxtpg;
			else
				bnry = curr;

			/* watch out for NIC overflow, reset Boundry if invalid */
			if (bnry <= sc->we_rx_off) {
				outb(base + ds0_bnry, sc->we_rx_end - 1);
				bnry = sc->we_rx_off;
			} else
				outb(base + ds0_bnry, bnry - 1);
		}
		if (sc->we_if.if_flags & IFF_LINK2)
			outb(base-WD_NIC_OFFSET+WD_LAAR,
			    WD_ADDR19 | WDLAAR_L16EN);

		/* refresh our copy of CURR */
		PAGE1(base);
		curr = inb(base + ds1_curr);
	}
	sc->we_bdry = bnry;
}
 
/*
 * Process an ioctl request.
 */
weioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	register struct ifaddr *ifa = (struct ifaddr *)data;
	struct we_softc *sc = wecd.cd_devs[ifp->if_unit];
	int s = splimp(), error = 0;

	switch (cmd) {

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			weinit(ifp->if_unit);	/* before arpwhohas */
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
				ina->x_host = *(union ns_host *)(sc->we_addr);
			else {
				/* 
				 * The manual says we can't change the address 
				 * while the receiver is armed,
				 * so reset everything
				 */
				ifp->if_flags &= ~IFF_RUNNING; 
				bcopy((caddr_t)ina->x_host.c_host,
				    (caddr_t)sc->we_addr, sizeof(sc->we_addr));
			}
			weinit(ifp->if_unit); /* does ne_setaddr() */
			break;
		    }
#endif
		default:
			weinit(ifp->if_unit);
			break;
		}
		break;

	case SIOCSIFFLAGS:
		if (WD_MFR(sc->we_type) != WD_WD || sc->we_lan16 == 0)
			ifp->if_flags &= ~IFF_LINK2;
		if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) == IFF_RUNNING) {
			ifp->if_flags &= ~IFF_RUNNING;
			westop(sc->we_base);
		} else if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) == IFF_UP ||
		    (ifp->if_flags ^ sc->we_ifoldflags) &
		    (IFF_LINK0|IFF_LINK1|IFF_LINK2|IFF_PROMISC|IFF_ALLMULTI))
			weinit(ifp->if_unit);
		sc->we_ifoldflags = ifp->if_flags;
		break;

#ifdef MULTICAST
	/*
	 * Update our multicast list.
	 */
	case SIOCADDMULTI:
		error = ether_addmulti((struct ifreq *)data, &sc->we_ac);
		goto reset;

	case SIOCDELMULTI:
		error = ether_delmulti((struct ifreq *)data, &sc->we_ac);
	reset:
		if (error == ENETRESET) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
		    	westop(sc->we_base); /* XXX for ds_setmcaf? */
			weinit(ifp->if_unit);
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
 
#define	wedataaddr(sc, eh, off, type) \
	((type) (((caddr_t)((eh)+1)+(off) >= (sc)->we_vmem_end) ? \
		(((caddr_t)((eh)+1)+(off))) - (sc)->we_vmem_end \
		+ (sc)->we_vmem_ring: \
		((caddr_t)((eh)+1)+(off))))

/*
 * Pass a packet to the higher levels.
 * We deal with the trailer protocol here.
 */
void
weread(sc, buf, len)
	register struct we_softc *sc;
	caddr_t buf;
	int len;
{
	register struct ether_header *eh;
    	struct mbuf *m, *weget();
	int off, resid;

	len -= sizeof(struct ether_header);

	/*
	 * Deal with trailer protocol: if type is trailer type
	 * get true type from first 16-bit word past data.
	 * Remember that type was trailer by setting off.
	 */
	eh = (struct ether_header *)buf;
	eh->ether_type = ntohs((u_short)eh->ether_type);
	if (eh->ether_type >= ETHERTYPE_TRAIL &&
	    eh->ether_type < ETHERTYPE_TRAIL+ETHERTYPE_NTRAILER) {
		off = (eh->ether_type - ETHERTYPE_TRAIL) * 512;
		if (off >= ETHERMTU)
			return;		/* sanity */
		eh->ether_type = ntohs(*wedataaddr(sc, eh, off, u_short *));
		resid = ntohs(*(wedataaddr(sc, eh, off+2, u_short *)));
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
	if (sc->we_bpf) {
		resid = len + sizeof(struct ether_header);
		eh->ether_type = htons((u_short)eh->ether_type);
		if (buf + resid > sc->we_vmem_end) {
			unsigned int toend;

			toend = sc->we_vmem_end - buf;
			bcopy(buf, sc->we_bpf_bounce, toend);
			bcopy(sc->we_vmem_ring,
			       &sc->we_bpf_bounce[toend], resid - toend);
			bpf_tap(sc->we_bpf, sc->we_bpf_bounce, resid);
		} else
			bpf_tap(sc->we_bpf, buf, resid);
		eh->ether_type = ntohs((u_short)eh->ether_type);

		/*
		 * Note that the interface cannot be in promiscuous mode if
		 * there are no bpf listeners.  And if we are in promiscuous
		 * mode, we have to check if this packet is really ours.
		 */
		if ((sc->we_if.if_flags & IFF_PROMISC) &&
		    bcmp(eh->ether_dhost, sc->we_addr, 
		    sizeof(eh->ether_dhost)) != 0 &&
#ifdef MULTICAST
		    !ETHER_IS_MULTICAST(eh->ether_dhost)
#else
		    bcmp(eh->ether_dhost, etherbroadcastaddr, 
		    sizeof(eh->ether_dhost)) != 0
#endif
		    )
			return;
	}
#endif

	/*
	 * Pull packet off interface.  Off is nonzero if packet
	 * has trailing header; neget will then force this header
	 * information to be at the front, but we still have to drop
	 * the type and length which are at the front of any trailer data.
	 */
	m = weget(buf, len, off, sc);
	if (m == 0)
		return;
	ether_input(&sc->we_if, eh, m);
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
weget(buf, totlen, off0, sc)
	caddr_t buf;
	int totlen, off0;
	struct we_softc *sc;
{
	struct mbuf *top, **mp, *m;
	int off = off0, len;
	register caddr_t cp = buf;
	char *epkt;
	int tc = totlen;

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
	m->m_pkthdr.rcvif = &sc->we_if;
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
		/* only do up to end of buffer */
		if (cp >= sc->we_vmem_end) {
			cp -= sc->we_vmem_end - sc->we_vmem_ring;
			epkt -= sc->we_vmem_end - sc->we_vmem_ring;
		}
		if (cp + len > sc->we_vmem_end) {
			unsigned toend = sc->we_vmem_end - cp;

			bcopy(cp, mtod(m, caddr_t), toend);
			cp = sc->we_vmem_ring;
			bcopy(cp, mtod(m, caddr_t)+toend, len - toend);
			cp += len - toend;
			epkt = cp + totlen;
		} else {
			bcopy(cp, mtod(m, caddr_t), (unsigned)len);
			cp += len;
		}
		*mp = m;
		mp = &m->m_next;
		if (cp == epkt) {
			cp = buf;
			epkt = cp + tc;
		}
	}
	return (top);
}

/*
 * determine if SMC card is 790 based or the older 690 and return
 * either WD_790 or zero
 */
int
is_smc_790(io)
	int io;
{
	int save;
	unsigned char save_addr[8];
	int i;

	if ((inb(io + SMC_REV) & SMCREV_CHIP) != SMCREV_CHIP790)
		return (0);
	save = hwr_swh(io, SMCHWR_SWH);
	for (i = 0; i < 8; ++i)
		save_addr[i] = inb(io + WD_ROMADDR(i));
	outb(io + SMC_HWR, save & ~SMCHWR_SWH);
	for (i = 0; i < 8; ++i)
		if (save_addr[i] != inb(io+WD_ROMADDR(i)))
			break;
	if (i == 8)
		outb(io + SMC_HWR, save);	/* else leave in 690 mode */
	dprintf(("testing card at %x for 790; i=%d\n", io, i));
	return (i != 8);
}

/*
 * set or clear the SMCHWR_SWH flag depending upon argument 
 * (zero or SMCHWR_SWH), and return the previous setting.
 */
int
hwr_swh(io, flag)
	int io;
	int flag;
{
	int save = inb(io + SMC_HWR);

	outb(io + SMC_HWR, (save &~ SMCHWR_SWH) | flag);
	return (save);
}

int
we_ram_790(io, i16, addr, size)
	int io, i16, addr, size;
{
	int save;
	int code;

	/* set 16 bit mode */
	if (i16) 
		outb(io + SMC_BPR, SMCBPR_M16EN | inb(io + SMC_BPR));
	/* figure out the windows size from the memory size */
	switch (size) {
	case 0:
	case 8 * 1024:
	default:
		code = SMCRAR_RAM8K;
		break;
	case 16 * 1024:
		code = SMCRAR_RAM16K;
		break;
	case 32 * 1024:
		code = SMCRAR_RAM32K;
		break;
	case 64 * 1024:
		code = SMCRAR_RAM64K;
		break;
	}

	save = hwr_swh(io, SMCHWR_SWH);	/* set HWR_SWH */
	code |= ((addr >> 13) & SMCRAR_LOWADDR);
	if (addr & (1 << 17))
		code |= SMCRAR_RA17;
	outb(io + SMC_RAR, code);
	outb(io + SMC_HWR, save);		/* restore SMC_HWR */
	if (size == 0)
		/* disable RAM */
		outb(io + SMC_CR, inb(io + SMC_CR) &~ SMCCR_MENB);
	else
		/* enable RAM */
		outb(io + SMC_CR, (save = inb(io + SMC_CR) | SMCCR_MENB));
	dprintf(("set RAR = %x\n", code));
	return (0);
}

int
wd_set_irq_790(io, irq)
	int io, irq;
{
	static char irq_map_790[16] = {
	/* IRQ		0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15	*/
	/* MAP */	0, 0, 1, 2, 0, 3, 0, 4, 0, 1, 5, 6, 0, 0, 0, 7 };
	int save = hwr_swh(io, SMCHWR_SWH);
	int i = irq_map_790[irq];
	int map = ((i&3) << 2) | ((i&4) << 4);
	int gpr = inb(io + SMC_GCR);

	outb(io + SMC_GCR, SMCGCR_LIT | map |
	    (gpr & (SMCGCR_GPOUT | SMCGCR_ZWSEN)));
	outb(io + SMC_ICR, SMCICR_EIL);		/* enable interrupts */
	outb(io + SMC_HWR, save);
	return (0);
}

#ifdef MULTICAST
/*
 * Compute crc for ethernet address
 */
u_long
ds_crc(ep)
	u_char *ep;
{
#define POLYNOMIAL 0x04c11db6
	register u_long crc = 0xffffffffL;
	register int carry, i, j;
	register u_char b;
	
	for (i = 6; --i >= 0; ) {
		b = *ep++;
		for (j = 8; --j >= 0; ) {
			carry = ((crc & 0x80000000L) ? 1 : 0) ^ (b & 0x01);
			crc <<= 1;
			b >>= 1;
			if (carry)
				crc = ((crc ^ POLYNOMIAL) | carry);
		}
	}
	return crc;
#undef POLYNOMIAL
}

/*
 * Compute the multicast address filter from the
 * list of multicast addresses we need to listen to.
 */
void
ds_getmcaf(ac, mcaf)
	struct arpcom *ac;
	u_long *mcaf;
{
	register u_int index;
	register u_char *af = (u_char *) mcaf;
	register struct ether_multi *enm;
	register struct ether_multistep step;
	
	mcaf[0] = 0;
	mcaf[1] = 0;
	
	ETHER_FIRST_MULTI(step, ac, enm);
	while (enm != NULL) {
		if (bcmp(enm->enm_addrlo, enm->enm_addrhi, 6) != 0) {
			mcaf[0] = 0xffffffff;
			mcaf[1] = 0xffffffff;
			return;
		}
		index = ds_crc(enm->enm_addrlo) >> 26;
		af[index >> 3] |= 1 << (index & 7);
		
		ETHER_NEXT_MULTI(step, enm);
	}
}
#endif
