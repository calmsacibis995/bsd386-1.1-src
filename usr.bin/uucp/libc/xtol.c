/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Convert HEXADECIMAL format string to long integer.
**
**	RCSID $Id: xtol.c,v 1.1.1.1 1992/09/28 20:08:49 trent Exp $
**
**	$Log: xtol.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:49  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:48:05  piers
 * Initial revision
 *
*/

long
xtol(s)
	register char *	s;
{
	register long	n;
	register int	c;
	register int	yet;

	for ( n = 0, yet = 0 ; c = *s++ ; )
	{
		switch ( c )
		{
		case ' ': case '\t':	/* Ignore leading white space */
			if ( !yet )
				continue;
			return n;
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			c -= '0';
			break;
		default:
			switch ( c |= 040 )
			{
			case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
				c -= 'a';
				c += 10;
				break;
			default:
				return n;
			}
		}
		n = n * 16 + c;
		yet = 1;
	}

	return n;
}
