/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: clocal.c,v 1.2 1993/12/23 18:29:39 sanders Exp $
 */

#include <stdio.h>
#include <sys/termios.h>

extern int BR;

set_clocal(fd)
	int fd;
{
	struct termios t;

	tcgetattr(fd, &t);
	t.c_iflag &= IXON | IXOFF;
	t.c_iflag |= IGNBRK | IGNPAR;
	t.c_oflag = 0;
	t.c_cflag &= CCTS_OFLOW | CRTS_IFLOW;
	t.c_cflag |= CLOCAL | CS8 | CREAD | HUPCL;
	t.c_lflag = 0;
	t.c_cc[VMIN] = 1;
	t.c_cc[VTIME] = 0;
	if (BR != -1)
		cfsetspeed(&t, BR);
	tcsetattr(fd, TCSANOW, &t);
}

set_lparms(fd)
	int fd;
{
	struct termios t;

	(void) tcgetattr(fd, &t);
	t.c_iflag &= IXON | IXOFF;
	t.c_iflag |= IGNBRK | IGNPAR;
	t.c_oflag = 0;
	t.c_cflag &= CCTS_OFLOW | CRTS_IFLOW;
	t.c_cflag |= CS8 | CREAD | HUPCL;
	t.c_lflag = 0;
	t.c_cc[VMIN] = 1;
	t.c_cc[VTIME] = 0;
	if (BR != -1)
		cfsetspeed(&t, BR);
	(void) tcsetattr(fd, TCSANOW, &t);
}

set_hupcl(fd)
	int fd;
{
	struct termios t;

	tcgetattr(fd, &t);
	t.c_cflag |= HUPCL;
	tcsetattr(fd, TCSANOW, &t);
}
