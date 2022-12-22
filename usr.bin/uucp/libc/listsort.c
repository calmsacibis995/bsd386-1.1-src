/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Sort a list in place using merge sort.
**
**	RCSID $Id: listsort.c,v 1.1.1.1 1992/09/28 20:08:50 trent Exp $
**
**	$Log: listsort.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:50  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/


typedef struct ListEl *	ListP;

struct ListEl
{
	ListP	next;
};



void
listsort(
	ListP *		list,
	int		(*compar)(char *, char *)
			/* Same as compar in qsort(3) */
)
{
	register ListP *mt;
	register ListP	tmp;
	ListP		a;
	register ListP	b;
	register int	j;
	register ListP	t;
	ListP		sorted[32];	/* (2**33)-1 items */

	if ( (t = *list) == (ListP)0 )
		return;

	for ( j = 0 ; j < 32 ; j++ )
		sorted[j] = (ListP)0;

	while ( (a = t) != (ListP)0 && (b = a->next) != (ListP)0 )
	{
		t = b->next;

		if ( (*compar)((char *)a, (char *)b) > 0 )
		{
			b->next = a;
			a->next = (ListP)0;
			a = b;
		}
		else
			b->next = (ListP)0;

		for ( j = 0 ; ; j++ )
		{
			if ( (b = sorted[j]) == (ListP)0 )
			{
				sorted[j] = a;
				break;
			}

			/** Merge equal length lists a and b **/

			sorted[j] = (ListP)0;

			for ( mt = &a ;; )
			{
				if ( (*compar)((char *)*mt, (char *)b) > 0 )
				{
					tmp = b->next;
					b->next = *mt;
					*mt = b;

					if ( (b = tmp) == (ListP)0 )
						break;
				}

				mt = &(*mt)->next;

				if ( *mt == (ListP)0 )
				{
					*mt = b;
					break;
				}
			}
		}
	}

	for ( j = 0 ; a == (ListP)0 ; j++ )
		a = sorted[j];

	for ( ; j < 32 ; j++ )
	{
		if ( (b = sorted[j]) != (ListP)0 )
		{
			for ( mt = &a ;; )
			{
				if ( (*compar)((char *)*mt, (char *)b) > 0 )
				{
					tmp = b->next;
					b->next = *mt;
					*mt = b;

					if ( (b = tmp) == (ListP)0 )
						break;
				}

				mt = &(*mt)->next;

				if ( *mt == (ListP)0 )
				{
					*mt = b;
					break;
				}
			}
		}
	}

	*list = a;
}
