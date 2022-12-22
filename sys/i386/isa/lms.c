/*	BSDI $Id: lms.c,v 1.4 1993/12/23 04:22:46 karels Exp $	*/

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
 * Logitech Bus Mouse driver.
 * Written April 26th, 1992 for the BSDI/386 system.
 * Please send bugs and enhancements to erik@eab.retix.com
 * Copyright 1992, Erik Forsberg.
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
#include "vnode.h"
#include "bmsvar.h"

#include "isavar.h"
#include "icu.h"
#include "machine/cpu.h"

#define DATA	0		/* Offset for data port, read-only */
#define SIGN	1		/* Offset for signature port, read-write */
#define INTR	2		/* Offset for interrupt port, read-only */
#define CNTRL	2		/* Offset for control port, write-only */
#define CONFIG	3		/* Offset for configuration port, read-write */

#define LMSUNIT(dev)	(minor(dev) & 1)

struct lms_softc {		/* Driver status information */
	struct device sc_dev;	/* base device */
	struct isadev sc_id;	/* ISA device */
	struct intrhand sc_ih;	/* interrupt handler */
	int sc_addr;		/* I/O port base */
	struct clist inq;	/* Input queue */
	struct selinfo rsel;	/* Process selecting for Input */
	unsigned char state;	/* Driver state */
	unsigned char status;	/* Mouse button status */
	unsigned char button;	/* Previous mouse button status bits */
	int x, y;		/* accumulated motion in the X,Y axis */
};

extern int lmsprobe __P ((struct device *, struct cfdata *, void *));
extern void lmsattach __P ((struct device *, struct device *, void *));
extern int lmsopen __P ((dev_t, int, int, struct proc *));
extern int lmsclose __P ((dev_t, int, int, struct proc *));
extern int lmsread __P ((dev_t, struct uio *, int));
extern int lmsioctl __P ((dev_t, int, caddr_t, int, struct proc *));
extern int lmsselect __P ((dev_t, int, struct proc *));
extern int lmsintr __P ((struct lms_softc *));

#define MSBSZ	1020		/* Output queue size (multiple of 5 please) */

#define OPEN	1		/* Device is open */
#define ASLP	2		/* Waiting for mouse data */

struct cfdriver lmscd =
	{ NULL, "lms", lmsprobe, lmsattach, sizeof(struct lms_softc) };

int
lmsprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	int val;
	struct isa_attach_args *ia = (struct isa_attach_args *) aux;

	/* Configure and check for port present */

	outb(ia->ia_iobase + CONFIG, 0x91);
	DELAY(10);
	outb(ia->ia_iobase + SIGN, 0x0C);
	DELAY(10);
	val = inb(ia->ia_iobase + SIGN);
	DELAY(10);
	outb(ia->ia_iobase + SIGN, 0x50);

	/* Check if something is out there */

	if (val != 0x0C || inb(ia->ia_iobase + SIGN) != 0x50)
		return (0);

	/* not much good without an interrupt vec */

	if (ia->ia_irq == IRQUNK)
		return (0);

	/* Present */

	return (1);
}

void
lmsattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	struct lms_softc *sc = (struct lms_softc *) self;

	printf("\n");

	/* Save I/O base address */

	sc->sc_addr = ia->ia_iobase;

	/* Disable mouse interrupts */

	outb(sc->sc_addr + CNTRL, 0x10);

	/* Setup initial state */

	sc->state = 0;

	isa_establish (&sc->sc_id, &sc->sc_dev);
	sc->sc_ih.ih_fun = lmsintr;
	sc->sc_ih.ih_arg = (void *) sc;
	intr_establish (ia->ia_irq, &sc->sc_ih, DV_TTY);
}

int
lmsopen(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	int unit = LMSUNIT(dev);
	struct lms_softc *sc;

	/* Validate unit number */

	if (unit >= lmscd.cd_ndevs || (sc = lmscd.cd_devs[unit]) == NULL)
		return (ENXIO);

	/* Disallow multiple opens */

	if (sc->state & OPEN)
		return (EBUSY);

	/* Initialize state */

	sc->state |= OPEN;
	sc->status = 0;
	sc->button = 0;
	sc->x = 0;
	sc->y = 0;

	/* Allocate and initialize a clist buffer */

	sc->inq.c_cc = 0;
	sc->inq.c_cf = sc->inq.c_cl = NULL;
	sc->inq.c_cq = (char *) malloc(MSBSZ, M_CLIST, M_WAITOK);
	sc->inq.c_ct = (char *) malloc(MSBSZ / NBBY, M_CLIST, M_WAITOK);
	bzero(sc->inq.c_ct, MSBSZ / NBBY);
	sc->inq.c_ce = (char *) (sc->inq.c_cq + MSBSZ - 1);
	sc->inq.c_cs = MSBSZ;

	/* Enable Bus Mouse interrupts */

	outb(sc->sc_addr + CNTRL, 0);

	/* Successful open */

	return (0);
}

int
lmsclose(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	int unit, ioport;
	struct lms_softc *sc;

	/* Get unit and associated info */

	unit = LMSUNIT(dev);
	sc = lmscd.cd_devs[unit];
	ioport = sc->sc_addr;

	/* Disable further Mouse interrupts */

	outb(ioport + CNTRL, 0x10);

	/* Complete the close */

	sc->state &= ~OPEN;

	/* Release memory held by clist buffer */

	free (sc->inq.c_cq, M_CLIST);
	free (sc->inq.c_ct, M_CLIST);

	/* close is almost always successful */

	return (0);
}

int
lmsread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	int s, error;
	unsigned length;
	struct lms_softc *sc;
	unsigned char buffer[100];

	/* Get device information */

	sc = lmscd.cd_devs[LMSUNIT(dev)];

	/* Block until mouse activity occured */

	s = spltty ();
	while (sc->inq.c_cc == 0) {
		if (flag & IO_NDELAY) {
			splx(s);
			return (EWOULDBLOCK);
		}
		sc->state |= ASLP;
		error = tsleep (sc, PZERO | PCATCH, "lmsin", 0);
		if (error != 0) {
			splx(s);
			return (error);
		}
	}

	/* Transfer as many chunks as possible */

	while (sc->inq.c_cc > 0 && uio->uio_resid > 0) {
		length = min (sc->inq.c_cc, uio->uio_resid);
		if (length > sizeof (buffer))
			length = sizeof (buffer);

		/* Remove a small chunk from input queue */

		(void) q_to_b (&sc->inq, buffer, length);

		/* Copy data to user process */

		error = uiomove (buffer, length, uio);
		if (error)
			break;
	}

	/* Allow interrupts again */

	splx(s);
	return (error);
}

int
lmsioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	caddr_t addr;
	int cmd, flag;
	struct proc *p;
{
	struct lms_softc *sc;
	struct mouseinfo info;
	int s, error;

	/* Get device information */

	sc = lmscd.cd_devs[LMSUNIT(dev)];

	/* Perform IOCTL command */

	switch (cmd) {

	case MOUSEIOCREAD:

		/* Dont modify info while calculating */

		s = spltty();

		/* Build mouse status octet */

		info.status = sc->status;
		if (sc->x || sc->y)
			info.status |= MOVEMENT;

		/* Encode X and Y motion as good as we can */

		if (sc->x > 127)
			info.xmotion = 127;
		else if (sc->x < -128)
			info.xmotion = -128;
		else
			info.xmotion = sc->x;

		if (sc->y > 127)
			info.ymotion = 127;
		else if (sc->y < -128)
			info.ymotion = -128;
		else
			info.ymotion = sc->y;

		/* Reset historical information */

		sc->x = 0;
		sc->y = 0;
		sc->status &= ~BUTCHNGMASK;

		/* Allow interrupts and copy result buffer */

		splx(s);
		error = copyout(&info, addr, sizeof(struct mouseinfo));
		break;

	default:
		error = EINVAL;
		break;
	}

	/* Return error code */

	return (error);
}

int
lmsintr(sc)
	struct lms_softc *sc;
{
	int ioport = sc->sc_addr;
	char buffer[5];
	char hi, lo;
	char buttons;
	char changed;
	char dx, dy;

	outb(ioport + CNTRL, 0xAB);
	hi = inb(ioport + DATA) & 15;
	outb(ioport + CNTRL, 0x90);
	lo = inb(ioport + DATA) & 15;
	dx = (hi << 4) | lo;
	outb(ioport + CNTRL, 0xF0);
	hi = inb(ioport + DATA);
	outb(ioport + CNTRL, 0xD0);
	lo = inb(ioport + DATA);
	outb(ioport + CNTRL, 0);

	dy = ((hi & 15) << 4) | (lo & 15);
	if (dy == -128)
		dy = 127;
	else
		dy = -dy;
	buttons = (~hi >> 5) & 7;
	changed = buttons ^ sc->button;
	sc->button = buttons;
	sc->status = buttons | (sc->status & ~BUTSTATMASK) | (changed << 3);

	/* Update accumulated motions */

	sc->x += dx;
	sc->y += dy;

	/* Return if not open or no changes */

	if ((sc->state & OPEN) == 0 || (dx == 0 && dy == 0 && changed == 0))
		return (1);

	/* Build a Mouse event record */

	buffer[0] = (buttons ^ BUTSTATMASK) | 0x80;
	buffer[1] = dx;
	buffer[2] = dy;
	buffer[3] = 0;
	buffer[4] = 0;
	(void) b_to_q (buffer, 5, &sc->inq);
	if (sc->state & ASLP) {
		sc->state &= ~ASLP;
		wakeup (sc);
	}
	selwakeup(&sc->rsel);
	return (1);
}

int
lmsselect(dev, rw, p)
	dev_t dev;
	int rw;
	struct proc *p;
{
	int s, ret;
	struct lms_softc *sc = lmscd.cd_devs[LMSUNIT(dev)];

	/* Silly to select for Output */

	if (rw == FWRITE)
		return (1);

	/* Return true if a mouse event available */

	s = spltty();
	if (sc->inq.c_cc != 0)
		ret = 1;
	else {
		ret = 0;
		selrecord(p, &sc->rsel);
	}

	splx(s);
	return (ret);
}
