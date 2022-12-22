/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: buf.h,v 1.7 1994/02/04 15:35:14 karels Exp $
 */

/*
 * Copyright (c) 1982, 1986, 1989 Regents of the University of California.
 * All rights reserved.
 *
 * (c) UNIX System Laboratories, Inc.  All or some portions of this file
 * are derived from material licensed to the University of California by
 * American Telephone and Telegraph Co. or UNIX System Laboratories, Inc.
 * and are reproduced herein with the permission of UNIX System Laboratories,
 * Inc.
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
 *	from @(#)buf.h	7.11 (Berkeley) 5/9/90
 */

/*
 * The header for buffers in the buffer pool and otherwise used
 * to describe a block i/o request is given here.
 *
 * Each buffer in the pool is usually doubly linked into 2 lists:
 * hashed into a chain by <vp,lblkno> so it can be located in the cache,
 * and (when not busy) on the LRU queue. This list is circular and
 * doubly linked for easy removal.
 *
 * When not on the LRU queue, a buffer is ``checked out'' to a device
 * driver that may use the LRU list pointers to keep track of it in its
 * I/O active queue.
 */

struct buf {
	struct	buf *b_forw, *b_back;	/* hash chain (MUST BE FIRST) */
	struct	buf *b_next, **b_prev;	/* position on LRU list if not BUSY */
	struct	buf *b_blockf, **b_blockb;/* associated vnode */
#define	b_actf	b_next			/* alternate names for driver queue */
#define	av_forw	b_next			/*    head - isn't history wonderful */
	struct	buf *b_actl;		/* driver queue tail */
	long	b_flags;		/* too much goes here to describe */
	long	b_bcount;		/* transfer count */
	long	b_bufsize;		/* size of allocated buffer */
#define	b_active b_bcount		/* driver queue head: drive active */
	short	b_error;		/* returned after I/O */
	dev_t	b_dev;			/* major+minor device name */
	union {
	    caddr_t b_addr;		/* low order core address */
	    int	*b_words;		/* words for clearing */
	    struct fs *b_fs;		/* superblocks */
	    struct csum *b_cs;		/* superblock summary information */
	    struct cg *b_cg;		/* cylinder group block */
	    struct dinode *b_dino;	/* ilist */
	    daddr_t *b_daddr;		/* indirect block */
	} b_un;
	daddr_t	b_lblkno;		/* logical block number */
	daddr_t	b_blkno;		/* block # on device */
	long	b_resid;		/* words not transferred after error */
#define	b_errcnt b_resid		/* while i/o in progress: # retries */
	struct  proc *b_proc;		/* proc doing physical or swap I/O */
	int	(*b_iodone)();		/* function called by iodone */
	struct	vnode *b_vp;		/* vnode for dev */
	int	b_pfcent;		/* center page when swapping cluster */
	struct	ucred *b_rcred;		/* ref to read credentials */
	struct	ucred *b_wcred;		/* ref to write credendtials */
	int	b_dirtyoff;		/* offset in buffer of dirty region */
	int	b_dirtyend;		/* offset of end of dirty region */
	caddr_t	b_saveaddr;		/* original b_addr for PHYSIO */
};

#ifdef	KERNEL
extern	int maxbufmem;		/* max buffer cache memory */
struct	buf *bhhead;		/* LRU queue head */
struct	buf **bhtail;		/* LRU queue tail */
struct	buf *swbuf;		/* swap I/O headers */
int	nswbuf;
struct	buf bswlist;		/* head of free swap header list */
struct	buf *bclnlist;		/* head of cleaned page list */

struct	buf *getblk();
struct	buf *geteblk();
struct	buf *getnewbuf();

unsigned minphys();
#endif

/*
 * These flags are kept in b_flags.
 */
#define	B_WRITE		0x000000	/* non-read pseudo-flag */
#define	B_READ		0x000001	/* read when I/O occurs */
#define	B_DONE		0x000002	/* transaction finished */
#define	B_ERROR		0x000004	/* transaction aborted */
#define	B_BUSY		0x000008	/* not on av_forw/back list */
#define	B_PHYS		0x000010	/* physical IO */
#define	B_FORMAT	0x000020	/* format operation */
#define	B_WANTED	0x000040	/* issue wakeup when BUSY goes off */
#define	B_AGE		0x000080	/* delayed write for correct aging */
#define	B_ASYNC		0x000100	/* don't wait for I/O completion */
#define	B_DELWRI	0x000200	/* write at exit of avail list */
#define	B_TAPE		0x000400	/* this is a magtape (no bdwrite) */
#define	B_UAREA		0x000800	/* add u-area to a swap operation */
#define	B_PAGET		0x001000	/* page in/out of page table space */
#define	B_DIRTY		0x002000	/* dirty page to be pushed out async */
#define	B_PGIN		0x004000	/* pagein op, so swap() can count it */
#define	B_CACHE		0x008000	/* did bread find us in the cache ? */
#define	B_INVAL		0x010000	/* does not contain valid info  */
#define	B_LOCKED	0x020000	/* locked in core (not reusable) */
#define	B_SPARE_1	0x040000	/* spare */
#define	B_BAD		0x100000	/* bad block revectoring in progress */
#define	B_CALL		0x200000	/* call b_iodone from iodone */
#define	B_RAW		0x400000	/* set by physio for raw transfers */
#define	B_SPARE_2	0x800000	/* spare */

/*
 * Zero out a buffer's data portion.
 */
#define	clrbuf(bp) { \
	blkclr((bp)->b_un.b_addr, (unsigned)(bp)->b_bcount); \
	(bp)->b_resid = 0; \
}

/*
 * Remove a buffer from the free list.
 * Must be called at splbio().
 */
#define bremfree(bp) \
	*(bp)->b_prev = (bp)->b_next; \
	if ((bp)->b_next) \
		(bp)->b_next->b_prev = (bp)->b_prev; \
	else \
		bhtail = (bp)->b_prev

/*
 * Allocbuf() is the copying form of reallocbuf().
 */
#define	allocbuf(buf, size)	reallocbuf((buf), (size), 1);

#define B_CLRBUF	0x1	/* request allocated buffer be cleared */
#define B_SYNC		0x2	/* do all allocations synchronously */
