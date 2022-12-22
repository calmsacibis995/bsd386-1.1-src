/*
 * Copyright (c) 1991 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	$Id: bmap.c,v 1.4 1992/01/04 18:48:47 kolstad Exp $
 *
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
 *	from @(#)ufs_bmap.c	7.13 (Berkeley) 5/8/91
 */

#include "saio.h"

/*
 * Return the physical block number on a disk,
 * given the logical block number in an inode.
 * Returns ~0 on errors.
 */
int
bmap(iobp, bn, bnp)
	register struct iob *iobp;
	register daddr_t bn;
	daddr_t *bnp;
{
	register struct dinode *dip = &iobp->i_ino;
	register struct fs *fs = &iobp->i_fs;
	register daddr_t nb;
	daddr_t *bap;
	int i, j, sh;

	if (bn < 0)
		return (EIO);

	/*
	 * The first NDADDR blocks are direct blocks.
	 */
	if (bn < NDADDR) {
		nb = dip->di_db[bn];
		if (nb == 0) {
			*bnp = (daddr_t)-1;
			return (0);
		}
		*bnp = fsbtodb(fs, nb) + iobp->i_boff;
		return (0);
	}

	/*
	 * Determine the number of levels of indirection.
	 */
	sh = 1;
	bn -= NDADDR;
	for (j = NIADDR; j > 0; j--) {
		sh *= NINDIR(fs);
		if (bn < sh)
			break;
		bn -= sh;
	}
	if (j == 0)
		return (EIO);

	/*
	 * Fetch through the indirect blocks.
	 */
	nb = dip->di_ib[NIADDR - j];
	if (nb == 0) {
		*bnp = (daddr_t)-1;
		return (0);
	}
	for (; j <= NIADDR; j++) {
		if (iobp->i_ibno[NIADDR - j] != nb) {
			/* cache miss */
			iobp->i_bn = fsbtodb(fs, nb) + iobp->i_boff;
			iobp->i_ma = (char *) &iobp->i_iblock[NIADDR - j][0];
			iobp->i_cc = fs->fs_bsize;
			if (devread(iobp) == -1)
				return EIO;
			iobp->i_ibno[NIADDR - j] = nb;
		}
		bap = iobp->i_iblock[NIADDR - j];
		sh /= NINDIR(fs);
		i = (bn / sh) % NINDIR(fs);
		nb = bap[i];
		if (nb == 0) {
			*bnp = (daddr_t)-1;
			return (0);
		}
	}

	*bnp = fsbtodb(fs, nb) + iobp->i_boff;
	return (0);
}


/*
 * Read size bytes at logical fs block lbn in the file described by iobp
 * into the address addr.  The caller warrants that the transfer is
 * block-aligned and contiguous.
 *
 * There was no bread() on the net2 distribution, but ours is simple.
 */
int
bread(iobp, addr, lbn, size)
	register struct iob *iobp;
	char *addr;
	daddr_t lbn;
	size_t size;
{
	daddr_t pbn;

	if (errno = bmap(iobp, lbn, &pbn))
		return errno;
	if (pbn == (daddr_t) -1) {
		/* a hole */
		bzero(addr, size);
		return 0;
	}
	iobp->i_bn = pbn;
	iobp->i_ma = addr;
	iobp->i_cc = size;
	if (devread(iobp) == -1)
		return EIO;
	return 0;
}
