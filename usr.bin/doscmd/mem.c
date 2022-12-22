/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
 
/*	Krystal $Id: mem.c,v 1.2 1994/01/14 23:36:23 polk Exp $ */

#include <stdio.h>
#include "doscmd.h"

#define	Mark(x)	(*(char *) (x))
#define Owner(x) (*(u_short *) ((char *)(x)+1))
#define Size(x) (*(u_short *) ((char *)(x)+3))
#define Next(x) ((char *)(x) + (Size(x)+1)*16)

static char *next_p = (char *)0;
static char *end_p  = (char *)0xB0000L;

char *core_alloc(int *size)
{
	char *ret;
	if (*size) {
		if (*size & 0xfff) {
			*size = (*size & ~0xfff) + 0x1000;
		}
	} else {
		*size = end_p - next_p;
	}

	if (next_p + *size > end_p) {
		return NULL;
	}

	ret = next_p;
	next_p += *size;
	return ret;
}

void
set_mcb(char *mp, int owner, int size)
{
	Mark(mp) = 'M';
	Owner(mp) = owner;
	Size(mp) = size;
}

void
set_lastmcb(char *mp)
{
	Mark(mp) = 'Z';
	Owner(mp) = 0xffff;
	Size(mp) = 0;
}

void
mem_free_owner(int owner)
{
    char	*mp;

    debug(D_MEMORY, "    : freeow(%04x)\n", owner);
    for(mp = dosmem; Mark(mp) == 'M'; mp = Next(mp)) {
	if (Owner(mp) == owner)
	    mem_adjust(((int)mp + 16) >> 4, 0, NULL);
    }
}

void
mem_print()
{
    char *mp;

    for(mp = dosmem; Mark(mp) == 'M'; mp = Next(mp)) {
	debug(D_MEMORY, "%04x: %04x %05x\n", (int)mp/16+1, Owner(mp), Size(mp));
    }
}

void	
mem_change_owner(int addr, int owner)
{
	char	*mp;

    	debug(D_MEMORY, "%04x: owner (%04x)\n", addr, owner);
	addr *= 16;
	for(mp = dosmem; Mark(mp) == 'M'; mp = Next(mp))
	    if ((int)(mp + 16) == addr)
		break;

	if (Mark(mp) != 'M') {
	    debug(D_ALWAYS, "%04x: illegal block in change owner\n", addr);
	}

	Owner(mp) = owner;
}

void
mem_init (void)
{
	int	curaddr, offset, base, avail_memory;

	base = 0x600;
	core_alloc(&base);

	avail_memory = MAX_AVAIL_SEG * 16 - base;
	dosmem = core_alloc(&avail_memory);

	if (!dosmem || dosmem != (char *)base)
		fatal("internal memory error\n");

	dosmem_size = avail_memory / 16;

	debug(D_MEMORY, "dosmem = 0x%x base = 0x%x avail = 0x%x (%dK)\n",
			dosmem, base, dosmem_size, avail_memory / 1024);

	set_mcb(dosmem, 0, dosmem_size - 2);
	set_lastmcb(Next(dosmem));
}

int
mem_alloc (int size, int owner, int *biggestp)
{
	char *mp;
	int biggest;
	int rest;

	biggest = 0;
	for (mp = dosmem; Mark(mp) == 'M'; mp = Next(mp)) {
    	    	char *nmp;

		if (Owner(mp))
			continue;

    	    	nmp = Next(mp);
	    	while (Mark(nmp) == 'M' && Owner(nmp) == 0) {
			Size(mp) += Size(nmp) + 1;
			nmp = Next(mp);
    	    	}

		if (Size(mp) >= size) {
			Owner(mp) = owner;
			if (Size(mp) != size) {
				rest = Size(mp) - size;
				Size(mp) = size;
				set_mcb(Next(mp), 0, rest - 1);
			}
			if (biggestp)
				*biggestp = size;
			debug(D_MEMORY, "%04x: alloc (%05x, owner %04x)\n",
					(int)mp/16 + 1, size, owner);
			return (int)mp/16 + 1;
		}

		if (Size(mp) > biggest)
			biggest = Size(mp);
	}

	if (biggestp)
		*biggestp = biggest;

	debug(D_MEMORY, "%04x: alloc (%05x, owner %04x) failed\n",
			0, size, owner);
	return 0;
}

int
mem_adjust (int addr, int size, int *availp)
{
	char *mp, *mp1;
	int delta, nxtsiz, rest;

	*availp = size;

	debug(D_MEMORY, "%04x: adjust(%05x)\n", addr, size);

	addr *= 16;
	for (mp = dosmem; Mark(mp) == 'M'; mp = Next(mp))
		if ((int)(mp + 16) == addr)
			break;

	if (Mark(mp) != 'M') {
		debug(D_MEMORY, "%04x: illegal block\n", addr);
		return -2;
	}

	if (Owner(mp) == 0) {
		debug(D_MEMORY, "%04x: unallocated block\n", addr);
		return -2;
	}

	if (size == Size(mp))
		return 0;

	mp1 = Next(mp);
	while (Mark(mp1) == 'M' && Owner(mp1) == 0) {
		Size(mp) += Size(mp1) + 1;
		mp1 = Next(mp);
	}

	if (size > Size(mp)) {
		if (availp)
			*availp = Size(mp);
		return -1;
    	}

	rest = Size(mp) - size;
	Size(mp) = size;
	mp1 = Next(mp);
	set_mcb(mp1, 0, rest - 1);
	return 0;
}

		

#ifdef	MEM_TEST
mem_check ()
{
	struct mem_block *mp;
	for (mp = mem_blocks.next; mp != &mem_blocks; mp = mp->next) {
		if (mp->addr + mp->size != mp->next->addr)
			break;
		if (mp->inuse && mp->size == 0)
			return (-1);
	}

	if (mp->next != &mem_blocks)
		return (-1);
	return (0);
}

mem_print ()
{
	struct mem_block *mp;

	for (mp = mem_blocks.next; mp != &mem_blocks; mp = mp->next) {
		printf ("%8x %8x %9d %c ",
			mp, mp->addr, mp->size, mp->inuse ? 'u' : 'f');
		if (mp->addr + mp->size != mp->next->addr)
			printf ("*");
		printf ("\n");
	}
}

char *blocks[10];

main ()
{
	int i;
	int n;
	int newsize;

	mem_init (0, 300);

	for (i = 0; i < 100000; i++) {
		n = random () % 10;

		if (blocks[n]) {
			newsize = random () % 20;
			if ((newsize & 1) == 0)
				newsize = 0;
			
			if (0)
				printf ("adjust %d %x %d\n",
					n, blocks[n], newsize);
			mem_adjust (blocks[n], newsize, NULL);
			if (newsize == 0)
				blocks[n] = NULL;
		} else {
			while ((newsize = random () % 20) == 0)
				;
			if (0)
				printf ("alloc %d %d\n", n, newsize);
			blocks[n] = mem_alloc (newsize, NULL);
		}
		if (mem_check () < 0) {
			printf ("==== %d\n", i);
			mem_print ();
		}
	}

	mem_print ();
}

#endif /* MEM_TEST */

