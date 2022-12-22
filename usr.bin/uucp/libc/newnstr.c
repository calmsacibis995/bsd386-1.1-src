/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Allocate new string with given length and copy arg into it.
**
**	RCSID $Id: newnstr.c,v 1.1.1.1 1992/09/28 20:08:49 trent Exp $
**
**	$Log: newnstr.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:49  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include	"global.h"

char *
newnstr(s, n)
	char *		s;
	int		n;
{
	register char *	cp;

	if ( s == NULLSTR )
	{
		s = EmptyStr;
		n = 0;
	}

	cp = strncpy(Malloc(n+1), s, n);

	cp[n] = '\0';

	return cp;
}
