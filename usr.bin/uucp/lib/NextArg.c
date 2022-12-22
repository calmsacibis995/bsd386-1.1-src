/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Parse string for shell arguments,
**	pass back pointer to first,
**	return pointer to next.
**
**	RCSID $Id: NextArg.c,v 1.1.1.1 1992/09/28 20:08:37 trent Exp $
**
**	$Log: NextArg.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:37  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include	"global.h"

char *
NextArg(str, res)
	register char *	str;
	register char *	res;
{
	register int	c;
	register char *	x;

	static char	space[]	= " \t\n";
	static char	seps[]	= "<>|;&";
	static char	all[]	= " \t\n<>|;&";

	Trace((4, "NextArg([%.*s], %#lx)", 16, str, (long)res));

	str += strspn(str, space);

	if ( (c = *str) == '\0' )
		return NULLSTR;

	if ( strchr(seps, c) != NULLSTR )
		c = 1;
	else
	if ( c == '(' && (x = strchr(str+1, ')')) != NULLSTR )
		c = x - str + 1;
	else
	if ( c == '\'' && (x = strchr(str+1, c)) != NULLSTR )
		c = x - str + 1;
	else
		c = strcspn(str, all);

	if ( res != NULLSTR )
	{
		(void)strncpy(res, str, c);
		res[c] = '\0';
		Trace((3, "NextArg => %s", res));
	}

	return str + c;
}
