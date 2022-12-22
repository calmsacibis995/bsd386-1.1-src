/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: conf.c,v 1.7 1994/02/06 20:13:18 karels Exp $
 */

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * (c) UNIX System Laboratories, Inc.  All or some portions of this file
 * are derived from material licensed to the University of California by
 * American Telephone and Telegraph Co. or UNIX System Laboratories, Inc.
 * and are reproduced herein with the permission of UNIX System Laboratories,
 * Inc.
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
 *	@(#)conf.c	7.3 (Berkeley) 5/4/91
 */

#include "saio.h"

int	wdstrategy __P((struct iob *, int));
int	wdopen __P((struct iob *));
void	wdclose __P((struct iob *));
#define	wdioctl	noioctl

int	fdstrategy __P((struct iob *, int));
int	fdopen __P((struct iob *));
void	fdclose __P((struct iob *));
#define	fdioctl noioctl

int	sdstrategy __P((struct iob *, int));
int	sdopen __P((struct iob *));
void	sdclose __P((struct iob *));
#define	sdioctl	noioctl

/* entries for host adapters which munge i_dev/i_adapter and call sdopen() */
int	ahaopen __P((struct iob *));
int	eahaopen __P((struct iob *));

struct devsw devsw[] = {
	{ "wd",	wdstrategy,	wdopen,	wdclose, wdioctl },	/* 0 = wd */
	{ NULL },			/* swapdev place holder */
	{ "fd",	fdstrategy,	fdopen,	fdclose, fdioctl },	/* 2 = fd */
	{ NULL },			/* XXX 3 = QIC-02/QIC-36 tape */
	{ "sd",	sdstrategy,	sdopen,	sdclose, sdioctl },	/* 4 = sd */
	{ NULL },			/* XXX 5 = SCSI tape */
	{ NULL },			/* XXX 6 */
	{ NULL },			/* XXX 7 */
	{ NULL },			/* XXX 8 */
	{ NULL },			/* XXX 9 */
	{ NULL },			/* XXX 10 */
	{ "aha", NULL,		ahaopen, NULL, NULL },
	{ "eaha", NULL,		eahaopen, NULL, NULL },
};
int	ndevs = (sizeof(devsw)/sizeof(devsw[0]));

int	bootdebug;	/* should be elsewhere? */
