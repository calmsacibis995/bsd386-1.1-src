/*	BSDI $Id: vm_swap.c,v 1.8 1993/12/29 18:30:41 karels Exp $	*/

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
 *	@(#)vm_swap.c	7.18 (Berkeley) 5/6/91
 */

#include "param.h"
#include "systm.h"
#include "buf.h"
#include "conf.h"
#include "proc.h"
#include "namei.h"
#include "dmap.h"		/* XXX */
#include "vnode.h"
#include "specdev.h"
#include "map.h"
#include "file.h"

#include "swap_pager.h"

/*
 * Indirect driver for multi-device paging.
 */

struct	swapstats swapstats;
int	nswdev2 = 1;

/*
 * Set up swap devices.
 * Initialize linked list of free swap
 * headers. These do not actually point
 * to buffers, but rather to pages that
 * are being swapped in and out.
 */
swapinit()
{
	register int i;
	register struct buf *sp = swbuf;
	struct swdevt *swp;
	int error;

	/*
	 * Count swap devices, and adjust total swap space available.
	 * Some of this space will not be available until a swapon()
	 * system is issued, usually when the system goes multi-user.
	 * Also, if we have more than one device, we currently arrange
	 * things as if there were twice as many devices, with alternating
	 * devices and holes; see the comment before swfree().
	 */
	for (swp = swdevt; swp->sw_dev; swp++) {
		swapstats.swap_nswdev++;
		if (swp->sw_nblks > swapstats.swap_max)
			swapstats.swap_max = swp->sw_nblks;
	}
	if (swapstats.swap_nswdev == 0)
		panic("swapinit");
	if (swapstats.swap_nswdev > 1) {
		swapstats.swap_max = roundup(swapstats.swap_max, dmmax);
		nswdev2 = swapstats.swap_nswdev * 2;
		swapstats.swap_max *= nswdev2;
	}
	if (bdevvp(swdevt[0].sw_dev, &swdevt[0].sw_vp))
		panic("swapvp");
	if (error = swfree(&proc0, 0))
		printf("Warning, no swap space (swfree errno %d)\n", error);

	/*
	 * Now set up swap buffer headers.
	 */
	bswlist.av_forw = sp;
	for (i = 0; i < nswbuf - 1; i++, sp++)
		sp->av_forw = sp + 1;
	sp->av_forw = NULL;
}

swstrategy(bp)
	register struct buf *bp;
{
	int sz, off, seg, index;
	register struct swdevt *sp;
	struct vnode *vp;

#ifdef GENERIC
	/*
	 * A mini-root gets copied into the front of the swap
	 * and we run over top of the swap area just long
	 * enough for us to do a mkfs and restor of the real
	 * root (sure beats rewriting standalone restor).
	 */
#define	MINIROOTSIZE	4096
	if (rootdev == dumpdev)
		bp->b_blkno += MINIROOTSIZE;
#endif
	sz = howmany(bp->b_bcount, DEV_BSIZE);
	if (bp->b_blkno + sz > swapstats.swap_max) {
		bp->b_flags |= B_ERROR;
		biodone(bp);
		return;
	}
	if (swapstats.swap_nswdev > 1) {
		off = bp->b_blkno % dmmax;
		if (off+sz > dmmax) {
			bp->b_flags |= B_ERROR;
			biodone(bp);
			return;
		}
		seg = bp->b_blkno / dmmax;
#ifdef DEBUG
		if (seg & 1)
			panic("swstrategy: block between devs");
#endif
		index = (seg % nswdev2) / 2;
		seg /= nswdev2;
		bp->b_blkno = seg*dmmax + off;
	} else
		index = 0;
	sp = &swdevt[index];
	if ((bp->b_dev = sp->sw_dev) == 0)
		panic("swstrategy");
	if (sp->sw_vp == NULL) {
		bp->b_error |= B_ERROR;
		biodone(bp);
		return;
	}
	VHOLD(sp->sw_vp);
	if ((bp->b_flags & B_READ) == 0) {
		if (vp = bp->b_vp) {
			vp->v_numoutput--;
			if ((vp->v_flag & VBWAIT) && vp->v_numoutput <= 0) {
				vp->v_flag &= ~VBWAIT;
				wakeup((caddr_t)&vp->v_numoutput);
			}
		}
		sp->sw_vp->v_numoutput++;
	}
	if (bp->b_vp != NULL)
		brelvp(bp);
	bp->b_vp = sp->sw_vp;
	VOP_STRATEGY(bp);
}

/*
 * System call swapon(name) enables swapping on device name,
 * which must be in the swdevsw.  Return EBUSY
 * if already swapping on this device.
 */
/* ARGSUSED */
swapon(p, uap, retval)
	struct proc *p;
	struct args {
		char	*name;
	} *uap;
	int *retval;
{
	register struct vnode *vp;
	register struct swdevt *sp;
	register struct nameidata *ndp;
	dev_t dev;
	int error;
	struct nameidata nd;

	if (error = suser(p->p_ucred, &p->p_acflag))
		return (error);
	ndp = &nd;
	ndp->ni_nameiop = LOOKUP | FOLLOW;
	ndp->ni_segflg = UIO_USERSPACE;
	ndp->ni_dirp = uap->name;
	if (error = namei(ndp, p))
		return (error);
	vp = ndp->ni_vp;
	if (vp->v_type != VBLK) {
		vrele(vp);
		return (ENOTBLK);
	}
	dev = (dev_t)vp->v_rdev;
	if (major(dev) >= nblkdev) {
		vrele(vp);
		return (ENXIO);
	}
	for (sp = &swdevt[0]; sp->sw_dev; sp++)
		if (sp->sw_dev == dev) {
			if (sp->sw_freed) {
				vrele(vp);
				return (EBUSY);
			}
			sp->sw_vp = vp;
			if (error = swfree(p, sp - swdevt)) {
				vrele(vp);
				return (error);
			}
			return (0);
		}
	vrele(vp);
	return (EINVAL);
}

/*
 * Swfree(index) frees the index'th portion of the swap map.
 * Each of the swapstats.swap_nswdev devices provides 1/swap_nswdev'th
 * of the swap space, which is laid out with blocks of dmmax pages circularly
 * among the devices.
 *
 * Currently, if there is more than one device, we lay out the space
 * as if there were 2 * swapstats.swap_nswdev devices, with the odd devices
 * missing, to prevent allocations from spanning devices.
 */
swfree(p, index)
	struct proc *p;
	int index;
{
	register struct swdevt *sp;
	register swblk_t vsbase;
	register long blk;
	struct vnode *vp;
	register swblk_t dvbase;
	register int nblks;
	int error;

	sp = &swdevt[index];
	vp = sp->sw_vp;
	if (error = VOP_OPEN(vp, FREAD|FWRITE, p->p_ucred, p))
		return (error);
	sp->sw_freed = 1;
	swapstats.swap_devs++;
	nblks = sp->sw_nblks;
	for (dvbase = 0; dvbase < nblks; dvbase += dmmax) {
		blk = nblks - dvbase;
		if ((vsbase = 2 * index * dmmax + dvbase * nswdev2) >=
		    swapstats.swap_max)
			panic("swfree");
		if (blk > dmmax)
			blk = dmmax;
		if (vsbase == 0) {
			/*
			 * First of all chunks... initialize the swapmap.
			 */
			rminit(swapmap, (long) 0, (long) 0, "swap", nswapmap);
			swapstats.swap_mapsize = nswapmap;
		}
		if (dvbase == 0) {
			/*
			 * Don't use the first cluster of the device
			 * in case it starts with a label or boot block.
			 */
			blk -= ctod(CLSIZE);
			vsbase += ctod(CLSIZE);
		}
		rmfree(swapmap, blk, vsbase);
		swapstats.swap_total += blk;
		swapstats.swap_free += blk;
	}
	return (0);
}
