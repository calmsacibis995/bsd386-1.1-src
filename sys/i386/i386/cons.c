/*	BSDI $Id: cons.c,v 1.8 1994/01/30 01:52:04 karels Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	@(#)cons.c	7.2 (Berkeley) 5/9/91
 */


#include "sys/param.h"
#include "sys/proc.h"
#include "sys/user.h"
#include "sys/systm.h"
#include "sys/buf.h"
#include "sys/ioctl.h"
#include "sys/stat.h"
#include "sys/tty.h"
#include "sys/file.h"
#include "sys/conf.h"

#include "cons.h"

extern	struct consdev constab[];
struct	tty *constty = 0;	/* virtual console output device */
struct	consdev *cn_tab;	/* physical console device info */
struct	tty *cn_tty;		/* XXX: console tty struct for w(1) */

cninit()
{
	register struct consdev *cp;

	/*
	 * Collect information about all possible consoles
	 * and find the one with highest priority
	 */
	for (cp = constab; cp->cn_probe; cp++) {
		(*cp->cn_probe)(cp);
		if (cp->cn_pri > CN_DEAD &&
		    (cn_tab == NULL || cp->cn_pri > cn_tab->cn_pri))
			cn_tab = cp;
	}
	/*
	 * No console, we can handle it
	 */
	if ((cp = cn_tab) == NULL)
		return;
	/*
	 * Turn on console
	 */
	cn_tty = cp->cn_tp;
	(*cp->cn_init)(cp);
}

/*
 * cnopen and cnclose pass S_IFLNK instead of (presumably) S_IFCHR
 * as the mode, so that the driver can record whether the normal device
 * and the console are open separately (so that close happens only when
 * both are closed).
 */
cnopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	if (cn_tab == NULL)
		return (0);
	dev = cn_tab->cn_dev;
	return ((*cdevsw[major(dev)].d_open)(dev, flag, S_IFLNK, p));
}
 
cnclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	if (cn_tab == NULL)
		return (0);
	dev = cn_tab->cn_dev;
	return ((*cdevsw[major(dev)].d_close)(dev, flag, S_IFLNK, p));
}
 
cnread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
{
	if (cn_tab == NULL)
		return (0);
	dev = cn_tab->cn_dev;
	return ((*cdevsw[major(dev)].d_read)(dev, uio, flag));
}
 
cnwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
{
	if (cn_tab == NULL)
		return (0);
	dev = cn_tab->cn_dev;
	return ((*cdevsw[major(dev)].d_write)(dev, uio, flag));
}
 
cnioctl(dev, cmd, data, flag, p)
	dev_t dev;
	caddr_t data;
	struct proc *p;
{
	int error;

	if (cn_tab == NULL)
		return (0);
	/*
	 * Superuser can always use this to wrest control of console
	 * output from the "virtual" console.
	 */
	if (cmd == TIOCCONS && constty) {
		error = suser(p->p_ucred, (u_short *) NULL);
		if (error)
			return (error);
		constty = NULL;
		return (0);
	}
	dev = cn_tab->cn_dev;
	return ((*cdevsw[major(dev)].d_ioctl)(dev, cmd, data, flag, p));
}

/*ARGSUSED*/
cnselect(dev, rw, p)
	dev_t dev;
	int rw;
	struct proc *p;
{
	if (cn_tab == NULL)
		return (1);
	dev = cn_tab->cn_dev;
	return ((*cdevsw[major(dev)].d_select)(dev, rw, p));
}

cngetc()
{
	if (cn_tab == NULL)
		return (0);
	return ((*cn_tab->cn_getc)(cn_tab->cn_dev));
}

cnputc(c)
	register int c;
{
	if (cn_tab == NULL)
		return;
	if (c) {
		if (c == '\n')
			(*cn_tab->cn_putc)(cn_tab->cn_dev, '\r');
		(*cn_tab->cn_putc)(cn_tab->cn_dev, c);
	}
}

/*
 * Ask a yes/no question on the console, read and return the result as 0/1
 * (default is no/0).  If there is no console, the result is always 0.
 * Appends " (y/n)? " to the supplied question.
 */
cnquery(s)
	char *s;
{
	int c;

	printf("%s (y/n)? ", s);
	if (c = cngetc())
		cnputc(c);
	return (c == 'y' || c == 'Y');
}
