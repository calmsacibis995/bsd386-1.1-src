/*
 * Copyright (c) 1991 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	$Id: ioctl.c,v 1.3 1992/01/04 18:48:57 kolstad Exp $
 */

#include "saio.h"

/*
 * Ioctl call for standalone code.
 */
int
ioctl(fd, command, arg)
	int fd;
	u_long command;
	caddr_t arg;
{
	register struct iob *iobp;

	if (fd < 0 || fd >= SOPEN_MAX + 3) {
		errno = EBADF;
		return -1;
	}
	if (fd < 3) {
		errno = EIO;
		return -1;
	}
	fd -= 3;
	iobp = &iob[fd];
	if ((iobp->i_flgs & F_ALLOC) == 0) {
		errno = EBADF;
		return -1;
	}

	if (devioctl(iobp, command, arg) == -1) {
		errno = EIO;
		return -1;
	}
	return 0;
}
