/*-
 * Copyright (c) 1991 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: read.c,v 1.6 1992/08/05 20:45:00 karels Exp $
 */

/*
 * Copyright (c) 1982, 1986, 1989 Regents of the University of California.
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
 *	from @(#)ufs_vnops.c	7.64 (Berkeley) 5/16/91
 */

#include "saio.h"

/*
 * Read from a file.
 * This code assumes that devread() can transfer to
 * a buffer that is not block-aligned in memory.
 */
int
read(fd, b0, count)
	int fd;
	void *b0;
	register size_t count;
{
	register char *buf = b0;
	register struct iob *iobp;
	register struct fs *fs;
	register struct dinode *dip;
	register daddr_t lbn;
	off_t off;
	size_t size, resid;
	long n, on;
	char *addr;

	if (fd < 0 || fd >= SOPEN_MAX + 3) {
		errno = EBADF;
		return -1;
	}

	if (fd < 3) {
#ifndef SMALL
		gets(buf);
		return strlen(buf);	/* XXX + 1? */
#else
		errno = EBADF;
		return -1;
#endif
	}
	fd -= 3;
	iobp = &iob[fd];
	if ((iobp->i_flgs & F_ALLOC) == 0) {
		errno = EBADF;
		return -1;
	}
	if ((iobp->i_flgs & F_FILE) == 0) {
		iobp->i_ma = buf;
		iobp->i_cc = count;
		if (devread(iobp) == -1) {
			errno = EIO;
			return -1;
		}
		return count;
	}
	fs = &iobp->i_fs;
	dip = &iobp->i_ino;
	off = iobp->i_off;

	/*
	 * This is a simplified version of the kernel's ufs_read().
	 */
	if (count == 0)
		return 0;
	if (off < 0)
		return EIO;
	if (off >= dip->di_size)
		return 0;
	resid = MIN(off + count, dip->di_size) - off;

	do {
		lbn = lblkno(fs, off);
		on = blkoff(fs, off);
		n = MIN((unsigned)(fs->fs_bsize - on), resid);
		size = dblksize(fs, dip, lbn);
		if (size <= resid && on == 0)
			addr = buf;
		else
			addr = iobp->i_buf;
		if (errno = bread(iobp, addr, lbn, size))
			return (errno);
		if (addr == iobp->i_buf)
			bcopy(&iobp->i_buf[on], buf, n);
		buf += n;
		off += n;
		resid -= n;
	} while (resid > 0);

	iobp->i_off += count;
	return count;
}
