/*
 * Copyright (c) 1991 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	$Id: write.c,v 1.3 1992/02/20 22:52:37 karels Exp $
 */

#include "saio.h"

/*
 * Write system call for standalone code.
 * Not currently supported.
 */
int
write(fd, b0, s0)
	int fd;
	void *b0;
	size_t s0;
{
	struct iob *iobp;
	char *buf = b0;
	size_t size = s0;

	if (fd < 0 || fd >= SOPEN_MAX + 3) {
		errno = EBADF;
		return -1;
	}

	if (fd < 3) {
		while (size-- > 0)
			putchar(*buf++);
		return s0;
	}
	fd -= 3;
	iobp = &iob[fd];
	if ((iobp->i_flgs & F_ALLOC) == 0) {
		errno = EBADF;
		return -1;
	}

	if ((iobp->i_flgs & F_FILE) == 0) {
		iobp->i_ma = buf;
		iobp->i_cc = size;
		if (devwrite(iobp) == -1) {
			errno = EIO;
			return -1;
		}
		return s0;
	}

	/* not yet */
	errno = EIO;
	return -1;
}
