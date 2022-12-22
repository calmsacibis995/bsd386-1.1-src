/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: isa.c,v 1.30 1994/01/05 20:56:12 karels Exp $
 */

/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)isa.c	7.2 (Berkeley) 5/13/91
 */

/*
 * code to manage AT bus
 */

#include "param.h"
#include "systm.h"
#include "conf.h"
#include "file.h"
#include "malloc.h"
#include "uio.h"
#include "syslog.h"
#include "vm/vm_param.h"
#include "machine/cpu.h"
#include "device.h"
#include "reboot.h"		/* XXX */

#include "dma.h"
#include "isa.h"
#include "isavar.h"
#include "i386/eisa/eisa.h"
#include "icu.h"

#define	IDTVEC(name)	__CONCAT(X,name)
/* default interrupt vector table */
extern	IDTVEC(intr0), IDTVEC(intr1), IDTVEC(intr2), IDTVEC(intr3),
	IDTVEC(intr4), IDTVEC(intr5), IDTVEC(intr6), IDTVEC(intr7),
	IDTVEC(intr8), IDTVEC(intr9), IDTVEC(intr10), IDTVEC(intr11),
	IDTVEC(intr12), IDTVEC(intr13), IDTVEC(intr14), IDTVEC(intr15);
extern	IDTVEC(adintr0), IDTVEC(adintr1), IDTVEC(adintr2), IDTVEC(adintr3),
	IDTVEC(adintr4), IDTVEC(adintr5), IDTVEC(adintr6), IDTVEC(adintr7),
	IDTVEC(adintr8), IDTVEC(adintr9), IDTVEC(adintr10), IDTVEC(adintr11),
	IDTVEC(adintr12), IDTVEC(adintr13), IDTVEC(adintr14), IDTVEC(adintr15);

isa_type isa_bustype;
	
/*
 * Fill in interrupt table for used by auto intr discovery code
 * during configuration of kernel, setup interrupt control unit
 *
 * All of these call the dummy interrupt vector isa_adintr() (below).
 */
isa_autoirq()
{
	int sel = GSEL(GCODE_SEL, SEL_KPL);

	/* first icu */
	setidt(32, &IDTVEC(adintr0), SDT_SYS386IGT, SEL_KPL, sel);
	setidt(33, &IDTVEC(adintr1), SDT_SYS386IGT, SEL_KPL, sel);
	setidt(34, &IDTVEC(adintr2), SDT_SYS386IGT, SEL_KPL, sel);
	setidt(35, &IDTVEC(adintr3), SDT_SYS386IGT, SEL_KPL, sel);
	setidt(36, &IDTVEC(adintr4), SDT_SYS386IGT, SEL_KPL, sel);
	setidt(37, &IDTVEC(adintr5), SDT_SYS386IGT, SEL_KPL, sel);
	setidt(38, &IDTVEC(adintr6), SDT_SYS386IGT, SEL_KPL, sel);
	setidt(39, &IDTVEC(adintr7), SDT_SYS386IGT, SEL_KPL, sel);

	/* second icu */
	setidt(40, &IDTVEC(adintr8), SDT_SYS386IGT, SEL_KPL, sel);
	setidt(41, &IDTVEC(adintr9), SDT_SYS386IGT, SEL_KPL, sel);
	setidt(42, &IDTVEC(adintr10), SDT_SYS386IGT, SEL_KPL, sel);
	setidt(43, &IDTVEC(adintr11), SDT_SYS386IGT, SEL_KPL, sel);
	setidt(44, &IDTVEC(adintr12), SDT_SYS386IGT, SEL_KPL, sel);
	setidt(45, &IDTVEC(adintr13), SDT_SYS386IGT, SEL_KPL, sel);
	setidt(46, &IDTVEC(adintr14), SDT_SYS386IGT, SEL_KPL, sel);
	setidt(47, &IDTVEC(adintr15), SDT_SYS386IGT, SEL_KPL, sel);

	/* reset coprocessor interrupt latch */
	outb(IO_NPX+1, 0);

	if (isa_bustype == BUS_EISA) {
		/* take care of some EISA obligations */
		outb(IO_ENMI, 0);	/* turn off extended NMI features */
		outb(IO_ELCR1, 0);	/* edge-triggered interrupts (0-7) */
		outb(IO_ELCR2, 0);	/* edge-triggered interrupts (8-15) */
	}

	/* initialize 8259's */
	outb(IO_ICU1, 0x11);	/* ICW1 */
	outb(IO_ICU1+1, 32);	/* ICW2: high bits of interrupt vectors */
	outb(IO_ICU1+1, 0x4);	/* ICW3: cascade interrupt */
#ifdef AUTO_EOI
	/*
	 * Some machines with old 8259s won't boot with automatic EOI.
	 * Note, this must correspond with EOI sequence in INTR macro (icu.h).
	 */
	outb(IO_ICU1+1, 3);	/* ICW4: auto EOI */
	outb(IO_ICU1, 0x6b);	/* set special mask mode, and ISR for read */
#else
	outb(IO_ICU1+1, 1);	/* ICW4 */
	outb(IO_ICU1, 0xb);	/* OCW3: set ISR on IO_ICU1 read */
#endif
	outb(IO_ICU1+1, 0xff);	/* OCW1: mask all interrupts */

	outb(IO_ICU2, 0x11);
	outb(IO_ICU2+1, 40);
	outb(IO_ICU2+1, 2);	/* ICW3: master's IRQ for this slave */
#ifdef AUTO_EOI
	outb(IO_ICU2+1, 3);
	outb(IO_ICU2, 0x6b);
#else
	outb(IO_ICU2+1, 1);
	outb(IO_ICU2, 0xb);
#endif
	outb(IO_ICU2+1, 0xff);

	INTREN(IRQ_SLAVE);
	INTREN(IRQ0);		/* clock is special, not config'ed normally */
}

matchbyname(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{

	return (strcmp(cf->cf_driver->cd_name, (char *) aux) == 0);
}

#define	NIRQ	16

static int irqmap = IRQ0|IRQ_SLAVE;	/* clock and slave are reserved */

/*
 * Chose a free IRQ given a mask of choices;
 * return a mask containing one free IRQ,
 * or 0 if none if the IRQs in the mask are free.
 */
isa_irqalloc(irqmask)
	int irqmask;
{
	int irq;

	irqmask &= ~irqmap;
	/* lower irqs are more likely to be required elsewhere */
	for (irq = 1 << 15; irq; irq >>= 1)
		if (irqmask & irq)
			return (irq);
	/* no useable irqs */
	return (0);
}

/*
 * We keep track of the allocation of low-numbered I/O ports
 * as that is where most of the action is.
 */
#define	ISA_NPORT_CHECK	0x400

#define	FREE8	0		/* NBBY (== 8) consecutive free ports */
#define	BUSY8	0xff		/* NBBY (== 8) consecutive busy ports */
#define	BUSY2	0x03		/* 2 busy ports, then 6 free */
#define	BUSY4	0x0f		/* 4 busy ports, then 4 free */
#define	FREE16	FREE8, FREE8
#define	BUSY16	BUSY8, BUSY8
#define	FREE64	FREE16, FREE16, FREE16, FREE16

/* initialized to include internally-used ports like dma, icu, etc. */
u_char	isa_portmap[ISA_NPORT_CHECK / NBBY] = {
	BUSY8,  FREE8,  FREE16, BUSY2, FREE8, FREE16,		/*   0 -  3f */
	BUSY4,  FREE8,  FREE16,  0x02, FREE8, BUSY2, FREE8,	/*  40 -  7f */
	BUSY16, FREE16, BUSY2,  FREE8, FREE16,			/*  80 -  bf */
	BUSY16, BUSY16, FREE16, FREE16,				/*  c0 -  ff */
	FREE64,			/* 100 - 13f */
	FREE64,			/* 140 - 17f */
	FREE64,			/* 180 - 1bf */
	FREE64,			/* 1c0 - 1ff */
	FREE64,			/* 200 - 23f */
	FREE64,			/* 240 - 27f */
	FREE64,			/* 280 - 2bf */
	FREE64,			/* 2c0 - 2ff */
	FREE64,			/* 300 - 33f */
	FREE64,			/* 340 - 37f */
	FREE64,			/* 380 - 3bf */
	FREE64,			/* 3c0 - 3ff */
};

/*
 * Check whether "size" ports are free beginning at "base".
 * The minimum size is 1, but 0 is interpretted as 1
 * for convenience when the size has not yet been set.
 */
isa_portcheck(base, size)
	int base, size;
{
	register int i;

	if (size == 0)
		size = 1;
	for (i = base; i < ISA_NPORT_CHECK &&  i < base + size; i++)
		if (isset(isa_portmap, i))
			return (0);
	return (1);
}

/*
 * Mark "size" ports as allocated beginning at "base".
 */
void
isa_portalloc(base, size)
	int base, size;
{
	register int i;

	for (i = base; i < ISA_NPORT_CHECK &&  i < base + size; i++)
		setbit(isa_portmap, i);
}

int autodebug = 0;

/*
 * Probe for a device on the ISA bus
 */
isa_devprobe(dev, cf, aux)
	struct device *dev;
	struct cfdata *cf;
	void *aux;
{
	struct isa_attach_args iaa, *ia = &iaa;
	int matchlevel;
	int isaprint();
	
	ia->ia_iobase = cf->cf_iobase;	
	ia->ia_iosize = cf->cf_iosize;		/* usually not yet known */
	ia->ia_maddr = (caddr_t)cf->cf_maddr;	/* start of io mem */
	ia->ia_msize = cf->cf_msize;		/* len of io mem */
	ia->ia_irq = cf->cf_irq;		/* usually IRQUNK */ 
	ia->ia_drq = cf->cf_drq;
	if (autodebug) {
		printf("\nprobe %s%d", cf->cf_driver->cd_name, cf->cf_unit);
		isaprint(ia, NULL);
		if (autodebug > 1) {
			printf(" [yn]? ");
			if (getchar() == 'n') {
				printf("\n");
				return (0);
			}
		}
		printf(": ");
	}

	/*
	 * Check that at least the base port is available.
	 * The size is usually 0 at this point.  The driver may
	 * make a complete check if desirable, including any
	 * discontiguous ports.
	 */
	if (ia->ia_iobase && isa_portcheck(ia->ia_iobase, ia->ia_iosize) == 0) {
	 	if (autodebug)
			printf("base port in use\n");
	 	return (0);
	}
	/* 
	 * driver dependent match is responsible for filling in the proper
	 * ia_irq, ia_drq, ia_maddr, ia_iosize, etc.
	 */
	if (!(matchlevel = (*cf->cf_driver->cd_match)(dev, cf, (void *)ia))) {
	 	if (autodebug)
			printf("probe failed\n");
		return (0);
	}

	/*
	 * Check for port overlap again, as we should now
	 * have the correct size.
	 */
	if (ia->ia_iobase && isa_portcheck(ia->ia_iobase, ia->ia_iosize) == 0) {
		printf("Warning: probe of %s%d may have modified ports 0x%x - 0x%x, already in use\n",
		    cf->cf_driver->cd_name, cf->cf_unit,
		    ia->ia_iobase + 1, ia->ia_iobase + ia->ia_iosize - 1);
		return (0);
	}

	/* 
	 * irq conflict map -- warn if more than one device uses
	 * an irq; sharing may or may not work
	 */
	if (irqmap & ia->ia_irq)
		printf("WARNING: conflict at irq %d\n", ffs(ia->ia_irq) - 1);
	irqmap |= ia->ia_irq;

	isa_portalloc(ia->ia_iobase, ia->ia_iosize);
	if (autodebug)
		printf("attached\n");
	config_attach(dev, cf, ia, isaprint);
	return (matchlevel);
}

/*
 * Determine the type of bus present.
 * Currently, we check for eisa and default to isa.
 * Note: this is called early, from init386,
 * so that we know the bus type when isa_autoirq is called.
 * This should be fixed.
 */
isa_probe()
{

	if (!bcmp(ISA_HOLE_VADDR(EISA_ID_OFFSET), EISA_ID, EISA_ID_LEN))
		isa_bustype = BUS_EISA;
	else
		isa_bustype = BUS_ISA;
	return (1);
}

/*
 * Attach the isa bus.
 *
 * Our main job is to attach the ISA bus and iterate down the list 
 * of `isa devices' (children of that node).
 */
static void
isa_attach(parent, dev, aux)
	struct device *parent, *dev;
	void *aux;
{
	extern int do_page;
	int old_page = do_page;

	if (isa_bustype == BUS_EISA) {
		printf(": eisa");
#if defined(DEBUG) && defined(EISA)
		eisa_print_devmap();
#endif
	}
	printf("\n");

	if (autodebug == 1)		/* not if asking per device */
		do_page = 1;
	config_search(isa_devprobe, dev, aux);
	isa_normalirq();
	do_page = old_page;
}

struct cfdriver isacd =
    { NULL, "isa", matchbyname, isa_attach, sizeof(struct isa_softc) };

isaprint(aux, name)
	void *aux;
	char *name;
{
	register struct isa_attach_args *ia = aux;

	if (name)
		printf(" at %s", name);
	if (ia->ia_iobase)
		printf(" iobase 0x%x", ia->ia_iobase);
	if (ia->ia_irq)
		if (ia->ia_irq == IRQUNK)
			printf(" irq ?");
		else
			printf(" irq %d", ffs(ia->ia_irq) - 1);
	if (ia->ia_drq)
		if (ia->ia_drq == (u_short)-1)
			printf(" drq ?");
		else
			printf(" drq %d", ia->ia_drq);
	if (ia->ia_maddr)
		printf(" maddr 0x%x-0x%x", ia->ia_maddr,
		    ia->ia_maddr + ia->ia_msize - 1);

	return (UNCONF);
}

/*
 * Each attached device calls isa_establish after it initializes
 * its isadev portion.  This links the device into the isa chain.
 */
void
isa_establish(id, dev)
        register struct isadev *id;
        register struct device *dev;
{
        register struct isa_softc *sc = (struct isa_softc *)dev->dv_parent;

        id->id_dev = dev;
        id->id_bchain = sc->sc_isadev;
        sc->sc_isadev = id;
}


int mask0, mask1, mask2, mask3, mask4, mask5, mask6, mask7, 
    mask8, mask9, mask10, mask11, mask12, mask13, mask14, mask15;

int *masktbl[NIRQ] = {
	&mask0, &mask1, &mask2, &mask3, &mask4, &mask5, &mask6, &mask7, 
	&mask8, &mask9, &mask10, &mask11, &mask12, &mask13, &mask14, &mask15
};

struct intrhand *intrhand[NIRQ] = {
	NULL,		/* intr 0: Clock */
	NULL,		/* intr 1: Keyboard */
	NULL,		/* intr 2 */
	NULL,		/* intr 3 */
	NULL,		/* intr 4 */
	NULL,		/* intr 5 */
	NULL,		/* intr 6 */
	NULL,		/* intr 7 */
	NULL,		/* intr 8 */
	NULL,		/* intr 9 */
	NULL,		/* intr 10 */
	NULL,		/* intr 11 */
	NULL,		/* intr 12 */
	NULL,		/* intr 13: coproc */
	NULL,		/* intr 14: harddrive */
	NULL,		/* intr 15 */
};

int	isa_straycount[16];	/* unclaimed interrupts on all irq's */
int	isa_defaultstray[2];	/* "default interrupts" on irq 7/15 */

/*
 * Attach an interrupt handler to the vector chain for the given level.
 * Each driver's attach routine must call this to register its interrupt.
 */
void
intr_establish(intr, ih, devtype)
	int intr;
	struct intrhand *ih;
	enum devclass devtype;
{
	register struct intrhand **p, *q;
	int index = ffs(intr) - 1;

	(void) splhigh();
	for (p = &intrhand[index]; (q = *p) != NULL; p = &q->ih_next)
		continue;
	*p = ih;
	ih->ih_next = NULL;
	switch (devtype) {
		case DV_DISK:
		case DV_TAPE:
			INTRMASK(biomask, intr);
			*masktbl[index] |= intr;
			break;
		case DV_TTY:
			INTRMASK(ttymask, intr);
			*masktbl[index] |= intr;
			break;
		case DV_NET:
			INTRMASK(impmask, intr);
			*masktbl[index] |= intr;
			break;
		case DV_CLK:
			*masktbl[index] |= highmask;
			break;
		case DV_COPROC:
			*masktbl[index] |= intr;
			break;
	}
	irqmap |= intr;
	INTREN(intr);
	(void) spl0();
}

/*
 * Normal interrupt vector (Torek-style) which acts as a switch to 
 * the driver.
 */
void
isa_intrswitch(frame)
	struct intrframe frame;
{
	register struct intrhand *ih;
	int expected = 0;

	for (ih = intrhand[frame.if_vec]; ih; ih = ih->ih_next) {
		/* 
		 * Call the appropriate driver's interrupt handler.
		 * For now at least, can't tell if interrupts
		 * will be 1-for-1 with requests, so call all handlers,
		 * ignoring return values.
		 */
		/*
		 * awkward code below avoids compiler strangeness with
		 * ?: conditional
	  	 */
		if (ih->ih_arg) {
			if ((*ih->ih_fun)(ih->ih_arg)) {
				expected++;
				ih->ih_count++;
			}
		} else {
			if ((*ih->ih_fun)(&frame)) {
				expected++;
				ih->ih_count++;
			}
		}
	}
	if (expected == 0) {
		isa_straycount[frame.if_vec]++;
		/* Ignore stray clock and intr 7 (problem on some machines) */
		if (frame.if_vec != 0 && frame.if_vec != 7)
			log(LOG_ERR, "stray interrupt on ISA irq %d\n",
			    frame.if_vec);
	}
}

volatile int autointr;

/*
 * This is the "dummy" interrupt vector used during autoconfig
 * to catch interrupts generated by prodding the cards.
 */ 
isa_adintr(frame)
	struct intrframe frame;
{

	/* ignore clk & kbd */
	if (frame.if_vec != 0 && frame.if_vec != 1 && autointr == 0)
		autointr = frame.if_vec;
}

/*
 * Used by each driver to "prod" the card and get it to generate
 * an interrupt, and then return which interrupt it got.
 *
 * Driver supplies prod routine (as force argument) and arg to
 * that routine.
 */  
isa_discoverintr(force, arg)
	void (*force)();
	void *arg;
{
	volatile int *aip = &autointr;
	int i;

	if (autodebug)
		printf("check intr: ");
	splx(0);			/* all interrupts enabled */
	DELAY(10000);
	*aip = 0;

	/* 
 	 * call driver-specific cattle prod 
	 */
	(*force)(arg);

	/* 
	 * wait for card to interrupt 
	 */
	for (i = 0; (i < 100) && (!*aip); i++)
		DELAY(10000);

	(void) spl0();
	if (autodebug) {
		if (*aip)
			printf("irq %d ", *aip);
		else
			printf("none ");
	}
	return (1 << *aip);
}

isa_got_intr()
{

	return (autointr ? (1 << autointr) : 0);
}

int tty_net_devs = 0;	/* any tty-class devices that may be used for net? */
int orig_impmask;

/*
 * Set up "real" interrupt vectors [we're ready to run!]
 */
isa_normalirq()
{
	int sel = GSEL(GCODE_SEL, SEL_KPL), i;
	extern int safepri;

	/*
	 * Make sure we catch all the interrupt mask group for
	 * each interrupt 
	 */
	for (i = 0; i < NIRQ; i++) {
		if (*masktbl[i] & biomask)
			*masktbl[i] |= biomask;
		if (*masktbl[i] & impmask)
			*masktbl[i] |= impmask;
		/*
		 * block bio, net devices during tty interrupts
		 * to minimize latency for ttys; this give ttys
		 * priority over all but clock.
		 */
		if (*masktbl[i] & ttymask)
			*masktbl[i] |= ttymask | biomask | impmask;
		*masktbl[i] |= nonemask;
	}

	/*
	 * If we're using networks over tty devices (SLIP, PPP, parallel-port
	 * Ethernet) then any tty intr is potentially a net intr.
	 * Note: adding ttymask to impmask after the individual interrupts'
	 * masks were set (above) means that network device interrupts
	 * will not block tty device interrupts, but that splimp()
	 * will block both.
	 */
	orig_impmask = impmask;
	if (tty_net_devs)
		impmask |= ttymask;

	/*
	 * Block unassigned interrupts at all times.
	 */
	biomask |= nonemask;
	ttymask |= nonemask;
	impmask |= nonemask;
	protomask |= nonemask;

	safepri = nonemask;		/* allow NFS sync after crash */

	/* first icu */
#if 0	/* clock is done in enablertclock */
	setidt(32, &IDTVEC(intr0),  SDT_SYS386IGT, SEL_KPL, sel);
#endif
	setidt(33, &IDTVEC(intr1),  SDT_SYS386IGT, SEL_KPL, sel);
	setidt(34, &IDTVEC(intr2),  SDT_SYS386IGT, SEL_KPL, sel);
	setidt(35, &IDTVEC(intr3),  SDT_SYS386IGT, SEL_KPL, sel);
	setidt(36, &IDTVEC(intr4),  SDT_SYS386IGT, SEL_KPL, sel);
	setidt(37, &IDTVEC(intr5),  SDT_SYS386IGT, SEL_KPL, sel);
	setidt(38, &IDTVEC(intr6),  SDT_SYS386IGT, SEL_KPL, sel);
	setidt(39, &IDTVEC(intr7),  SDT_SYS386IGT, SEL_KPL, sel);

	/* second icu */
	setidt(40, &IDTVEC(intr8),  SDT_SYS386IGT, SEL_KPL, sel);
	setidt(41, &IDTVEC(intr9),  SDT_SYS386IGT, SEL_KPL, sel);
	setidt(42, &IDTVEC(intr10),  SDT_SYS386IGT, SEL_KPL, sel);
	setidt(43, &IDTVEC(intr11),  SDT_SYS386IGT, SEL_KPL, sel);
	setidt(44, &IDTVEC(intr12),  SDT_SYS386IGT, SEL_KPL, sel);
	setidt(45, &IDTVEC(intr13),  SDT_SYS386IGT, SEL_KPL, sel);
	setidt(46, &IDTVEC(intr14),  SDT_SYS386IGT, SEL_KPL, sel);
	setidt(47, &IDTVEC(intr15),  SDT_SYS386IGT, SEL_KPL, sel);
}

tty_net_devices(change)
	int change;
{

	tty_net_devs += change;
	if (tty_net_devs)
		impmask |= ttymask;
	else
		impmask = orig_impmask;
}

struct at_dma_state at_dma_state[8];

/*
 * The routine at_setup_dmachan should be called once by each driver
 * that will be using DMA, generally from the attach routine,
 * or at least from the top half.
 * NOTE: the memory in the bounce buffer must be physically contiguous,
 * and must not span a 64K boundary (128K for channels 5-7);
 * thus this is currently unsafe for maxsz > NBPG.
 */
void
at_setup_dmachan(chan, maxsz)
	int chan;
	int maxsz;
{
	register struct at_dma_state *atd = &at_dma_state[chan];

	if (atd->bounce == NULL) {
		atd->bounce = (caddr_t) malloc(maxsz, M_DEVBUF, M_WAITOK);
		atd->bphys = kvtop(atd->bounce);
		atd->maxsz = maxsz;
	}
}

/*
 * Start DMA on channel "chan" to copy "nbytes" bytes to (read)
 * or from (write) physical memory address "addr".
 */
void
at_dma(isread, addr, nbytes, chan)
	int isread;
	caddr_t addr;
	int nbytes;
	int chan;
{
	register struct at_dma_state *atd = &at_dma_state[chan];
	int phys;
	int s, needcopy;

	/*
	 * If the transfer spans one or more page boundaries, the physical
	 * pages may well not be contiguous.  Could check, but that means
	 * one or two extra calls to pmap_extract(); just copy if
	 * we span pages for now.
	 */
	if (trunc_page(addr) != trunc_page(addr + nbytes - 1))
		needcopy = 1;
	else {
		phys = kvtop(addr);
		if (phys + nbytes - 1 > ISA_MAXADDR)
			needcopy = 1;
		else
			needcopy = 0;
	}

	atd->addr = NULL;
	if (needcopy) {
		if (nbytes > atd->maxsz)
			panic("at_dma size");
		phys = atd->bphys;

		/*
		 * copy to bounce buffer now on write,
		 * save address for copy after DMA on read
		 */
		if (isread) {
			atd->addr = addr;
			atd->nbytes = nbytes;
		} else
			bcopy(addr, atd->bounce, nbytes);
	}
	/*
	 * Start DMA
	 */
	if (chan > 3) {
		/*
		 * Channels 5-7 use 16-bit transfers, with size in words.
		 * (Channel 4 is used internally to cascade 8237A's.)
		 */
		chan -= 4;
		nbytes = (nbytes >> 1) - 1;
		s = splhigh();
		outb(DMA2_CLEARFF, 0);
		if (isread)
			outb(DMA2_MODE, DMA_CNUM(chan) | DMA_READ);
		else
			outb(DMA2_MODE, DMA_CNUM(chan) | DMA_WRITE);
		/*
		 * Address bit 0 is ignored;
		 * address bits 1-16 go into the 8237A address register;
		 * address bits 17-23 go into the page register bits 1-7.
		 */
		outb(DMA2_ADDR(chan), phys >> 1);
		outb(DMA2_ADDR(chan), phys >> 9);
		outb(DMA2_PAGEREG(chan), phys >> 16);
		outb(DMA2_COUNT(chan), nbytes);
		outb(DMA2_COUNT(chan), nbytes >> 8);
		outb(DMA2_MASK, DMA_ECHAN(chan));
	} else {
		nbytes--;
		s = splhigh();
		outb(DMA1_CLEARFF, 0);
		if (isread)
			outb(DMA1_MODE, DMA_CNUM(chan) | DMA_READ);
		else
			outb(DMA1_MODE, DMA_CNUM(chan) | DMA_WRITE);
		outb(DMA1_ADDR(chan), phys);
		outb(DMA1_ADDR(chan), phys >> 8);
		outb(DMA1_PAGEREG(chan), phys >> 16);
		outb(DMA1_COUNT(chan), nbytes);
		outb(DMA1_COUNT(chan), nbytes >> 8);
		outb(DMA1_MASK, DMA_ECHAN(chan));
	}	
	splx(s);
}

/*
 * Abort a DMA operation.
 * Could dig out the count and return actual/resid count,
 * but we don't need that now.
 */
void
at_dma_abort(chan)
	int chan;
{

	if (chan > 3)
		outb(DMA2_MASK, DMA_DCHAN(chan - 4));
	else
		outb(DMA1_MASK, DMA_DCHAN(chan));
}

/*
 * Set cascade mode on a DMA channel
 */
void
at_dma_cascade(chan)
	int chan;
{

	if (chan > 3) {
		chan -= 4;
		outb(DMA2_MODE, DMA_CNUM(chan) | DMA_CASCADE);
		outb(DMA2_MASK, DMA_ECHAN(chan));
	} else {
		outb(DMA1_MODE, DMA_CNUM(chan) | DMA_CASCADE);
		outb(DMA1_MASK, DMA_ECHAN(chan));
	}
}

/*
 * Handle a NMI, possibly a machine check.
 * return true to panic system, false to ignore.
 */
isa_nmi(cd)
{

	log(LOG_CRIT, "\nNMI port 61 %x, port 70 %x\n", inb(0x61), inb(0x70));
	return (0);
}
