/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Linear search algorithm, generalized from Knuth (6.1) Algorithm S.
**
**	RCSID $Id: lfind.c,v 1.1.1.1 1992/09/28 20:08:50 trent Exp $
**
**	$Log: lfind.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:50  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/



char *
lfind(key, base, nelp, width, compar)
	char *		key;		/* Key to be located */
	register char *	base;		/* Beginning of table */
	int *		nelp;		/* Pointer (NB) to number of elements */
	register int	width;		/* Width of an element */
	int		(*compar)();	/* Comparison function */
{
	register char *	u;		/* Last element in table */
	int		nel = *nelp;

	if ( --nel < 0 )
		return (char *)0;

	u = base + nel*width;

	while ( u >= base )
	{
		if ( (*compar)(key, base) == 0 )
			return base;

		base += width;
	}

	return (char *)0;
}
