/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: bms.c,v 1.6 1993/12/23 04:22:29 karels Exp $
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
 *  Microsoft Bus Mouse driver.
 *  Written April 26th, 1992 for the BSDI/386 system.
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
#include "vnode.h"
#include "bmsvar.h"

#include "isavar.h"
#include "icu.h"
#include "machine/cpu.h"

#define ADDR	0	/* Offset for register select */
#define DATA	1	/* Offset for InPort data */
#define IDENT	2	/* Offset for identification register */

#define BMSUNIT(dev)	(minor(dev) & 1)

struct bms_softc {	/* Driver status information */
	struct	device sc_dev;	/* base device */
	struct	isadev sc_id;	/* ISA device */
	struct	intrhand sc_ih;	/* interrupt handler */
	int	sc_addr;	/* I/O port base */
	struct	clist inq;	/* Input queue */
	struct	selinfo rsel;	/* Process selecting for Input */
	u_char	state;		/* Mouse driver state */
	u_char	status;		/* Mouse button status */
	int	x, y;		/* accumulated motion in the X,Y axis */
};

int	bmsprobe __P((struct device *parent, struct cfdata *cf, void *aux));
void	bmsattach __P((struct device *parent, struct device *dev, void *args));
int	bmsopen __P((dev_t, int, int, struct proc *));
int	bmsclose __P((dev_t, int, int, struct proc *));
int	bmsread __P((dev_t, struct uio *, int));
int	bmsioctl __P((dev_t, int, caddr_t, int, struct proc *));
int	bmsselect __P((dev_t, int, struct proc *));
int	bmsintr __P((struct bms_softc *));


#define MSBSZ	1020		/* Output queue size (multiple of 5 please) */

#define OPEN	1		/* Device is open */
#define ASLP	2		/* Waiting for mouse data */

struct cfdriver bmscd =
	{ NULL, "bms", bmsprobe, bmsattach, sizeof(struct bms_softc) };

/* ARGSUSED */
bmsprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;

	/* Read identification register to see if present */

	if (inb(ia->ia_iobase+IDENT) != 0xDE)
		return (0);

	/* Seems it was there, reset */

	outb(ia->ia_iobase+ADDR, 0x87);

	if (ia->ia_irq == IRQUNK)
		return (0);	/* not much good without an interrupt vec */

	return (1);
}

/* ARGSUSED */
void
bmsattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	register struct bms_softc *sc = (struct bms_softc *) self;

	printf("\n");
	/* Save I/O base address */
	sc->sc_addr = ia->ia_iobase;

	/* Setup initial state */
	sc->state = 0;

	isa_establish(&sc->sc_id, &sc->sc_dev);
	sc->sc_ih.ih_fun = bmsintr;
	sc->sc_ih.ih_arg = (void *)sc;
	intr_establish(ia->ia_irq, &sc->sc_ih, DV_TTY);
}

bmsopen(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	register int unit = BMSUNIT(dev);
	register struct bms_softc *sc;
	register int ioport;

	/* Validate unit number */

	if (unit >= bmscd.cd_ndevs || (sc = bmscd.cd_devs[unit]) == NULL)
		return (ENXIO);

	/* Disallow multiple opens */

	if (sc->state & OPEN)
		return (EBUSY);

	/* Initialize state */

	sc->state |= OPEN;
	sc->status = 0;
	sc->x = 0;
	sc->y = 0;

	/* Allocate and initialize a clist buffer */

	sc->inq.c_cc = 0;
	sc->inq.c_cf = sc->inq.c_cl = NULL;
	sc->inq.c_cq = (char *) malloc(MSBSZ, M_CLIST, M_WAITOK);
	sc->inq.c_ct = (char *) malloc(MSBSZ/NBBY, M_CLIST, M_WAITOK);
	bzero(sc->inq.c_ct, MSBSZ/NBBY);
	sc->inq.c_ce = (char *) (sc->inq.c_cq + MSBSZ - 1);
	sc->inq.c_cs = MSBSZ;

	/* Setup Bus Mouse */

	ioport = sc->sc_addr;
	outb(ioport+ADDR, 7);
	outb(ioport+DATA, 0x09);

	/* Successful open */

	return (0);
}

bmsclose(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	register int unit = BMSUNIT(dev);
	register int ioport;
	register struct bms_softc *sc = bmscd.cd_devs[unit];

	/* Reset Bus Mouse */

	ioport = sc->sc_addr;
	outb(ioport+ADDR, 0x87);

	/* Complete the close */

	sc->state &= ~OPEN;

	/* Release memory held by clist buffer */

	free(sc->inq.c_cq, M_CLIST);
	free(sc->inq.c_ct, M_CLIST);

	/* close is almost always successful */

	return (0);
}

bmsread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	int s, error;
	unsigned length;
	struct bms_softc *sc = bmscd.cd_devs[BMSUNIT(dev)];
	unsigned char buffer[100];

	/* Block until mouse activity occured */

	s = spltty();
	while (sc->inq.c_cc <= 0) {
		if (flag & IO_NDELAY) {
			splx(s);
			return (EWOULDBLOCK);
		}
		sc->state |= ASLP;
		error = tsleep(sc, PZERO | PCATCH, "bmsin", 0);
		if (error != 0) {
			splx(s);
			return (error);
		}
	}

	/* Transfer as many chunks as possible */

	while (sc->inq.c_cc > 0 && uio->uio_resid > 0) {
		length = min(sc->inq.c_cc, uio->uio_resid);
		if (length > sizeof(buffer))
			length = sizeof(buffer);

		/* Remove a small chunk from input queue */

		(void) q_to_b(&sc->inq, buffer, length);

		/* Copy data to user process */

		error = uiomove(buffer, length, uio);
		if (error)
			break;
		}

	/* Allow interrupts again */

	splx(s);
	return (error);
}

bmsioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	caddr_t addr;
	int cmd, flag;
	struct proc *p;
{
	struct bms_softc *sc = bmscd.cd_devs[BMSUNIT(dev)];
	struct mouseinfo info;
	int s, error;

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

bmsintr(sc)
	struct bms_softc *sc;
{
	int ioport = sc->sc_addr;
	char buffer[5];
	char x, y, sts;

	/* Freeze InPort registers */

	outb(ioport+ADDR, 7);
	outb(ioport+DATA, 0x29);

	/* Read mouse status */

	outb(ioport+ADDR, 0);
	sts = inb(ioport+DATA);

	/* Check if any movement detected */

	if (sts & 0x40) {
		outb(ioport+ADDR, 1);
		x = inb(ioport+DATA);
		if (x == -128)
			x = -127;
		outb(ioport+ADDR, 2);
		y = inb(ioport+DATA);
                if (y == -128)
                        y = 127;
                else
                        y = -y;
	} else {
		x = 0;
		y = 0;
	}

	/* Unfreeze InPort Registers (re-enables interrupts) */

	outb(ioport+ADDR, 7);
	outb(ioport+DATA, 0x09);

	/* Update accumulated movements */

	sc->x += x;
	sc->y += y;

	/* Inclusive OR status changes, but always save only last state */

	sc->status |= sts & BUTCHNGMASK;
	sc->status = (sc->status & ~BUTSTATMASK) | (sts & BUTSTATMASK);

	/* If device in use and any change occurred ... */

	if (sc->state & OPEN && sts & 0x78) {
		sts &= BUTSTATMASK;
		buffer[0] = 0x80 | (sts ^ BUTSTATMASK);
		buffer[1] = x;
		buffer[2] = y;
		buffer[3] = 0;
		buffer[4] = 0;
		(void) b_to_q(buffer, 5, &sc->inq);
		if (sc->state & ASLP) {
			sc->state &= ~ASLP;
			wakeup(sc);
		}
		selwakeup(&sc->rsel);
	}

	return (1);
}

bmsselect(dev, rw, p)
	dev_t dev;
	int rw;
	struct proc *p;
{
	int s, ret;
	struct bms_softc *sc = bmscd.cd_devs[BMSUNIT(dev)];

	/* Silly to select for Output */

	if (rw == FWRITE)
		return (1);

	/* Return true if a mouse event available */

	s = spltty();
	if (sc->inq.c_cc > 0)
		ret = 1;
	else {
		ret = 0;
		selrecord(p, &sc->rsel);
	}

	splx(s);
	return (ret);
}
