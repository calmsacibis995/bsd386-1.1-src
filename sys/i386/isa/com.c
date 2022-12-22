/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: com.c,v 1.25 1994/01/12 19:58:16 karels Exp $
 */

/*-
 * Copyright (c) 1991 The Regents of the University of California.
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
 *	@(#)com.c	7.5 (Berkeley) 5/16/91
 */

/*
 * COM driver, based on HP dca driver
 * uses National Semiconductor NS16450/NS16550AF UART
 *
 * Includes modifications based on those of
 * brad huntting <huntting@glarp.com> and others
 * for CTS, RTR (RTS for receive), etc.
 *
 * Also supports muxtiplexed interrupt cards (AST-4, MU-440, etc).
 */
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
#include "malloc.h"
#include "stat.h"
#include "syslog.h"
#include "device.h"

#include "isavar.h"
#include "comreg.h"
#include "icu.h"
#include "ic/ns16550.h"

#include "machine/cpu.h"

#ifndef	FIFO_TRIGGER
#define FIFO_TRIGGER	FIFO_TRIGGER_4
#endif

struct com_softc {
	struct	device sc_dev;  /* base device */
	struct	isadev sc_id;   /* ISA device */
	struct	intrhand sc_ih; /* interrupt vectoring */
	int	sc_addr;	/* i/o base port */
	int	sc_flags;	/* below */
	int	sc_mstat;	/* modem status */
	struct	tty *sc_tty;	/* tty state */
	struct	ttydevice_tmp sc_ttydev;	/* tty stuff */

	/* support for muxtiplexed interrupt cards (AST-4, MU-440, etc) */
	int	sc_mcr_ienable; /* MCR_IENABLE for normal ports, 0 for mux */
	int	sc_irq;		/* copy of config irq */
	struct	com_softc *sc_next; /* chain of ports on same mux card */
	int	sc_overflows;	/* count of receive silo overflows */
	int	sc_txlost;	/* count of lost transmit interrupts */
	struct	atshutdown sc_shutdown;		/* shutdown for port */
};

/* sc_flags */
#define	SC_HASFIFO	0x01	/* is 16550 with fifo */
#define	SC_KGDB		0x02	/* is kgdb port */
#define	SC_TTYOPEN	0x04	/* standard tty device is open */
#define	SC_CNOPEN	0x08	/* console device is open */

int 	comprobe(), comintr(), comstart(), comparam();
void	comattach();
void	comshutdown __P((void *));
int	comcngetc(), comcnputc();
static	struct com_softc *commuxbase __P((int muxgroup, int irq));
void	commuxattach __P((struct com_softc *sc, struct com_softc *basesc));

struct cfdriver comcd =
	{ NULL, "com", comprobe, comattach, sizeof(struct com_softc) };

extern	int comconsole;
extern	int com_cnaddr;		/* console base address */
int	comdefaultrate = TTYDEF_SPEED;	/* for console, kgdb */
struct	tty com_cntty;
int	commajor;

extern	struct tty *constty;
#ifdef KGDB
#include "machine/remote-sl.h"

extern int kgdb_dev;
extern int kgdb_rate;
extern int kgdb_debug_init;
#endif
int	com_getc __P((int));
void	com_putc __P((int, int));

#define	UNIT(x)		minor(x)

#define	MUXFLAGS(flags)		(flags)
#define	ISMUX(flags)		((flags) != 0)
#define	MUXPORT(flags)		((flags) & 0x3ff)
#define	MUXGROUP(flags)		((flags) & 0xffff)
#define	MUXSUBUNIT(flags)	(((flags) >> 16) & 0xf)

/* ARGSUSED */
comprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	void comforceintr();
	int i, stat;
	int muxflags;

	/* force access to id reg */
	outb(ia->ia_iobase+com_cfcr, 0);
	outb(ia->ia_iobase+com_iir, 0);
	for (i = 0; i < 17; i++) {
		if ((inb(ia->ia_iobase+com_iir) & 0x38) == 0)
			goto ok;
		(void) inb(ia->ia_iobase+com_data);	/* clear pending recv */
	}
	return (0);
ok:
	ia->ia_iosize = COM_NPORT;

	muxflags = MUXFLAGS(cf->cf_flags);

	if (ISMUX(muxflags)) {
		if (MUXSUBUNIT(muxflags) != 0) {
			if (commuxbase(MUXGROUP(muxflags), 0) == 0)
				return (0);
			if (MUXPORT(muxflags) != 0) {
				ia->ia_irq = 0;
				return (1);
			}
		}
		if (MUXPORT(muxflags))
			outb(MUXPORT(muxflags), MUX_IENABLE);
	}

	i = 1;
	if (ia->ia_irq == IRQUNK) {
		ia->ia_aux = cf;
		ia->ia_irq = isa_discoverintr(comforceintr, (void *)ia);
		stat = inb(ia->ia_iobase+com_iir); 	/* clear intr */
		outb(ia->ia_iobase+com_mcr, MCR_INTROFF);
		outb(ia->ia_iobase+com_ier, 0);
		if (ffs(ia->ia_irq) - 1 == 0) {
			printf("com%d (%x): no intr\n", cf->cf_unit,
			    ia->ia_iobase);
			i = 0;
		}
	}
	/* Need to reinitialize if this is the console */
	if (comconsole != -1 && ia->ia_iobase == com_cnaddr)
		cominit(com_cnaddr, comdefaultrate);
	if (MUXSUBUNIT(muxflags) != 0 &&
	    commuxbase(MUXGROUP(muxflags), ia->ia_irq) != 0)
	    	ia->ia_irq = 0;		/* suppress warnings */
	return (i);
}

void
comforceintr(ia)
	struct isa_attach_args *ia;
{
	register int com;
	struct cfdata *cf = ia->ia_aux;
	int flags;

	com = ia->ia_iobase;

        /*
         * Set up clock rate divisor -- if ports weren't
         * initialized by BIOS this register can hold
         * an arbitrary value, probably disabling the chip.
         */
        outb(com+com_cfcr, CFCR_DLAB);
        outb(com+com_dlbl, COMTICK/115200);
        outb(com+com_dlbh, (COMTICK/115200) >> 8);
        outb(com+com_cfcr, CFCR_8BITS);

        /*
         * Clear pending status reports.
         */
        (void) inb(com+com_iir);
        (void) inb(com+com_lsr);
        (void) inb(com+com_msr);

	/* 
	 * On AST style boards that have the "compatibility mode" feature
	 * (the MU-440 for sure, and perhaps all AST derived boards),
	 * we have to be sure that MCR_IENABLE is off.  Otherwise, we
	 * will route the interrupt lines from the first two uarts to
	 * IRQ 4 and 3, in addition to whatever line is selected for
	 * the multiplexed interrupt.
	 */
	flags = MUXFLAGS(cf->cf_flags);
	if (ISMUX(flags) && MUXPORT(flags))
		outb(com+com_mcr, MCR_INTROFF);
	else
		outb(com+com_mcr, MCR_IENABLE);
	outb(com+com_ier, IER_ETXRDY | IER_ERLS);

	/*
	 * Wait briefly for an interrupt.  If we don't get one, this
	 * might be one of the clone chips that do not do transmit-ready
	 * interrupts when first enabled, only when a transmit completes.
	 * We transmit 0xff at 115.2 bkps to minimize the chance
	 * that anything on the line will notice.
	 */
	DELAY(100);
	if (!isa_got_intr())
		outb(com+com_data, 0xff);
}

/* ARGSUSED */
void
comattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	register struct com_softc *sc = (struct com_softc *) self;
	struct com_softc *basesc;
	u_char unit;
	int port = ia->ia_iobase;

	sc->sc_addr = port;
	unit = sc->sc_dev.dv_unit;
	if (unit == comconsole)
		DELAY(100000);

	/* look for a NS 16550AF UART with FIFOs */
	outb(port+com_fifo, FIFO_ENABLE|FIFO_RCV_RST|FIFO_XMT_RST|FIFO_TRIGGER);
	DELAY(100);
	if ((inb(port+com_iir) & IIR_FIFO_MASK) == IIR_FIFO_MASK) {
		sc->sc_flags |= SC_HASFIFO;
		printf(": buffered");
	}
	printf("\n");
	sc->sc_shutdown.func = comshutdown;
	sc->sc_shutdown.arg = (void *)sc;
	atshutdown(&sc->sc_shutdown, ATSH_ADD);

	if (ISMUX(sc->sc_dev.dv_flags) && MUXPORT(sc->sc_dev.dv_flags))
		sc->sc_mcr_ienable = MCR_INTROFF;
	else {
		sc->sc_mcr_ienable = MCR_IENABLE;
#ifndef COM_SHARE_IRQ
		outb(port+com_mcr, sc->sc_mcr_ienable);
#endif
	}
	/*
	 * If we are a MUX subunit other than 0, or if we share an irq
	 * with a previously-configured line, attach to the base unit
	 * for interrupt processing.  We need to process all interrupts
	 * from all devices on the same irq to avoid losing interrupts.
	 */
	if (basesc = commuxbase(MUXGROUP(sc->sc_dev.dv_flags), ia->ia_irq)) {
		if (basesc == sc)
			basesc = NULL;
		else
			commuxattach(sc, basesc);
	}
	sc->sc_irq = ia->ia_irq;

#ifdef KGDB
	if (kgdb_dev == makedev(commajor, unit)) {
		if (comconsole == unit)
			kgdb_dev = -1;	/* can't debug over console port */
		else {
			sc->sc_flags |= SC_KGDB;
#ifdef COM_SHARE_IRQ
			outb(port+com_mcr, sc->sc_mcr_ienable);
#endif
			(void) cominit(sc->sc_addr, kgdb_rate);
			kgdb_attach(com_getc, com_putc, sc->sc_addr);
			if (kgdb_debug_init)
				kgdb_connect(1);
			else
				printf("com%d: kgdb enabled\n", unit);
		}
	}
#endif
	if (unit == comconsole || (comconsole == -1 && unit == 0))
		sc->sc_tty = &com_cntty;
	else {
		sc->sc_tty = malloc(sizeof(struct tty), M_DEVBUF, M_WAITOK);
		bzero(sc->sc_tty, sizeof(struct tty));
	}

	isa_establish(&sc->sc_id, &sc->sc_dev);

	if (basesc == NULL) {
		sc->sc_ih.ih_fun = comintr;
		sc->sc_ih.ih_arg = (void *)sc;
		intr_establish(ia->ia_irq, &sc->sc_ih, DV_TTY);
	}
	strcpy(sc->sc_ttydev.tty_name, comcd.cd_name);
	sc->sc_ttydev.tty_unit = unit;
	sc->sc_ttydev.tty_base = unit;
	sc->sc_ttydev.tty_count = 1;
	sc->sc_ttydev.tty_ttys = sc->sc_tty;
	tty_attach(&sc->sc_ttydev);
}

/*
 * Find "base" port for multiplexor group.
 * For AST-type multiplexors, this is subunit 0 with the same MUXPORT register.
 * For other cards, this is the first port with the same IRQ.
 */
static struct com_softc *
commuxbase(muxgroup, irq)
	int muxgroup, irq;
{
	int baseunit;
	int basemux;
	struct com_softc *basesc;

	for (baseunit = 0; baseunit < comcd.cd_ndevs; baseunit++) {
		if ((basesc = comcd.cd_devs[baseunit]) == NULL)
			continue;
		basemux = basesc->sc_dev.dv_flags;

		if (irq && basesc->sc_irq == irq)
			return (basesc);
		if (!ISMUX(basemux) || MUXSUBUNIT(basemux) != 0)
			continue;

		if (MUXGROUP(basemux) == muxgroup)
			return (basesc);
	}
	return (NULL);
}

void
commuxattach(sc, basesc)
	struct com_softc *sc, *basesc;
{

	while (basesc->sc_next)
		basesc = basesc->sc_next;
	basesc->sc_next = sc;
}

/*
 * Shutting down system, we disable interrupts and
 * disable the fifo on buffered UARTs.
 */
void
comshutdown(arg)
	void *arg;
{
	int com = ((struct com_softc *)arg)->sc_addr;

	outb(com+com_mcr, MCR_OFF | MCR_INTROFF);
	outb(com+com_ier, 0);
	outb(com+com_fifo, 0);
}

/* ARGSUSED */
#ifdef __STDC__
comopen(dev_t dev, int flag, int mode, struct proc *p)
#else
comopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
#endif
{
	register struct com_softc *sc;
	register struct tty *tp;
	register int unit;
	int s, error = 0;

	unit = UNIT(dev);
	if (unit >= comcd.cd_ndevs || (sc = comcd.cd_devs[unit]) == NULL)
		return (ENXIO);
	tp = sc->sc_tty;
	tp->t_oproc = comstart;
	tp->t_param = comparam;
	tp->t_dev = dev;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		tp->t_state |= TS_WOPEN;
		if (tp->t_ispeed == 0) {
			tp->t_termios = deftermios;
			/* Make sure console is always "hardwired." */
			if (unit == comconsole)
				 tp->t_cflag |= CLOCAL;
		} else
			ttychars(tp);
		comparam(tp, &tp->t_termios);
		ttsetwater(tp);
	} else if (tp->t_state&TS_XCLUDE && p->p_ucred->cr_uid != 0)
		return (EBUSY);
	s = spltty();
	outb(sc->sc_addr+com_mcr, MCR_ON | sc->sc_mcr_ienable);
#ifdef COM_SHARE_IRQ
	comintr(sc);	/* make sure that interrupt line goes high */
#endif
	DELAY(1000);
	if ((sc->sc_mstat = inb(sc->sc_addr+com_msr)) & MSR_DCD)
		tp->t_state |= TS_CARR_ON;
	while ((flag&O_NONBLOCK) == 0 && (tp->t_cflag&CLOCAL) == 0 &&
	    (tp->t_state & TS_CARR_ON) == 0) {
		tp->t_state |= TS_WOPEN;
		if (error = ttysleep(tp, (caddr_t)&tp->t_rawq, TTIPRI | PCATCH,
		    ttopen, 0))
			break;
	}
	splx(s);
	if (error == 0) {
		error = (*linesw[tp->t_line].l_open)(dev, tp);
		if (mode == S_IFLNK)
			sc->sc_flags |= SC_CNOPEN;
		else
			sc->sc_flags |= SC_TTYOPEN;
	}
	return (error);
}

/*
 * When closing the com console, we need to make sure that the normal
 * device and the console are both closed before resetting things
 * and resetting the hardware.
 */
/*ARGSUSED*/
comclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	register struct com_softc *sc;
	register struct tty *tp;
	register com;
	int unit;

	unit = UNIT(dev);
	sc = comcd.cd_devs[unit];
	com = sc->sc_addr;
	tp = sc->sc_tty;
	if (mode == S_IFLNK)
		sc->sc_flags &= ~SC_CNOPEN;
	else
		sc->sc_flags &= ~SC_TTYOPEN;
	if ((sc->sc_flags & (SC_TTYOPEN|SC_CNOPEN)) == 0) {
		(*linesw[tp->t_line].l_close)(tp, flag);
		outb(com+com_cfcr, inb(com+com_cfcr) & ~CFCR_SBREAK);
#ifdef KGDB
		/*
		 * Revert to 8 bits, no parity, and do not
		 * disable interrupts if debugging.
		 */
		if (kgdb_dev == dev)
			outb(com+com_cfcr, CFCR_8BITS);
		else {
#endif
			outb(com+com_ier, 0);
#ifdef COM_SHARE_IRQ
			if (tp->t_cflag&HUPCL || tp->t_state&TS_WOPEN ||
			    (tp->t_state&TS_ISOPEN) == 0)
				outb(com+com_mcr, MCR_OFF | MCR_INTROFF);
			else
				outb(com+com_mcr, MCR_ON | MCR_INTROFF);
#else
			if (tp->t_cflag&HUPCL || tp->t_state&TS_WOPEN ||
			    (tp->t_state&TS_ISOPEN) == 0)
				outb(com+com_mcr, MCR_OFF | sc->sc_mcr_ienable);
#endif
#ifdef KGDB
		}
#endif
		ttyclose(tp);
		if (sc->sc_overflows || sc->sc_txlost) {
			log(LOG_WARNING,
		"com%d: %d silo overflows, %d lost transmit interrupts\n",
			    unit, sc->sc_overflows, sc->sc_txlost);
			sc->sc_overflows = 0;
			sc->sc_txlost = 0;
		}
	}
	return (0);
}

comread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
{
	struct com_softc *sc = comcd.cd_devs[UNIT(dev)];
	struct tty *tp = sc->sc_tty;

	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}

comwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
{
	int unit = UNIT(dev);
	struct com_softc *sc = comcd.cd_devs[unit];
	register struct tty *tp = sc->sc_tty;

	/*
	 * (XXX) We disallow virtual consoles if the physical console is
	 * a serial port.  This is in case there is a display attached that
	 * is not the console.  In that situation we don't need/want the X
	 * server taking over the console.
	 */
	if (constty && unit == comconsole)
		constty = NULL;
	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

comselect(dev, flag, p)
	dev_t dev;
	int flag;
	struct proc *p;
{
	struct com_softc *sc = comcd.cd_devs[UNIT(dev)];

	return (ttyselect(sc->sc_tty, flag, p));
}

comintr(sc)
	struct com_softc *sc;
{
	register com;
	register u_char code;
	register int c;
	int unit;
	register struct tty *tp;
	int keep_going;
	struct com_softc *basesc;

	basesc = sc;
	keep_going = 0;
again:
	unit = sc->sc_dev.dv_unit;
	tp = sc->sc_tty;

	com = sc->sc_addr;
	while (1) {
		code = inb(com+com_iir);
		switch (code & IIR_IMASK) {
		case IIR_RLS:
			keep_going = 1;
			code = inb(com+com_lsr);
			goto receive;

		case IIR_RXTOUT:
		case IIR_RXRDY:
			keep_going = 1;
			code = 0;

receive:
			do {
				c = inb(com+com_data);
				if (tp->t_state & TS_ISOPEN) {
				    if (code & (LSR_BI|LSR_FE|LSR_PE|LSR_OE)) {
					if (code & (LSR_BI | LSR_FE))
						c |= TTY_FE;
					else if (code & LSR_PE)
						c |= TTY_PE;
					if (code & LSR_OE)
						sc->sc_overflows++;
				    }
				    (*linesw[tp->t_line].l_rint)(c, tp);
				}

#ifdef KGDB
				else if (sc->sc_flags & SC_KGDB &&
				    c == FRAME_END)
					kgdb_connect(0); /* trap into kgdb */
#endif
			} while ((code = inb(com+com_lsr)) & LSR_RXRDY);

			/*
			 * If the chip is ready for another character
			 * to transmit and we are expecting a transmit
			 * interrupt, the interrupt may have been lost.
			 * Some chips incorrectly clear the transmit interrupt
			 * when we read the interrupt ID register for the
			 * receive interrupt when both are pending.
			 * In any case, we fall through if we can
			 * restart transmission.
			 */
			if (!(code & LSR_TXRDY) || !(tp->t_state & TS_BUSY))
				break;

			code = inb(com+com_iir);
			if ((code & IIR_IMASK) < IIR_TXRDY)
				sc->sc_txlost++;
			/* FALLTHROUGH */

		case IIR_TXRDY:
			keep_going = 1;
			tp->t_state &= ~(TS_BUSY|TS_FLUSH);
			(*linesw[tp->t_line].l_start)(tp);
			break;

		default:
			if (code & IIR_NOPEND) {
				/*
				 * Done with this line.  If there are more
				 * lines on this IRQ, move to the next one.
				 * If we have multiple lines on this IRQ
				 * and at least one had something, start
				 * over until we get a clean sweep to avoid
				 * losing an interrupt.
				 */
				if ((sc = sc->sc_next) != 0)
					goto again;

				if (keep_going && basesc->sc_next) {
					keep_going = 0;
					sc = basesc;
					goto again;
				}
				return (1);
			}

			log(LOG_WARNING, "com%d: weird interrupt, id 0x%x\n",
			    unit, code);
			/* FALLTHROUGH */

		case IIR_MLSC:
			keep_going = 1;
			commint(tp, sc);
			break;
		}
	}
	/* NOTREACHED */
}

commint(tp, sc)
	register struct tty *tp;
	struct com_softc *sc;
{
	register int stat;
	int com = sc->sc_addr;

	sc->sc_mstat = stat = inb(com+com_msr);
	if (stat & MSR_DDCD) {
		if (stat & MSR_DCD)
			(void)(*linesw[tp->t_line].l_modem)(tp, 1);
		else if ((*linesw[tp->t_line].l_modem)(tp, 0) == 0)
			outb(com+com_mcr, MCR_OFF | sc->sc_mcr_ienable);
	} else if ((stat & MSR_DCTS) && (tp->t_state & TS_ISOPEN) &&
	   (tp->t_cflag & CCTS_OFLOW)) {
		/* the line is up and we want to do rts/cts flow control */
		if (stat & MSR_CTS) {
			tp->t_state &=~ TS_TTSTOP;
			ttstart(tp);
		} else
			tp->t_state |= TS_TTSTOP;
	}
}

static int
tiocm2mcr(data)
	register data;
{
	register m = 0;

	if (data & TIOCM_DTR)
		m |= MCR_DTR;
	if (data & TIOCM_RTS)
		m |= MCR_RTS;
	return (m);
}

comioctl(dev, cmd, data, flag)
	dev_t dev;
	caddr_t data;
{
	register struct com_softc *sc;
	register struct tty *tp;
	register com;
	register int error;

	sc = comcd.cd_devs[UNIT(dev)];
	tp = sc->sc_tty;
	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
	if (error >= 0)
		return (error);
	error = ttioctl(tp, cmd, data, flag);
	if (error >= 0)
		return (error);

	com = sc->sc_addr;
	switch (cmd) {

	case TIOCSBRK:
		outb(com+com_cfcr, inb(com+com_cfcr) | CFCR_SBREAK);
		break;

	case TIOCCBRK:
		outb(com+com_cfcr, inb(com+com_cfcr) & ~CFCR_SBREAK);
		break;

	case TIOCSDTR:
		/* only DTR, RTS exist */
		outb(com+com_mcr, MCR_ON | sc->sc_mcr_ienable);
		break;

	case TIOCCDTR:
		/* only DTR, RTS exist */
		outb(com+com_mcr, MCR_OFF | sc->sc_mcr_ienable);
		break;

	case TIOCMSET:
		outb(com+com_mcr,
		     tiocm2mcr(*(int *) data) | sc->sc_mcr_ienable);
		break;

	case TIOCMBIS:
		outb(com+com_mcr, inb(com+com_mcr) | tiocm2mcr(*(int *)data));
		break;

	case TIOCMBIC:
		outb(com+com_mcr, inb(com+com_mcr) &~ tiocm2mcr(*(int *)data));
		break;

	case TIOCMGET:
		{
			register m = inb(com+com_mcr), bits = TIOCM_LE;

			if (m & MCR_DTR)
				bits |= TIOCM_DTR;
			if (m & MCR_RTS)
				bits |= TIOCM_RTS;
			m = inb(com+com_msr);
			if (m & MSR_CTS)
				bits |= TIOCM_CTS;
			if (m & MSR_DCD)
				bits |= TIOCM_CAR;
			if (m & MSR_DSR)
				bits |= TIOCM_DSR;
			if (m & (MSR_RI|MSR_TERI))
				bits |= TIOCM_RI;
			*(int *)data = bits;
			break;
		}

	default:
		return (ENOTTY);
	}
	return (0);
}

static int
comspeed(speed)
	register long speed;
{
#define divrnd(n, q)	(((n) * 2 / (q) + 1) / 2) /* divide and round off */

	register long x, err;

	if (speed == 0)
		return (0);
	if (speed < 0)
		return (-1);

	x = divrnd(COMTICK, speed);
	if (x <= 0)
		return (-1);

	err = divrnd(1000 * COMTICK, x * speed) - 1000;
	if (err < 0)
		err = -err;
	if (err > SPEED_TOLERANCE)
		return (-1);
	return (x);
	
#undef divrnd
}

comparam(tp, t)
	register struct tty *tp;
	register struct termios *t;
{
	register struct com_softc *sc;
	register com;
	register int cfcr, cflag = t->c_cflag, oldcflag = tp->t_cflag;
	int s;
	int ospeed;

	/* short-circuit the common case where there is no hardware change */
	if (oldcflag == cflag && tp->t_state&TS_ISOPEN &&
	    tp->t_ispeed == t->c_ispeed && tp->t_ospeed == t->c_ospeed)
	    	return (0);

	/* check requested parameters */
	ospeed = comspeed(t->c_ospeed);
        if (ospeed < 0 || t->c_ispeed != t->c_ospeed)
                return (EINVAL);
	if ((oldcflag & CLOCAL) == 0 && cflag & CLOCAL)
		wakeup((caddr_t) &tp->t_rawq);
        /* and copy to tty */
        tp->t_ispeed = t->c_ispeed;
        tp->t_ospeed = t->c_ospeed;
        tp->t_cflag = cflag;

	sc = comcd.cd_devs[UNIT(tp->t_dev)];
	com = sc->sc_addr;
	if (ospeed == 0) {
		/* hang up line */
		outb(com+com_mcr, MCR_OFF | sc->sc_mcr_ienable);
		return (0);
	}
	s = spltty();
	outb(com+com_ier, IER_ERXRDY | IER_ETXRDY | IER_ERLS | IER_EMSC);
	outb(com+com_cfcr, inb(com+com_cfcr) | CFCR_DLAB);
	outb(com+com_dlbl, ospeed & 0xFF);
	outb(com+com_dlbh, ospeed >> 8);
	switch (cflag&CSIZE) {
	case CS5:
		cfcr = CFCR_5BITS; break;
	case CS6:
		cfcr = CFCR_6BITS; break;
	case CS7:
		cfcr = CFCR_7BITS; break;
	case CS8:
		cfcr = CFCR_8BITS; break;
	}
	if (cflag&PARENB) {
		cfcr |= CFCR_PENAB;
		if ((cflag&PARODD) == 0)
			cfcr |= CFCR_PEVEN;
	}
	if (cflag&CSTOPB)
		cfcr |= CFCR_STOPB;
	outb(com+com_cfcr, cfcr);
	/* if the previous speed was 0, need to re-enable DTR */
	outb(com+com_mcr, MCR_ON | sc->sc_mcr_ienable);

	if (sc->sc_flags & SC_HASFIFO)
		outb(com+com_fifo, FIFO_ENABLE | FIFO_TRIGGER);

	if (oldcflag & CCTS_OFLOW && (cflag & CCTS_OFLOW) == 0 &&
	    (sc->sc_mstat & MSR_CTS) == 0) {
		tp->t_state &= ~TS_TTSTOP;
		ttstart(tp);
	}
	splx(s);
	return (0);
}

comstart(tp)
	register struct tty *tp;
{
	struct com_softc *sc = comcd.cd_devs[UNIT(tp->t_dev)];
	register com;
	int s, i;

	com = sc->sc_addr;
	s = spltty();
	if ((tp->t_state & (TS_XON_PEND|TS_XOFF_PEND)) &&
	    (inb(com+com_lsr) & LSR_TXRDY)) {
		tp->t_state |= TS_BUSY;
		if (tp->t_state & TS_XON_PEND) {
			outb(com+com_data, tp->t_cc[VSTART]);
			tp->t_state &= ~TS_XON_PEND;
		} else {
			outb(com+com_data, tp->t_cc[VSTOP]);
			tp->t_state &= ~TS_XOFF_PEND;
		}
		goto out;
	}
	if (tp->t_state & (TS_TIMEOUT|TS_TTSTOP))
		goto out;
	if (tp->t_outq.c_cc <= tp->t_lowat) {
		if (tp->t_state&TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t)&tp->t_outq);
		}
		selwakeup(&tp->t_wsel);
	}
	if (tp->t_outq.c_cc == 0) {
		if (tp->t_state & TS_BUSY && inb(com+com_lsr) & LSR_TXRDY) {
			/* sc->sc_txlost++; not necessarily */
			tp->t_state &= ~(TS_BUSY|TS_FLUSH);
		}
		goto out;
	}
	if (tp->t_cflag & CCTS_OFLOW && (sc->sc_mstat & MSR_CTS) == 0) {
		tp->t_state |= TS_TTSTOP;	/* for pstat */
		goto out;
	}
	if (inb(com+com_lsr) & LSR_TXRDY) {
		tp->t_state |= TS_BUSY;
		if (sc->sc_flags & SC_HASFIFO)
			i = 16;
		else
			i = 1;
		for (; i && tp->t_outq.c_cc; i--)
			outb(com+com_data, getc(&tp->t_outq));
	}
out:
	splx(s);
}

/*
 * Stop output on a line.
 */
/*ARGSUSED*/
comstop(tp, flag)
	register struct tty *tp;
{
	register int s;

	s = spltty();
	if (tp->t_state & TS_BUSY) {
		if ((tp->t_state&TS_TTSTOP) == 0)
			tp->t_state |= TS_FLUSH;
	}
	splx(s);
}

/*
 * Following are all routines needed for COM to act as console
 */
#include "i386/i386/cons.h"

comcnprobe(cp)
	struct consdev *cp;
{
	int unit;

	/* locate the major number */
	for (commajor = 0; commajor < nchrdev; commajor++)
		if (cdevsw[commajor].d_open == comopen)
			break;

	/* XXX: ick */
	if ((unit = comconsole) == -1)
		unit = CONUNIT;

	/* make sure hardware exists?  XXX */

	/* initialize required fields */
	cp->cn_dev = makedev(commajor, unit);
	cp->cn_tp = &com_cntty;
	if (comconsole != -1)
		cp->cn_pri = CN_REMOTE;	/* Force a serial port console */
	else
		cp->cn_pri = CN_NORMAL;
}

comcninit(cp)
	struct consdev *cp;
{
	int unit = UNIT(cp->cn_dev);

	cominit(com_cnaddr, comdefaultrate);
	comconsole = unit;
}

cominit(com, rate)
	int com, rate;
{
	int s;
	short stat;

#ifdef lint
	stat = unit; if (stat) return;
#endif
	s = splhigh();
	outb(com+com_cfcr, CFCR_DLAB);
	rate = comspeed(rate);
	outb(com+com_dlbl, rate & 0xFF);
	outb(com+com_dlbh, rate >> 8);
	outb(com+com_cfcr, CFCR_8BITS);
	outb(com+com_ier, IER_ERXRDY | IER_ETXRDY | IER_ERLS);
	outb(com+com_fifo, FIFO_ENABLE|FIFO_RCV_RST|FIFO_XMT_RST|FIFO_TRIGGER);
	stat = inb(com+com_iir);
	splx(s);
}

/* ARGSUSED */
comcngetc(dev)
{
	int c, s;

	s = splhigh();
	c = com_getc(com_cnaddr);
	splx(s);
	return (c);
}

/*
 * Console kernel output character routine.
 */
/* ARGSUSED */
comcnputc(dev, c)
	dev_t dev;
	register int c;
{
	int s = splhigh();

	com_putc(com_cnaddr, c);
	splx(s);
}

/*
 * com/kgdb input character routine.  Assumes that interrupts are off.
 */
com_getc(com)
	register com;
{
	short stat;
	int c;

#ifdef lint
	stat = com; if (stat) return (0);
#endif
	while (((stat = inb(com+com_lsr)) & LSR_RXRDY) == 0)
		;
	c = inb(com+com_data);
	stat = inb(com+com_iir);
	return (c);
}

/*
 * com/kgdb output character routine.  Assumes that interrupts are off.
 */
void
com_putc(com, c)
	register com;
	int c;
{
	register int timo;
	short stat;

#ifdef lint
	stat = com; if (stat) return;
#endif
	/* wait for any pending transmission to finish */
	timo = 50000;
	while (((stat = inb(com+com_lsr)) & LSR_TXRDY) == 0 && --timo)
		;
	outb(com+com_data, c);
	/* wait for this transmission to complete */
	timo = 1500000;
	while (((stat = inb(com+com_lsr)) & LSR_TXRDY) == 0 && --timo)
		;
#if 0
	/* clear any interrupts generated by this transmission */
	stat = inb(com+com_iir);
#endif
}
