/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: termctl.c,v 1.1 1993/12/18 23:21:40 sanders Exp $
 */

#include <stdio.h>
#include <strings.h>
#include <termios.h>
#include <sys/ioctl.h>
#include "tip.h"

set_clocal(fd)
	int fd;
{
	struct termios t;

	tcgetattr(fd, &t);
	t.c_cflag |= CLOCAL;
	tcsetattr(fd, TCSANOW, &t);
}

set_hupcl(fd)
	int fd;
{
	struct termios t;

	tcgetattr(fd, &t);
	t.c_cflag |= HUPCL;
	tcsetattr(fd, TCSANOW, &t);
}

set_exclusive(fd)
	int fd;
{
	ioctl(fd, TIOCEXCL, 0);
}

clear_exclusive(fd)
	int fd;
{
	ioctl(fd, TIOCNXCL, 0);
}

/*
 * Disable some of the special input characters
 */
disable_chars(tt)
	struct termios *tt;
{
	int disable = _POSIX_VDISABLE;

	tt->c_cc[VINTR] = disable;
	tt->c_cc[VQUIT] = disable;
	tt->c_cc[VSUSP] = disable;
#ifdef __bsdi__
	tt->c_cc[VDSUSP] = disable;
	tt->c_cc[VDISCARD] = disable;
	tt->c_cc[VLNEXT] = disable;
#endif
}

tty_setup(fd, tt)
	int fd;
	struct termios *tt;
{
        tcsetattr(fd, TCSAFLUSH, tt);
}
 
/*
 * Set up the "remote" tty's state
 */     
remote_ttysetup(fd, speed, mode)
	int fd;
        int speed;
	char *mode;
{
        struct termios t;
	char *fail;
  
        tcgetattr(fd, &t);
        cfsetspeed(&t, speed);
        cfmakeraw(&t);
        if (boolean(value(TAND)))
                t.c_iflag |= IXOFF;
        else
                t.c_iflag &= ~IXOFF;
	if (mode && (cfsetterm(&t, mode, &fail) != 0))
		fprintf(stderr, "cfsetterm: %s: Bad value\n", fail);
        tcsetattr(fd, TCSAFLUSH, &t);
}

/*
 * setup local tty modes
 */
local_ttysetup(fd, tt1, tt2)
	int fd;
	struct termios *tt1;
	struct termios *tt2;
{
        tcgetattr(0, tt1);			/* save defaults */
        tcgetattr(0, tt2);			/* change this one */
	cfmakeraw(tt2);
        tt2->c_lflag |= ISIG;			/* XXX: keep ISIG? */
        disable_chars(tt2);
        tcsetattr(0, TCSAFLUSH, tt2);
}

/*
 * Turn tandem mode on or off for remote tty.
 */
tandem(fd, option)
	int fd;
        char *option;
{
        struct termios t;
	extern struct termios tt_raw;

        tcgetattr(fd, &t);
        if (strcmp(option,"on") == 0) {
                t.c_iflag |= IXOFF;
                tt_raw.c_iflag |= IXOFF;
        } else {
                t.c_iflag &= ~IXOFF;
                tt_raw.c_iflag &= ~IXOFF;
        }
        tcsetattr(fd, TCSAFLUSH, &t);
        tcsetattr(0, TCSAFLUSH, &tt_raw);
}

/*
 * Send a break.
 */
genbrk()
{
	extern int FD;

        tcsendbreak(FD, 0);
}
