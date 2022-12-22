/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: slattach.c,v 1.3 1993/12/19 19:26:37 sanders Exp $
 */
 
/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rick Adams.
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
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1988 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)slattach.c	4.6 (Berkeley) 6/1/90";
#endif /* not lint */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <paths.h>
#include <errno.h>
#include <err.h>

#define DEFAULT_BAUD	9600

int slipdisc = SLIPDISC;

char devname[MAXPATHLEN];
char hostname[MAXHOSTNAMELEN];

main(argc, argv)
	int argc;
	char *argv[];
{
	register int fd;
	register char *dev = argv[1];
	struct termios tio;
	speed_t speed;

	if (argc < 2 || argc > 3)
		errx(1, "Usage: %s ttyname [baudrate]\n", argv[0]);
	speed = argc == 3 ? atoi(argv[2]) : DEFAULT_BAUD;

	if (strncmp(_PATH_DEV, dev, sizeof(_PATH_DEV) - 1)) {
		(void) sprintf(devname, "%s/%s", _PATH_DEV, dev);
		dev = devname;
	}
	/* force the open with O_NONBLOCK and then set CLOCAL later */
	if ((fd = open(dev, O_RDWR | O_NONBLOCK)) < 0)
		err(1, "%s", dev);
	if (tcgetattr(fd, &tio) != 0)
		err(1, "%s", dev);
	cfsetspeed(&tio, speed);
	cfmakeraw(&tio);
	tio.c_cflag |= CLOCAL;
	if (tcsetattr(fd, TCSAFLUSH, &tio) != 0)
		err(1, "%s", dev);
	if (fcntl(fd, F_SETFL, 0) == -1)	/* clear O_NONBLOCK */
		err(1, "%s", dev);
	if (ioctl(fd, TIOCSETD, &slipdisc) < 0)
		err(1, "%s", dev);
	daemon(0, 1);
	while (1)
		sigpause(0L);
}
