/*-
 * Copyright (c) 1992, 1993, 1994 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI $Id: rc.c,v 1.14 1994/01/06 00:07:25 karels Exp $
 */

/*
 * SDL Communications RISCom/8 8-port Async Mux driver
 */

/*
 * History:
 * ??/??/92	[avg]	- initial revision
 * 8/3/92	[avg]	- autoconfig code
 */

/* #define RCDEBUG 1 */

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

#include "isa.h"
#include "isavar.h"
#include "icu.h"
#include "machine/cpu.h"

#include "rcreg.h"
#include "ic/cl-cd180.h"

#ifdef RCDEBUG
int rcdebug = RCDEBUG;

#define dprintf(x)	if (rcdebug) printf x

rcdoutb(a, v)
	int a, v;
{

	dprintf(("outb(%x,%x)\n", a, v));
	outb(a, v);
}

rcdinb(a)
	int a;
{
	int v = inb(a);

	dprintf(("inb(%x)->%x\n", a, v));
	return (v);
}

#define outb		rcdoutb
#define inb		rcdinb

#else	/* !RCDEBUG */

#define dprintf(x)	/* nothing */
#endif

#define UNIT(dev)	(minor(dev) >> 3)
#define LINE(dev)	(minor(dev) & 07)

struct rc_softc {
	struct	device rc_dev;		/* base device */
	struct	isadev rc_id;		/* ISA device */
	struct  intrhand rc_ih;		/* interrupt vectoring */
	struct	ttydevice_tmp rc_ttydev; /* tty stuff */
	struct	tty rc_tty[CD180_NCHAN]; /* Per-channel tty structures */
	int	rc_addr;		/* base i/o address */
	short	rc_softdtr;		/* software copy of DTR */
	short	rc_txint;		/* TX interrupt is in progress */
	char	rc_init[CD180_NCHAN];	/* line has been inited since reset */
	char	rc_cmd[CD180_NCHAN];	/* command bytes per channels */
	char	rc_pendesc[CD180_NCHAN]; /* pending escapes */
	u_int	rc_orun[CD180_NCHAN];	/* overruns */
};

/*
 * rcmctl commands
 */
enum rcmctl_cmds { GET, SET, BIS, BIC };

int rcprobe __P((struct device *, struct cfdata *, void *));
void rcattach __P((struct device *, struct device *, void *));
int rcopen __P((dev_t, int, int, struct proc *));
int rcclose __P((dev_t, int, int, struct proc *));
int rcread __P((dev_t, struct uio *, int));
int rcwrite __P((dev_t, struct uio *, int));
int rcintr __P((struct rc_softc *));
int rcstart __P((struct tty *));
int rcioctl __P((dev_t, int, caddr_t, int));
void rcchancmd __P((int, int));
int rcparam __P((struct tty *, struct termios *));
int rcstop __P((struct tty *, int));
static void rccd180init __P((int));
static void rcforceintr __P((void *));
static int rcmctl __P((dev_t, enum rcmctl_cmds, int));

#ifdef RC_STATS
unsigned int int_cnt[CD180_NCHAN*4][3]; /* intr stats on all ports */
					/*
					** 0: Rx total
					** 1: Tx total
					** 2: RxEx total
					*/
unsigned int loops[4][2];		/*
					** 0: total
					** 1: peak
					*/
#endif

struct cfdriver rccd = {
	NULL, "rc", rcprobe, rcattach, sizeof(struct rc_softc)
};

/*
 * Probe routine
 */
rcprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	int i;

	if ((ia->ia_iobase & ~RC_BASEBITS) != RC_MINBASE) {
		printf("rc%d: illegal base address %x\n", cf->cf_unit,
		    ia->ia_iobase);
		return (0);
	}

	routb(ia->ia_iobase, CD180_PPRL, 0x5a);
	if (rinb(ia->ia_iobase, CD180_PPRL) != 0x5a)
		return (0);
	routb(ia->ia_iobase, CD180_PPRL, 0xa5);
	if (rinb(ia->ia_iobase, CD180_PPRL) != 0xa5)
		return (0);

	if (ia->ia_irq == IRQUNK)
		ia->ia_irq = isa_discoverintr(rcforceintr, aux);
	i = ffs(ia->ia_irq) - 1;
	if (i == 0)
		return (0);
	if (!RC_VALIDIRQ(i)) {
		printf("rc%d: illegal IRQ %d\n", cf->cf_unit, i);
		return (0);
	}
	ia->ia_iosize = RC_NPORT;
	return (1);
}

/*
 * Attach routine
 */
void 
rcattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	register struct rc_softc *sc = (struct rc_softc *) self;
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	register base = ia->ia_iobase;

	sc->rc_addr = base;

	/* init the chip */
	rccd180init(base);

	/* drop DTR on all lines */
	routb(base, RC_DTR, ~0);

	/* Initialize interrupt/id structures */
	isa_establish(&sc->rc_id, &sc->rc_dev);
	sc->rc_ih.ih_fun = rcintr;
	sc->rc_ih.ih_arg = (void *) sc;
	intr_establish(ia->ia_irq, &sc->rc_ih, DV_TTY);

	strcpy(sc->rc_ttydev.tty_name, rccd.cd_name);
	sc->rc_ttydev.tty_unit = sc->rc_dev.dv_unit;
	sc->rc_ttydev.tty_base = sc->rc_dev.dv_unit * CD180_NCHAN;
	sc->rc_ttydev.tty_count = CD180_NCHAN;
	sc->rc_ttydev.tty_ttys = sc->rc_tty;
	tty_attach(&sc->rc_ttydev);
	printf("\n");
}

/*
 * Force interrupt
 */
static void 
rcforceintr(arg)
	void *arg;
{
	register int base = ((struct isa_attach_args *) arg)->ia_iobase;

	/* init chip */
	rccd180init(base);

	/* enable TX interrupts from line 0 -- should get one immediately */
	routb(base, CD180_CAR, 0);
	rcchancmd(base, CCR_TXEN);
	routb(base, CD180_IER, IER_TXRDY);
}

/*
 * Initialize the board and CL-CD180 chip
 */
static void
rccd180init(base)
	register int base;
{
	routb(base, RC_CTOUT, 0);		/* clear timeout */
	rcchancmd(base, CCR_HARDRESET);	/* reset chip */
	DELAY(50000);				/* wait 0.05 sec */
	routb(base, CD180_GIVR, RC_ID);		/* chip id */
	routb(base, CD180_GICR, 0);		/* clear unused bits */
	routb(base, CD180_PILR1, RC_ACK_MINT);	/* modem intr prio */
	routb(base, CD180_PILR2, RC_ACK_TINT);	/* tx intr prio */
	routb(base, CD180_PILR3, RC_ACK_RINT);	/* rcv intr prio */

	/* set up prescaler value to get 1ms ticks */
	routb(base, CD180_PPRH,  (RC_OSCFREQ/1000) >> 8);
	routb(base, CD180_PPRL,  (RC_OSCFREQ/1000) & 0xff);
}

/*
 * Open line
 */
rcopen(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	register struct tty *tp;
	int s;
	int error;
	register int base;
	int unit, chan;
	struct rc_softc *sc;

	unit = UNIT(dev);
	if (unit >= rccd.cd_ndevs || (sc = rccd.cd_devs[unit]) == 0)
		return (ENXIO);
	sc = rccd.cd_devs[unit];
	base = sc->rc_addr;
	tp = &sc->rc_tty[LINE(dev)];
	chan = LINE(dev);
	tp->t_oproc = rcstart;
	tp->t_param = rcparam;
	tp->t_dev = dev;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		tp->t_state |= TS_WOPEN;
		if (tp->t_ispeed == 0)
			tp->t_termios = deftermios;
		else
			ttychars(tp);
		rcparam(tp, &tp->t_termios);
		ttsetwater(tp);
	} else if ((tp->t_state & TS_XCLUDE) && p->p_ucred->cr_uid != 0)
		return (EBUSY);

	error = 0;
	s = spltty();
	(void) rcmctl(dev, SET, TIOCM_DTR|TIOCM_RTS);

	routb(base, CD180_CAR, chan);
	if (rinb(base, CD180_MSVR) & MSVR_CD) {
		dprintf(("CD PRESENT\n"));
		tp->t_state |= TS_CARR_ON;
	}
	if (!(flag & O_NONBLOCK)) {
		while (!(tp->t_cflag & CLOCAL) && !(tp->t_state & TS_CARR_ON)) {
			tp->t_state |= TS_WOPEN;
			error = ttysleep(tp, (caddr_t)&tp->t_rawq,
			    TTIPRI | PCATCH, ttopen, 0);
			if (error) {
				/*
				 * Disable line and drop DTR.
				 * Note, this is wrong if another open might
				 * be in progress.
				 */
				routb(base, CD180_CAR, LINE(dev));
				rcchancmd(base, CCR_TXDIS | CCR_RXDIS);
				sc->rc_init[chan] = 0;
				(void) rcmctl(dev, SET, 0);
				break;
			}
		}
	}
	splx(s);
	if (error == 0)
		error = (*linesw[tp->t_line].l_open)(dev, tp);
#ifdef RC_STATS
	int_cnt[minor(dev)][0] = 0;
	int_cnt[minor(dev)][1] = 0;
	int_cnt[minor(dev)][2] = 0;
	loops[UNIT(dev)][0] = 0;
	loops[UNIT(dev)][1] = 0;
#endif
	return (error);
}

/*
 * Close line
 */
rcclose(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	struct rc_softc *sc = rccd.cd_devs[UNIT(dev)];
	int chan = LINE(dev);
	register struct tty *tp = &sc->rc_tty[chan];
	register base = sc->rc_addr;
	int s;

	(*linesw[tp->t_line].l_close)(tp, flag);

	/* Disable line */
	s = spltty();
	routb(base, CD180_CAR, chan);
	rcchancmd(base, CCR_TXDIS | CCR_RXDIS);
	sc->rc_init[chan] = 0;
	routb(base, CD180_IER, 0);
	splx(s);

	/* Hang up */
	if ((tp->t_cflag & HUPCL) || (tp->t_state & TS_WOPEN) ||
	    (tp->t_state & TS_ISOPEN) == 0)
	    (void) rcmctl(dev, SET, 0);

	ttyclose(tp);
	if (sc->rc_orun[chan]) {
		log(LOG_ERR, "rc%d line %d: %d overruns\n", UNIT(dev), chan,
		    sc->rc_orun[chan]);
		sc->rc_orun[chan] = 0;
	}
#ifdef RC_STATS
	{
		int i;
		printf("rc%d:%d\n\t", UNIT(dev), LINE(dev));
		for (i = 0; i < 3; i++)
			printf("I%d=%x ", i, int_cnt[minor(dev)][i]);
		for (i = 0; i < 2; i++)
			printf("L%d=%x ", i, loops[UNIT(dev)][i]);
		printf("\n");
	}
#endif
	return (0);
}

/*
 * Read from line
 */
rcread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct rc_softc *sc = rccd.cd_devs[UNIT(dev)];
	struct tty *tp = &sc->rc_tty[LINE(dev)];

	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}

/*
 * Write to line
 */
rcwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct rc_softc *sc = rccd.cd_devs[UNIT(dev)];
	struct tty *tp = &sc->rc_tty[LINE(dev)];

	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

rcselect(dev, flag, p)
	dev_t dev;
	int flag;
	struct proc *p;
{
	struct rc_softc *sc = rccd.cd_devs[UNIT(dev)];

	return (ttyselect(&sc->rc_tty[LINE(dev)], flag, p));
}

/*
 * Interrupt routine
 */
rcintr(sc)
	register struct rc_softc *sc;
{
	register base = sc->rc_addr;
	register struct tty *base_tp, *tp;
	register b, c;
	register unsigned cnt;
	register chan;
#ifdef RC_STATS
	int	cur_loops = 0;
#endif

	dprintf(("*I%d*", sc->rc_dev.dv_unit));
	if (base == 0) {
		printf("rc%d: bogus interrupt\n", sc->rc_dev.dv_unit);
		return (1);
	}

	base_tp = sc->rc_tty;
	
	/* Read Board Status Register */
	while ((b = ~rinb(base, RC_BSR)) &
	    (RC_BSR_TOUT|RC_BSR_RINT|RC_BSR_TINT|RC_BSR_MINT)) {
	    /*
	     * Need to add some code to allow return if this card is hogging
	     */

	    /* Board timeout? */
	    if (b & RC_BSR_TOUT) {
		printf("rc%d: hardware failure\n", sc->rc_dev.dv_unit);
		routb(base, RC_CTOUT, 0);
		rcchancmd(base, CCR_HARDRESET);
		/* This is bogus, nothing will work now. */
		return (1);
	    }

#ifdef RC_STATS
	    cur_loops++;
#endif

	    /* Receiver interrupt */
	    if (b & RC_BSR_RINT) {
		/* Ack interrupt */
		b = rinb(base, RC_ACK_RINT);
		if (b != (RC_ID|GIVR_IT_RCV) && b != (RC_ID|GIVR_IT_REXC)) {
			printf("rc%d: bad CD180 receive interrupt vector (%x)\n",
			    sc->rc_dev.dv_unit, b);
			goto out;
		}

		/* Get line number */
		chan = rinb(base, CD180_GICR) >> GICR_CHAN_OFF;
		tp = base_tp + chan;
#ifdef RC_STATS
		int_cnt[(sc->rc_dev.dv_unit * CD180_NCHAN) + chan][0]++;
#endif

		/* Get the count of received characters */
		cnt = rinb(base, CD180_RDCR);

		/* If the line wasn't opened, throw data into bit bucket */
		if ((tp->t_state & TS_ISOPEN) == 0) {
			while (cnt--) {
				(void) rinb(base, CD180_RCSR);
				(void) rinb(base, CD180_RDR);
			}
			goto out;
		}

		/* If all data is good don't check status bytes */
		if (b == (RC_ID|GIVR_IT_RCV)) {
			while (cnt--)
				(*linesw[tp->t_line].l_rint)(rinb(base,
				    CD180_RDR), tp);
			goto out;
		}
		
		/* Read all the characters */
		while (cnt--) {
			b = rinb(base, CD180_RCSR);
			if (b & RCSR_TOUT) {
				/* pop up stack */
				(void) rinb(base, CD180_RDR);
				break;
			}
			c = rinb(base, CD180_RDR);
			if (b & RCSR_PE)
				c |= TTY_PE;
			if (b & (RCSR_BREAK | RCSR_FE)) 
				c |= TTY_FE;
			if (b & RCSR_OE)
				sc->rc_orun[chan]++;
			(*linesw[tp->t_line].l_rint)(c, tp);
		}
		goto out;
	    }

	    /* TX interrupt? */
	    if (b & RC_BSR_TINT) {
		/* Ack interrupt */
		b = rinb(base, RC_ACK_TINT);
		if (b != (RC_ID|GIVR_IT_TX)) {
			printf("rc%d: bad CD180 transmit interrupt vector (%x)\n",
			    sc->rc_dev.dv_unit, b);
			goto out;
		}

		/* Get line number */
		chan = rinb(base, CD180_GICR) >> GICR_CHAN_OFF;
		tp = base_tp + chan;
#ifdef RC_STATS
		int_cnt[(sc->rc_dev.dv_unit * CD180_NCHAN) + chan][1]++;
#endif

		/* (Re-)start transmit */
		if (tp->t_state & TS_FLUSH) {
			tp->t_state &= ~(TS_BUSY|TS_FLUSH);
			/* Disable TX interrupts */
			routb(base, CD180_IER, IER_CD|IER_RXD);
		} else {
			tp->t_state &= ~TS_BUSY;
			sc->rc_txint = 1;
			(*linesw[tp->t_line].l_start)(tp);
			sc->rc_txint = 0;
			/* If nothing to send, disable TX interrupts */
			if ((tp->t_state&TS_BUSY) == 0)	
				routb(base, CD180_IER, IER_CD|IER_RXD);
		}
		goto out;
	    }

	    /* Modem Ctl interrupt? */
	    if (b & RC_BSR_MINT) {
		/* Ack interrupt */
		b = rinb(base, RC_ACK_MINT);
		if (b != (RC_ID|GIVR_IT_MODEM)) {
			printf("rc%d: bad CD180 modem ctl interrupt vector (%x)\n",
			    sc->rc_dev.dv_unit, b);
			goto out;
		}

		/* Get line number */
		chan = rinb(base, CD180_GICR) >> GICR_CHAN_OFF;
		tp = base_tp + chan;
#ifdef RC_STATS
		int_cnt[(sc->rc_dev.dv_unit * CD180_NCHAN) + chan][2]++;
#endif

		if ((rinb(base, CD180_MCR) & MCR_CDCHG)) {
			/* Get the value of CD */
			b = rinb(base, CD180_MSVR);
			if (b & MSVR_CD)
				(void) (*linesw[tp->t_line].l_modem)(tp, 1);	
			else if ((*linesw[tp->t_line].l_modem)(tp, 0) == 0)
			    (void) rcmctl(tp->t_dev, SET, 0);

			/* Clear change bits */
			routb(base, CD180_MCR, 0);
		}
		goto out;
	    }

	    printf("rc%d: extra interrupt\n", sc->rc_dev.dv_unit);

	    /* End interrupt service */
out:	    routb(base, CD180_EOIR, 0);
	}
#ifdef RC_STATS
	loops[sc->rc_dev.dv_unit][0] += cur_loops;
	if (loops[sc->rc_dev.dv_unit][1] < cur_loops)
	    loops[sc->rc_dev.dv_unit][1] = cur_loops;
#endif
	routb(base, RC_BSR, 0);
	return (1);
}

/*
 * Start transmission
 */
rcstart(tp)
	register struct tty *tp;
{
	register base;
	register c, count;
	int s, chan;
	register struct rc_softc *sc;

	/*
	 * Check if there is work to do and we're able to do more.
	 */
	s = spltty();
	if (tp->t_state & TS_BUSY ||
	    (tp->t_state & (TS_TIMEOUT|TS_TTSTOP) &&
	    (tp->t_state & (TS_XON_PEND|TS_XOFF_PEND)) == 0))
		goto out;

	if (tp->t_outq.c_cc <= tp->t_lowat) {
		if (tp->t_state & TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t)&tp->t_outq);
		}
		selwakeup(&tp->t_wsel);
	}

	sc = rccd.cd_devs[UNIT(tp->t_dev)];
	base = sc->rc_addr;
	chan =  LINE(tp->t_dev);

	/*
	 * If not in interrupt context, TDR is not available.
	 * Simply enable TX interrupt if there is output to be done.
	 */
	if (sc->rc_txint == 0) {
		if (tp->t_outq.c_cc ||
		    tp->t_state & (TS_XON_PEND|TS_XOFF_PEND) ||
		    sc->rc_cmd[chan] || sc->rc_pendesc[chan]) {
			tp->t_state |= TS_BUSY;
			routb(base, CD180_CAR, chan);
			routb(base, CD180_IER, IER_CD|IER_RXD|IER_TXRDY);
		}
		goto out;
	}

	/*
	 * Process pending commands
	 */
	count = 8;
	if (c = sc->rc_cmd[chan]) {
		sc->rc_cmd[chan] = 0;
		routb(base, CD180_TDR, CD180_C_ESC);
		routb(base, CD180_TDR, c);
		count -= 2;
	}
	if (sc->rc_pendesc[chan]) {
		sc->rc_pendesc[chan] = 0;
		routb(base, CD180_TDR, CD180_C_ESC);
		count--;
	}
	

	if (tp->t_state & (TS_XON_PEND|TS_XOFF_PEND)) {
		if (tp->t_state & TS_XON_PEND) {
			routb(base, CD180_TDR, tp->t_cc[VSTART]);
			tp->t_state &= ~TS_XON_PEND;
		} else {
			routb(base, CD180_TDR, tp->t_cc[VSTOP]);
			tp->t_state &= ~TS_XOFF_PEND;
		}
		if (tp->t_state & (TS_TIMEOUT|TS_TTSTOP))
			count = 0;
		else
			count--;
	}

	/*
	 * Run regular output queue
	 */
	while (tp->t_outq.c_cc && count--) {
		c = getc(&tp->t_outq);
		if (c == CD180_C_ESC) {
			if (count == 0)		/* oops */
				sc->rc_pendesc[chan]++;
			else {
				routb(base, CD180_TDR, CD180_C_ESC);
				count--;
			}
		}
		routb(base, CD180_TDR, c);
	}
	if (count < 8)
		tp->t_state |= TS_BUSY;

out:
	splx(s);
}

/*
 * Ioctl routine
 */
rcioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	register struct rc_softc *sc = rccd.cd_devs[UNIT(dev)];
	register struct tty *tp = &sc->rc_tty[LINE(dev)];
	register int error;
	int s;
 
	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
	if (error >= 0)
		return (error);
	error = ttioctl(tp, cmd, data, flag);
	if (error >= 0)
		return (error);

	s = spltty();
	switch (cmd) {
	case TIOCSBRK:
		/* Start sending BREAK */
		sc->rc_cmd[LINE(tp->t_dev)] = CD180_C_SBRK;
		rcstart(tp);
		break;

	case TIOCCBRK:
		/* Stop sending BREAK */
		sc->rc_cmd[LINE(tp->t_dev)] = CD180_C_EBRK;
		rcstart(tp);
		break;

	case TIOCSDTR:			/* set DTR & RTS */
		(void) rcmctl(dev, SET, TIOCM_DTR|TIOCM_RTS);
		break;

	case TIOCCDTR:			/* clear DTR & RTS */
		(void) rcmctl(dev, SET, 0);
		break;

	case TIOCMSET:
		(void) rcmctl(dev, SET, * (int *) data);
		break;

	case TIOCMBIS:
		(void) rcmctl(dev, BIS, * (int *) data);
		break;

	case TIOCMBIC:
		(void) rcmctl(dev, BIC, * (int *) data);
		break;

	case TIOCMGET:
		* (int *) data = rcmctl(dev, GET, 0);
		break;

	default:
		splx(s);
		return (ENOTTY);
	}
	splx(s);
	return (0);
}

int rc_fifothresh_lo = 4;	/* FIFO depth, half of FIFO, < 38.4k */
int rc_fifothresh_hi = 2;	/* FIFO depth, >= 38.4k */

int rc_doenable = 0;	/* should not be needed, defeats optimization if set */
/*
 * Set parameters and enable the line
 */
rcparam(tp, t)
	register struct tty *tp;
	register struct termios *t;
{
	int s, chan;
	register struct rc_softc *sc;
	register base;
	register c;

	/* short-circuit the common case where there is no hardware change */
	if (tp->t_cflag == t->c_cflag && tp->t_state&TS_ISOPEN &&
	    tp->t_ispeed == t->c_ispeed && tp->t_ospeed == t->c_ospeed)
	    	return (0);
	/*
	 * With a 9.8304 MHz clock you can't get exactly 57600 bps,
	 * but it seems to work well enough for some receivers.
	 * 76800 bps is exact. Robbie Dhillon at SDL Communications
	 * says they can do anything that doesn't exceed
	 * the aggregate capacity of the chip (~460800 bps).
	 */
	if (t->c_ospeed != 0 &&
	    (t->c_ospeed < 50 || t->c_ospeed > 76800 ||
	     t->c_ispeed < 50 || t->c_ispeed > 76800))
		return (EINVAL);

	if ((tp->t_cflag & CLOCAL) == 0 && t->c_cflag & CLOCAL)
		wakeup((caddr_t) &tp->t_rawq);

	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;

	/* Select line */
	sc = rccd.cd_devs[UNIT(tp->t_dev)];
	base = sc->rc_addr;
	chan = LINE(tp->t_dev);
	s = spltty();
	routb(base, CD180_CAR, chan);

	/* ospeed == 0 is for HANGUP */
	if (tp->t_ospeed == 0) {
		(void) rcmctl(tp->t_dev, SET, 0);
#if 0
		rcchancmd(base, CCR_TXDIS | CCR_RXDIS);
		sc->rc_init[chan] = 0;
#endif
		splx(s);
		return (0);
	}

	/*
	 * Calculate coefficients for input/output bit clocks
	 * The code is a bit hairy because on high speed the
	 * quartz oscillator frequency does not divide to
	 * the standard rates well so we use the best
	 * approximation.
	 */
	c = (RC_OSCFREQ + tp->t_ispeed/2) / tp->t_ispeed;
	c = (c + CD180_TPC/2) / CD180_TPC;
	routb(base, CD180_RBPRH, c >> 8);
	routb(base, CD180_RBPRL, c & 0xff);

	c = (RC_OSCFREQ + tp->t_ospeed/2) / tp->t_ospeed;
	c = (c + CD180_TPC/2) / CD180_TPC;
	routb(base, CD180_TBPRH, c >> 8);
	routb(base, CD180_TBPRL, c & 0xff);
	
	/* Load COR1 */
	switch (tp->t_cflag & CSIZE) {
	case CS5:
		c = COR1_5BITS;
		break;
	case CS6:
		c = COR1_6BITS;
		break;
	case CS7:
		c = COR1_7BITS;
		break;
	case CS8:
		c = COR1_8BITS;
		break;
	}
	if (tp->t_cflag & CSTOPB)
		c |= COR1_2SB;
	if (tp->t_cflag & PARENB) {
		c |= COR1_NORMPAR;
		if (tp->t_cflag & PARODD)
			c |= COR1_ODDP;
		if ((tp->t_iflag & INPCK) == 0)
			c |= COR1_IGNORE;
	} else
		c |= COR1_IGNORE;
	routb(base, CD180_COR1, c);

	/* Load COR2 */
	c = COR2_ETC;
	if (tp->t_cflag & CCTS_OFLOW)
		c |= COR2_CTSAE;
#ifdef wrong
	/*
	 * COR2_RTSAO enables traditional RTS (high when there is something
	 * to transmit), not RTR (high when ready to receive).
	 */
	if (tp->t_cflag & CRTS_IFLOW)
		c |= COR2_RTSAO;
#endif

	/* there should be some logic to enable on-chip Xon/Xoff flow ctl */
	routb(base, CD180_COR2, c);

	/* Load COR3 */
	if (tp->t_ispeed >= 38400)
		routb(base, CD180_COR3, rc_fifothresh_hi);	/* FIFO depth */
	else
		routb(base, CD180_COR3, rc_fifothresh_lo);	/* FIFO depth */
	/* set the Receive Timeout Period to 20ms */
	routb(base, CD180_RTPR, 20);

	/* Inform CD180 engine about new values in COR registers */
	rcchancmd(base, CCR_CORCHG1 | CCR_CORCHG2 | CCR_CORCHG3);
	DELAY(500);
	
	if (sc->rc_init[chan] == 0 || rc_doenable) {
		sc->rc_init[chan] = 1;
		/* Load modem control parameters */
		routb(base, CD180_MCOR1, MCOR1_CDZD);
		routb(base, CD180_MCOR2, MCOR2_CDOD);

		/* Finally enable transmitter and receiver */
		rcchancmd(base, CCR_TXEN | CCR_RXEN);
		c = rinb(base, CD180_IER) & IER_TXRDY;
		routb(base, CD180_IER, c | IER_CD | IER_RXD);
	}
	splx(s);
	return (0);
}

/*
 * Write a command to the Channel Command Register,
 * making sure it is not busy before writing the command.
 * The channel must already have been selected.
 */
void
rcchancmd(base, cmd)
	int base;
	int cmd;
{
	int i;

	for (i = 0; i < 100; i++) {
		if (rinb(base, CD180_CCR) == 0)
			goto ready;
		DELAY(100);
	}
	printf("rc: ccr not ready\n");
ready:
	routb(base, CD180_CCR, cmd);
}

/*
 * Stop output on a line
 */
/*ARGSUSED*/
rcstop(tp, flag)
	register struct tty *tp;
	int flag;
{
	int s;

	s = spltty();
	if (tp->t_state & TS_BUSY) {
		if ((tp->t_state & TS_TTSTOP) == 0)
			tp->t_state |= TS_FLUSH;
	}
	splx(s);
	return (0);
}

/*
 * Modem control routine.
 */
static int
rcmctl(dev, cmd, bits)
	dev_t dev;
	enum rcmctl_cmds cmd;
	int bits;
{
	register struct rc_softc *sc = rccd.cd_devs[UNIT(dev)];
	register base = sc->rc_addr;
	register line = LINE(dev);
	register msvr;

	dprintf(("RCMCTL%d cmd = %d bits = %x\n", minor(dev), cmd, bits));

	routb(base, CD180_CAR, line);

	switch (cmd) {
	case GET:
		msvr = rinb(base, CD180_MSVR);
		bits = TIOCM_LE;
		if (msvr & MSVR_DTR)
			bits |= TIOCM_DTR;
		if (msvr & MSVR_RTS)
			bits |= TIOCM_RTS;
		if (msvr & MSVR_CTS)
			bits |= TIOCM_CTS;
		if (msvr & MSVR_DSR)
			bits |= TIOCM_DSR;
		if (msvr & MSVR_CD)
			bits |= TIOCM_CAR;
		if (~rinb(base, RC_RI) & (1 << line))
			bits |= TIOCM_RI;
		return (bits);

	case SET:
		routb(base, CD180_MSVR, 0);
		sc->rc_softdtr &= ~(1 << line);
		/* FALL THROUGH */
	case BIS:
		if (bits & TIOCM_RTS)
			routb(base, CD180_MSVR, MSVR_RTS);
		if (bits & TIOCM_DTR)
			sc->rc_softdtr |= 1 << line;
		routb(base, RC_DTR, ~(sc->rc_softdtr));
		break;

	case BIC:
		if (bits & TIOCM_RTS)
			routb(base, CD180_MSVR, 0);
		if (bits & TIOCM_DTR)
			sc->rc_softdtr &= ~(1 << line);
		routb(base, RC_DTR, ~(sc->rc_softdtr));
		break;
	}

	/* Enable/disable receiver on open/close */
	if (cmd == SET) {
		if (bits == 0) {
			routb(base, CD180_CAR, line);
			rcchancmd(base, CCR_RXDIS);
		} else {
			routb(base, CD180_CAR, line);
			rcchancmd(base, CCR_RXEN);
		}
	}
	return (0);
}
