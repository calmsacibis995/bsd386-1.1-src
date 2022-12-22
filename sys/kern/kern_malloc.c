/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: kern_malloc.c,v 1.13 1993/11/10 21:34:52 karels Exp $
 */

/*
 * Copyright (c) 1987, 1991 The Regents of the University of California.
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
 *	@(#)kern_malloc.c	7.25 (Berkeley) 5/8/91
 */

#include "param.h"
#include "proc.h"
#include "map.h"
#include "kernel.h"
#include "malloc.h"
#include "syslog.h"
#include "vm/vm.h"
#include "vm/vm_kern.h"

#ifdef DEBUG
static void check_addr();
#endif

struct kmembuckets bucket[MINBUCKET + 16];
struct kmemstats kmemstats[M_LAST];
struct kmemusage *kmemusage;
char *kmembase, *kmemlimit;
char *memname[] = INITKMEMNAMES;

#define	GC_NOFORCE	0	/* compact free listonly if excessive */
#define	GC_FORCE	1	/* squeeze every last page out */

static int needgc;		/* some bucket should be compacted */
static int gcfree;		/* pages to target before garbage compaction */

/*
 * Check whether we want to try garbage compaction on a bucket:
 * if the amount over the high-water mark contains at least gcfree pages
 * of memory, attempt coelescing completely free pages.
 */
#define	WANTGC(kbp)	((long) ((kbp)->kb_totalfree - (kbp)->kb_highwat) > \
				gcfree * (kbp)->kb_elmpercl)

/*
 * Allocate a block of memory.
 * If caller is willing to wait, we may block if we are over the limit
 * for that type; we may also block when allocating memory (or first
 * using it) if there is insufficient free memory.  We never return
 * failure if the caller is willing to wait, but we may die trying.
 */
void *
malloc(size, type, waitflag)
	unsigned long size;
	int type, waitflag;
{
	register struct kmembuckets *kbp;
	register struct kmemusage *kup;
	long indx, npg, alloc, allocsize;
	int s;
	caddr_t va, cp, savedlist;
	register struct kmemstats *ksp = &kmemstats[type];

	if (((unsigned long)type) > M_LAST)
		panic("malloc - bogus type");

	indx = BUCKETINDX(size);
	kbp = &bucket[indx];
again:
	s = splimp();
	while (ksp->ks_memuse >= ksp->ks_limit) {
		if (waitflag == M_NOWAIT) {
			splx(s);
			return ((void *) NULL);
		}
		if (ksp->ks_limblocks < 65535)
			ksp->ks_limblocks++;
		tsleep((caddr_t)ksp, PSWP+2, memname[type], 0);
	}
	if (kbp->kb_next == NULL) {
		/*
		 * If we noticed during a previous free that at least one
		 * bucket had too many free chunks and was worth compacting,
		 * we do that during the next allocation that is willing
		 * to block and needs to allocate memory.
		 */
		if (needgc && waitflag == M_WAITOK) {
			splx(s);
			(void) malloc_gc(GC_NOFORCE);
			goto again;
		}
		if (size > MAXALLOCSAVE)
			allocsize = roundup(size, CLBYTES);
		else
			allocsize = 1 << indx;
		npg = clrnd(btoc(allocsize));
		va = (caddr_t) kmem_malloc(kmem_map, (vm_size_t)ctob(npg),
		    waitflag);
		if (va == NULL) {
			splx(s);
			if (waitflag == M_WAITOK) {
				log(LOG_ERR,
				    "malloc: kmem_map is full, attempting compaction\n");
				if (malloc_gc(GC_FORCE))
					goto again;
				panic("malloc: kmem_map too small");
			}
			return ((void *) NULL);
		}
		kbp->kb_total += kbp->kb_elmpercl;
		kup = btokup(va);
		kup->ku_indx = indx;
		if (allocsize > MAXALLOCSAVE) {
			if (npg > 65535)
				panic("malloc: allocation too large");
			kup->ku_pagecnt = npg;
			ksp->ks_memuse += allocsize;
			goto out;
		}
		kup->ku_freecnt = kbp->kb_elmpercl;
		kbp->kb_totalfree += kbp->kb_elmpercl;
		/*
		 * Just in case we blocked while allocating memory,
		 * and someone else also allocated memory for this
		 * bucket, don't assume the list is still empty.
		 */
		savedlist = kbp->kb_next;
		kbp->kb_next = va + (npg * NBPG) - allocsize;
		for (cp = kbp->kb_next; cp > va; cp -= allocsize)
			*(caddr_t *)cp = cp - allocsize;
		*(caddr_t *)cp = savedlist;
	}
	va = kbp->kb_next;
#ifdef DEBUG
	check_addr(va, btokup(va)->ku_indx, 0);
#endif
	kbp->kb_next = *(caddr_t *)va;
#ifdef DEBUG
	if (kbp->kb_next) {
		struct kmemusage *nkup = btokup(kbp->kb_next);
		if (nkup->ku_indx != indx)
			panic("malloc: next wrong bucket");
		check_addr(kbp->kb_next, nkup->ku_indx, 0);
	}
#endif
	kup = btokup(va);
#ifdef DIAGNOSTIC
	if (kup->ku_indx != indx)
		panic("malloc: wrong bucket");
	if (kup->ku_freecnt == 0)
		panic("malloc: lost data");
#endif
	kup->ku_freecnt--;
	kbp->kb_totalfree--;
	ksp->ks_memuse += 1 << indx;
out:
	kbp->kb_calls++;
	ksp->ks_inuse++;
	ksp->ks_calls++;
	if (ksp->ks_memuse > ksp->ks_maxused)
		ksp->ks_maxused = ksp->ks_memuse;
	splx(s);
	return ((void *) va);
}

#if 0 && defined(DEBUG) && defined(i386)
caddr_t	lastfreepc;
#endif

/*
 * Free a block of memory allocated by malloc.
 */
void
free(addr, type)
	void *addr;
	int type;
{
	register struct kmembuckets *kbp;
	register struct kmemusage *kup;
	long size;
	int s;
	register struct kmemstats *ksp = &kmemstats[type];

	kup = btokup(addr);
	size = 1 << kup->ku_indx;
#ifdef DEBUG
	check_addr(addr, kup->ku_indx, type);
#if 0 && defined(i386)
	lastfreepc = *(caddr_t *)((caddr_t) &addr - sizeof (caddr_t));
#endif
#endif
	kbp = &bucket[kup->ku_indx];
	s = splimp();
	/*
	 * If freeing a size that is at least pagesize,
	 * we can free the actual memory without coalescing
	 * free sections; no other parts of the pages are on
	 * the free list.  If we're over the highwater mark,
	 * free the actual memory rather than putting it
	 * on the free list.  Note that buckets over MAXALLOCSAVE
	 * have highwater marks set to zero.
	 */
	if (size >= CLBYTES && kbp->kb_totalfree >= kbp->kb_highwat) {
		if (size > MAXALLOCSAVE)
			size = ctob(kup->ku_pagecnt);
		kmem_free(kmem_map, (vm_offset_t)addr, size);
		kup->ku_indx = 0;
		kup->ku_pagecnt = 0;
		kbp->kb_total--;
		kbp->kb_couldfree++;
	} else {
		kup->ku_freecnt++;
		if (kup->ku_freecnt >= kbp->kb_elmpercl) {
#ifdef DIAGNOSTIC
			if (kup->ku_freecnt > kbp->kb_elmpercl)
				panic("free: multiple frees");
#endif
			if (kbp->kb_totalfree > kbp->kb_highwat) {
				kbp->kb_couldfree++;
				/*
				 * If this bucket has enough free chunks that it
				 * is worth doing compaction, make a note of it.
				 * We don't do it now, as we might be called
				 * from interrupt level; we prefer to wait
				 * until a more convenient time.
				 */
				if (WANTGC(kbp))
					needgc = 1;
			}
		}
		kbp->kb_totalfree++;
#if 0
		if (size < NBPG) {
			/*
			 * Clean out the freed memory (foil readers) and
			 * record the pc of our caller (trace writers).
			 * Assumes that MINALLOCSIZE >= 8.
			 */
			bzero(addr, size);
		}
#ifdef i386
		*((caddr_t *)((caddr_t) addr + size) - 1) = lastfreepc;
#endif
#endif
		*(caddr_t *)addr = kbp->kb_next;
		kbp->kb_next = addr;
	}
	ksp->ks_memuse -= size;
	if (ksp->ks_memuse + size >= ksp->ks_limit &&
	    ksp->ks_memuse < ksp->ks_limit)
		wakeup((caddr_t)ksp);
	ksp->ks_inuse--;
	splx(s);
}

#ifdef DEBUG

static long addrmask[] = { 0x00000000,
	0x00000001, 0x00000003, 0x00000007, 0x0000000f,
	0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,
	0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff,
	0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
};

static void
check_addr(va, index, type)
	caddr_t va;
	int index;
	int type;
{
	long size;
	u_long alloc;

	if ((unsigned) index < MINBUCKET || (unsigned) index > MINBUCKET + 15) {
		printf("check_addr: out of range index 0x%x\n", index);
		panic("check_addr: index");
	}

	if ((char *) va < kmembase || (char *) va >= kmemlimit) {
		printf("check_addr: out of range addr 0x%x\n", va);
		panic("check_addr: bounds");
	}

	size = 1 << index;
	if (size > NBPG * CLSIZE)
		alloc = addrmask[BUCKETINDX(NBPG * CLSIZE)];
	else
		alloc = addrmask[index];
	if (((u_long)va & alloc) != 0) {
		printf("check_addr: unaligned addr 0x%x, size %d, type %d\n",
			va, size, type);
		panic("check_addr: unaligned addr");
	}
}
#endif /* DEBUG */

extern int kmemsize;
int bufmap;		/* extra kmem space for buffers, set elsewhere */

/*
 * Initialize the kernel memory allocator
 */
kmeminit()
{
	register struct kmembuckets *kbp;
	register long indx;
	extern int maxbufmem;		/* max memory for buffers */
	int size;

#if	((MAXALLOCSAVE & (MAXALLOCSAVE - 1)) != 0)
		ERROR!_kmeminit:_MAXALLOCSAVE_not_power_of_2
#endif
#if	(MAXALLOCSAVE > MINALLOCSIZE * 32768)
		ERROR!_kmeminit:_MAXALLOCSAVE_too_big
#endif
#if	(MAXALLOCSAVE < CLBYTES)
		ERROR!_kmeminit:_MAXALLOCSAVE_too_small
#endif

	/*
	 * The kmemsize limit is initialized externally, and may
	 * be adjusted by the machine-dependent layer at boot time.
	 */
	kmemusage = (struct kmemusage *) kmem_alloc(kernel_map,
		(vm_size_t)(kmemsize / CLBYTES * sizeof(struct kmemusage)));
	kmem_map = kmem_suballoc(kernel_map, (vm_offset_t)&kmembase,
		(vm_offset_t)&kmemlimit, (vm_size_t)kmemsize, FALSE);
	kbp = bucket;
	for (indx = 0; indx < MINBUCKET + 16; indx++, kbp++) {
		size = 1 << indx;
		if (size >= CLBYTES)
			kbp->kb_elmpercl = 1;
		else
			kbp->kb_elmpercl = CLBYTES / size;
		if (size <= MAXALLOCSAVE)
			kbp->kb_highwat = 5 * kbp->kb_elmpercl;
		else
			kbp->kb_highwat = 0;
	}
	for (indx = 0; indx < M_LAST; indx++)
		kmemstats[indx].ks_limit = (kmemsize - bufmap) * 6 / 10;
	kmemstats[M_BUFFER].ks_limit += maxbufmem;
	gcfree = kmemsize / (CLBYTES * 20);
}

/*
 * Attempt to coelesce free memory in a bucket (< CLBTES) with has an
 * excessivly long free list.  This is done by removing the chunks on pages
 * that are now completely free, and freeing the pages once that is done.
 * We do most of the work without blocking interrupts, unlinking much
 * of the free list from the bucket so we can sort it undisturbed.
 */
static int
bucket_gc(kbp)
	struct kmembuckets *kbp;
{
	struct kmemusage *kup;
	caddr_t next, head, head2, *prev, *prev2;
	int i, s, freed;

	/*
	 * Remove the portion of the free list after
	 * the first kb_highwat / 4 chunks for cleaning.
	 */
	s = splimp();
	next = kbp->kb_next;
	for (i = kbp->kb_highwat / 4; --i > 0; ) {
		next = *(caddr_t *) next;
#ifdef DEBUG
		if (next == NULL)
			panic("malloc gc next");
#endif
	}
	head = *(caddr_t *) next;
	*(caddr_t *) next = NULL;
	splx(s);

	/*
	 * "head" now points to the list we have removed from the bucket
	 * for cleaning.  For each chunk on this list, increment
	 * the count of freeable chunks for each page.  Note that
	 * we assume that the counts are zero on entry; we zero them on exit.
	 */
	for (next = head; next; next = *(caddr_t *) next)
		btokup(next)->ku_gccnt++;

	/*
	 * Now walk the list to be cleaned again.
	 * This time, if a chunk is on a page that is completely free,
	 * simply drop it from the list; we will then free the pages.
	 * While we're checking, move chunks that are on pages that
	 * are over half free onto a separate list headed by "head2",
	 * which we will place after the other chunks on the free list.
	 * Relink the other chunks onto the first list.  While we go
	 * through the original list, the forward pointers of the last
	 * elements on the two lists point to random places; we terminate
	 * the lists when we're finished.
	 */
	next = head;
	prev = &head;
	head2 = NULL;
	prev2 = &head2;
	for (; next; next = *(caddr_t *) next) {
		if (btokup(next)->ku_gccnt == kbp->kb_elmpercl)
			*prev = *(caddr_t *) next;
		else if (btokup(next)->ku_gccnt > kbp->kb_elmpercl / 2) {
			*prev2 = next;
			prev2 = (caddr_t *) next;
		} else {
			*prev = next;
			prev = (caddr_t *) next;
		}
	}
	/*
	 * Place second list after the first list, terminating the first
	 * list.  Then, while blocking allocations, prepend the cleaned list
	 * to whatever is now in the bucket.  This way, we get to look at
	 * those next time.
	 */
	*prev = head2;
	if (head2 == NULL)
		prev2 = prev;
	s = splimp();
	*prev2 = kbp->kb_next;
	kbp->kb_next = head;
	splx(s);

	freed = 0;
	kup = kmemusage;
	for (i = 0; i < kmemsize / CLBYTES; i++, kup++) {
		if (kup->ku_gccnt == kbp->kb_elmpercl) {
			kmem_free(kmem_map, kmembase + CLBYTES * i, CLBYTES);
			kbp->kb_total -= kbp->kb_elmpercl;
			kbp->kb_totalfree -= kbp->kb_elmpercl;
			freed++;
		}
		kup->ku_gccnt = 0;
	}
	return (freed);
}

/*
 * Initiate garbage collection/compaction of the malloc arena.
 * Scan the buckets up to size CLBYTES for buckets that are
 * worth coalescing and freeing the excess memory.  Buckets
 * at or above CLBYTES can free memory in free().  As this will take
 * a while, we do this only in the top half of the kernel, calling
 * it from malloc() when wait is allowed.  As the kernel is not
 * preemptible here and we don't sleep, malloc_gc should never
 * be re-entered.
 */
malloc_gc(force)
	int force;
{
	struct kmembuckets *kbp;
	long indx;
	int freed;
#ifdef DEBUG
	static int doing_gc;

	if (doing_gc)
		panic("malloc_gc reentered");
	doing_gc = 1;
#endif

	needgc = 0;
	freed = 0;
	kbp = &bucket[MINBUCKET];
	for (indx = MINBUCKET; indx < BUCKETINDX(CLBYTES); indx++, kbp++)
		if (WANTGC(kbp) ||
		    (force && ((long) (kbp->kb_totalfree - kbp->kb_highwat) >
		    kbp->kb_elmpercl)))
		    	freed += bucket_gc(kbp);

#ifdef DEBUG
	doing_gc = 0;
/* log(LOG_INFO, "malloc_gc freed %d\n", freed); */
#endif
	return (freed);
}
