/*
 * Copyright (c) 1991 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	$Id: sys.c,v 1.6 1992/09/29 01:17:48 karels Exp $
 */

#include "saio.h"
#include <ufs/dir.h>
#include <setjmp.h>
#include <string.h>


jmp_buf exception;

int errno;

struct iob iob[SOPEN_MAX];

int opendev;	/* zero for now */

void
_stop(s)
	char *s;
{
	printf("%s\n", s);
	printf("looping... press reset\n");
	for (;;)
		;
}

/*
 * Read the current filesystem struct into i_fs.
 */
int
findfs(iobp)
	register struct iob *iobp;
{
	iobp->i_bn = iobp->i_boff + SBLOCK;
	iobp->i_ma = (char *) iobp->i_buf;
	iobp->i_cc = SBSIZE;
	if (devread(iobp) == -1)
		return EIO;
	iobp->i_fs = * (struct fs *) iobp->i_buf;
	return 0;
}

/*
 * A crude version of the system's namei() function --
 * given a device and a name, find the inode.
 */
int
namei(iobp, path)
	register struct iob *iobp;
	char *path;
{
	struct fs *fs = &iobp->i_fs;
	struct dinode *dip = &iobp->i_ino;
	register char *p, *e;
	size_t len;
	register size_t off, doff;
	register struct direct *dp;
	size_t size;

	/*
	 * All lookups are absolute.
	 */
	if (errno = findinode(iobp, ROOTINO))
		return errno;

	while (*path == '/')
		++path;

	/*
	 * Loop over pathname elements.
	 */
	for (p = path; *p; p = e) {
		if (e = index(p, '/')) {
			len = e - p;
			++e;
		} else {
			len = strlen(p);
			e = p + len;
		}

		if ((iobp->i_ino.di_mode & IFMT) != IFDIR)
			return (ENOTDIR);

		for (off = 0; off < dip->di_size; off += size) {
			size = dip->di_size - off > fs->fs_bsize ?
			    fs->fs_bsize : dip->di_size - off;
			if (errno = bread(iobp, iobp->i_buf,
			    lblkno(fs, off), size))
				return (errno);
			for (doff = 0; doff < size; doff += dp->d_reclen) {
				dp = (struct direct *) &iobp->i_buf[doff];
				if (dp->d_ino == 0)
					continue;
				if (dp->d_namlen == len &&
				    bcmp(p, dp->d_name, len) == 0) {
					off = dip->di_size + 1;
					break;
				}
			}
		}

		if (off == dip->di_size)
			return (ENOENT);

		if (errno = findinode(iobp, dp->d_ino))
			return (errno);
	}

	return 0;
}

/*
 * Read an inode.
 */
int
findinode(iobp, ino)
	register struct iob *iobp;
	unsigned long ino;
{
	struct fs *fs = &iobp->i_fs;

	iobp->i_bn = fsbtodb(fs, itod(fs, ino)) + iobp->i_boff;
	iobp->i_ma = iobp->i_buf;
	iobp->i_cc = fs->fs_bsize;
	if (devread(iobp) == -1)
		return EIO;
	iobp->i_ino = ((struct dinode *) iobp->i_buf)[itoo(fs, ino)];
	return 0;
}
