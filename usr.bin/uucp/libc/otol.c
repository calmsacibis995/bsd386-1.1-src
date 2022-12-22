/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Convert OCTAL format string to long integer.
**
**	RCSID $Id: otol.c,v 1.1.1.1 1992/09/28 20:08:49 trent Exp $
**
**	$Log: otol.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:49  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:48:05  piers
 * Initial revision
 *
*/

long
otol(p)
	register char *	p;
{
	register long	n = 0;
	register int	c;

	while ( (c = *p++) == ' ' || c == '\t' );

	do
	{
		if ( (c -= '0') > 7 || c < 0 )
			break;
		n = n * 8 + c;
	}
	while
		( c = *p++ );

	return n;
}
