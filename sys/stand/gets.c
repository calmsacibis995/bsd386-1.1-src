/*
 * Copyright (c) 1991 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	$Id: gets.c,v 1.5 1992/01/04 18:48:55 kolstad Exp $
 */

#include "saio.h"

/*
 * That same old ugly gets() routine, but for standalone code.
 * We assume that the buffer is at least 100 bytes in size.
 * We also assume that the local getchar() always returns a byte, not EOF.
 * We further assume that CRT-style erase processing works.
 * Erase characters are DEL and backspace; kill is ^U or ^W.
 */
char *
gets(buf)
	char *buf;
{
	register char *cp = buf;
	register char *limit = &buf[99];
	register int c;

	while ((c = getchar()) != '\n') {
		if (c == '\025' || c == '\027') {
			/* ^U or ^W: line kill */
			putchar('^');
			putchar(c == '\025' ? 'U' : 'W');
			putchar('\n');
			cp = buf;
			continue;
		}

		if (c == '\b' || c == '\177') {
			/* ^H or DEL: erase */
			if (cp == buf)
				continue;
			putchar('\b');
			putchar(' ');
			putchar('\b');
			--cp;
			continue;
		}

		*cp = c;
		if (++cp >= limit)
			break;
	}
	*cp = '\0';
	return buf;
}

#ifndef SMALL
/*
 * A standalone version of a C library getc() function.
 * We assume that this file is only accessed using getc() (blech).
 */
int
getc(fd)
	int fd;
{
	register struct iob *iobp;
	off_t off;
	int count;

	if (fd < 3)
		return getchar();
	if ((iob[fd - 3].i_flgs & F_ALLOC) == 0)
		return -1;
	iobp = &iob[fd - 3];

	if (iobp->i_off >= iobp->i_ino.di_size)
		return -1;

	off = blkoff(&iobp->i_fs, iobp->i_off);
	if (off) {
		++iobp->i_off;
		return iobp->i_buf[off];
	}

	off = iobp->i_off;
	if (read(fd, iobp->i_buf, iobp->i_fs.fs_bsize) == -1)
		return -1;
	iobp->i_off = off + 1;
	return iobp->i_buf[0];
}
#endif
