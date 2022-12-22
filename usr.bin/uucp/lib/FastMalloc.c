/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Safe, fast "malloc" for programs that don't need to `free(3)'.
**
**	RCSID $Id: FastMalloc.c,v 1.1.1.1 1992/09/28 20:08:42 trent Exp $
**
**	$Log: FastMalloc.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:42  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include	"global.h"


#ifdef	FAST_MALLOC

static char *	Last;
static char *	Next;
static char *	End;
static int	Left;

#define		BRKINCR		8192

#ifndef	ALIGN
#define		ALIGN		sizeof(char *)
#endif
#define		MASK		(ALIGN-1)

#ifdef	MALLOC_SIZES
#include	<stdio.h>
static Ulong	Malloc_sizes[32];
#endif	/* MALLOC_SIZES */

extern char *	sbrk();

#undef	free



char *
Malloc(
	int		rsize
)
{
	register char *	cp;
	register int	incr;
	register int	size = rsize;

	DODEBUG(if(size==0)Fatal("Malloc(0)"));

#	ifdef	MALLOC_SIZES
	register int	i;

	for ( i = 0, incr = 4 ; incr < size ; incr <<= 1, i++ );
	Malloc_sizes[i]++;
#	endif	/* MALLOC_SIZES */

	size += MASK;
	size &= ~MASK;

	if ( Left < size )
	{
		incr = (size-Left) | (BRKINCR-1);
		incr++;

		while ( (cp = sbrk(incr)) == (char *)-1 )
			SysError("no memory for malloc(%d)", rsize);

		Trace((2, "Malloc(%d) => sbrk(%d) => %#lx", rsize, incr, (Ulong)cp));

		if ( cp == End )
			Left += incr;
		else
		{
			if ( End != NULLSTR )
				Trace((2, "Malloc() => non-contiguous (%#lx<>%#lx)", (Ulong)End, (Ulong)cp));

			Left = incr;
			Next = cp;
		}

		End = cp + incr;
	}

	cp = Next;
	Last = cp;
	Next += size;
	Left -= size;

	Trace((5, "Malloc(%d) => %#lx", rsize, (Ulong)cp));

	return cp;
}



char *
Malloc0(
	int	size
)
{
	return strclr(Malloc(size), size);
}



/*
**	Routines used by UNIX library functions.
*/

char *
calloc(
	int	nelem,
	int	size
)
{
	return Malloc0(nelem*size);
}



void
Free(
	char *	p
)
{
	Trace((6, "\tfree(%#lx)", (Ulong)p));

	if ( p != Last )
		return;

	Left += Next - Last;
	Next = Last;
	Last = NULLSTR;
}



void
free(
	char *	p
)
{
	Free(p);
}



char *
malloc(
	int	size
)
{
	return Malloc(size);
}



char *
realloc(
	char *		old,
	unsigned	size
)
{
	register char *	new;

	Free(old);
	new = Malloc((int)size);
	bcopy(old, new, size);
	return new;
}



#ifdef	MALLOC_SIZES
void
Print_MS(
	FILE *		fd
)
{
	register int	i;
	register Ulong	s, j, k;

	(void)fprintf(fd, "Malloc sizes histogram:\n");

	for ( k = 0, j = 4, i = 0 ; i < 32 ; i++, k = j, j <<= 1 )
		if ( (s = Malloc_sizes[i]) != 0 )
			(void)fprintf(fd, "%10ld-%-10ld%12ld\n", k, j, s);
}
#endif	/* MALLOC_SIZES */



#else	/* FAST_MALLOC */
static int _dummy_Malloc(){}
#endif	/* FAST_MALLOC */
