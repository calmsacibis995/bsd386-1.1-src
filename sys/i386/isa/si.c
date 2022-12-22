/*	BSDI $Id: si.c,v 1.4 1994/01/30 07:37:17 karels Exp $	*/

#ifndef lint
static char copyright1[] =  "@(#) (c) Specialix International, 1990,1992",
            copyright2[] =  "@(#) (c) Andy Rutter 1993",
            rcsid[] = "Id: si.c,v 1.11 1993/10/18 08:25:10 andy Exp andy";
#endif	/* not lint */

/*
 *	Device driver for Specialix range (SLXOS) of serial line multiplexors.
 *
 * Module:	si.c
 * Target:	BSDI/386
 * Author:	Andy Rutter, andy@acronym.co.uk
 *
 * Revision: 1.11
 * Date: 1993/10/18 08:25:10
 * State: Exp
 *
 * Derived from:	SunOS 4.x version
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notices, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notices, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Andy Rutter of
 *	Advanced Methods and Tools Ltd. based on original information
 *	from Specialix International.
 * 4. Neither the name of Advanced Methods and Tools, nor Specialix
 *    International may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
 * NO EVENT SHALL THE AUTHORS BE LIABLE
 */

#define	SI_DEBUG			/* turn driver debugging on */

#include "param.h"
#include "systm.h"
#include "ioctl.h"
#include "tty.h"
#include "proc.h"
#include "user.h"
#include "conf.h"
#include "file.h"
#include "uio.h"
#include "kernel.h"
#include "syslog.h"
#include "device.h"
#include "malloc.h"

#include "i386/isa/icu.h"
#include "i386/isa/isa.h"
#include "i386/isa/isavar.h"

#include "i386/isa/sireg.h"

#include "si.h"

/*
 * This device driver is designed to interface the Specialix International
 * range of serial multiplexor cards (SLXOS) to BSDI/386 on an ISA bus machine.
 *
 * The controller is interfaced to the host via dual port ram
 * and a (programmable - SIPLUS) interrupt at IRQ 11,12 or 15.
 */

#undef	STATIC_TTY		/* tty compatibility mode */

#define	DEF_IXANY	SPF_IXANY	/* allow IXANY in t_iflag to work */
#define	DEF_COOKMODE	SPF_COOKWELL_ALWAYS
#define	POLL		/* turn on poller to generate buffer empty interrupt */

enum si_mctl { GET, SET, BIS, BIC };

static void si_download __P((struct si_loadcode *));
static void si_boot __P((void));
static void si_command __P((struct si_port *, int, int));
static void si_write_enable __P((struct si_port *));
static int si_modem __P((struct si_port *, enum si_mctl, int));
static int si_drainwait __P((struct si_port *, char *, int));
static int si_Sioctl __P((dev_t, int, caddr_t, int, struct proc *));
static int si_direct __P((struct si_port *, struct uio *, int));
static int si_start __P((struct tty *));
static void si_lstart __P((struct si_port *));

static int si_Nports = 0;
static int si_debug = 0;

#ifdef	STATIC_TTY
/* This is incredibly wasteful of space */
# ifdef	SI_XPRINT
struct tty si_tty[NSI * SI_MAXPORT * 2];	/* 128 elements */
#else
struct tty si_tty[NSI * SI_MAXPORT];		/* 64 elements */
# endif
#endif

static char *si_sleepmsg[] = {
	"sicommand",
	"siparam",
	"siwrite",
	"siclose",
	"sidirect",
	"siioctl"
};
#define	SIMSG_COMMAND		(si_sleepmsg[0])
#define	SIMSG_PARAM		(si_sleepmsg[1])
#define	SIMSG_WRITE		(si_sleepmsg[2])
#define	SIMSG_CLOSE		(si_sleepmsg[3])
#define	SIMSG_DIRECT		(si_sleepmsg[4])
#define	SIMSG_IOCTL		(si_sleepmsg[5])

static struct speedtab bdrates[] = {
	B0,	U0,		B50,	U50,		B75,	U75,
	B110,	U110,		B134,	U134,		B150,	U150,
	B200,	U200,		B300,	U300,		B600,	U600,
	B1200,	U1200,		B1800,	U1800,		B2400,	U2400,
	B4800,	U4800,		B9600,	U9600,		B19200,	U19200,
	B38400, U38400,
	-1,	-1
};
/* populated with approx character/sec rates - translated at card
 * initialisation time to chars per tick of the clock */
static int done_chartimes = 0;
static struct speedtab chartimes[] = {
	B0,	8,		B50,	5760,		B75,	8,
	B110,	11,		B134,	120,		B150,	15,
	B200,	8,		B300,	30,		B600,	60,
	B1200,	120,		B1800,	200,		B2400,	240,
	B4800,	480,		B9600,	960,		B19200,	1920,
	B38400, 3840,
	-1,	-1
};
static int in_intr = 0;			/* Inside interrupt handler? */
static int buffer_space = 128;		/* Amount of free buffer space
					   which must be available before
					   the writer is unblocked */

#ifdef	SI_XPRINT
static int xp_start __P((struct tty *));
static int xp_ioctl __P((struct si_softc *, struct si_port *, int, caddr_t, int));
static void xp_xmit __P((struct si_port *));
#endif

#ifdef POLL
#define	POLL_INTERVAL	(hz/2)
static int init_finished = 0;
static int si_poll __P((void));
#endif

/*
 * Array of adapter types and the corresponding RAM size. The order of
 * entries here MUST match the ordinal of the adapter type.
 */
static struct si_type si_type[] = {
	{ "EMPTY", 0 },
	{ "SIHOST", SIHOST_RAMSIZE },
	{ "SI2", 0 },				/* MCA */
	{ "SIPLUS", SIPLUS_RAMSIZE },
	{ "SIEISA", 0 },
};

/* Autoconfig stuff */
int siprobe __P((struct device *, struct cfdata *, void *));
int siparam __P((struct tty *, struct termios *));
void siattach __P((struct device *, struct device *, void *));
int siintr __P((struct si_softc *));
struct cfdriver sicd =
	{ NULL, "si", siprobe, siattach, sizeof(struct si_softc) };

/* Look for a valid board at the given mem addr */
siprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	struct si_softc *sc;
	int unit, type;
	u_int i, ramsize;
	volatile BYTE was, *ux;
	caddr_t physaddr;
	volatile caddr_t maddr;

	DPRINT((0, DBG_AUTOBOOT, "SLXOS siprobe(%x, %x, %x) physaddr %x\n",
		parent, cf, aux, ia->ia_maddr));
	if (ia->ia_maddr >= (caddr_t)0x100000) {
		printf("SLXOS si%d: iomem (%x) out of range\n",
			cf->cf_unit, ia->ia_maddr);
		return(0);
	}
#ifdef STATIC_TTY
	if ((unit = cf->cf_unit) >= NSI) {
		/* THIS IS IMPOSSIBLE */
		return(0);
	}
#endif
	if (((u_int)ia->ia_maddr & 0x7fff) != 0) {
		DPRINT((0, DBG_AUTOBOOT|DBG_FAIL,
			"SLXOS si%d: iomem (%x) not on 32k boundary\n",
			unit, ia->ia_maddr));
		return(0);
	}
	maddr = ISA_HOLE_VADDR(ia->ia_maddr);

	for (i=0; i < sicd.cd_ndevs; i++) {
		if ((sc = sicd.cd_devs[i]) == NULL)
			continue;
		if (sc->sc_paddr == ia->ia_maddr) {
			DPRINT((0, DBG_AUTOBOOT|DBG_FAIL,
				"SLXOS si%d: iomem (%x) already configured to si%d\n",
				unit, sc->sc_paddr, i));
			return(0);
		}
	}

	/* Is there anything out there? (0x17 is just an arbitrary number) */
	*maddr = 0x17;
	if (*maddr != 0x17) {
		DPRINT((0, DBG_AUTOBOOT|DBG_FAIL,
			"SLXOS si%d: 0x17 check fail at phys 0x%x\n",
			unit, ia->ia_maddr));
fail:
		return(0);
	}
	/*
	 * OK, now to see if whatever responded is really an SI card.
	 * Try for a MK II first (SIPLUS)
	 */
	for (i=SIPLSIG; i<SIPLSIG+8; i++)
		if ((*(maddr+i) & 7) != (~(BYTE)i & 7))
			goto try_mk1;

	/* It must be an SIPLUS */
	*(maddr + SIPLRESET) = 0;
	*(maddr + SIPLIRQCLR) = 0;
	*(maddr + SIPLIRQSET) = 0x10;
	type = SIPLUS;
	ramsize = SIPLUS_RAMSIZE;
	goto got_card;

	/*
	 * Its not a MK II, so try for a MK I (SIHOST)
	 */
try_mk1:
	*(maddr+SIRESET) = 0x0;		/* reset the card */
	*(maddr+SIINTCL) = 0x0;		/* clear int */
	*(maddr+SIRAM) = 0x17;
	if (*(maddr+SIRAM) != (BYTE)0x17)
		goto fail;
	*(maddr+0x7ff8) = 0x17;
	if (*(maddr+0x7ff8) != (BYTE)0x17) {
		DPRINT((0, DBG_AUTOBOOT|DBG_FAIL,
			"SLXOS si%d: 0x17 check fail at phys 0x%x = 0x%x\n",
			unit, ia->ia_maddr+0x77f8, *(maddr+0x77f8)));
		goto fail;
	}

	/* It must be an SIHOST (maybe?) - there must be a better way XXXX */
	type = SIHOST;
	ramsize = SIHOST_RAMSIZE;

got_card:
	DPRINT((0, DBG_AUTOBOOT, "SLXOS: found type %d card, try memory test\n", type));
	/* Try the acid test */
	ux = (BYTE *)(maddr + SIRAM);
	for (i=0; i<ramsize; i++, ux++)
		*ux = (BYTE)(i&0xff);
	ux = (BYTE *)(maddr + SIRAM);
	for (i=0; i<ramsize; i++, ux++) {
		if ((was = *ux) != (BYTE)(i&0xff)) {
			DPRINT((0, DBG_AUTOBOOT|DBG_FAIL,
				"SLXOS si%d: match fail at phys 0x%x, was %x should be %x\n", 
				unit, ia->ia_maddr+i, was, i&0xff));
			goto fail;
		}
	}

	/* clear out the RAM */
	ux = (BYTE *)(maddr + SIRAM);
	for (i=0; i<ramsize; i++)
		*ux++ = 0;
	ux = (BYTE *)(maddr + SIRAM);
	for (i=0; i<ramsize; i++) {
		if ((was = *ux++) != 0) {
			DPRINT((0, DBG_AUTOBOOT|DBG_FAIL,
				"SLXOS si%d: clear fail at phys 0x%x, was %x\n",
				unit, ia->ia_maddr+i, was));
			goto fail;
		}
	}
	/*
	 * Success, we've found a valid board, now fill in
	 * the adapter structure.
	 */
	switch (type) {
	case SIPLUS:
		if (ia->ia_irq == IRQUNK) {
			ia->ia_irq = isa_irqalloc(IRQ11|IRQ12|IRQ15);
			if (ia->ia_irq == 0) {
				DPRINT((0, DBG_AUTOBOOT|DBG_FAIL,
					"SLXOS si%d: no free IRQ\n", unit));
				return(0);
			}
		} else if ((ia->ia_irq&(IRQ11|IRQ12|IRQ15)) == 0) {
bad_irq:
			DPRINT((0, DBG_AUTOBOOT|DBG_FAIL,
				"SLXOS si%d: bad IRQ value - %d\n",
				unit, ia->ia_irq));
			return(0);
		}
		ia->ia_msize = SIPLUS_MEMSIZE;
		break;
	case SIHOST:
		if (ia->ia_irq == IRQUNK) {
			DPRINT((0, DBG_AUTOBOOT|DBG_FAIL,
				"SLXOS: SIHOST irq must be configured\n"));
			return(0);
		} else if ((ia->ia_irq&(IRQ11|IRQ12|IRQ15)) == 0)
			goto bad_irq;
		ia->ia_msize = SIHOST_MEMSIZE;
		break;
	case SIEISA:
	case SI2:		/* MCA */
	default:
		printf("SLXOS si%d: %s not supported\n", unit, si_type[type].st_typename);
		return(0);
	}
	ia->ia_aux = (void *)&si_type[type];
	return(1);
}

/*
 * Attach the device. Should probe for ports here, but the adapter CPU must
 * have code downloaded first & we can't do that until /etc/rc is run and
 * si_download() & si_boot() called.
 */
void
siattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	struct si_softc *sc = (struct si_softc *)self;
	
	DPRINT((0, DBG_AUTOBOOT, "SLXOS siattach(0x%x, 0x%x, 0x%x)\n",
		parent, self, aux));
	sc->sc_irq = ia->ia_irq;
	sc->sc_type = (struct si_type *)ia->ia_aux;
	sc->sc_paddr = ia->ia_maddr;
	sc->sc_maddr = ISA_HOLE_VADDR(ia->ia_maddr);

	/* Initialize interrupt/id structures */
	isa_establish(&sc->sc_id, &sc->sc_dev);
	sc->sc_ih.ih_fun = siintr;
	sc->sc_ih.ih_arg = (void *) sc;
	sc->sc_ports = NULL;			/* mark as uninitialised */
	intr_establish(ia->ia_irq, &sc->sc_ih, DV_TTY);
	printf("\n");
}

/*
 * Download the supplied code into each card.
 */
static void
si_download(sidp)
	struct si_loadcode *sidp;
{
	struct si_softc *sc;
	caddr_t maddr;
	u_int ramsize;
	int i;
	
	DPRINT((0, DBG_DOWNLOAD, "SLXOS si_download: offset %x, nbytes %d\n",
		sidp->sd_offset, sidp->sd_nbytes));
	for (i=0; i < sicd.cd_ndevs; i++) {
		if ((sc = sicd.cd_devs[i]) == NULL)
			continue;
		/* If a not a recognised type or has already been
		 * initialised skip it. */
		if (sc->sc_type == NULL || sc->sc_ports != NULL)
			continue;
		if (sidp->sd_offset+sidp->sd_nbytes+SIRAM > sc->sc_ramsize) {
			printf("SLXOS si%d: download beyond ram at offset 0x%x\n",
				i, sidp->sd_offset);
			continue;
		}
		maddr = sc->sc_maddr + SIRAM + sidp->sd_offset;
		bcopy(sidp->sd_bytes, maddr, sidp->sd_nbytes);
	}
}

/*
 * Initialise each card
 */
static void
si_boot()
{
	struct si_softc *sc;
	struct si_port *pp;
	struct si_channel *ccbp;
	struct si_reg *regp;
	struct si_module *modp;
	struct tty *tp;
#ifdef	SI_XPRINT
	struct tty *xtp;
#endif
	struct speedtab *spt;
	caddr_t maddr;
	int unit, nport, x, y;

	DPRINT((0, DBG_ENTRY|DBG_DOWNLOAD, "SLXOS: si_boot ndevs %d\n", sicd.cd_ndevs));
	for (unit=0; unit < sicd.cd_ndevs; unit++) {
		if ((sc = sicd.cd_devs[unit]) == NULL)
			continue;
		/* If a not a recognised type or has already been
		 * initialised skip it. */
		if (sc->sc_type == NULL || sc->sc_ports != NULL)
			continue;
		sc->sc_flags = /* XXXX: FLAGS; */ 0;
		maddr = sc->sc_maddr;
		switch (sc->sc_type - si_type) {
		case SI2:
		case SIEISA:
			/* must get around to writing the code for
			 * these one day */
			continue;
		case SIHOST:
			*(maddr+SIRESET_CL) = 0;
			*(maddr+SIINTCL_CL) = 0;
			break;
		case SIPLUS:
			*(maddr+SIPLRESET) = 0x10;
			switch (sc->sc_irq) {
			case IRQ11:
				*(maddr+SIPLIRQ11) = 0x10;
				break;
			case IRQ12:
				*(maddr+SIPLIRQ12) = 0x10;
				break;
			case IRQ15:
				*(maddr+SIPLIRQ15) = 0x10;
				break;
			}
			*(maddr+SIPLIRQCLR) = 0x10;
			break;
		}

		DELAY(1000000);			/* wait around for a second */

		regp = (struct si_reg *)maddr;
		y = 0;
						/* wait max of 5 sec for init OK */
		while (regp->initstat == 0 && y++ < 10) {
			DELAY(500000);
		}
		switch (regp->initstat) {
		case 0:
			printf("SLXOS si%d: startup timeout - aborting\n", unit);
			sc->sc_ramsize = 0;
			sc->sc_revision = regp->revision;
			sc->sc_type = NULL;
			return;
		case 1:
				/* set throttle to 100 intr per second */
			regp->int_count = 25000;
				/* rx intr max of 25 timer per second */
			regp->rx_int_count = 4;
			regp->int_pending = 0;		/* no intr pending */
			regp->int_scounter = 0;	/* reset counter */
			sc->sc_ramsize = regp->memsize;
			sc->sc_revision = regp->revision;
			break;
		case 0xff:
			/*
			 * No modules found, so give up on this one.
			 */
			printf("SLXOS si%d: %s - no ports found\n", unit,
				sc->sc_type->st_typename);
			sc->sc_ramsize = regp->memsize;
			continue;
		default:
			printf("SLXOS si%d: Z280 version error - initstat %x\n",
				unit, regp->initstat);
			sc->sc_ramsize = 0;
			continue;
		}

		/*
		 * First time around the ports just count them in order
		 * to allocate some memory.
		 */
		nport = 0;
		modp = (struct si_module *)(maddr + 0x80);
		for (;;) {
			DPRINT((0, DBG_DOWNLOAD, "SLXOS si%d: ccb addr 0x%x\n", unit, modp));
			switch (modp->sm_type & (~MMASK)) {
			case M232:
			case M422:
				DPRINT((0, DBG_DOWNLOAD,
					"SLXOS si%d: Found 232/422 module, %d ports\n",
					unit, (int)(modp->sm_type & MMASK)));
				if (si_Nports == SI_MAXPORT) {
					printf("SLXOS si%d: extra ports ignored\n", unit);
					continue;
				}
				x = modp->sm_type & MMASK;
				nport += x;
				si_Nports += x;
				break;
			default:
				printf("SLXOS si%d: unknown module type %d\n",
					unit, modp->sm_type);
				break;
			}
			if (modp->sm_next == 0)
				break;
			modp = (struct si_module *)
				(maddr + (unsigned)(modp->sm_next & 0x7fff));
		}
		sc->sc_ports = (struct si_port *)malloc(sizeof(struct si_port)*nport,
			M_DEVBUF, M_NOWAIT);
		if (sc->sc_ports == 0) {
mem_fail:
			printf("SLXOS si%d: fail to malloc memory for port structs\n",
				unit);
			continue;
		}
		bzero(sc->sc_ports, sizeof(struct si_port)*nport);
		sc->sc_nport = nport;

#ifdef	STATIC_TTY
		sc->sc_ttydev.tty_base = sc->sc_dev.dv_unit * SI_MAXPORTPERCARD;
		sc->sc_ttydev.tty_ttys = &si_tty[sc->sc_dev.dv_unit * SI_MAXPORTPERCARD];
# ifdef	SI_XPRINT
		sc->sc_xttydev.tty_ttys = &si_tty[(sc->sc_dev.dv_unit * SI_MAXPORTPERCARD) + 64];
# endif
#else
		sc->sc_ttydev.tty_ttys = (struct tty *)malloc(sizeof(struct tty)*nport,
			M_DEVBUF, M_NOWAIT);
		if (sc->sc_ttydev.tty_ttys == 0)
			goto mem_fail;
		bzero(sc->sc_ttydev.tty_ttys, sizeof(struct tty)*nport);
# ifdef	SI_XPRINT
		sc->sc_xttydev.tty_ttys = (struct tty *)malloc(sizeof(struct tty)*nport,
			M_DEVBUF, M_NOWAIT);
		if (sc->sc_xttydev.tty_ttys == 0)
			goto mem_fail;
		bzero(sc->sc_xttydev.tty_ttys, sizeof(struct tty)*nport);
# endif
#endif
		strcpy(sc->sc_ttydev.tty_name, sicd.cd_name);
		sc->sc_ttydev.tty_unit = sc->sc_dev.dv_unit;
		sc->sc_ttydev.tty_count = nport;
		tty_attach(&sc->sc_ttydev);
#ifdef	SI_XPRINT
		strcpy(sc->sc_xttydev.tty_name, "si_xp");
		sc->sc_xttydev.tty_unit = sc->sc_dev.dv_unit;
		sc->sc_xttydev.tty_count = nport;
		tty_attach(&sc->sc_xttydev);
#endif

		/*
		 * Scan round the ports again, this time initialising.
		 */
		pp = sc->sc_ports;
		tp = sc->sc_ttydev.tty_ttys;
#ifdef	SI_XPRINT
		xtp = sc->sc_xttydev.tty_ttys;
#endif
		modp = (struct si_module *)(maddr + 0x80);
		for (;;) {
			switch (modp->sm_type & (~MMASK)) {
			case M232:
			case M422:
				nport = (modp->sm_type & MMASK);
				ccbp = (struct si_channel *)((char *)modp+0x100);
				for (x = 0; x < nport; x++, pp++, ccbp++, tp++) {
					pp->sp_ccb = ccbp;	/* save the address */
					pp->sp_tty = tp;
#ifdef	TTY_BLKINPUT
					if ((pp->sp_rbuf = malloc(SLXOS_BUFFERSIZE,
					    M_DEVBUF, M_NOWAIT)) == 0)
						goto mem_fail;
#endif
					pp->sp_pend = IDLE_CLOSE;
					pp->sp_flags = DEF_IXANY | DEF_COOKMODE;
					if ((sc->sc_flags & (1<<x)) == 0)
						pp->sp_flags |= SPF_MODEM;
					pp->sp_state = 0;	/* internal flag */
#ifdef	SI_XPRINT
					pp->sp_xtty = xtp++;
					pp->sp_xpopen = 0;
					pp->sp_xpcps = XP_CPS;
					bcopy(XP_ON, pp->sp_xpon, sizeof(XP_ON));
					bcopy(XP_OFF, pp->sp_xpoff, sizeof(XP_OFF));
#endif
				}
				break;
			default:
				break;
			}
			if (modp->sm_next == 0) {
				printf("SLXOS si%d: %s - %d ports\n", unit,
					sc->sc_type->st_typename,
					sc->sc_nport);
				break;
			}
			modp = (struct si_module *)
				(maddr + (unsigned)(modp->sm_next & 0x7fff));
		}
	}
	if (done_chartimes == 0) {
		for (spt = chartimes ; spt->sp_speed != -1; spt++) {
			if ((spt->sp_code /= hz) == 0)
				spt->sp_code = 1;
		}
		done_chartimes = 1;
	}
}

int
siopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	int oldspl, error;
	int card, port;
	register struct si_softc *sc;
	register struct tty *tp;
	struct si_channel *ccbp;
	struct si_port *pp;

	if (ISCONTROL(dev)) {
		if (error = suser(p->p_cred->pc_ucred, 0))
			return(error);
		return(0);
	}
	card = SI_CARD(dev);
	if (card >= sicd.cd_ndevs || (sc = sicd.cd_devs[card]) == NULL)
		return (ENXIO);
	if (sc->sc_type == NULL) {
		DPRINT((0, DBG_OPEN|DBG_FAIL, "SLXOS si%d: type %s??\n",
			card, sc->sc_type->st_typename));
no_dev:
		return(ENXIO);
	}
	if ((port = SI_PORT(dev)) >= sc->sc_nport) {
		DPRINT((0, DBG_OPEN|DBG_FAIL, "SLXOS si%d: nports %d\n",
			card, sc->sc_nport));
		goto no_dev;
	}

	pp = sc->sc_ports + port;
	tp = pp->sp_tty;			/* the "real" tty */
	ccbp = pp->sp_ccb;			/* Find control block */
	DPRINT((pp, DBG_ENTRY|DBG_OPEN, "siopen(%x,%x,%x,%x)\n",
		dev, flag, mode, p));
	
	if (ISXPRINT(dev)) {
#ifndef	SI_XPRINT
		DPRINT((pp, DBG_OPEN|DBG_FAIL|DBG_XPRINT,
			"XPRINT not installed\n"));
		goto no_dev;
#else
		if ((tp->t_state & TS_ISOPEN) == 0) {
			DPRINT((pp, DBG_OPEN|DBG_FAIL|DBG_XPRINT,
				"si(%d,%d): open of xp with no open si\n",
				SI_CARD(dev), SI_PORT(dev)));
			goto no_dev;
		}
		if (pp->sp_state & SS_XPXCLUDE) {
			DPRINT((pp, DBG_OPEN|DBG_XPRINT,
				"si(%d,%d): XPRINT already open and EXCLUSIVE set\n",
				SI_CARD(dev), SI_PORT(dev)));
			return(EBUSY);
		}
		tp = pp->sp_xtty;	/* now point to the XPRINT tty */
		oldspl = spltty();
		if (pp->sp_xpopen == 0) {
			tp->t_addr = (caddr_t)pp;
			tp->t_oproc = xp_start;
			tp->t_param = siparam;
			tp->t_line = 0;

			ttychars(tp);
			tp->t_iflag = TTYDEF_IFLAG;
			tp->t_oflag = TTYDEF_OFLAG;
			tp->t_cflag = TTYDEF_CFLAG;
			tp->t_lflag = TTYDEF_LFLAG;
			tp->t_state |= TS_CARR_ON;

			ttsetwater(tp);

			pp->sp_xpopen = 1;
		}
		error = 0;
		goto out;
#endif
	} /* end of -- if (ISXPRINT()) { */

#ifdef	POLL
	/*
	 * We've now got a device, so start the poller.
	 */
	if (init_finished == 0) {
		timeout(si_poll, (caddr_t)0L, POLL_INTERVAL);
		init_finished = 1;
	}
#endif

	oldspl = spltty();			/* Keep others out */
	
	if (tp->t_state & (TS_ISOPEN|TS_WOPEN)) {
		if (tp->t_state & TS_XCLUDE && p->p_ucred->cr_uid != 0) {
			DPRINT((pp, DBG_OPEN|DBG_FAIL,
				"already open and EXCLUSIVE set\n"));
			splx(oldspl);
			return(EBUSY);
		}
	} else {
		tp->t_dev = dev;
		DPRINT((pp, DBG_OPEN, "first open\n"));
		tp->t_addr = (caddr_t)pp;		/* link backwards */
		tp->t_oproc = si_start;
		tp->t_param = siparam;
		tp->t_line = 0;				/* default line disp */
		
		tp->t_state |= TS_WOPEN;
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = TTYDEF_CFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;

		if (pp->sp_flags & SPF_COOKWELL_ALWAYS)
			SPF_COOK_WELL(pp);
		else
			SPF_COOK_RAW(pp);
		pp->sp_state = 0;
#ifdef	SI_XPRINT
		pp->sp_xpopen = 0;
#endif
		(void) si_modem(pp, SET, TIOCM_DTR|TIOCM_RTS);
		(void) siparam(tp, &tp->t_termios);
		ttsetwater(tp);
	}

	error = 0;
	if (SPF_ISMODEM(pp) == 0) {		/* check softCAR state */
		tp->t_state |= TS_CARR_ON;
		pp->sp_state |= SS_LOPEN;
	} else {
		if (ccbp->hi_op & IP_DCD) {
			DPRINT((pp, DBG_OPEN, "modem_carr on\n"));
			tp->t_state |= TS_CARR_ON;
			ttwakeup(tp);
		} else {
			while ((flag&O_NONBLOCK) == 0 &&
			   (tp->t_cflag&CLOCAL) == 0 &&
	    		   (tp->t_state & TS_CARR_ON) == 0) {
				DPRINT((pp, DBG_OPEN, "sleeping for carrier\n"));
				tp->t_state |= TS_WOPEN;
				error = ttysleep(tp, (caddr_t)&tp->t_rawq,
						TTIPRI|PCATCH, ttopen, 0);
				if (error) {
					DPRINT((pp, DBG_OPEN|DBG_FAIL,
						"sleep broken with error %d\n",
						error));
					tp->t_state &= ~TS_WOPEN;
					(void) si_modem(pp, SET, 0);
					si_command(pp, FCLOSE, SI_NOWAIT);
					goto out;
				}
			}
		}
		pp->sp_state |= SS_MOPEN;
	}
out:
	splx(oldspl);
	
	DPRINT((pp, DBG_OPEN, "leaving siopen\n"));
	if (error == 0)
		error = (*linesw[tp->t_line].l_open)(dev, tp);
	return(error);
}

int
siclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	register struct si_port *pp;
	register struct tty *tp;
	int oldspl;

	if (ISCONTROL(dev))
		return(0);

	oldspl = spltty();

	pp = DEV2PP(dev);
	tp = pp->sp_tty;
	DPRINT((pp, DBG_ENTRY|DBG_CLOSE, "siclose(%x,%x,%x,%x)\n", 
		dev, flag, mode, p));

#ifdef	SI_XPRINT
	/*
	 * If this is a close for the XPRINT channel, just turn that off.
	 * Any buffered characters will continue to trickle out.
	 */
	if (ISXPRINT(dev)) {
		tp = pp->sp_xtty;
		(*linesw[tp->t_line].l_close)(tp, flag);
		pp->sp_xpopen = 0;
		goto ret;
	}
#endif
	(void) si_drainwait(pp, SIMSG_CLOSE, 0);

#ifdef	SI_XPRINT
	if (pp->sp_xpopen) {
		pp->sp_xtty->t_state &= ~TS_BUSY;
		(*linesw[pp->sp_xtty->t_line].l_close)(pp->sp_xtty, flag);
		pp->sp_xpopen = 0;
	}
#endif

	(*linesw[tp->t_line].l_close)(tp, flag);

	pp->sp_state &= ~SS_BUSY;

	if (tp->t_cflag&HUPCL || tp->t_state&TS_WOPEN ||
	    (tp->t_state&TS_ISOPEN) == 0) {
		(void) si_modem(pp, SET, 0);
	}
	ttyclose(tp);
	pp->sp_state = SS_CLOSED;
	si_command(pp, FCLOSE, SI_NOWAIT);

ret:
	DPRINT((pp, DBG_CLOSE|DBG_EXIT, "close done, returning\n"));
	splx(oldspl);
	return(0);
}

/*
 * User level stuff - read and write
 */
siread(dev, uio, flag)
	register dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;
	
	if (ISCONTROL(dev)) {
		DPRINT((0, DBG_ENTRY|DBG_FAIL|DBG_READ, "siread(CONTROLDEV!!)\n"));
		return(ENOTTY);
	}
	if (ISXPRINT(dev))
		return(ENOTTY);
	tp = DEV2TP(dev);
	DPRINT(((struct siport *)tp->t_addr, DBG_ENTRY|DBG_READ,
		"siread(%x,%x,%x)\n", dev, uio, flag));
	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}

siwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
{
	register struct si_port *pp;
	register struct tty *tp;
	int error = 0;
	
	if (ISCONTROL(dev)) {
		DPRINT((0, DBG_ENTRY|DBG_FAIL|DBG_WRITE, "siwrite(CONTROLDEV!!)\n"));
		return(ENOTTY);
	}
	pp = DEV2PP(dev);
	tp = pp->sp_tty;
	DPRINT((pp, DBG_WRITE, "siwrite(%x,%x,%x)\n", dev, uio, flag));
	/*
	 * If writes are currently blocked, wait on the "real" tty
	 */
	while (pp->sp_state & SS_BLOCKWRITE) {
		pp->sp_state |= SS_WAITWRITE;
		DPRINT((pp, DBG_WRITE, "in siwrite, wait for SS_BLOCKWRITE to clear\n"));
		if (error = ttysleep(tp, (caddr_t)pp, TTOPRI|PCATCH,
				     SIMSG_WRITE, 0))
			return(error);
	}

#ifdef	SI_XPRINT
	if (ISXPRINT(dev)) {
		tp = pp->sp_xtty;
		error = (*linesw[tp->t_line].l_write)(tp, uio, flag);
	} else
#endif
	{
		if (SPF_ISCOOKWELL(pp)) {
			error = (*linesw[tp->t_line].l_write)(tp, uio, flag);
		} else {
			if (tp->t_state & TS_CARR_ON)		/* Check for carrier */
				error = si_direct(pp, uio, flag);	/* Go for it */
			else
				error = EIO;
		}
	}
	return (error);
}

#ifndef	STATIC_TTY
siselect(dev, flag, p)
	dev_t dev;
	int flag;
	struct proc *p;
{
	struct si_softc *sc = sicd.cd_devs[SI_CARD(dev)];
	struct si_port *pp;

	if (SI_PORT(dev) >= sc->sc_nport)
		return(1);
	if (ISCONTROL(dev))
		return(0);
	pp = DEV2PP(dev);
#ifdef	SI_XPRINT
	if (ISXPRINT(dev))
		return (ttyselect(pp->sc_xtty, flag, p));
	else
#endif
		return (ttyselect(pp->sp_tty, flag, p));
}
#endif

siioctl(dev, cmd, data, flag, p)
	dev_t dev;
	caddr_t data;
	struct proc *p;
{
	struct si_port *pp;
	register struct tty *tp;
	int error;

	if (IS_SI_IOCTL(cmd))
		return(si_Sioctl(dev, cmd, data, flag, p));
	if (ISCONTROL(dev))
		return(ENOTTY);

	pp = DEV2PP(dev);
#ifdef	SI_XPRINT
	if (ISXPRINT(dev))
		tp = pp->sp_xtty;
	else
#endif
		tp = pp->sp_tty;

	DPRINT((pp, DBG_ENTRY|DBG_IOCTL, "siioctl(%x,%x,%x,%x)\n",
		dev, cmd, data, flag));
	/*
	 * Wait for output to drain here if there are any commands
	 * that require the port to be in a quiescent state.
	 */
	switch (cmd) {
	case TIOCSETAW: case TIOCSETAF:
	case TIOCDRAIN: case TIOCSETP:
		error = si_drainwait(pp, SIMSG_IOCTL, SS_BLOCKWRITE);
		if (error > 0)
			goto out;
	}
	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
	if (error >= 0)
		goto out;
	error = ttioctl(tp, cmd, data, flag);
	if (error >= 0)
		goto out;
	
	switch (cmd) {
	case TIOCSBRK:
		si_command(pp, SBREAK, SI_NOWAIT);
		break;
	case TIOCCBRK:
		si_command(pp, EBREAK, SI_NOWAIT);
		break;
	case TIOCSDTR:
		(void) si_modem(pp, SET, TIOCM_DTR|TIOCM_RTS);
		break;
	case TIOCCDTR:
		(void) si_modem(pp, SET, 0);
		break;
	case TIOCMSET:
		(void) si_modem(pp, SET, *(int *)data);
		break;
	case TIOCMBIS:
		(void) si_modem(pp, BIS, *(int *)data);
		break;
	case TIOCMBIC:
		(void) si_modem(pp, BIC, *(int *)data);
		break;
	case TIOCMGET:
		*(int *)data = si_modem(pp, GET, 0);
		break;
	default:
		error = ENOTTY;
	}
	error = 0;
out:
	DPRINT((pp, DBG_IOCTL|DBG_EXIT, "siioctl ret %d\n", error));
	si_write_enable(pp);
	return(error);
}
	
/*
 * Handle the Specialix ioctls. All MUST be called via the CONTROL device
 */
static int
si_Sioctl(dev_t dev, int cmd, caddr_t data, int flag, struct proc *p)
{
	struct si_softc *xsc;
	register struct si_port *xpp;
	struct si_reg *regp;
	struct si_tcsi *dp;
	BYTE *bp;
	int i, *ip, error = 0;
	int card, port;
	unsigned short *usp;

	DPRINT((0, DBG_ENTRY|DBG_IOCTL, "si_Sioctl(%x,%x,%x,%x)\n",
		dev, cmd, data, flag));
	if (!ISCONTROL(dev)) {
		DPRINT((0, DBG_IOCTL|DBG_FAIL, "not called from control device!\n"));
		return(ENODEV);
	}
	ip = (int *)data;
	switch (cmd) {
	case TCSIDOWNLOAD:
		si_download((struct si_loadcode *)data);
		return(0);
	case TCSIBOOT:
		si_boot();
		dp = (struct si_tcsi *)data;
		dp->tc_int = si_Nports;
		return(0);
	case TCSIPORTS:
		*ip = si_Nports; return(0);
	case TCSISDBG_ALL:
		si_debug = *ip; return(0);
	case TCSIGDBG_ALL:
		*ip = si_debug; return(0);
	default:
		/*
		 * Check that a controller for this port exists
		 */
		dp = (struct si_tcsi *)data;
		card = dp->tc_card;
		if (card < 0 ||
#ifdef	STATIC_TTY
		    card >= NSI ||
#endif
		    (xsc = sicd.cd_devs[card]) == NULL ||
		    xsc->sc_type == NULL)
			return(ENOENT);
		/*
		 * And check that a port exists
		 */
		port = dp->tc_port;
		if (port < 0 || port >= xsc->sc_nport)
			return(ENOENT);
		xpp = xsc->sc_ports + port;
		regp = (struct si_reg *)xsc->sc_maddr;
	}
	switch (cmd) {
	case TCSIXPON:
	case TCSIGXPON:
	case TCSIXPOFF:
	case TCSIGXPOFF:
	case TCSIXPCPS:
	case TCSIGXPCPS:
#ifdef	SI_XPRINT
		return(xp_ioctl(xsc, xpp, cmd, data, flag));
#else
		dp->tc_int = -1;
		break;
#endif
	case TCSIDEBUG:
#ifdef	SI_DEBUG
		if (error = suser(p->p_cred->pc_ucred, 0))
			return(error);
		if (xpp->sp_debug)
			xpp->sp_debug = 0;
		else {
			xpp->sp_debug = DBG_ALL;
			DPRINT((xpp, DBG_IOCTL, "debug toggled %s\n",
				(xpp->sp_debug&DBG_ALL)?"ON":"OFF"));
		}
		break;
#else
no_debug:
		return(ENODEV);
#endif
	case TCSISDBG_LEVEL:
	case TCSIGDBG_LEVEL:
#ifdef	SI_DEBUG
		if (cmd == TCSIGDBG_LEVEL)
			dp->tc_dbglvl = xpp->sp_debug;
		else
			xpp->sp_debug = dp->tc_dbglvl;
		break;
#else
		goto no_debug;
#endif
	case TCSIGRXIT:
		dp->tc_int = regp->rx_int_count;
		break;
	case TCSIRXIT:
		regp->rx_int_count = dp->tc_int;
		break;
	case TCSIGIT:
		dp->tc_int = regp->int_count;
		break;
	case TCSIIT:
		regp->int_count = dp->tc_int;
		break;
	case TCSIGMIN:
		dp->tc_int = buffer_space;
		break;
	case TCSIMIN:
		buffer_space = dp->tc_int;
		break;
	case TCSIIXANY:
		switch (dp->tc_int) {
		case -1:
			dp->tc_int = (xpp->sp_flags & SPF_IXANY)?1:0;
			break;
		case 1:
			xpp->sp_flags |= SPF_IXANY;
			break;
		case 0:
			xpp->sp_flags &= ~SPF_IXANY;
			break;
		default:
			return(EINVAL);
		}
		break;
	case TCSICOOKMODE:
		switch (dp->tc_int) {
		case -1:
			dp->tc_int = (xpp->sp_flags & SPF_COOKWELL_ALWAYS)?1:0;
			break;
		case 1:
			xpp->sp_flags |= SPF_COOKWELL_ALWAYS;
			break;
		case 0:
			xpp->sp_flags &= ~SPF_COOKWELL_ALWAYS;
			break;
		default:
			return(EINVAL);
		}
		break;
	case TCSIFLOW:
		i = dp->tc_int;
		if (i == -1)
			dp->tc_int = xpp->sp_flags & (SPF_CTSOFLOW|SPF_RTSIFLOW);
		else {
			if (i & ~(SPF_CTSOFLOW|SPF_RTSIFLOW) != 0)
				return(EINVAL);
			if (i & SPF_CTSOFLOW)
				xpp->sp_flags |= SPF_CTSOFLOW;
			else
				xpp->sp_flags &= ~SPF_CTSOFLOW;
			if (i & SPF_RTSIFLOW)
				xpp->sp_flags |= SPF_RTSIFLOW;
			else
				xpp->sp_flags &= ~SPF_RTSIFLOW;
		}
		break;
	case TCSISTATE:
		dp->tc_int = xpp->sp_ccb->hi_ip;
		break;
	case TCSIPPP:
		switch (dp->tc_int) {
		case -1:
			dp->tc_int = (xpp->sp_flags & SPF_PPP)?1:0;
			break;
		case 1:
			xpp->sp_flags |= SPF_PPP;
			break;
		case 0:
			xpp->sp_flags &= ~SPF_PPP;
			break;
		default:
			return(EINVAL);
		}
		break;
	case TCSIMODEM:
		/* anything other than 1 or 0 is a query */
		if (dp->tc_modemflag == 1) {	/* make this line a modem */
			xpp->sp_flags |= SPF_MODEM;
			xpp->sp_state &= ~SS_LOPEN;
			xpp->sp_state |= SS_MOPEN;
		} else if (dp->tc_modemflag == 0) { /* make this line local */
			xpp->sp_flags &= ~SPF_MODEM;
			xpp->sp_state &= ~SS_MOPEN;
			xpp->sp_state |= SS_LOPEN;
			if (xpp->sp_tty->t_state & TS_WOPEN) {
				xpp->sp_tty->t_state |= TS_CARR_ON;
				ttwakeup(xpp->sp_tty);
			}
		}
		if (SPF_ISMODEM(xpp))
			dp->tc_modemflag = 1;
		else
			dp->tc_modemflag = 0;
		break;
	default:
		return(EINVAL);
	}
	return(0);		/* success */
}

/*
 *	siparam()	: Configure line params
 */
int
siparam(tp, t)
	register struct tty *tp;
	register struct termios *t;
{
	register struct si_port *pp = (struct si_port *)tp->t_addr;
	register struct si_channel *ccbp;
	int oldspl, cflag, iflag, oflag, lflag;
	int error;

	DPRINT((pp, DBG_ENTRY|DBG_PARAM, "siparam(%x,%x)\n", tp, t));
	cflag = t->c_cflag;
	iflag = t->c_iflag;
	oflag = t->c_oflag;
	lflag = t->c_lflag;
	DPRINT((pp, DBG_PARAM, "OFLAG 0x%x CFLAG 0x%x IFLAG 0x%x\n",
		t->c_oflag, cflag, iflag));

	/* Wait for quiescent TX before setting parameters */
	error = si_drainwait(pp, SIMSG_PARAM, SS_BLOCKWRITE);
	if (error > 0)
		goto out;
	
        /* copy parameters to tty */
        tp->t_ispeed = t->c_ispeed;
        tp->t_ospeed = t->c_ospeed;
        tp->t_cflag = cflag;
        tp->t_iflag = iflag;
        tp->t_oflag = oflag;
        tp->t_lflag = lflag;

	/* Hangup if ospeed == 0 */
	if (t->c_ospeed == 0) {
		(void) si_modem(pp, BIC, TIOCM_DTR|TIOCM_RTS);
		goto out;
	}

	oldspl = spltty();

	ccbp = pp->sp_ccb;
	CACHEOFF
	if (iflag & IGNBRK)			/* Breaks */
		ccbp->hi_break = BR_IGN;
	else
		ccbp->hi_break = 0;
	if (iflag & BRKINT)			/* Interrupt on break? */
		ccbp->hi_break |= BR_INT;
	if (iflag & PARMRK)			/* Parity mark? */
		ccbp->hi_break |= BR_PARMRK;
	if (iflag & IGNPAR)			/* Ignore chars with parity errors? */
		ccbp->hi_break |= BR_PARIGN;

	/* Set I/O speeds - should really set I & O speeds independently */
	ccbp->hi_csr = ttspeedtab(t->c_ispeed, bdrates);

	if (cflag & CSTOPB)				/* Stop bits */
		ccbp->hi_mr2 = MR2_2_STOP;
	else
		ccbp->hi_mr2 = MR2_1_STOP;
	if (!(cflag & PARENB))				/* Parity */
		ccbp->hi_mr1 = MR1_NONE;
	else
		ccbp->hi_mr1 = MR1_WITH;
	if (cflag & PARODD)
		ccbp->hi_mr1 |= MR1_ODD;

	ccbp->hi_txon = t->c_cc[VSTART];
	ccbp->hi_txoff = t->c_cc[VSTOP];

	ccbp->hi_rxon = t->c_cc[VSTART];
	ccbp->hi_rxoff = t->c_cc[VSTOP];
							/* Bug fix for V1.10 */
	if ((cflag & CS8) == CS8) {			/* 8 data bits? */
		ccbp->hi_mr1 |= MR1_8_BITS;
		ccbp->hi_mask = 0xFF;
	} else if ((cflag & CS7) == CS7) {		/* 7 data bits? */
		ccbp->hi_mr1 |= MR1_7_BITS;
		ccbp->hi_mask = 0x7F;
	} else if ((cflag & CS6) == CS6) {		/* 6 data bits? */
		ccbp->hi_mr1 |= MR1_6_BITS;
		ccbp->hi_mask = 0x3F;
	} else {					/* Must be 5 */
		ccbp->hi_mr1 |= MR1_5_BITS;
		ccbp->hi_mask = 0x1F;
	}

	if (iflag & ISTRIP)
		ccbp->hi_mask &= 0x7F;
	
				/* Monitor DCD etc. if a modem */
	if (!(cflag & CLOCAL) && SPF_ISMODEM(pp))
		ccbp->hi_prtcl = SP_DCEN;
	else
		ccbp->hi_prtcl = 0;

	if ((iflag & IXANY) && pp->sp_flags&SPF_IXANY)
		ccbp->hi_prtcl |= SP_TANY;
	if (iflag & IXON)
		ccbp->hi_prtcl |= SP_TXEN;
	if (iflag & IXOFF)
		ccbp->hi_prtcl |= SP_RXEN;
	if (iflag & INPCK)
		ccbp->hi_prtcl |= SP_PAEN;

	/*
	 * Enable H/W RTS/CTS handshaking. The default TA/MTA is
	 * a DCE, hence the reverse sense of RTS and CTS
	 *
	 * Output - RTS must be raised before data can be sent */
	if (cflag & CCTS_OFLOW || pp->sp_flags&SPF_CTSOFLOW)
		ccbp->hi_mr2 |= MR2_RTSCONT;
	/* Input - CTS is raised when port is ready to receive data */
	if (cflag & CRTS_IFLOW || pp->sp_flags&SPF_RTSIFLOW)
		ccbp->hi_mr1 |= MR1_CTSCONT;

	/*
	 * Do cook stuff 
	 */
	if (pp->sp_flags & SPF_COOKWELL_ALWAYS) {
		SPF_COOK_WELL(pp);
	} else if (!(oflag & OPOST)) {
		SPF_COOK_RAW(pp);
	} else if (oflag & ~(OPOST|ONLCR)) {
		SPF_COOK_WELL(pp);
	} else if (oflag == (OPOST|ONLCR)) {
		SPF_COOK_MEDIUM(pp);
	} else
		SPF_COOK_RAW(pp);

	if (SPF_ISCOOKMEDIUM(pp))
		ccbp->hi_prtcl |= SP_CEN;

	if ((cflag & CLOCAL) || (ccbp->hi_op & IP_DCD)) {
		tp->t_state |= TS_CARR_ON;
		wakeup((caddr_t)&tp->t_rawq);
	}

	
	if (ccbp->hi_stat == IDLE_CLOSE)		/* Not yet open */
		si_command(pp, LOPEN, SI_WAIT);
	else
		si_command(pp, CONFIG, SI_WAIT);

	/* If the previous speed was 0, need to re-enable the modem signals */
	(void) si_modem(pp, SET, TIOCM_DTR|TIOCM_RTS);

	DPRINT((pp, DBG_PARAM, "siparam, config completed ok MR1 %x MR2 %x\n",
		ccbp->hi_mr1, ccbp->hi_mr2));	

	splx(oldspl);
	CACHEON
out:
	si_write_enable(pp);
	return(error);
}

/*
 * Wait for buffered TX characters to drain away. If <blockwrite> is
 * set prevent further buffer additions. IT IS THE RESPONSIBILITY OF
 * THE CALLER TO RE-ENABLE WRITES.
 */
static int
si_drainwait(pp, msg, blockwrite)
	register struct si_port *pp;
	char *msg;
	int blockwrite;
{
	register struct tty *tp = pp->sp_tty;
	register struct si_channel *ccbp = pp->sp_ccb;
	int error, oldspl;
	BYTE x;
	
	DPRINT((pp, DBG_ENTRY|DBG_DRAIN, "si_drainwait(%x,%s,%x)\n",
		pp, msg, blockwrite));
	error = 0;
	oldspl = spltty();
	if (blockwrite)
		pp->sp_state |= SS_BLOCKWRITE;
	CACHEOFF
	x = (int)ccbp->hi_txipos - (int)ccbp->hi_txopos;
 	while ((x != 0 && tp->t_state&TS_BUSY) || tp->t_state & TS_ASLEEP
 	       || ccbp->hi_stat == IDLE_BREAK) {
		DPRINT((pp, DBG_DRAIN, "  x %d. t_state %x sp_state %x hi_stat %x\n",
			x, tp->t_state, pp->sp_state, ccbp->hi_stat));
		CACHEON
		if (pp->sp_state & SS_BUSY) {
			error = ttysleep(tp, (caddr_t)&pp->sp_state,
					TTOPRI|PCATCH, msg, 0);
			if (error) {
				DPRINT((pp, DBG_DRAIN|DBG_FAIL,
				  "%s, wait on SS_BUSY broken by signal %d\n",
				  msg, error));
				break;
			}
		} else {
			int time = ttspeedtab(tp->t_ospeed, chartimes);

			if (time == 0 || time > x)
				time = 1;
			else
				time = x/time + 1;
			error = ttysleep(tp, (caddr_t)pp,
				TTOPRI|PCATCH, msg, time);
     			if (error != EWOULDBLOCK) {
				DPRINT((pp, DBG_DRAIN|DBG_FAIL,
				  "%s, wait for drain broken by signal %d\n",
				  msg, error));
				break;
			}
			error = 0;
		}
		CACHEOFF
		x = (int)ccbp->hi_txipos - (int)ccbp->hi_txopos;
	}
	CACHEON
	tp->t_state &= ~TS_BUSY;
	splx(oldspl);
	return(error);
}

static void
si_write_enable(pp)
	register struct si_port *pp;
{
	pp->sp_state &= ~SS_BLOCKWRITE;
	if (pp->sp_state & SS_WAITWRITE) {
		pp->sp_state &= ~SS_WAITWRITE;
		wakeup((caddr_t)pp);
	}
}

/*
 * Set/Get state of modem control lines.
 * Due to DCE-like behaviour of the adapter, some signals need translation:
 *	TIOCM_DTR	DSR
 *	TIOCM_RTS	CTS
 */
static int
si_modem(pp, cmd, bits)
	struct si_port *pp;
	enum si_mctl cmd;
	int bits;
{
	register struct si_channel *ccbp;
	int x;

	DPRINT((pp, DBG_ENTRY|DBG_MODEM, "si_modem(%x,%s,%x)\n", pp, si_mctl2str(cmd), bits));
	ccbp = pp->sp_ccb;		/* Find channel address */
	CACHEOFF
	switch (cmd) {
	case GET:
		x = ccbp->hi_ip;
		bits = TIOCM_LE;
		if (x & IP_DCD)		bits |= TIOCM_CAR;
		if (x & IP_DTR)		bits |= TIOCM_DTR;
		if (x & IP_RTS)		bits |= TIOCM_RTS;
		if (x & IP_RI)		bits |= TIOCM_RI;
		return(bits);
	case SET:
		ccbp->hi_op &= ~(OP_DSR|OP_CTS);
	case BIS:
		x = 0;
		if (bits & TIOCM_DTR)
			x |= OP_DSR;
		if (bits & TIOCM_RTS)
			x |= OP_CTS;
		ccbp->hi_op |= x;
		break;
	case BIC:
		if (bits & TIOCM_DTR)
			ccbp->hi_op &= ~OP_DSR;
		if (bits & TIOCM_RTS)
			ccbp->hi_op &= ~OP_CTS;
	}
	CACHEON
}

/*
 * Handle change of modem state
 */
static void
si_modem_state(pp, tp, hi_ip)
	register struct si_port *pp;
	register struct tty *tp;
	register int hi_ip;
{
							/* if a modem dev */
	if (hi_ip & IP_DCD) {
		if ( !(tp->t_state & TS_CARR_ON)) {
			DPRINT((pp, DBG_INTR, "modem carr on t_line %d\n",
				tp->t_line));
			(void)(*linesw[tp->t_line].l_modem)(tp, 1);
		}
	} else {
		if (tp->t_state & TS_CARR_ON) {
			DPRINT((pp, DBG_INTR, "modem carr off\n"));
			if ((*linesw[tp->t_line].l_modem)(tp, 0))
				(void) si_modem(pp, SET, 0);
		}
	}
}

/*
 * Poller to catch missed interrupts.
 */
#ifdef POLL
int
si_poll()
{
	register struct si_softc *sc;
	register int i;
	struct si_reg *regp;
	int lost, oldspl;
	
	DPRINT((0, DBG_POLL, "si_poll()\n"));
	if (in_intr)
		goto out;
	oldspl = spltty();	
	lost = 0;
	for (i=0; i<sicd.cd_ndevs; i++) {
		if ((sc = sicd.cd_devs[i]) == NULL)
			continue;
		if (sc->sc_type == NULL)
			continue;
		regp = (struct si_reg *)sc->sc_maddr;
		/*
		 * See if there has been a pending interrupt for 2 seconds
		 * or so. The test <int_scounter >= 200) won't correspond
		 * to 2 seconds if int_count gets changed.
		 */
		if (regp->int_pending != 0) {
			if (regp->int_scounter >= 200 &&
			    regp->initstat == 1) {
				printf("SLXOS si%d: lost intr\n", i);
				lost++;
			}
		} else
			regp->int_scounter = 0;
		
	}
	if (lost)
		siintr((struct si_softc *)0);	/* call intr with fake vector */
	splx(oldspl);

out:
	timeout(si_poll, (caddr_t)0L, POLL_INTERVAL);
}
#endif	/* ifdef POLL */

/*
 * The interrupt handler polls ALL ports on ALL adapters each time
 * it is called.
 */
int
siintr(Isc)
	struct si_softc *Isc;
{
	register struct si_softc *sc;
	register struct si_port *pp;
	struct si_channel *ccbp;
	register struct tty *tp;
	caddr_t maddr;
	BYTE op, ip;
	int x, card, port;
	register BYTE *z;
	BYTE c;
	static int in_poll = 0;

	
	DPRINT((0, (Isc == 0)?DBG_POLL:DBG_INTR, "siintr(0x%x)\n", Isc));
	if (in_intr != 0) {
		if (Isc == NULL)	/* should never happen */
			return(0);
		printf("SLXOS si%d: Warning interrupt handler re-entered\n",
			Isc==0 ? -1 : Isc->sc_dev.dv_unit);
		return(0);
	}
	if (Isc == NULL)
		in_poll = 1;
	in_intr = 1;
	
	/*
	 * When we get an int we poll all the channels and do ALL pending
	 * work, not just the first one we find. This allows all cards to
	 * share the same vector.
	 */
	for (card=0; card < sicd.cd_ndevs; card++) {
		if ((sc = sicd.cd_devs[card]) == NULL)
			continue;
		if (sc->sc_type == NULL)
			continue;
		switch(sc->sc_type - si_type) {
		case SIHOST :
			maddr = sc->sc_maddr;
			((struct si_reg *)maddr)->int_pending = 0;
							/* flag nothing pending */	
			*(maddr+SIINTCL) = 0x00;	/* Set IRQ clear */
			*(maddr+SIINTCL_CL) = 0x00;	/* Clear IRQ clear */
			break;
		case SIPLUS:
			maddr = sc->sc_maddr;
			((struct si_reg *)maddr)->int_pending = 0;
			*(maddr+SIPLIRQCLR) = 0x00;
			*(maddr+SIPLIRQCLR) = 0x10;
			break;
		case SIEMPTY:
		default:
			continue;
		}
	     
		for (pp = sc->sc_ports, port=0; port < sc->sc_nport;
		     pp++, port++) {
			ccbp = pp->sp_ccb;
			tp = pp->sp_tty;

			/*
			 * See if a command has completed ?
			 */
			CACHEOFF
			if (ccbp->hi_stat != pp->sp_pend) {
				DPRINT((pp, DBG_INTR,
					"siintr hi_stat = 0x%x, pend = %d\n",
					ccbp->hi_stat, pp->sp_pend));
				switch(pp->sp_pend) {
				case LOPEN:
				case MPEND:
				case MOPEN:
				case CONFIG:
					pp->sp_pend = ccbp->hi_stat;
					CACHEON
						/* sleeping in si_command */
					wakeup(&pp->sp_state);
					CACHEOFF
					break;
				default:
					pp->sp_pend = ccbp->hi_stat;
				}
	 		}
	 		CACHEON

			if (ccbp->hi_stat != IDLE_CLOSE) {
				/*
				 * Do modem state change if not a local device
				 */
				if ((pp->sp_state&SS_LOPEN) == 0 &&
				    (tp->t_state&CLOCAL) == 0) {
					CACHEOFF
					si_modem_state(pp, tp, ccbp->hi_ip);
					CACHEON
				}
				/*
				 * Do break processing
				 */
				CACHEOFF
				if (ccbp->hi_state & ST_BREAK) {
					(*linesw[tp->t_line].l_rint)(TTY_FE, tp);
					ccbp->hi_state &= ~ST_BREAK;   /* A Bit iffy this */
					DPRINT((pp, DBG_INTR, "si_intr break\n"));
				}
				/*
				 * Do RX stuff - if no carrier then
				 * dump any characters.
				 */
#ifdef	TTY_BLKINPUT
#define	TTIN(pp, tp, c)	(pp)->sp_rbuf[(pp)->sp_rcnt++] = (c);
				pp->sp_rcnt = 0;
#else
#define	TTIN(pp, tp, c)	(*linesw[(tp)->t_line].l_rint)((c), (tp))
#endif
				if ((tp->t_state & TS_CARR_ON) == 0) {
					ccbp->hi_rxopos = ccbp->hi_rxipos;
				} else
				while ((c = ccbp->hi_rxipos - ccbp->hi_rxopos) != 0 &&
				      !(pp->sp_state & SS_TBLK)) {
					op = ccbp->hi_rxopos;
					ip = ccbp->hi_rxipos;
					z = ccbp->hi_rxbuf + op;
					DPRINT((pp, DBG_INTR,
						"c = %d, op = %d, ip = %d\n",
						c, op, ip));
					if (c <= SLXOS_BUFFERSIZE - op) {
						DPRINT((pp, DBG_INTR,
							"\tsingle copy\n"));
						op += c;
						do
							TTIN(pp, tp, *z++);
						while(--c > 0);
					} else {
		        			x = SLXOS_BUFFERSIZE - op;
						op += c;
						c -= x;
						DPRINT((pp, DBG_INTR, "\tdouble part 1 %d\n", x));
						do
							TTIN(pp, tp, *z++);
						while(--x > 0);
						z = ccbp->hi_rxbuf;
						DPRINT((pp, DBG_INTR, "\tdouble part 2 %d\n", c));
						do
							TTIN(pp, tp, *z++);
						while(--c > 0);
					}
					ccbp->hi_rxopos = op;
#ifdef	TTY_BLKINPUT
					(*linesw[(tp)->t_line].l_input)(tp,
						pp->sp_rbuf, pp->sp_rcnt);
#endif

					DPRINT((pp, DBG_INTR,
						"c = %d, op = %d, ip = %d\n",
						c, op, ip));
				} /* end of RX while */
				/* 
				 * Do TX stuff
				 */
				CACHEON
				(*linesw[tp->t_line].l_start)(tp);
			} /* end if open (stat != IDLE_CLOSE) */
		} /* end of for (all ports on this controller) */
	} /* end of for (all controllers) */

	in_poll = in_intr = 0;
	DPRINT((0, (Isc==0)?DBG_POLL:DBG_INTR, "end of siintr()\n"));
	return(1);		/* say it was expected */
}

static char tbuf[SLXOS_BUFFERSIZE];
static int
si_direct(pp, uio, flag)
	struct si_port *pp;
	struct uio *uio;
{
	struct tty *tp = pp->sp_tty;
	struct si_channel *ccbp = pp->sp_ccb;
	int count, amount, oldspl, error;
	unsigned int i;
	
	DPRINT((pp, DBG_ENTRY|DBG_DIRECT, "si_direct(%x,%x,%x)\n", pp, uio, flag));
	error = 0;
	oldspl = spltty();
	while (uio->uio_resid && (tp->t_state & TS_CARR_ON) && (error == 0)) {
		CACHEOFF
		count = (int)ccbp->hi_txipos - (int)ccbp->hi_txopos;
		CACHEON
		while ((BYTE)count >= 255) {
			DPRINT((pp, DBG_DIRECT,
				"waiting for space in si_direct - count %d\n",
				(BYTE)count));
			tp->t_state |= TS_ASLEEP;
			pp->sp_state |= SS_BUSY;
			if (error = ttysleep(tp, (caddr_t)&pp->sp_state,
				     TTOPRI|PCATCH, SIMSG_DIRECT, 0)) {
				DPRINT((pp, DBG_DIRECT|DBG_FAIL,
					"direct - sleep broken by signal\n"));
				goto out;
			}
			tp->t_state &= ~TS_ASLEEP;
			pp->sp_state &= ~SS_BUSY;
			if (! (tp->t_state & TS_CARR_ON))
				goto out;
			CACHEOFF
			count = (int)ccbp->hi_txipos - (int)ccbp->hi_txopos;
			CACHEON
		}
		amount = min(uio->uio_resid, (255 - (BYTE)count));
		CACHEOFF
		i = (unsigned int)ccbp->hi_txipos;
		CACHEON
		DPRINT((pp, DBG_DIRECT, "direct: amount %d i %d\n", amount, i));
		uiomove(tbuf, amount, uio);
		if ((SLXOS_BUFFERSIZE - i) >= amount)
			/*error = uiomove((char *)&ccbp->hi_txbuf[i], amount, uio);*/
			bcopy(tbuf, (char *)&ccbp->hi_txbuf[i], amount);
		else {
			/*error = uiomove((char *)&ccbp->hi_txbuf[i], SLXOS_BUFFERSIZE-i, uio);
			if (error == 0)
				error = uiomove((char *)&ccbp->hi_txbuf[0],
					amount - (SLXOS_BUFFERSIZE-i), uio);*/
			bcopy(tbuf, (char *)&ccbp->hi_txbuf[i], SLXOS_BUFFERSIZE-i);
			bcopy(tbuf + (u_int)(SLXOS_BUFFERSIZE-i), (char *)&ccbp->hi_txbuf[0],
				amount -(SLXOS_BUFFERSIZE-i));
		}
		CACHEOFF
		ccbp->hi_txipos += amount;
		CACHEON
		tp->t_state |= TS_BUSY;
	}
out:
	splx(oldspl);
	return(error);
}

static int
si_start(tp)
	register struct tty *tp;
{
	struct si_port *pp;
	struct si_channel *ccbp;
	register struct clist *qp;
	register char *dptr;
	BYTE ipos;
	int oldspl, x, nchar, buffer_full;
	int call_lstart;
	
	oldspl = spltty();
	qp = &tp->t_outq;
	pp = (struct si_port *)tp->t_addr;
	call_lstart = buffer_full = 0;
	DPRINT((pp, DBG_ENTRY|DBG_START,
		"si_start(%x) t_state %x sp_state %x t_outq.c_cc %d\n",
		tp, tp->t_state, pp->sp_state, qp->c_cc));
#ifdef	STATIC_TTY
	if (tp->t_state & (TS_TIMEOUT|TS_TTSTOP))
#else
	if (tp->t_state & (TS_TIMEOUT|TS_TTSTOP) &&
	    (tp->t_state & (TS_XON_PEND|TS_XOFF_PEND)) == 0)
#endif
		goto out;
	ccbp = pp->sp_ccb;
	CACHEOFF
	x = (int)ccbp->hi_txipos - (int)ccbp->hi_txopos;
	CACHEON
	DPRINT((pp, DBG_START, "x %d\n", x));
	if ((x == 0) && (tp->t_state & TS_BUSY))
		tp->t_state &= ~TS_BUSY;
	if (qp->c_cc <= tp->t_lowat) {
		if (tp->t_state&TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t)qp);
		}
#ifdef	STATIC_TTY
		if (tp->t_wsel) {
			selwakeup(tp->t_wsel, tp->t_state & TS_WCOLL);
			tp->t_wsel = 0;
			tp->t_state &= ~TS_WCOLL;
		}
#else
		selwakeup(&tp->t_wsel);
#endif
	}
	/* Free buffer space ? */
	if ((BYTE) x < (SLXOS_BUFFERSIZE-buffer_space)) {
			/* Direct i/o ? */
		DPRINT((pp, DBG_START, "drained sp_state %x\n", pp->sp_state));
		if (pp->sp_state & SS_BUSY)
			wakeup(&pp->sp_state);
		pp->sp_state &= ~SS_BUSY;
	}
	nchar = qp->c_cc;
	if (tp->t_state & (TS_XON_PEND|TS_XOFF_PEND))
		nchar++;
	/* Handle the case where ttywait() is called on process exit */
	if (tp->t_session != NULL && tp->t_session->s_leader != NULL &&
	    (tp->t_session->s_leader->p_flag & SWEXIT)) {
		call_lstart++;		/* nchar == 0 */
		goto do_lstart;
	} else if (nchar == 0)
		goto out;
	CACHEOFF
	dptr = (char *)ccbp->hi_txbuf;
	ipos = (BYTE)ccbp->hi_txipos;
	while (qp->c_cc || tp->t_state & (TS_XON_PEND|TS_XOFF_PEND)) {
		CACHEOFF
		x = (int)ipos - (int)ccbp->hi_txopos;
		CACHEON
		if ((BYTE)x >= 255) {
			pp->sp_state |= SS_BUSY;
			buffer_full++;
			break;
		}
#ifndef	STATIC_TTY
		if (tp->t_state & (TS_XON_PEND|TS_XOFF_PEND)) {
			*(dptr + ipos++) = (tp->t_state & TS_XON_PEND) ?
			    tp->t_cc[VSTART] : tp->t_cc[VSTOP];
			tp->t_state &= ~(TS_XON_PEND | TS_XOFF_PEND);
			if (tp->t_state & (TS_TIMEOUT|TS_TTSTOP))
				break;
		} else
#endif
			*(dptr + ipos++) = getc(qp);
	}
	ccbp->hi_txipos = ipos;
	if ((pp->sp_state & SS_BLOCKWRITE) == 0)
		tp->t_state |= TS_BUSY;
	
	/*
	 * This nonsense is to ensure that a call to l_start, is generated
	 * smartly after the characters are transmitted. Normally the SI
	 * adapter doesn't generate interrupts until the transmit buffer has
	 * been filled. This worries upper level protocol code which relies
	 * on the side effect of another call to l_start to reset watchdog
	 * timers (e.g. 2 seconds in if_appp.c).
	 * Also needed to generate a wakeup for process exit (ttywait) 
	 * in case there is only one port active.
	 * BTW, this can fail if the * H/W handshaking gets trapped.
	 */
do_lstart:
	if (((pp->sp_flags & SPF_PPP) != 0 &&
	    buffer_full == 0 && tp->t_line != 0) || call_lstart != 0) {
		int time = ttspeedtab(tp->t_ospeed, chartimes);

		if (time > 0) {
			if (time < nchar)
				time = nchar / time;
			else
				time = 1;
			timeout(si_lstart, (caddr_t)pp, time);
		} else
			printf("SLXOS si%d: bad char time value!!\n",
				SI_CARD(tp->t_dev));
	}

out:
	splx(oldspl);
	DPRINT((pp, DBG_EXIT|DBG_START, "leave si_start()\n"));
}

static void
si_lstart(pp)
	register struct si_port *pp;
{
	register struct tty *tp;

	DPRINT((pp, DBG_ENTRY|DBG_LSTART, "si_lstart(%x) sp_state %x\n",
		pp, pp->sp_state));
	if (pp->sp_state == SS_CLOSED)
		return;
	tp = pp->sp_tty;
	/* deal with the process exit case */
	if (tp->t_state & TS_ASLEEP) {
		tp->t_state &= ~TS_ASLEEP;
		wakeup((caddr_t)&tp->t_outq);
	}
	if (tp->t_line)
		(*linesw[tp->t_line].l_start)(tp);
}

/*
 * Stop output on a line
 */
sistop(tp, flag)
	register struct tty *tp;
{
	int oldspl;

	DPRINT(((struct si_port *)tp->t_addr, DBG_ENTRY|DBG_STOP,
		"sistop(%x,%x)\n", tp, flag));
	oldspl = spltty();
	if (tp->t_state & TS_BUSY) {
		if (! tp->t_state & TS_TTSTOP) {
			tp->t_state |= TS_FLUSH;
			si_command((struct si_port *)tp->t_addr, WFLUSH, SI_NOWAIT);
			tp->t_state &= ~(TS_BUSY|TS_TTSTOP|TS_TIMEOUT);
		}
	}
	splx(oldspl);
}

static void
si_command(pp, cmd, waitflag)
	struct si_port *pp;		/* port control block (local) */
	int cmd;
	int waitflag;
{
	int oldspl;
	struct si_channel *ccbp = pp->sp_ccb;
	int x;

	DPRINT((pp, DBG_ENTRY|DBG_PARAM, "si_command(%x,%x,%d): hi_stat 0x%x\n",
		pp, cmd, waitflag, ccbp->hi_stat));

	oldspl = spltty();		/* Keep others out */

	while((x = ccbp->hi_stat) != IDLE_OPEN && x != IDLE_CLOSE && x != cmd) {
		if (in_intr) {			/* Prevent sleep in intr */
			DPRINT((pp, DBG_PARAM,
				"cmd stack - completing %d\trequested %d\n",
				x, cmd));
			splx(oldspl);
			return;
		} else if (ttysleep(pp->sp_tty, (caddr_t)&pp->sp_state, TTIPRI|PCATCH,
				SIMSG_COMMAND, 1)) {
			splx(oldspl);
			return;
		}
	}
	if (pp->sp_pend != IDLE_OPEN) {
		switch(pp->sp_pend) {
		case LOPEN:
		case MPEND:
		case MOPEN:
		case CONFIG:
			CACHEON
			wakeup(&pp->sp_state);
			CACHEOFF
			break;
		default:
			break;
		}
	}
	pp->sp_pend = cmd;		/* New command pending */
	ccbp->hi_stat = cmd;		/* Post it */

	if (waitflag) {
		if (in_intr) {		/* If in interrupt handler */
			DPRINT((pp, DBG_PARAM,
				"attempt to sleep in si_intr - cmd req %d\n",
				cmd));
			splx(oldspl);
			return;
		} else while(ccbp->hi_stat != IDLE_OPEN) {
			if (ttysleep(pp->sp_tty, (caddr_t)&pp->sp_state, TTIPRI|PCATCH,
			    SIMSG_COMMAND, 0))
				break;
		}
	}
	splx(oldspl);
}

#ifdef	SI_DEBUG
static void
si_dprintf(pp, flags, str, a1, a2, a3, a4, a5, a6)
	struct si_port *pp;
	int flags;
	char *str;
	int a1, a2, a3, a4, a5, a6;
{
	if ((pp == NULL && (si_debug&flags)) ||
	    (pp != NULL && ((pp->sp_debug&flags) || (si_debug&flags)))) {
	    	if (pp != NULL)
	    		printf("SLXOS %ci%d(%d): ", flags&DBG_XPRINT?'x':'s',
	    			SI_CARD(pp->sp_tty->t_dev),
	    			SI_PORT(pp->sp_tty->t_dev));
		printf(str, a1, a2, a3, a4, a5, a6);
	}
}

static char *
si_mctl2str(cmd)
	enum si_mctl cmd;
{
	switch (cmd) {
	case GET:	return("GET");
	case SET:	return("SET");
	case BIS:	return("BIS");
	case BIC:	return("BIC");
	}
}
#endif

#ifdef	SI_XPRINT
static void
xp_xmit(pp)
	struct si_port *pp;
{
	struct tty *xtp;
	register struct clist *qp;
	struct si_channel *ccbp;
	register char *dptr, *sptr;
	BYTE ipos;
	int oldspl, count, xpcount;

	DPRINT((pp, DBG_ENTRY|DBG_XPRINT, "xp_xmit(%x)\n", pp));
	ccbp = pp->sp_ccb;
	xtp = pp->sp_xtty;

	oldspl = spltty();

	CACHEOFF;
	if ((ccbp->hi_txipos != ccbp->hi_txopos) ||
	    (ccbp->hi_stat != IDLE_OPEN) ||
	    (pp->sp_tty->t_state & TS_ISOPEN) == 0) {
		CACHEON;
		DPRINT((pp, DBG_XPRINT, "not ready, delay\n"));
		timeout(xp_xmit, pp, hz/10);
		splx(oldspl);
		return;
	}

	dptr = (char *)ccbp->hi_txbuf;
	ipos = (BYTE)ccbp->hi_txipos;
	CACHEON
	sptr = pp->sp_xpon;
	while (*sptr)
		*(dptr + ipos++) = *(sptr++);

	xpcount = XP_COUNT(pp->sp_xpcps);
	count = 0;
	qp = &xtp->t_outq;
	DPRINT((pp, DBG_XPRINT, "want %d, xpcount %d\n", qp->c_cc, xpcount));
	while (qp->c_cc > 0 && count < xpcount) {
		*(dptr + ipos++) = getc(qp);
		count++;
	}

	/* if no characters transmitted, then print channel is done */
	if (count == 0) {
		pp->sp_state &= ~SS_XPBUSY;
		xtp->t_state &= ~TS_BUSY;
		/* hi_txipos hasn't been updated, so <xpon> won't get sent */
		if (xtp->t_state&TS_ASLEEP) {
			xtp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t)qp);
		}
		splx(oldspl);
		return;
	}

	sptr = pp->sp_xpoff;
	while (*sptr)
		*(dptr + ipos++) = *(sptr++);

	ccbp->hi_txipos = ipos;

	timeout(xp_xmit, pp, XP_DELAY(pp->sp_xpcps, count));
	splx(oldspl);
}

static int
xp_start(tp)
	struct tty *tp;
{
	struct si_port *pp = (struct si_port *)tp->t_addr;
	int oldspl;

	oldspl = spltty();
	if ((pp->sp_state & SS_XPBUSY) == 0) {
		pp->sp_state |= SS_XPBUSY;
		xp_xmit(pp);
	}
	splx(oldspl);
}

static int
xp_ioctl(sc, pp, cmd, data, flag)
	struct si_softc *sc;
	struct si_port *pp;
	int cmd, flag;
	caddr_t data;
{
	struct si_tcsi *dp = (struct si_tcsi *)data;
	char *acp, *bcp;

	switch (cmd) {
	case TCSIGXPON:
		acp = pp->sp_xpon;
		goto ret_str;
	case TCSIGXPOFF:
		acp = pp->sp_xpoff;
ret_str:
		bcp = dp->tc_string;
		while (*acp)
			*bcp++ = *acp++;
		*bcp = '\0';
		break;
	case TCSIXPON:
		acp = pp->sp_xpon;
		goto set_str;
	case TCSIXPOFF:
		acp = pp->sp_xpoff;
set_str:
		bcp = dp->tc_string;
		if (strlen(bcp) > XP_MAXSTR)
			return(ENOTTY);
		while (*bcp)
			*acp++ = *bcp++;
		*acp = '\0';
		break;
	case TCSIXPCPS:
		pp->sp_xpcps = dp->tc_int;
		break;
	case TCSIGXPCPS:
		dp->tc_int = pp->sp_xpcps;
		break;
	}
	return(0);
}
#endif	/* SI_XPRINT */
