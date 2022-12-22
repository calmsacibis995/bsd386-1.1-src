/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: pcaux.c,v 1.2 1994/01/12 03:58:14 karels Exp $
 * modified by BSDI as of the date in the line above
 */

/*
 * Copyright (c) Computer Newspaper Services Limited 1993
 * All rights reserved. 
 * 
 * License to use, copy, modify, and distribute this work and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that you also ensure modified files carry prominent notices
 * stating that you changed the files and the date of any change, ensure
 * that the above copyright notice appear in all copies, that both the
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Computer Newspaper Services not
 * be used in advertising or publicity pertaining to distribution or use
 * of the work without specific, written prior permission from Computer
 * Newspaper Services.
 * 
 * By copying, distributing or modifying this work (or any derived work)
 * you indicate your acceptance of this license and all its terms and
 * conditions.
 * 
 * COMPUTER NEWSPAPER SERVICES PROVIDE THIS SOFTWARE "AS IS", WITHOUT
 * ANY WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING, BUT
 * NOT LIMITED TO ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR
 * A PARTICULAR PURPOSE, AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.  THE
 * ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE SOFTWARE,
 * INCLUDING ANY DUTY TO SUPPORT OR MAINTAIN, BELONGS TO THE LICENSEE.
 * SHOULD ANY PORTION OF THE SOFTWARE PROVE DEFECTIVE, THE LICENSEE (NOT
 * COMPUTER NEWSPAPER SERVICES) ASSUMES THE ENTIRE COST OF ALL
 * SERVICING, REPAIR AND CORRECTION.  IN NO EVENT SHALL COMPUTER
 * NEWSPAPER SERVICES BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
 * USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * Id: pcaux.c,v 1.4 1993/03/12 17:42:40 jsp Exp
 */

/*
 * Information Systems Engineering Group
 * Jan-Simon Pendry
 */

/*
 * code to work ps/2 style auxilliary port
 * on keyboard controller for use as a mouse port.
 */

#include "sys/param.h"
#include "sys/conf.h"
#include "sys/ioctl.h"
#include "sys/proc.h"
#include "sys/file.h"
#include "sys/user.h"
#include "sys/tty.h"
#include "sys/uio.h"
#include "sys/callout.h"
#include "sys/systm.h"
#include "sys/kernel.h"
#include "sys/reboot.h"
#include "sys/syslog.h"
#include "sys/device.h"

#include "icu.h"
#include "isavar.h"

#include "ic/i8042.h"

#define S_NOBLOCK	0	/* XXX from pccons.c */
#define S_NBLOCK	1

struct	pcauxsoftc {
	struct	device cs_dev;	/* base device */
	struct 	isadev cs_id;	/* ISA device */
	struct	intrhand cs_ih;	/* interrupt vectoring */
	struct	tty cs_tty;	/* tty struct */
#if 0
	struct	ttydevice_tmp cs_ttydev;	/* tty stuff */
#endif
};

extern struct tty *pcauxtty;	/* in pccons.c */

static struct termios auxtermios = {
	( 0 ),
	( 0 ),
	( CREAD | CS8 ),
	( 0 ),
	{ _POSIX_VDISABLE, _POSIX_VDISABLE, _POSIX_VDISABLE, _POSIX_VDISABLE,
	  _POSIX_VDISABLE, _POSIX_VDISABLE, _POSIX_VDISABLE, _POSIX_VDISABLE,
	  _POSIX_VDISABLE, _POSIX_VDISABLE, _POSIX_VDISABLE, _POSIX_VDISABLE,
	  _POSIX_VDISABLE, _POSIX_VDISABLE, _POSIX_VDISABLE, _POSIX_VDISABLE,
	  1,               0,               _POSIX_VDISABLE, _POSIX_VDISABLE },
	( B1200 ),
	( B1200 ),
};

int	pcxprobe();
void	pcxattach();
int	pcxstart();
int	pcxparam();

struct cfdriver pcauxcd = 
	{ NULL, "pcaux", pcxprobe, pcxattach, sizeof(struct pcauxsoftc) };

extern	int kb_command __P((int port, u_char data));
extern	int kb_data_cmd __P((int port, u_char cmd, u_char data));

#define	setcmdbyte(val)	kb_data_cmd(KBSTAT, KCMD_WCMD, (val))

pcxprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	extern int pccmdbyte;
	int i;
	char *s;

	if (ia->ia_iobase != KBDATA)
		return (0);

	/*
	 * Set command byte "disable aux device" bit
	 * so that we can see if the enable aux device command
	 * clears the bit.  Also enable aux device interrupts.
	 */
	if (setcmdbyte(pccmdbyte|KBC_DISABLEAUX|KBC_ENABLEAUXI) == -1) {
		s = "setcmdbyte";
		goto timeo;
	}

	/* enable aux port interrupt */
	if (kb_data_cmd(KBSTAT, KCMD_WCMD,
	    pccmdbyte|KBC_ENABLEAUXI|KBC_DISABLEAUX) == -1) {
		s = "enable aux port";
		goto timeo;
	}

	kb_command(KBSTAT, KCMD_ENABLEAUX);

	/* read command byte to check whether KBC_DISABLEAUX cleared */
	if (kb_command(KBSTAT, KCMD_RCMD) == -1) {
		s = "read cmd byte";
		goto timeo;
	}
	i = 1000000;
	while ((inb(KBSTAT)&KBS_ORDY) == 0)
		if (--i <= 0) {
			s = "output cmd byte";
			goto timeo;
		}
	if (inb(KBDATA) & KBC_DISABLEAUX) {
		setcmdbyte(pccmdbyte);
		return (0);
	}
	pccmdbyte |= KBC_ENABLEAUXI;
	/* setcmdbyte(pccmdbyte); should still be set */

	/* write ENABLE cmd to aux port */
	if (kb_data_cmd(KBSTAT, KCMD_WRITEAUX, KAUX_ENABLE) == -1) {
		s = "enable command";
		goto timeo;
	}

	ia->ia_irq = IRQ12;	/* mouse always here */
	return (1);

timeo:
	/* warn about hangs in case keyboard controller is wedged */
	printf("pcaux%d: %s: timeout\n", cf->cf_unit, s);
	return (0);
}

void
pcxattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;	
{
	register struct pcauxsoftc *sc = (struct pcauxsoftc *) self;
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	int pcxrint();

	printf("\n");

	sc->cs_ih.ih_fun = pcxrint;
	sc->cs_ih.ih_arg = sc;
	intr_establish(ia->ia_irq, &sc->cs_ih, DV_TTY);
	pcauxtty = &sc->cs_tty;
#if 0
	strcpy(sc->cs_ttydev.tty_name, pcauxcd.cd_name);
	sc->cs_ttydev.tty_count = 1;
	sc->cs_ttydev.tty_ttys = &sc->cs_tty;
	tty_attach(&sc->cs_ttydev);
#endif
}

/* ARGSUSED */
#ifdef __STDC__
pcxopen(dev_t dev, int flag, int mode, struct proc *p)
#else
pcxopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
#endif
{
	register struct tty *tp;

	tp = pcauxtty;
	if (tp == NULL || minor(dev) != 0)
		return (ENXIO);
	tp->t_oproc = pcxstart;
	tp->t_param = pcxparam;
	tp->t_dev = dev;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		tp->t_state |= TS_WOPEN;
		tp->t_termios = auxtermios;
		ttsetwater(tp);
	} else if (tp->t_state&TS_XCLUDE && p->p_ucred->cr_uid != 0)
		return (EBUSY);
	tp->t_state |= TS_CARR_ON;

	/* write ENABLE cmd to aux port */
	if (kb_data_cmd(KBSTAT, KCMD_WRITEAUX, KAUX_ENABLE) == -1)
		printf("pcaux%d: enable command: timeout\n", minor(dev));

	return ((*linesw[tp->t_line].l_open)(dev, tp));
}

pcxclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct tty *tp;
	
	tp = pcauxtty;
	(*linesw[tp->t_line].l_close)(tp, flag);
	ttyclose(tp);
	return (0);
}

/*ARGSUSED*/
pcxread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
{
	struct tty *tp;
	
	tp = pcauxtty;
	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}

/*ARGSUSED*/
pcxwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
{
	struct tty *tp;

	tp = pcauxtty;
	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

/*ARGSUSED*/
pcxselect(dev, flag, p)
	dev_t dev;
	int flag;
	struct proc *p;
{
	struct tty *tp;

	tp = pcauxtty;
	return (ttyselect(tp, flag, p));
}

/*
 * Field an interrupt from the aux port controller.
 * This is a shared device with pccons0 so let
 * that driver handle the interrupt.
 */
pcxrint(sc)
	struct pcauxsoftc *sc;
{

	(void) sgetc(pcauxtty, S_NOBLOCK);
	return (1);
}

pcxstart(tp)
	struct tty *tp;
{
	int s, c;

	while (tp->t_outq.c_cc) {
		s = spltty();
		c = getc(&tp->t_outq);
		if (kb_data_cmd(KBSTAT, KCMD_WRITEAUX, c) == -1)
			log(LOG_ERR, "pcaux: timeout writing aux device\n");
		splx(s);
	}
}

pcxparam(tp, t)
	register struct tty *tp;
	register struct termios *t;
{

        tp->t_ispeed = t->c_ispeed;
        tp->t_ospeed = t->c_ospeed;
        tp->t_cflag = t->c_cflag;

	return (0);
}

pcxioctl(dev, cmd, data, flag, p)
	dev_t dev;
	caddr_t data;
	struct proc *p;
{
	int error;
	struct tty *tp;

	tp = pcauxtty;

	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
	if (error >= 0)
		return (error);
	error = ttioctl(tp, cmd, data, flag);
	if (error >= 0)
		return (error);
	return (ENOTTY);
}
