/*-
 * Copyright (c) 1991, 1992, 1994 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: vfs_bio.c,v 1.14 1994/02/03 16:41:02 karels Exp $
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
 *	from: @(#)vfs_bio.c	7.40 (Berkeley) 5/8/91
 */

#include "param.h"
#include "systm.h"
#include "proc.h"
#include "buf.h"
#include "vnode.h"
#include "malloc.h"
#include "resourcevar.h"

#define	buf_setflags(vbuf, flags)	((vbuf)->b_flags |= (flags))
#define	buf_clrflags(vbuf, flags)	((vbuf)->b_flags &= ~(flags))
#define	buf_tstflags(vbuf, flags)	((vbuf)->b_flags & (flags))
#define	buf_isasync(vbuf)		(buf_tstflags(vbuf, B_ASYNC))
#define	buf_isfilled(vbuf)		(buf_tstflags(vbuf, B_DONE | B_DELWRI))
#define	buf_isread(vbuf)		(buf_tstflags(vbuf, B_READ))
#define	buf_iswrite(vbuf)		(buf_tstflags(vbuf, B_READ) == 0)
#define	buf_setread(vbuf)		(buf_setflags(vbuf, B_READ))

/*
 * Structures associated with buffer caching.
 */
union bufhash {
	union	bufhash *bh_head[2];
	struct	buf *bh_chain[2];
} *bufhashtbl, emptybufs;
#define	bh_forw	bh_chain[0]
#define	bh_back	bh_chain[1]

u_long	bufhash;			/* size of hash table - 1 */
long	bufcachemem;			/* memory occupied by cache buffers */
int	numbufcache;			/* number of buffer headers allocated */
struct	buf *bhhead, **bhtail;		/* LRU chain pointers */
int	bufneeded;			/* 1 => waiting for free buffer */

/*
 * Initialize buffer subsystem, allocating and initializing hash headers
 * based on amount of memory to be used for buffers.
 */
bufinit()
{
	register union bufhash *bhp;
	long bufhashsize;

	bhhead = 0;
	bhtail = &bhhead;
	bufhashsize = roundup(maxbufmem / (MAXBSIZE * 2) * (sizeof *bhp) / 2,
	    CLBYTES);
	bufhashtbl = (union bufhash *)malloc((u_long)bufhashsize,
	    M_BUFFER, M_WAITOK);
	for (bufhash = 1; bufhash <= bufhashsize / sizeof *bhp; bufhash <<= 1)
		/* void */;
	bufhash = (bufhash >> 1) - 1;
	for (bhp = &bufhashtbl[bufhash]; bhp >= bufhashtbl; bhp--) {
		bhp->bh_head[0] = bhp;
		bhp->bh_head[1] = bhp;
	}
	emptybufs.bh_head[0] = &emptybufs;
	emptybufs.bh_head[1] = &emptybufs;
}

/*
 * Get a new buffer of the specified size.
 * Allocate if not yet enough in existence, otherwise
 * take one from the front of the LRU list.
 */
struct buf *
getnewbuf(vbufsize)
	int vbufsize;
{
	register struct buf *vbuf;
	struct ucred *cred;
	int needfree, s;

	/*
	 * Allocate a new buffer.
	 */
	if (bufcachemem + vbufsize < maxbufmem) {
		numbufcache++;
		MALLOC(vbuf, struct buf *, sizeof *vbuf, M_BUFFER, M_WAITOK);
		bzero((char *)vbuf, sizeof *vbuf);
		vbuf->b_dev = NODEV;
		vbuf->b_rcred = NOCRED;
		vbuf->b_wcred = NOCRED;
		vbuf->b_flags = B_BUSY;
	} else {

		/*
		 * Free the cache slot at head of lru chain.
		 */
		needfree = vbufsize;
		for (;;) {
			s = splbio();
			if ((vbuf = bhhead) == NULL) {
				bufneeded = 1;
				sleep((caddr_t)&bufneeded, PRIBIO + 1);
				splx(s);
				continue;
			}
			bremfree(vbuf);

			/*
			 * Clean up any vestiges of the old buffer.
			 */
			if (buf_tstflags(vbuf, B_DELWRI)) {
				splx(s);
				buf_setflags(vbuf, B_BUSY | B_AGE);
				(void) bawrite(vbuf);
				continue;
			}
			/*
			 * Remove from old hash chain
			 */
			remque(vbuf);
			splx(s);
			if (vbuf->b_vp)
				brelvp(vbuf);
			if (vbuf->b_rcred != NOCRED) {
				cred = vbuf->b_rcred;
				vbuf->b_rcred = NOCRED;
				crfree(cred);
			}
			if (vbuf->b_wcred != NOCRED) {
				cred = vbuf->b_wcred;
				vbuf->b_wcred = NOCRED;
				crfree(cred);
			}

			/*
			 * We may need to expand this buffer, and reallocbuf
			 * may have expanded other buffers.  Also, some other
			 * code (e.g. the page daemon) might adjust maxbufmem
			 * downward.  Trim a buffer if we will be over our
			 * quota.  We may overrun for a short time, but this
			 * should make the cache gradually shrink to fit when
			 * there is a change in buffer size usage.
			 */
			needfree -= vbuf->b_bufsize;
			if (bufcachemem + needfree > maxbufmem) {
				bufcachemem -= vbuf->b_bufsize;
				FREE(vbuf->b_un.b_addr, M_BUFFER);
				FREE(vbuf, M_BUFFER);
				numbufcache--;
				continue;
			}
			break;
		}

		vbuf->b_dirtyoff = vbuf->b_dirtyend = 0;
	}
	vbuf->b_flags = B_BUSY;
	reallocbuf(vbuf, vbufsize, 0);
	return (vbuf);
}

int minbufsize = DEV_BSIZE;

/*
 * Change the buffer size of an existing buffer, 
 * copying the old data into the new buffer if requested to do so.
 */
reallocbuf(vbuf, vbufsize, copy)
	register struct buf *vbuf;
	int vbufsize, copy;
{
	int allocsize;
	caddr_t newaddr;

#ifdef DIAGNOSTIC
	if (vbufsize <= 0)
		panic("reallocbuf");
#endif
	if (vbufsize > CLBYTES / 2)
		allocsize = roundup(vbufsize, CLBYTES);
	else
		for (allocsize = minbufsize; allocsize < vbufsize;
		     allocsize <<= 1)
			/* void */;
	if (allocsize == vbuf->b_bufsize) {
		vbuf->b_bcount = vbufsize;
		return;
	}

	/*
	 * Although increasing the size of this buffer might push us
	 * over maxbufmem, we'll shrink later in getnewbuf rather than
	 * duplicating the logic for cleaning and freeing buffers.
	 */
	bufcachemem += allocsize - vbuf->b_bufsize;
	MALLOC(newaddr, caddr_t, allocsize, M_BUFFER, M_WAITOK);
	if (vbuf->b_bufsize > 0) {
		if (copy)
			bcopy(vbuf->b_un.b_addr, newaddr,
			    min(vbuf->b_bcount, vbufsize));
		FREE(vbuf->b_un.b_addr, M_BUFFER);
	}
	vbuf->b_un.b_addr = newaddr;
	vbuf->b_bufsize = allocsize;
	vbuf->b_bcount = vbufsize;
}

/*
 * Get a buffer identified with the vnode and logical block number,
 * creating one if none is found in the cache.
 */
struct buf *
getblk(vn, lbn, vbufsize)
	register struct vnode *vn;
	daddr_t lbn;
	int vbufsize;
{
	register struct buf *vbuf;
	register union bufhash *nhp;
	int hash, s;

#ifdef DIAGNOSTIC
	if (vbufsize <= 0 || vbufsize > MAXBSIZE)
		panic("getblk: bad size");
#endif
	hash = (((int)(vn)) >> 8) + (((int)(lbn))/(MAXBSIZE/DEV_BSIZE));
	nhp = &bufhashtbl[hash & bufhash];
	for (;;) {
		for (vbuf = nhp->bh_forw; vbuf != (struct buf *)nhp;
		    vbuf = vbuf->b_forw)
			if (vbuf->b_lblkno == lbn && vbuf->b_vp == vn &&
			    buf_tstflags(vbuf, B_INVAL) == 0)
				break;
		if (vbuf != (struct buf *)nhp) {
			/* found it in cache */
			s = splbio();
			if (buf_tstflags(vbuf, B_BUSY)) {
				buf_setflags(vbuf, B_WANTED);
				sleep((caddr_t)vbuf, PRIBIO + 1);
				splx(s);
				continue;
			}
			bremfree(vbuf);
			buf_setflags(vbuf, B_BUSY | B_CACHE);
			/*
			 * If this block was a delayed write that has now been
			 * written out, B_AGE will be set, and that would cause
			 * the buffer to be placed at the head of the LRU
			 * rather than the tail when we free it.  This can
			 * cause us to repeatedly write cylinder group blocks
			 * while writing a file.  Clear B_AGE, indicating that
			 * the block has been used since the write-back.
			 */
			buf_clrflags(vbuf, B_AGE);
			splx(s);
#ifdef DIAGNOSTIC
			if (vbuf->b_bcount != vbufsize)
				panic("getblk: stray size");
#endif
		} else {
			/* not in cache; create new */
			vbuf = getnewbuf(vbufsize);
			insque(vbuf, nhp);
			bgetvp(vn, vbuf);
			vbuf->b_blkno = lbn;
			vbuf->b_lblkno = lbn;
			vbuf->b_resid = 0;
			vbuf->b_error = 0;
		}
		return (vbuf);
	}
}

/*
 * Return a buffer of the specified size
 * not associated with a specific logical block.
 */
struct buf *
geteblk(vbufsize)
	int vbufsize;
{
	register struct buf *vbuf;

	vbuf = getnewbuf(vbufsize);
	insque(vbuf, &emptybufs);
	buf_setflags(vbuf, B_INVAL);
	vbuf->b_blkno = 0;
	vbuf->b_lblkno = 0;
	vbuf->b_resid = 0;
	vbuf->b_error = 0;
	return (vbuf);
}

/*
 * Find or create a vbuf for the specified vnode and logical block number,
 * and fill the contents if it is not resident.
 */
bread(vn, lbn, vbufsize, cred, retvbufp)
	struct vnode *vn;
	daddr_t lbn;
	int vbufsize;
	struct ucred *cred;
	struct buf **retvbufp;
{
	register struct buf *vbuf;
	struct proc *p = curproc;	/* XXX */

	*retvbufp = vbuf = getblk(vn, lbn, vbufsize);
	if (buf_isfilled(vbuf))
		return (0);
	buf_setread(vbuf);
	if (vbuf->b_rcred == NOCRED && cred != NOCRED) {
		crhold(cred);
		vbuf->b_rcred = cred;
	}
	VOP_STRATEGY(vbuf);
	p->p_stats->p_ru.ru_inblock++;	
	return (biowait(vbuf));
}

/*
 * Find or create a vbuf for the specified vnode and logical block number,
 * and fill the contents if it is not resident.
 * Also prepare for an expected read on the (presumably next) logical block.
 */
breada(vn, lbn, vbufsize, nextlbn, nextvbufsize, cred, retvbufp)
	struct vnode *vn;
	daddr_t lbn; int vbufsize;
	daddr_t nextlbn; int nextvbufsize;
	struct ucred *cred;
	struct buf **retvbufp;
{
	register struct buf *vbuf, *nextvbuf;
	struct proc *p = curproc;	/* XXX */

	/*
	 * Get a cache entry for lbn; if not resident, start a read.
	 */
	*retvbufp = vbuf = getblk(vn, lbn, vbufsize);
	if (buf_isfilled(vbuf) == 0) {
		buf_setread(vbuf);
		if (vbuf->b_rcred == NOCRED && cred != NOCRED) {
			crhold(cred);
			vbuf->b_rcred = cred;
		}
		VOP_STRATEGY(vbuf);
		p->p_stats->p_ru.ru_inblock++;
	}
	/*
	 * Get a cache entry for nextlbn; if not resident, start a read.
	 */
	nextvbuf = getblk(vn, nextlbn, nextvbufsize);
	if (buf_isfilled(nextvbuf))
		brelse(nextvbuf);
	else {
		buf_setflags(nextvbuf, B_ASYNC | B_READ);
		if (nextvbuf->b_rcred == NOCRED && cred != NOCRED) {
			crhold(cred);
			nextvbuf->b_rcred = cred;
		}
		VOP_STRATEGY(nextvbuf);
		p->p_stats->p_ru.ru_inblock++;
	}
	/*
	 * Return the desired block, waiting for I/O to
	 * complete if necessary.
	 */
	return (biowait(vbuf));
}

/*
 * Free buffer, writing contents back; do not wait for completion.
 * Buffer is freed when I/O completes.
 */
bawrite(vbuf)
	register struct buf *vbuf;
{

	buf_setflags(vbuf, B_ASYNC);
	(void) bwrite(vbuf);
}

/*
 * Free buffer, writing contents back, waiting for completion,
 * and then unlocking and freeing.
 */
bwrite(vbuf)
	register struct buf *vbuf;
{
	register int oldstate;
	int s, error;
	struct proc *p = curproc;	/* XXX */

	oldstate = vbuf->b_flags;
	buf_clrflags(vbuf, B_DELWRI | B_DONE | B_ERROR | B_READ);
	s = splbio();
	vbuf->b_vp->v_numoutput++;
	splx(s);
	VOP_STRATEGY(vbuf);
	/*
	 * If there is an error and B_ASYNC is set,
	 * the vnode may be dissociated from the buffer.
	 */
	if (oldstate & B_DELWRI) {
		s = splbio();
		if (vbuf->b_vp)
			reassignbuf(vbuf, vbuf->b_vp);
		splx(s);
	} else if (p)
		p->p_stats->p_ru.ru_oublock++;
	if (oldstate & B_ASYNC)
		return (0);
	error = biowait(vbuf);
	brelse(vbuf);
	return (error);
}

/*
 * Free a buffer that has been modified, marking it so that the contents
 * will be written eventually.  Used when additional modifications
 * are expected.
 */
bdwrite(vbuf)
	register struct buf *vbuf;
{
	struct proc *p = curproc;	/* XXX */

	if (buf_tstflags(vbuf, B_DELWRI) == 0) {
		buf_setflags(vbuf, B_DELWRI);
		reassignbuf(vbuf, vbuf->b_vp);
		if (p)
			p->p_stats->p_ru.ru_oublock++;
	}
	/*
	 * If this is a tape drive, the write must be now.
	 */
	if (VOP_IOCTL(vbuf->b_vp, 0, (caddr_t)B_TAPE, 0, NOCRED, p) == 0) {
		bawrite(vbuf);
	} else {
		buf_setflags(vbuf, B_DONE);
		brelse(vbuf);
	}
}

/*
 * Free vbuf.
 *
 * N.B. - Buffers marked B_AGE should be be given less than the full
 * LRU timeouts implemented by this code.  For now, we put buffers
 * marked B_DELWRI | B_AGE at the front of the queue, as they have
 * already gotten to the end of the LRU.  Other B_AGE buffers should
 * be held for a bit longer.
 */
brelse(vbuf)
	register struct buf *vbuf;
{
	struct ucred *cred;
	int wanted = 0;
	int s;

	/*
	 * For active buffers, retry the I/O, otherwise mark invalid
	 */
	if (buf_tstflags(vbuf, B_ERROR)) {
		if (buf_tstflags(vbuf, B_LOCKED))
			buf_clrflags(vbuf, B_ERROR);
		else
			buf_setflags(vbuf, B_INVAL);
	}
	/*
	 * Free the resources of invalid or incorrectly-read buffers,
	 */
	if (buf_tstflags(vbuf, B_ERROR | B_INVAL)) {
		if (vbuf->b_vp)
			brelvp(vbuf);
		if (vbuf->b_rcred != NOCRED) {
			cred = vbuf->b_rcred;
			vbuf->b_rcred = NOCRED;
			crfree(cred);
		}
		if (vbuf->b_wcred != NOCRED) {
			cred = vbuf->b_wcred;
			vbuf->b_wcred = NOCRED;
			crfree(cred);
		}
		buf_clrflags(vbuf, B_DELWRI);
	}
	s = splbio();
	if (bhhead == NULL) {
		/*
		 * Start LRU list.
		 */
		vbuf->b_next = NULL;
		vbuf->b_prev = &bhhead;
		bhtail = &vbuf->b_next;
		bhhead = vbuf;
	} else if (buf_tstflags(vbuf, B_ERROR | B_INVAL) ||
	    ((vbuf->b_flags & (B_DELWRI | B_AGE)) == (B_DELWRI | B_AGE))) {
		/*
		 * Place on front of LRU list
		 */
		vbuf->b_next = bhhead;
		vbuf->b_prev = &bhhead;
		bhhead->b_prev = &vbuf->b_next;
		bhhead = vbuf;
	} else {
		/*
		 * Place at end of LRU list
		 */
		vbuf->b_next = NULL;
		vbuf->b_prev = bhtail;
		*bhtail = vbuf;
		bhtail = &vbuf->b_next;
	}
	wanted = vbuf->b_flags & B_WANTED;
	buf_clrflags(vbuf, B_ASYNC | B_BUSY | B_WANTED );
	splx(s);

	if (wanted)
		wakeup((caddr_t)vbuf);
	if (bufneeded) {
		bufneeded = 0;
		wakeup((caddr_t)&bufneeded);
	}
}

/*
 * Await the completion of the I/O operation pending on vbuf,
 * then return any error that occurred.
 */
biowait(vbuf)
	register struct buf *vbuf;
{
	int s;

	s = splbio();
	while (buf_tstflags(vbuf, B_DONE) == 0)
		sleep((caddr_t)vbuf, PRIBIO);
	splx(s);
	if (buf_tstflags(vbuf, B_ERROR) == 0)
		return (0);
	return (vbuf->b_error ? vbuf->b_error : EIO);
}

/*
 * Callback from device drivers to indicate completion of an operation.
 */
biodone(vbuf)
	register struct buf *vbuf;
{

#ifdef DIAGNOSTIC
	if (buf_tstflags(vbuf, B_DONE))
		panic("biodone: dup");
#endif
	buf_setflags(vbuf, B_DONE);
	if (buf_iswrite(vbuf))
		vwakeup(vbuf);
	if (buf_tstflags(vbuf, B_CALL)) {
		buf_clrflags(vbuf, B_CALL);
		(*vbuf->b_iodone)(vbuf);
		return;
	}
	/*
	 * Asynchronous: release buffer
	 * Synchronous: wakeup those awaiting I/O completion
	 */
	if (buf_isasync(vbuf))
		brelse(vbuf);
	else {
		buf_clrflags(vbuf, B_WANTED);
		wakeup((caddr_t)vbuf);
	}
}
