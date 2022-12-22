/*
 * Copyright (c) 1991 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	$Id: close.c,v 1.3 1992/01/04 18:48:50 kolstad Exp $
 */

#include "saio.h"

/*
 * Close a file.
 */
int
close(fd)
	int fd;
{
	if (fd < 0 || fd >= SOPEN_MAX + 3) {
		errno = EBADF;
		return -1;
	}
	if (fd < 3)
		return 0;
	fd -= 3;
	if ((iob[fd].i_flgs & F_ALLOC) == 0) {
		errno = EBADF;
		return -1;
	}

	devclose(&iob[fd]);

	bzero(&iob[fd], sizeof iob[fd]);
	return 0;
}
