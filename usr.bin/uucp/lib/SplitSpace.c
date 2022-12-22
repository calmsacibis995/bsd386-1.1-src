/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Split arguments into separate char pointers on white space.
**	Inserts `max' `\0' characters into `arg'.
**	Returns number of arguments found.
**
**	RCSID $Id: SplitSpace.c,v 1.1.1.1 1992/09/28 20:08:41 trent Exp $
**
**	$Log: SplitSpace.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:41  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include	"global.h"


char *	LastField;		/* Pointer to data beyond last field extracted */


int
SplitSpace(
	char **		to,
	register char *	arg,
	int		max	/* Don't extract more than this */
)
{
	register int	c;
	register char *	cp;
	register int	count;

	Trace((3, "SplitSpace([%.32s], %d)", arg, max));

	count = 0;
	LastField = NULLSTR;

	if ( arg == NULLSTR )
		return count;

	for ( c = *arg++ ; c != '\0' ; )
	{
		while ( c == ' ' || c == '\t' || c == '\n' )
			c = *arg++;

		cp = arg-1;

		for ( ; c != '\0' ; c = *arg++ )
		{
			switch ( c )
			{
			case ' ': case '\t': case '\n':
				if ( count < max )
					arg[-1] = '\0';
				break;
			default:
				continue;
			}

			break;
		}

		if ( cp[0] != '\0' )
		{
			if ( ++count <= max )
			{
				*to++ = cp;
				Trace((7, "\tSplitSpace => %s", cp));
			}
			else
			if ( LastField == NULLSTR )
				LastField = cp;
		}
	}

	Trace((6, "\tSplitSpace %d strings", count));

	return count;
}
