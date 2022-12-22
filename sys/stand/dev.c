/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: dev.c,v 1.4 1992/09/29 01:04:06 karels Exp $
 */

/*
 * Copyright (c) 1982, 1986, 1988 Regents of the University of California.
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
 *	@(#)dev.c	7.14 (Berkeley) 5/5/91
 */

#include <sys/param.h>
#include <setjmp.h>
#include "saio.h"

/*
 * NB: the value "io->i_dev", used to offset the devsw[] array in the
 * routines below, is munged by the machine specific stand Makefiles
 * to work for certain boots.
 */

jmp_buf exception;

int
devread(io)
	register struct iob *io;
{
	int cc;

	io->i_flgs |= F_RDDATA;
	io->i_error = 0;
	cc = (*devsw[io->i_dev].dv_strategy)(io, F_READ);
	io->i_flgs &= ~F_TYPEMASK;
#ifndef SMALL
	if (scankbd())
		_longjmp(exception, 1);
#endif
	return (cc);
}

int
devwrite(io)
	register struct iob *io;
{
	int cc;

	io->i_flgs |= F_WRDATA;
	io->i_error = 0;
	cc = (*devsw[io->i_dev].dv_strategy)(io, F_WRITE);
	io->i_flgs &= ~F_TYPEMASK;
#ifndef SMALL
	if (scankbd())
		_longjmp(exception, 1);
#endif
	return (cc);
}

int
devopen(io)
	register struct iob *io;
{

	return ((*devsw[io->i_dev].dv_open)(io));
}

void
devclose(io)
	register struct iob *io;
{
	(*devsw[io->i_dev].dv_close)(io);
}

int
devioctl(io, cmd, arg)
	register struct iob *io;
	int cmd;
	caddr_t arg;
{
	return ((*devsw[io->i_dev].dv_ioctl)(io, cmd, arg));
}

/* ARGSUSED */
void
nullsys(io)
	struct iob *io;
{}

#ifndef SMALL
/* ARGSUSED */
int
nodev(io)
	struct iob *io;
{
	errno = EBADF;
	return(-1);
}
#endif

/* ARGSUSED */
int
noioctl(io, cmd, arg)
	struct iob *io;
	int cmd;
	caddr_t arg;
{
	return (ECMD);
}
