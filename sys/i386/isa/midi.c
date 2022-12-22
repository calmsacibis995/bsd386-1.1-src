/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * This code is derived from software contributed to BSDI by
 * Mike Durian.
 *
 *	BSDI $Id: midi.c,v 1.4 1994/01/30 04:18:27 karels Exp $
 */

/*-
 *	MPU-401 (and compatible) midi controller driver based on
 *	com.c	7.5 (Berkeley) 5/16/91
 */

#include "param.h"
#include "systm.h"
#include "ioctl.h"
#include "tty.h"
#include "proc.h"
#include "user.h"
#include "file.h"
#include "uio.h"
#include "malloc.h"
#include "device.h"
#include "syslog.h"

#include "midivar.h"
#include "midiioctl.h"
#include "isavar.h"
#include "icu.h"

struct midi_softc {      /* Driver status information */
        struct  device sc_dev;  /* base device */
        struct  isadev sc_id;   /* ISA device */
        struct  intrhand sc_ih; /* interrupt handler */
};

int midiprobe __P((struct device *parent, struct cfdata *cf, void *aux));
void midiattach __P((struct device *parent, struct device *dev, void *args));
int midiopen __P((dev_t, int, int, struct proc *));
int midiclose __P((dev_t, int, int, struct proc *));
int midiread __P((dev_t, struct uio *, int));
int midiioctl __P((dev_t, int, caddr_t, int, struct proc *));
int midiselect __P((dev_t, int));
int midiintr();
int midi_reset();
void midi_send_message __P((int));
void midi_conductor_message();

struct cfdriver midicd =
        { NULL, "midi", midiprobe, midiattach, sizeof(struct midi_softc) };

typedef enum {START, TIMED_EVENT, NEWSTATUS, MIDIDATA, SYSMSG, SYSEX, 
    NEEDDATA} mess_state;

struct clist midi_out, midi_in[NUM_TRKS], cond_in, cond_out;
mess_state mtp_state = START;
volatile int control_status;
int midimajor, track_status[NUM_TRKS], track_open[NUM_TRKS];
int cond_status, midi_addr;
unsigned char ioctl_data, out_r_state, in_r_state[NUM_TRKS];

#define STRIP_IOCTL(x)	((x) & 0xff)	

/* ARGSUSED */
midiprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	void midiforceintr();

	/* should be able to do a reset if it exists */
	midi_addr = ia->ia_iobase;
	control_status |= MIDI_INTELLIGENT;
	control_status |= MIDI_EXISTS;
	control_status &= ~MIDI_PLAYING;
	control_status &= ~MIDI_RECORDING;
	control_status &= ~MIDI_STOP_TRAN;
	control_status &= ~MIDI_TIMING_ON;
	control_status &= ~MIDI_NEED_DATA;
	control_status &= ~MIDI_RABORT;

	
	if (ia->ia_irq == IRQUNK) {
		ia->ia_irq = isa_discoverintr(midiforceintr, NULL);
		if (ffs(ia->ia_irq) - 1 == 0)
			return (0);
	}

	/*
	 * it appears that intr's are masked at this point, so
	 * we'll never get our ack from MRESET through
	 * midi_send_command.  Thus we do it by hand.
	 */
	if (midi_reset())
		return (1);

	if (midi_reset())
		return (1);

	control_status &= ~MIDI_EXISTS;
	return (0);
}


/* ARGSUSED */
void
midiattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{ 
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	register struct midi_softc *sc = (struct midi_softc *) self;
	register int i;

	printf("\n");
	/*
	 * two tries to reset - 1st one might not return ack cause
	 * the board is in UART mode
	 * second reset always works.
	 */
	/* intrs are still masked */
	if (!midi_reset())
		(void) midi_reset();

	isa_establish(&sc->sc_id, &sc->sc_dev);
        sc->sc_ih.ih_fun = midiintr;
        sc->sc_ih.ih_arg = sc;
	intr_establish(ia->ia_irq, &sc->sc_ih, DV_TTY);
}

int
midiopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	register int unit;
	int error = 0;
	int cltsize;
 
	unit = minor(dev);
	if (!(control_status & MIDI_EXISTS)) {
		if (unit == CONTROL_UNIT)
			control_status &= ~MIDI_OPEN;
		else if (unit == CONDUCTOR_UNIT)
			cond_status &= ~MIDI_OPEN;
		return (ENXIO);
	}
	switch (unit) {
	case CONTROL_UNIT:
		if (control_status & MIDI_OPEN)
			return (EBUSY);
		else
			control_status |= MIDI_OPEN;

		control_status |= MIDI_RBLOCK;
		control_status |= MIDI_BLOCKING;
		control_status &= (flag & O_NONBLOCK) ? ~MIDI_BLOCKING : ~0;

		cltsize = MIDI_Q_SIZE / NBBY;
		midi_out.c_cc = 0;
		midi_out.c_cf = midi_out.c_cl = NULL;
		midi_out.c_cq = (char *)malloc((u_long)MIDI_Q_SIZE, M_CLIST,
		    M_WAITOK);
		midi_out.c_ct = (char *)malloc((u_long)cltsize, M_CLIST,
		    M_WAITOK); 
		bzero(midi_out.c_ct, cltsize);
		midi_out.c_ce = (char *)(midi_out.c_cq + MIDI_Q_SIZE - 1);
		midi_out.c_cs = MIDI_Q_SIZE;
		break;
	case CONDUCTOR_UNIT:
		if (cond_status & MIDI_OPEN)
			return (EBUSY);
		else
			cond_status |= MIDI_OPEN;

		cond_status |= MIDI_RBLOCK;
		cond_status |= MIDI_BLOCKING;
		cond_status &= (flag & O_NONBLOCK) ? ~MIDI_BLOCKING : ~0;

		cltsize = MIDI_Q_SIZE / NBBY;
		cond_out.c_cc = 0;
		cond_out.c_cf = cond_out.c_cl = NULL;
		cond_out.c_cq = (char *)malloc((u_long)MIDI_Q_SIZE, M_CLIST,
		    M_WAITOK);
		cond_out.c_ct = (char *)malloc((u_long)cltsize, M_CLIST,
		    M_WAITOK); 
		bzero(cond_out.c_ct, cltsize);
		cond_out.c_ce = (char *)(cond_out.c_cq + MIDI_Q_SIZE - 1);
		cond_out.c_cs = MIDI_Q_SIZE;

		cond_in.c_cc = 0;
		cond_in.c_cf = cond_in.c_cl = NULL;
		cond_in.c_cq = (char *)malloc((u_long)MIDI_Q_SIZE, M_CLIST,
		    M_WAITOK);
		cond_in.c_ct = (char *)malloc((u_long)cltsize, M_CLIST,
		    M_WAITOK); 
		bzero(cond_in.c_ct, cltsize);
		cond_in.c_ce = (char *)(cond_in.c_cq + MIDI_Q_SIZE - 1);
		cond_in.c_cs = MIDI_Q_SIZE;
		break;
	default:
		track_status[unit] |= MIDI_BLOCKING;
		track_status[unit] &= (flag & O_NONBLOCK) ?
		    ~MIDI_BLOCKING : ~0;

		if (track_open[unit] == 0) {
			cltsize = MIDI_Q_SIZE / NBBY;
			midi_in[unit].c_cc = 0;
			midi_in[unit].c_cf = midi_in[unit].c_cl = NULL;
			midi_in[unit].c_cq = (char *)malloc((u_long)MIDI_Q_SIZE,
			    M_CLIST, M_WAITOK);
			midi_in[unit].c_ct = (char *)malloc((u_long)cltsize,
			    M_CLIST, M_WAITOK); 
			bzero(midi_in[unit].c_ct, cltsize);
			midi_in[unit].c_ce = (char *)(midi_in[unit].c_cq +
				MIDI_Q_SIZE - 1);
			midi_in[unit].c_cs = MIDI_Q_SIZE;
		}
		track_open[unit]++;
	}

	return (0);
}
 
int
midiclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	register int unit;
	int error;

	unit = minor(dev);

	switch (unit) {
	case CONTROL_UNIT:
		if (control_status & MIDI_PLAYING) {
			if (!midi_send_command((unsigned long)MSTOP))
				log(LOG_WARNING, "Issue of MSTOP failed\n");
			control_status &= ~MIDI_PLAYING;
		}
		if (control_status & MIDI_RECORDING) {
			control_status &= ~MIDI_RECORDING;
			(void) midi_send_command((unsigned long)MRECSTOP);
		}
		/* leave in nice state (ie reset) */
		(void) midi_send_command((unsigned long)MRESET);

		/* after reset we are in intelligent mode */
		control_status |= MIDI_INTELLIGENT;
		control_status &= ~MIDI_OPEN;
		control_status &= ~MIDI_PLAYING;
		control_status &= ~MIDI_RECORDING;
		control_status &= ~MIDI_RSLEEP;
		control_status &= ~MIDI_RABORT;
		free(midi_out.c_cq, M_CLIST);
		free(midi_out.c_ct, M_CLIST);
		break;
	case CONDUCTOR_UNIT:
		cond_status &= ~MIDI_OPEN;
		cond_status &= ~MIDI_WBLOCK;
		cond_status &= ~MIDI_RSLEEP;
		free(cond_out.c_cq, M_CLIST);
		free(cond_out.c_ct, M_CLIST);
		free(cond_in.c_cq, M_CLIST);
		free(cond_in.c_ct, M_CLIST);
		break;
	default:
		track_open[unit] = 0;
		track_status[unit] &= ~MIDI_WBLOCK;
		free(midi_in[unit].c_cq, M_CLIST);
		free(midi_in[unit].c_ct, M_CLIST);
		break;
	}
	return (0);
}
 
int
midiread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	int unit;
	int in;
	int i;
	register struct clist *cl_ptr;
	register volatile int *status_ptr;
	int error;

	unit = minor(dev);
	if (!(control_status & MIDI_EXISTS))
		return (ENXIO);

	/* can only read from control device */
	if (unit != CONTROL_UNIT && unit != CONDUCTOR_UNIT)
		return (ENODEV);

	if (unit == CONTROL_UNIT) {
		cl_ptr = &midi_out;
		status_ptr = &control_status;
	} else {
		cl_ptr = &cond_out;
		status_ptr = &cond_status;
	}

	if (cl_ptr->c_cc == 0) {
		if (!(*status_ptr & MIDI_BLOCKING))
			return (EWOULDBLOCK);
		else {
			*status_ptr |= MIDI_RSLEEP;
			if (error = tsleep(cl_ptr, PWAIT | PCATCH, "midiin", 0))
				return (error);
			*status_ptr &= ~MIDI_RSLEEP;
			if (*status_ptr & MIDI_RABORT) {
				*status_ptr &= ~MIDI_RABORT;
				return (0);
			}
		}
	}
	while (uio->uio_resid) {
		if ((in = getc(cl_ptr)) == -1) {
			*status_ptr |= MIDI_RBLOCK;
			if (!(*status_ptr & MIDI_BLOCKING)) {
				return (0);
			} else {
				*status_ptr |= MIDI_RSLEEP;
				if (error = tsleep(cl_ptr, PWAIT | PCATCH,
				    "midiin", 0))
					return (error);
				*status_ptr &= ~MIDI_RSLEEP;
				if (*status_ptr & MIDI_RABORT) {
					*status_ptr &= ~MIDI_RABORT;
					return (0);
				}
			}
		} else
			if (error = ureadc(in, uio))
				return (error);
	}
	return (0);
}
 
int
midiwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	int unit;
	int error;
	register struct clist *cl_ptr;
	register int *status_ptr;

	unit = minor(dev);

	if (!(control_status & MIDI_EXISTS))
		return (ENXIO);

	/* can only write to track units and conductor unit */
	if (unit == CONTROL_UNIT)
		return (ENODEV);

	if (!(control_status & MIDI_INTELLIGENT)) {
		/* conductor ain't anything if it ain't intelligent */
		if (unit == CONDUCTOR_UNIT)
			return (ENODEV);
		while (uio->uio_resid)
			if (!midi_send_data(uwritec(uio)))
				return (EIO);
		return (0);
	}

	if (unit == CONDUCTOR_UNIT) {
		status_ptr = &cond_status;
		cl_ptr = &cond_in;
	} else {
		status_ptr = &track_status[unit];
		cl_ptr = &midi_in[unit];
	}


	if (*status_ptr & MIDI_WBLOCK) {
		if (!(*status_ptr & MIDI_BLOCKING))
			return (EWOULDBLOCK);
		else {
			*status_ptr |= MIDI_WSLEEP;
			if (error = tsleep(cl_ptr, PWAIT | PCATCH,
			    "midiout", 0))
				return (error);
			*status_ptr &= ~MIDI_WSLEEP;
		}
	}

	/* if returns from sleep and should abort because of CLRQ */
	if (control_status & MIDI_WABORT)
		return (0);

	while (uio->uio_resid) {
		/*
		 * keep putting data into in queue until
		 * it's reached the high water mark
		 * unless there's just a little left
		 */
		if (cl_ptr->c_cc >= MIDI_HIGH_MARK) {
			*status_ptr |= MIDI_WBLOCK;
			if (uio->uio_resid + cl_ptr->c_cc >= MIDI_Q_SIZE) {
				if (!(*status_ptr & MIDI_BLOCKING))
					break;
				else {
					*status_ptr |= MIDI_WSLEEP;
					if (error = tsleep(cl_ptr,
					    PWAIT | PCATCH, "midiout", 0))
						return (error);
					*status_ptr &= ~MIDI_WSLEEP;
/*
					if (control_status & MIDI_WABORT)
						return (0);
*/
				}
			}
		}
				
		if (putc(uwritec(uio), cl_ptr) == -1)
			return (EIO);
	}

	return (0);
}
 
/* ARGSUSED */
int
midiintr(sc)
	struct midisoftc *sc;
{
	register u_char code;
	register int track;

	code = inb(midi_addr + MIDI_DATA);

	switch (mtp_state) {
	case START:
		if (code < 0xf0) {
			if (control_status & MIDI_NEED_DATA) {
				mtp_state = START;
				ioctl_data = code;
				control_status &= ~MIDI_NEED_DATA;
			} else {
				mtp_state = TIMED_EVENT;
				if (control_status & MIDI_RECORDING)
					/* timed event */
					putc(code, &midi_out);
			}
			return (1);
		}
		if (code == 0xf8 || code == 0xfc || code == 0xfd) {
			/*
			 * f8 - timer overflow
			 * fc - all play tracks ended
			 * fd - clock to PC message
			 */
			mtp_state = START;
			if (control_status & MIDI_RECORDING) {
				putc(code, &midi_out);
				control_status &= ~MIDI_RBLOCK;
				if (control_status & MIDI_RSLEEP)
					wakeup(&midi_out);
			}
			return (1);
		}
		if (code == 0xf9) {
			/* conductor track request */
			midi_conductor_message();
			return (1);
		}
		if (code == 0xff) {
			/* system message */
			mtp_state = SYSMSG;
			if (control_status & MIDI_RECORDING)
				putc(code, &midi_out);
			return (1);
		}
		if (code == 0xfe) {
			mtp_state = START;
			/* command acknowledge */
			if (control_status & MIDI_ABLOCK)
				control_status &= ~MIDI_ABLOCK;
			else
				log(LOG_WARNING, "Got bogus midi command "
				    "ack\n");
			return (1);
		}
		if (code >= 0xf0 && code <= 0xf7) {
			mtp_state = START;
			track = code - 0xf0;
			midi_send_message(track);
			return (1);
		}
	case TIMED_EVENT:
TEVENT:
		if (code < 0xf0) {
			/* midi channel/voice message */
			if (code & 0x80) {
				/* new running status */
				out_r_state = (code >> 4) & 0x0f;
				mtp_state = NEWSTATUS;
				if (control_status & MIDI_RECORDING)
					putc(code, &midi_out);
				return (1);
			} else {
				/* midi data byte */
				/*
				 * if timed messaged are off then
				 * this 1st data byte is actually the
				 * the second
				 * Only when midi not playing, data
				 * transfer is stopped state is active
				 * and timing byte on switch is off
				 */
				if (control_status & MIDI_RECORDING)
					putc(code, &midi_out);
				if (!(control_status & MIDI_PLAYING) &&
				    (control_status & MIDI_STOP_TRAN) &&
				    !(control_status & MIDI_TIMING_ON)) {
					mtp_state = START;
					control_status &= ~MIDI_RBLOCK;
					if (control_status & MIDI_RSLEEP)
						wakeup(&midi_out);
				} else
					mtp_state = MIDIDATA;
				return (1);
			}
		} else {
			/* mcc timed event */
			if (code == 0xf9 || code == 0xfc) {
				/*
				 * f9 - measure end message
				 * fc - recording track data end
				 */
				mtp_state = START;
				if (control_status & MIDI_RECORDING) {
					putc(code, &midi_out);
					control_status &= ~MIDI_RBLOCK;
					if (control_status & MIDI_RSLEEP)
						wakeup(&midi_out);
				}
				return (1);
			} else {
				mtp_state = START;
				if (control_status & MIDI_RECORDING) {
					putc(code, &midi_out);
					control_status &= ~MIDI_RBLOCK;
					if (control_status & MIDI_RSLEEP)
						wakeup(&midi_out);
				}
				log(LOG_ERR, "Bogus timed event 0x%02x\n",
				    code);
				return (1);
			}
		}
	case NEWSTATUS:
		/* first nonstatus byte of a midi message */
		/*
		 * if timed messaged are off then this 1st data byte is
		 * actually the the second.
		 * Only when midi not playing, data transfer is stopped
		 * state is active and timing byte on switch is off.
		 */
		if (control_status & MIDI_RECORDING)
			putc(code, &midi_out);
		if (!(control_status & MIDI_PLAYING) &&
		    (control_status & MIDI_STOP_TRAN) &&
		    !(control_status & MIDI_TIMING_ON)) {
			mtp_state = START;
			control_status &= ~MIDI_RBLOCK;
			if (control_status & MIDI_RSLEEP)
				wakeup(&midi_out);
			return (1);
		}
		/*
		 * The number of bytes depends on what the current
		 * running state is
		 */
		switch (out_r_state) {
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0e:
			mtp_state = MIDIDATA;
			break;
		case 0x0c:
		case 0x0d:
			mtp_state = START;
			if (control_status & MIDI_RECORDING) {
				control_status &= ~MIDI_RBLOCK;
				if (control_status & MIDI_RSLEEP)
					wakeup(&midi_out);
			}
			break;
		}
		return (1);
	case MIDIDATA:
		/* second data byte of a midi message */
		mtp_state = START;
		if (control_status & MIDI_RECORDING) {
			putc(code, &midi_out);
			control_status &= ~MIDI_RBLOCK;
			if (control_status & MIDI_RSLEEP)
				wakeup(&midi_out);
		}
		return (1);
	case SYSMSG:
		/* first byte of a system message. */
		if (code == 0xf0)
			/* system exclusive */
			mtp_state = SYSEX;
		else
			/* real time message */
			mtp_state = START;
		if (control_status & MIDI_RECORDING) {
			putc(code, &midi_out);
			if (mtp_state == START &&
			    control_status & MIDI_RSLEEP) {
				control_status &= ~MIDI_RBLOCK;
				wakeup(&midi_out);
			}
		}
		return (1);
	case SYSEX:
		/* system exclusive message */
		/*
		 * Supposedly the 0xf7 is optional as a terminator.
		 * Since all bytes in the SYSEX message are data bytes,
		 * we should be able to tell when it is over by looking
		 * for any byte with high bit set.  Of course this is
		 * the first byte of the next message.
		 */
		if (code & 0x80) {
			switch (code) {
			case 0xf7:
				mtp_state = START;
				if (control_status & MIDI_RECORDING) {
					putc(code, &midi_out);
					control_status &= ~MIDI_RBLOCK;
					if (control_status & MIDI_RSLEEP)
						wakeup(&midi_out);
				}
				break;
			case 0xf8:
			case 0xfc:
			case 0xfd:
				/*
				 * abnormal terminators which don't
				 * have timing bytes.
				 */
				mtp_state = START;
				if (control_status & MIDI_RECORDING) {
					putc(code, &midi_out);
					control_status &= ~MIDI_RBLOCK;
					if (control_status & MIDI_RSLEEP)
						wakeup(&midi_out);
				}
				break;
			case 0xf9:
				/* conductor track request */
				mtp_state = START;
				midi_conductor_message();
				break;
			case 0xff:
				/* another system message */
				/* system message */
				mtp_state = SYSMSG;
				if (control_status & MIDI_RECORDING)
					putc(code, &midi_out);
				break;
			case 0xfe:
				/* command acknowledge */
				mtp_state = START;
				if (control_status & MIDI_ABLOCK)
					control_status &= ~MIDI_ABLOCK;
				else
					log(LOG_WARNING, "Got bogus midi "
					    "command ack\n");
				break;
			case 0xf0:
			case 0xf1:
			case 0xf2:
			case 0xf3:
			case 0xf4:
			case 0xf5:
			case 0xf6:
				/*
				 * 0xf7 should be here too, but that's
				 * the proper terminator for MIDI
				 * sysex messages.
				 */
				mtp_state = START;
				track = code - 0xf0;
				midi_send_message(track);
				break;
			default:
				/*
				 * we already grabbed the timing
				 * byte.  This the the event byte.
				 */
				mtp_state = TIMED_EVENT;
				goto TEVENT;
			}
		} else
			if (control_status & MIDI_RECORDING)
				putc(code, &midi_out);
		return (1);
	}
	return (1);
}

int
midiioctl(dev, cmd, data, flag, p)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	MIDI_MESS *message;
	register int error;
	int i;
	register int unit = minor(dev);
 
	if (!(control_status & MIDI_EXISTS))
		return (ENXIO);

	/*
	 *	happily ignore all commands if in UART mode and command
	 *	isn't RESET
	 */
	if (!(control_status & MIDI_INTELLIGENT) && cmd != MRESET)
		return (0);

	if (unit != CONTROL_UNIT && cmd != FIONBIO && cmd != MFLUSHQ)
		return (ENOTTY);

	switch (cmd) {
	case FIONBIO:
		switch (unit) {
		case CONTROL_UNIT:
			if ((int)*data)
				control_status |= MIDI_BLOCKING;
			else
				control_status &= ~MIDI_BLOCKING;
			break;
		case CONDUCTOR_UNIT:
			if ((int)*data)
				cond_status |= MIDI_BLOCKING;
			else
				cond_status &= ~MIDI_BLOCKING;
			break;
		default:
			if ((int)*data)
				track_status[unit] |= MIDI_BLOCKING;
			else
				track_status[unit] &= ~MIDI_BLOCKING;
			break;
		}
		break;
	case MCLRQ:
		control_status |= MIDI_WABORT;
		for (i = 0; i < NUM_TRKS; i++) {
			midi_in[i].c_cc = 0;
			if (track_status[i] & MIDI_WSLEEP)
				wakeup(&(midi_in[i]));
		}
		control_status &= ~MIDI_WABORT;
		break;
	case MFLUSHQ:
		switch (unit) {
		case CONTROL_UNIT:
			return (ENOTTY);
		case CONDUCTOR_UNIT:
			if (cond_in.c_cc != 0) {
				cond_status |= MIDI_FBLOCK;
				if (error = tsleep(&cond_status, PWAIT | PCATCH,
				    "midicndflsh", 0))
					return (error);
				cond_status &= ~MIDI_FBLOCK;
			}
			break;
		default:
			if (midi_in[unit].c_cc != 0) {
				track_status[unit] |= MIDI_FBLOCK;
				if (error = tsleep(&(track_status[unit]),
				    PWAIT | PCATCH, "miditrkflsh", 0))
					return (error);
				track_status[unit] &= MIDI_FBLOCK;
			}
			break;
		}
		break;
	case MRESET:
		if (!midi_send_command(cmd))
			(void) midi_send_command(cmd);
		control_status &= ~MIDI_TIMING_ON;
		control_status |= MIDI_INTELLIGENT;
		control_status &= ~MIDI_STOP_TRAN;
		break;
	case MUART:
		if (!midi_send_command(cmd))
			return (EIO);
		control_status &= ~MIDI_INTELLIGENT;
		break;
	case MSTART:
	case MCONT:
		if (!midi_send_command(cmd))
			return (EIO);
		control_status |= MIDI_PLAYING;
		break;
	case MRECSTART:
	case MRECSTDBY:
	case MRECCONT:
		if (!midi_send_command(cmd))
			return (EIO);
		control_status |= MIDI_RECORDING;
		break;
	case MSTOP:
		if (!midi_send_command(cmd))
			return (EIO);
		control_status &= ~MIDI_PLAYING;
		break;
	case MRECSTOP:
		if (!midi_send_command(cmd))
			return (EIO);
		control_status &= ~MIDI_RECORDING;
		if (control_status & MIDI_RSLEEP) {
			control_status |= MIDI_RABORT;
			wakeup(&midi_out);
		}
		break;
	case MSTOPANDREC:
		if (!midi_send_command(cmd))
			return (EIO);
		control_status &= ~MIDI_RECORDING;
		control_status &= ~MIDI_PLAYING;
		if (control_status & MIDI_RSLEEP) {
			control_status |= MIDI_RABORT;
			wakeup(&midi_out);
		}
		break;
	case MSTARTANDREC:
	case MCONTANDREC:
	case MSTARTANDSTDBY:
		if (!midi_send_command(cmd))
			return (EIO);
		control_status |= MIDI_PLAYING;
		control_status |= MIDI_RECORDING;
		break;
	case MSTOPTRANOFF:
		if (!midi_send_command(cmd))
			return (EIO);
		control_status &= ~MIDI_STOP_TRAN;
		break;
	case MSTOPTRANON:
		if (!midi_send_command(cmd))
			return (EIO);
		control_status |= MIDI_STOP_TRAN;
		break;
	case MTIMEON:
		if (!midi_send_command(cmd))
			return (EIO);
		control_status |= MIDI_TIMING_ON;
		break;

	case MSNDSTART:
	case MSNDSTOP:
	case MSNDCONT:
	case MRTMOFF:
	case MEXTCONTOFF:
	case MEXTCONTON:
	case MCLRPC:
	case MCLRPM:
	case MCLRRECCNT:
	case MCONDOFF:
	case MCONDON:
	case MRESETRELTMP:
	case MMETOFF:
	case MMETACNTOFF:
	case MMETACNTON:
	case MMEASENDOFF:
	case MMEASENDON:
	case MCLKINTRN:
	case MCLKMIDI:
	case MRES48:
	case MRES72:
	case MRES96:
	case MRES120:
	case MRES144:
	case MRES168:
	case MRES192:
	case MCLKPCOFF:
	case MCLKPCON:
	case MSYSTHRU:
	case MTHRUOFF:
	case MBADTHRU:
	case MALLTHRU:
	case MMODEPC:
	case MCOMMPC:
	case MRTPC:
	case MCONTPCOFF:
	case MCONTPCON:
	case MSYSPCOFF:
	case MSYSPCON:
		if (!midi_send_command(cmd))
			return (EIO);
		break;

	/* send data byte too */
	case MSELTRKS:
	case MSNDPLAYCNT:
	case MSELCHN8:
	case MSELCHN16:
	case MSETBASETMP:
	case MSETRELTMP:
	case MSETRELTMPGRD:
	case MSETMETFRQ:
	case MSETBPM:
	case MCLKPCRATE:
	case MSETREMAP:
		if (!midi_send_command(cmd))
			return (EIO);
		if (!midi_send_data((unsigned char) *data))
			return (EIO);
		break;

	/* sends message too */
	case MMESTRK0:
	case MMESTRK1:
	case MMESTRK2:
	case MMESTRK3:
	case MMESTRK4:
	case MMESTRK5:
	case MMESTRK6:
	case MMESTRK7:
	case MMESSYS:
		/* messages are not MCC system messages.  No 0xff id */
		message = (MIDI_MESS *)data;
		if (message->data[0] > 0xfc)
			return (EINVAL);
		if (!midi_send_command(cmd))
			return (EIO);
		for (i = 0; i < message->num; i++)
			if (!midi_send_data(message->data[0]))
				return (EIO);
		break;
	
	/* get data byte from board */
	case MREQPC0:
	case MREQPC1:
	case MREQPC2:
	case MREQPC3:
	case MREQPC4:
	case MREQPC5:
	case MREQPC6:
	case MREQPC7:
	case MREQRCCLR:
	case MREQVER:
	case MREQREV:
	case MREQTMP:
		/*
		 * set input state so we'll get data
		 * The manual lies.  The data come before the ack
		 */
		control_status |= MIDI_NEED_DATA;
		if (!midi_send_command(cmd))
			return (EIO);
		for (i = 0; i < MIDI_TRIES && (control_status &
		    MIDI_NEED_DATA); i++);
		if (i == MIDI_TRIES)
			return (EIO);
		*data = ioctl_data;
		break;
		
	/* recognized but not implemented */
	case MALLOFF:
	case MCLKFSK:
	case MFSKINTRN:
	case MFSKMIDI:
		break;

	/* All channel Reference Table Commands */
	/* set reference table A */
	case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45:
	case 0x46: case 0x47: case 0x48: case 0x49: case 0x4a: case 0x4b:
	case 0x4c: case 0x4d: case 0x4e: case 0x4f:
	/* set reference table B */
	case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55:
	case 0x56: case 0x57: case 0x58: case 0x59: case 0x5a: case 0x5b:
	case 0x5c: case 0x5d: case 0x5e: case 0x5f:
	/* set reference table C */
	case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65:
	case 0x66: case 0x67: case 0x68: case 0x69: case 0x6a: case 0x6b:
	case 0x6c: case 0x6d: case 0x6e: case 0x6f:
	/* set reference table D */
	case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75:
	case 0x76: case 0x77: case 0x78: case 0x79: case 0x7a: case 0x7b:
	case 0x7c: case 0x7d: case 0x7e: case 0x7f:
	/* enable/disable channel reference tables */
	case 0x98: case 0x99: case 0x9a: case 0x9b: case 0x9c: case 0x9d:
	case 0x9e: case 0x9f:
		break;

	default:
		return (ENOTTY);
	}
	return (0);
}

int
midiselect(dev, which)
	dev_t dev;
	int which;
{
	register int unit;

	unit = minor(dev);
	switch (unit) {
	case CONTROL_UNIT:
		if (which == FREAD)
			return (!(control_status & MIDI_RBLOCK));
		else
			return (0);
		break;
	case CONDUCTOR_UNIT:
		if (which == FREAD)
			return (!(cond_status & MIDI_RBLOCK));
		else
			return (!(cond_status & MIDI_WBLOCK));
		break;
	default:
		if (which == FREAD)
			return (0);
		else
			return (!(track_status[unit] & MIDI_WBLOCK));
		break;
	}
}

int
midi_send_command(command)
	unsigned long command;
{
	register int i;
	int error;
	int intr;

	for (i = 0; i < MIDI_TRIES &&
	    (inb(midi_addr + MIDI_STATUS) & MIDI_RDY_RCV); i++);
	if (i == MIDI_TRIES)
		return (0);

	control_status |= MIDI_ABLOCK;
	outb(midi_addr + MIDI_COMMAND, (unsigned char) STRIP_IOCTL(command));
	if (control_status & MIDI_INTELLIGENT) {
		for (i = 0; i < MIDI_TRIES && (control_status & MIDI_ABLOCK);
		    i++);
		if (i == MIDI_TRIES) {
			control_status &= ~MIDI_ABLOCK;
			return (0);
		}
	} else
		control_status &= ~MIDI_ABLOCK;

	return (1);
}

int
midi_send_data(data)
	unsigned char data;
{
	register int i;

	for (i = 0; i < MIDI_TRIES &&
	    (inb(midi_addr + MIDI_STATUS) & MIDI_RDY_RCV); i++);
	if (i == MIDI_TRIES)
		return (0);

	outb(midi_addr + MIDI_DATA, data);
	return (1);
}

int
midi_recv_data()
{
	register int i;

	for (i = 0; i < MIDI_TRIES &&
	    (inb(midi_addr + MIDI_STATUS) & MIDI_DATA_AVL); i++);
	if (i == MIDI_TRIES)
		return (-1);

	return (inb(midi_addr + MIDI_DATA));
}

int
midi_send_noop()
{

	if (!midi_send_data(NOOP_DELAY))
		return (0);

	return (midi_send_data(MIDI_NOOP));
}
	
int
midi_reset()
{
	register i;

	for (i = 0; i < MIDI_TRIES &&
	    (inb(midi_addr + MIDI_STATUS) & MIDI_RDY_RCV); i++);
	if (i == MIDI_TRIES)
		return (0);

	outb(midi_addr + MIDI_COMMAND, STRIP_IOCTL(MRESET));
	for (i = 0; (i < MIDI_TRIES) &&
	    (inb(midi_addr + MIDI_STATUS) & MIDI_DATA_AVL); i++);
	if (i == MIDI_TRIES)
		return (0);

	if (inb(midi_addr + MIDI_DATA) != MIDI_ACK)
		return (0);

	return (1);
}

void
midi_send_message(track)
	int track;
{
	register int data[4];
	register int xbytes;
	register int i;
	/*
	 * The number of extra bytes that will follow the message.
	 * 0x80 - 0x8f maps to index 0, 0x90 - 0x9f maps to index 1 etc.
	 */
	static int extra_bytes[] = {2, 2, 2, 2, 1, 1, 2, 0};

	/*
	 * This is buggy.  It should unget data if it can't get enough
	 * for a full message.
	 */

	if ((data[0] = getc(&(midi_in[track]))) == -1) {
		/*
		 * board wants something and nothing is there
		 * send a noop
		 */
		midi_send_noop();
		if (track_status[track] & MIDI_FBLOCK)
			wakeup(&track_status[track]);
		return;
	}
	if (data[0] == MIDI_TOFLOW) {
		if (!midi_send_data((u_char) data[0])) {
			log(LOG_ERR, "Youch!  Couldn't send data in Midi "
			    "TDR\n");
			return;
		}
#ifdef MIDIDEBUG
		printf("Track %d - sent 0x%02x\n", track, data[0]);
#endif
		if (track_status[track] & MIDI_FBLOCK &&
		    midi_in[track].c_cc == 0)
			wakeup(&track_status[track]);
		return;
	}

	if ((data[1] = getc(&(midi_in[track]))) == -1) {
		midi_send_noop();
		if (track_status[track] & MIDI_FBLOCK &&
		    midi_in[track].c_cc == 0)
			wakeup(&track_status[track]);
		return;
	}


	if (data[1] & 0x80) {
		in_r_state[track] = (data[1] >> 4) & 0x0f;
		for (xbytes = 0; xbytes < extra_bytes[(data[1] >> 4) & 0x07];
		    xbytes++) {
			if ((data[xbytes + 2] = getc(&midi_in[track])) == -1) {
				midi_send_noop();
				if (track_status[track] & MIDI_FBLOCK &&
				    midi_in[track].c_cc == 0)
					wakeup(&track_status[track]);
				return;
			}
		}
	} else {
		/* use existing running state */
		switch (in_r_state[track]) {
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0e:
			if ((data[2] = getc(&midi_in[track])) == -1) {
				midi_send_noop();
				if (track_status[track] & MIDI_FBLOCK &&
				    midi_in[track].c_cc == 0)
					wakeup(&track_status[track]);
				return;
			}
			xbytes = 1;
			break;
		default:
			xbytes = 0;
			break;
		}
	}

#ifdef MIDIDEBUG
	printf("Track %d - sent", track);
#endif
	for (i = 0; i < xbytes + 2; i++) {
		if (!midi_send_data((u_char) data[i])) {
			log(LOG_ERR, "Youch!  Couldn't send data in Midi "
			    "TDR\n");
			break;
		}
#ifdef MIDIDEBUG
		printf(" 0x%02x", data[i]);
#endif
	}
#ifdef MIDIDEBUG
	printf("\n");
#endif

	if ((track_status[track] & MIDI_WBLOCK) &&
	    (midi_in[track].c_cc < MIDI_LOW_MARK)) {
		track_status[track] &= ~MIDI_WBLOCK;
		if (track_status[track] & MIDI_WSLEEP)
			wakeup(&(midi_in[track]));
	}
	if ((track_status[track] & MIDI_FBLOCK) && (midi_in[track].c_cc == 0))
		wakeup(&track_status[track]);

	return;
}

void
midi_conductor_message()
{
	register int data[4];
	static int num_to_get = 0;
	register int i;
	int code;

	/* get any data with message */
	for (; num_to_get; num_to_get--) {
		if ((code = midi_recv_data()) == -1)
			return;
		putc((u_char)code, &cond_out);
		cond_status &= ~MIDI_RBLOCK;
		if (cond_status & MIDI_RSLEEP)
			wakeup(&cond_out);
	}
	

	/*
	 * This is buggy.  It should unget data if it can't get enough
	 * for a full message.
	 */

	if ((data[0] = getc(&cond_in)) == -1) {
		/*
		 * board wants something and nothing is there
		 * send a noop
		 */
		midi_send_noop();
		if (cond_status & MIDI_FBLOCK)
			wakeup(&cond_status);
		return;
	}

	if (data[0] == MIDI_TOFLOW) {
#ifdef MIDIDEBUG
		printf("conductor - sent 0x%02x\n", data[0]);
#endif
		if (!midi_send_data((u_char) data[0])) {
			log(LOG_ERR, "Youch!  Couldn't send data in Midi "
			    "CDR\n");
			return;
		}
		return;
	}

	if ((data[1] = getc(&cond_in)) == -1) {
		if (cond_status & MIDI_FBLOCK)
			wakeup(&cond_status);
		midi_send_noop();
		return;
	}
	switch (data[1])
	{
	case STRIP_IOCTL(MREQPC0):
	case STRIP_IOCTL(MREQPC1):
	case STRIP_IOCTL(MREQPC2):
	case STRIP_IOCTL(MREQPC3):
	case STRIP_IOCTL(MREQPC4):
	case STRIP_IOCTL(MREQPC5):
	case STRIP_IOCTL(MREQPC6):
	case STRIP_IOCTL(MREQPC7):
	case STRIP_IOCTL(MREQRCCLR):
	case STRIP_IOCTL(MREQVER):
	case STRIP_IOCTL(MREQREV):
	case STRIP_IOCTL(MREQTMP):
		/* these return data */
		num_to_get = 1;
		if (!midi_send_data((u_char) data[0])) {
			log(LOG_ERR, "Youch!  Couldn't send data in Midi "
			    "CDR\n");
			return;
		}
		if (!midi_send_data((u_char) data[1])) {
			log(LOG_ERR, "Youch!  Couldn't send data in Midi "
			    "CDR\n");
			return;
		}
#ifdef MIDIDEBUG
		printf("conductor - sent 0x%02x 0x%02x\n", data[0], data[1]);
#endif
		break;
	case STRIP_IOCTL(MSELTRKS):
	case STRIP_IOCTL(MSNDPLAYCNT):
	case STRIP_IOCTL(MSELCHN8):
	case STRIP_IOCTL(MSELCHN16):
	case STRIP_IOCTL(MSETBASETMP):
	case STRIP_IOCTL(MSETRELTMP):
	case STRIP_IOCTL(MSETRELTMPGRD):
	case STRIP_IOCTL(MSETMETFRQ):
	case STRIP_IOCTL(MSETBPM):
	case STRIP_IOCTL(MCLKPCRATE):
	case STRIP_IOCTL(MSETREMAP):
		/* these send a data byte */
		if ((data[2] = getc(&cond_in)) == -1) {
			midi_send_noop();
			if (cond_status & MIDI_FBLOCK)
				wakeup(&cond_status);
			return;
		}
		if (!midi_send_data((u_char) data[0])) {
			log(LOG_ERR, "Youch!  Couldn't send data in Midi "
			    "CDR\n");
			return;
		}
		if (!midi_send_data((u_char) data[1])) {
			log(LOG_ERR, "Youch!  Couldn't send data in Midi "
			    "CDR\n");
			return;
		}
		if (!midi_send_data((u_char) data[2])) {
			log(LOG_ERR, "Youch!  Couldn't send data in Midi "
			    "CDR\n");
			return;
		}
#ifdef MIDIDEBUG
		printf("conductor - sent 0x%02x 0x%02x 0x%02x\n", data[0],
		    data[1], data[2]);
#endif
		break;
	case STRIP_IOCTL(MMESTRK0):
	case STRIP_IOCTL(MMESTRK1):
	case STRIP_IOCTL(MMESTRK2):
	case STRIP_IOCTL(MMESTRK3):
	case STRIP_IOCTL(MMESTRK4):
	case STRIP_IOCTL(MMESTRK5):
	case STRIP_IOCTL(MMESTRK6):
	case STRIP_IOCTL(MMESTRK7):
	case STRIP_IOCTL(MMESSYS):
		/* not dealing with this case yet */
		return;
	default:
		/* no outgoing or incoming data */
		if (!midi_send_data((u_char) data[0])) {
			log(LOG_ERR, "Youch!  Couldn't send data in Midi "
			    "CDR\n");
			return;
		}
		if (!midi_send_data((u_char) data[1])) {
			log(LOG_ERR, "Youch!  Couldn't send data in Midi "
			    "CDR\n");
			return;
		}
#ifdef MIDIDEBUG
		printf("conductor - sent 0x%02x 0x%02x\n", data[0], data[1]);
#endif
	}
	if ((cond_in.c_cc < MIDI_LOW_MARK) && (cond_status & MIDI_WBLOCK)) {
		cond_status &= ~MIDI_WBLOCK;
		if (cond_status & MIDI_WSLEEP)
			wakeup(&cond_in);
		return;
	}
	if (cond_in.c_cc == 0 && cond_status & MIDI_FBLOCK)
		wakeup(&cond_status);
}

void
midiforceintr()
{
	register int i, j;

	/* twice to insure the right state */
	if (!midi_reset()) {
		(void)midi_reset();
	}
}
