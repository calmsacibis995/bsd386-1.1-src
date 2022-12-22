/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI:   $Id: lp.c,v 1.12 1994/01/30 04:18:05 karels Exp $
 */

/*-
 * Copyright (c) 1992, 1993 Erik Forsberg.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO EVENT SHALL I BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  PC-AT Parallel Printer Port driver.
 *  Written April 8th, 1992 for the BSDI/386 system.
 *  Please send bugs and enhancements to erik@eab.retix.com
 *  Copyright 1992, Erik Forsberg.
 */

#include "param.h"
#include "kernel.h"
#include "systm.h"
#include "buf.h"
#include "device.h"
#include "malloc.h"
#include "ioctl.h"
#include "tty.h"
#include "file.h"
#include "proc.h"

#include "isa.h"
#include "isavar.h"
#include "icu.h"

#include "lpreg.h"

/*
 * LBHIWAT is LPBSZ reduced by twice LBSZ
 * to allow for up to 2:1 expansion of output
 * during processing (newline expansion).
 */
#define LPBSZ	1024		/* Output queue size */
#define LBSZ	132		/* Small line buffer */
#define LPHIWAT	(LPBSZ-2*LBSZ)	/* Output queue high water mark */
#define LPLOWAT	LBSZ		/* Output queue low water mark */

#define LPUNIT(dev)	minor(dev)

#define SLOWPOLL	(hz)
#define FASTPOLL	(hz/20)

/* Driver status information */
struct lp_softc {
	struct  device sc_dev;  /* base device */
	struct  isadev sc_id;   /* ISA device */
	struct  intrhand sc_ih; /* intrrupt vectoring */
	int	sc_addr;	/* Base address */
	int	sc_state;	/* State flags */
	struct	selinfo sc_wsel; /* Selecting process */
	struct	clist sc_outq;	/* Output queue */
	void	*sc_hook_softc;	/* Other parallel port user data */
	int	(*sc_hook_intr)(); /* Other parallel port user intr handler */
};

#define LP_OPEN		0x01	/* Driver is OPEN */
#define LP_TOUT		0x02	/* Watchdog timer running */
#define LP_MOD		0x04	/* Interrupt happened */
#define LP_ASLP		0x08	/* Upper half blocked */
#define LP_BUSY		0x10	/* Waiting for printer to become not BUSY */
#define LP_OPEN_HOOK	0x20	/* Other lp port user active (e.g. network) */

/*
 * By default, translate newline to <CR><LF>.
 * This is disabled by setting the config-file flags to 1.
 */
#define LPRAW(flags)	((flags) & 0x01)

int lpprobe __P((struct device *, struct cfdata *, void *));
void lpattach __P((struct device *, struct device *, void *));
int lpopen __P((dev_t, int, int, struct proc *));
int lpclose __P((dev_t, int, int, struct proc *));
int lpwrite __P((dev_t, struct uio *, int));
int lpselect __P((dev_t, int, struct proc *));
int lpintr __P((struct lp_softc *));

static void lptout __P((struct lp_softc *));
static void lpstart __P((struct lp_softc *));
static void lpforceintr __P((void *));

struct cfdriver lpcd = {
	NULL, "lp", lpprobe, lpattach, sizeof(struct lp_softc)
};

extern int autodebug;

lpprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	register int ioport = ia->ia_iobase;

	/* Check if Data Latch seems to be present */
	outb(ioport+LPDATA, 0x42);
	if (inb(ioport+LPDATA) != 0x42)
		return (0);
	outb(ioport+LPDATA, 0x00);
	if (inb(ioport+LPDATA) != 0)
		return (0);

	if (ia->ia_irq == IRQUNK) {
		/*
		 * Automatic IRQ locating -- unfortunately there is no
		 * reliable way  to get an interrupt from an off-line printer.
		 * Good news is * that printer ports practically ALWAYS
		 * have standard IRQs.  We assume 7 for unit 0.  Could assume
		 * 5 for unit 1, but there is too much chance for conflict.
		 */
		ia->ia_irq = isa_discoverintr(lpforceintr, (void *) ioport);
		outb(ioport+LPCTL, NOTINIT);
		if (ffs(ia->ia_irq) == 1) {
			if (cf->cf_unit == 0) {
				printf("lp%d: assuming default irq\n",
				    cf->cf_unit);
				ia->ia_irq = IRQ7;
			} else
				return (0);
		}
	} else if (isa_irqalloc(ia->ia_irq) == 0) {
		if (autodebug)
			printf("irq not available, ");
		return (0);
	}

	ia->ia_iosize = LP_NPORTS;
	return (1);
}

void
lpforceintr(arg)
	void *arg;
{
	register int ioport = (int) arg;

	/* Should immediately cause an interrupt */
	outb(ioport+LPCTL, IE|NOTINIT|SLCT);

	/* If it didn't work, push one char into the line */
	outb(ioport+LPCTL, IE|NOTINIT|SLCT|STROBE);
	DELAY(1);
	outb(ioport+LPCTL, IE|NOTINIT|SLCT);
}

void
lpattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct lp_softc *sc = (struct lp_softc *) self;
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;

	sc->sc_addr = ia->ia_iobase;
	sc->sc_state = 0;

	/* Initialize printer (100 usec /INIT assertion) */
	outb(ia->ia_iobase+LPCTL, SLCT);
	DELAY(100);

	/* Deassert /INIT */
	outb(ia->ia_iobase+LPCTL, SLCT|NOTINIT);

	/* Initialize interrupt handler */
	printf("\n");
	isa_establish(&sc->sc_id, &sc->sc_dev);
	sc->sc_ih.ih_fun = lpintr;
	sc->sc_ih.ih_arg = (void *)sc;
	intr_establish(ia->ia_irq, &sc->sc_ih, DV_TTY);
}

lpopen(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	int unit = LPUNIT(dev);
	struct lp_softc *sc;
	register int ioport;
	int s;

	/* Validate unit number */
	if (unit >= lpcd.cd_ndevs || (sc = lpcd.cd_devs[unit]) == NULL)
		return (ENXIO);

	/* Disallow multiple opens */
	if (sc->sc_state & (LP_OPEN | LP_OPEN_HOOK))
		return (EBUSY);

	/* This checks that the printer is on-line */
	ioport = sc->sc_addr;
#if 0
	/* doesn't work with some printers */
	if ((inb(ioport+LPSTATUS) & SLCTED) == 0)
		return (ENXIO);
#endif

	/* Port is now open */
	sc->sc_state |= LP_OPEN;

	/* Allocate and initialize a clist buffer */
	sc->sc_outq.c_cc = 0;
	sc->sc_outq.c_cf = sc->sc_outq.c_cl = NULL;
	sc->sc_outq.c_cq = (char *) malloc(LPBSZ, M_CLIST, M_WAITOK);
	sc->sc_outq.c_ct = (char *) malloc(LPBSZ/NBBY, M_CLIST, M_WAITOK);
	bzero(sc->sc_outq.c_ct, LPBSZ/NBBY);
	sc->sc_outq.c_ce = (char *) (sc->sc_outq.c_cq + LPBSZ - 1);
	sc->sc_outq.c_cs = LPBSZ;

	/* Start the watchdog timer (if not yet running) */
	s = spltty();
	if ((sc->sc_state & LP_TOUT) == 0) {
		sc->sc_state &= ~LP_MOD;
		sc->sc_state |= LP_TOUT;
		timeout(lptout, sc, SLOWPOLL);
	}
	splx(s);

	/* Successful open */
	return (0);
}

lpclose(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	int s;
	struct lp_softc *sc = lpcd.cd_devs[LPUNIT(dev)];

	/*
	 * Wait for queue to become empty (not really interruptable
	 * if the process is exiting).
	 */
	s = spltty();
	while (sc->sc_outq.c_cc)  {
		sc->sc_state |= LP_ASLP;
		if (tsleep(sc, PZERO | PCATCH, "lpclos", 0))
			break;
	}

	/* Mark as not open */
	sc->sc_state &= ~LP_OPEN;
	splx(s);

	/* Release memory held by clist buffer */
	free(sc->sc_outq.c_cq, M_CLIST);
	free(sc->sc_outq.c_ct, M_CLIST);
	return(0);
}

lpwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	int n, s, error;
	char buffer[LBSZ];
	register struct lp_softc *sc = lpcd.cd_devs[LPUNIT(dev)];

	/* Loop while more data remaining to be written */
	while ((n = min(LBSZ, uio->uio_resid)) > 0) {

		/* Block writer until entire line can be written */
		s = spltty();
		while (sc->sc_outq.c_cc >= LPHIWAT) {
			sc->sc_state |= LP_ASLP;
			error = tsleep(sc, PZERO | PCATCH, "lpwrit", 0);
			if (error != 0) {
				splx(s);
				return (error);
			}
		}
		splx(s);

		/* Transfer data into a temporary kernel buffer */
		error = uiomove(buffer, n, uio);
		if (error)
			return (error);

		/* Then into the circular buffer */
		if (LPRAW(sc->sc_dev.dv_flags))
			(void) b_to_q(buffer, n, &sc->sc_outq);
		else {
			register char *p;
			register char c;

			for (p = buffer; n > 0; n--) {
				if ((c = *p++) == '\n')
					(void) putc('\r', &sc->sc_outq);
				(void) putc(c, &sc->sc_outq);
			}
		}

		/* If interrupt not pending, start printer output */
		s = spltty();
		if ((sc->sc_state & LP_BUSY) == 0)
			lpstart(sc);
		splx(s);
	}
	return (0);
}

/*
 *  The interrupt routine runs when ACK goes inactive in
 *  the interface and occasionally from the watchdog timer.
 */
lpintr(sc)
	register struct lp_softc *sc;
{
	register int ioport = sc->sc_addr;

	if (sc->sc_hook_intr)
		return ((*sc->sc_hook_intr)(sc->sc_hook_softc));

	/* If not open, ignore interrupts */
	if ((sc->sc_state & LP_OPEN) == 0)
		return (1);

	/* Shutup the watchdog */
	sc->sc_state |= LP_MOD;
	sc->sc_state &= ~LP_BUSY;

	/* Output as many characters as possible */
	lpstart(sc);

	/* Output queue may have drained somewhat */
	if (sc->sc_outq.c_cc <= LPLOWAT) {
		if (sc->sc_state & LP_ASLP) {
			sc->sc_state &= ~LP_ASLP;
			wakeup((caddr_t) sc);
		}
		selwakeup(&sc->sc_wsel);
	}
	return (1);
}

static void
lpstart(sc)
	register struct lp_softc *sc;
{
	register char ch;
	register ioport = sc->sc_addr;

	/* While characters in the output queue ... */
	while (sc->sc_outq.c_cc) {

		/* Exit print loop if printer went BUSY */
		if ((inb(ioport+LPSTATUS) & NOTBUSY) == 0) {
			sc->sc_state |= LP_BUSY;
			break;
		}

		/* Place next character in the data latch */
		ch = getc(&sc->sc_outq.c_cc);
		outb(ioport+LPDATA, ch);
		DELAY(1);

		/* Assert STROBE for 1 usec */
		outb(ioport+LPCTL, IE|NOTINIT|SLCT|STROBE);
		DELAY(1);

		/* Deassert STROBE */
		outb(ioport+LPCTL, IE|NOTINIT|SLCT);
	}
}

lpselect(dev, rw, p)
	dev_t dev;
	int rw;
	struct proc *p;
{
	int s, ret;
	register struct lp_softc *sc = lpcd.cd_devs[LPUNIT(dev)];

	/* Silly to select for Input */
	if (rw == FREAD)
		return (1);		/* go ahead and try to read! */

	/* Return true if queue almost empty */
	s = spltty();
	if (sc->sc_outq.c_cc < LPLOWAT)
		ret = 1;
	else {
		ret = 0;
		selrecord(p, &sc->sc_wsel);
	}

	splx(s);
	return (ret);
}

static void
lptout(sc)
	register struct lp_softc *sc;
{
	int s;

	/* Cancel watchdog timer if printer has been closed */
	s = spltty();
	if ((sc->sc_state & LP_OPEN) == 0) {
		sc->sc_state &= ~LP_MOD;
		sc->sc_state &= ~LP_TOUT;
		splx(s);
		return;
	}

	/* Just restart watchdog timer if activity detected */
	if ((sc->sc_state & LP_MOD) != 0) {
		sc->sc_state &= ~LP_MOD;
		timeout(lptout, sc, SLOWPOLL);
		splx(s);
		return;
	}

	/*
	 *  No activity seen, assume interrupt lost.
	 *  No error message generated as it also occasionally
	 *  happens with slow printers (especially PostScript
	 *  printers crunching some heavy duty pages).
	 */
	if (sc->sc_outq.c_cc != 0) {
		lpintr(sc);
		timeout(lptout, sc, FASTPOLL);
	} else
		timeout(lptout, sc, SLOWPOLL);
	splx(s);
}

int
lpioctl(dev, cmd, data, flag)
	dev_t dev;
	caddr_t data;
{

	return (ENOTTY);
}

int
lphook(unit, op, softc, intr)
	int unit;
	int op;
	void *softc;
	int (*intr)();
{
	struct lp_softc *sc;
	int s;

	if (unit < 0 || unit >= lpcd.cd_ndevs ||
	    (sc = lpcd.cd_devs[unit]) == NULL)
		return (0);
		
	switch (op) {
	case LPHOOK_PROBE:
		return (1);

	case LPHOOK_PORT:
		return (sc->sc_addr);

	case LPHOOK_ATTACH:
		if (sc->sc_state & (LP_OPEN | LP_OPEN_HOOK))
			return (0);
		sc->sc_state |= LP_OPEN_HOOK;
		s = spltty();
		sc->sc_hook_softc = softc;
		sc->sc_hook_intr = intr;
		splx(s);
		return (1);

	case LPHOOK_DETACH:
		/* this may get called more than once per attach */
		if (sc->sc_state & LP_OPEN_HOOK) {
			sc->sc_state &= ~LP_OPEN_HOOK;
			s = spltty();
			sc->sc_hook_softc = NULL;
			sc->sc_hook_intr = NULL;
			splx(s);
		}
		return (1);

	default:
		return (0);
	}
}
