/*
 * Copyright (c) 1991 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	$Id: lseek.c,v 1.2 1992/01/04 18:48:59 kolstad Exp $
 */

#include "saio.h"

/*
 * Standalone lseek() call.
 * We can't include unistd.h to get cookie names
 * because we don't want the standard prototypes...
 */
off_t
lseek(fd, offset, cookie)
	register int fd;
	off_t offset;
	int cookie;
{
	if (fd < 0 || fd > SOPEN_MAX + 3) {
		errno = EBADF;
		return -1;
	}
	if (fd < 3) {
		errno = EIO;
		return -1;
	}
	fd -= 3;
	if ((iob[fd].i_flgs & F_ALLOC) == 0) {
		errno = EBADF;
		return -1;
	}

	switch (cookie) {
	case 0:
		if (offset < 0) {
			errno = EIO;
			return -1;
		}
		iob[fd].i_off = offset;
		break;
	case 1:
		if (iob[fd].i_off + offset < 0) {
			errno = EIO;
			return -1;
		}
		iob[fd].i_off += offset;
		break;
	case 2:
		if ((iob[fd].i_flgs & F_FILE) == 0 ||
		    iob[fd].i_ino.di_size + offset < 0) {
			errno = EIO;
			return -1;
		}
		iob[fd].i_off = iob[fd].i_ino.di_size + offset;
		break;
	default:
		errno = EIO;
		return -1;
	}

	return iob[fd].i_off;
}
