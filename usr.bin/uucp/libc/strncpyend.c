/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Copy s2 into s1 (up to `n')
**	and return address of null (or `n'th char) in s1.
**
**	RCSID $Id: strncpyend.c,v 1.1.1.1 1992/09/28 20:08:49 trent Exp $
**
**	$Log: strncpyend.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:49  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

char *
strncpyend(s1, s2, n)
	char *		s1;
	char *		s2;
	register int	n;
{
	register char *	cp1 = s1;
	register char *	cp2 = s2;

	do
		if ( --n < 0 )
			return cp1;
	while
		( *cp1++ = *cp2++ );

	return cp1 - 1;
}
