/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Clear a string 's' of length 'n'.
**
**	RCSID $Id: strclr.c,v 1.1.1.1 1992/09/28 20:08:50 trent Exp $
**
**	$Log: strclr.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:50  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

/*ARGSUSED*/
char *	
strclr(
	char *		s,
	register int	n
)
{
	/*
	**	If you can do this faster, in assembler,
	**	then you're a better man than I am, Gunga Din.
	*/

	if ( (n|(int)s) & (sizeof(long)-1) )		/* True for 1% of calls */
	{
		register char *	cp;
		register int	i;

		cp = s;

		if ( (i = (n+7) >> 3) == 0 && n == 0 )
			return cp;

		switch ( n & 7 )
		{
		default:
		case 0:	do {	*cp++ = '\0';
		case 7:		*cp++ = '\0';
		case 6:		*cp++ = '\0';
		case 5:		*cp++ = '\0';
		case 4:		*cp++ = '\0';
		case 3:		*cp++ = '\0';
		case 2:		*cp++ = '\0';
		case 1:		*cp++ = '\0';
			} while ( --i > 0 );
		}
	}
	else
	if ( n /= sizeof(long) )			/* This is 99% of cases */
	{
		register long *	lp;
		register int	i;

		lp = (long *)s;

		i = (n+7) >> 3;

		switch ( n & 7 )
		{
		default:
		case 0:	do {	*lp++ = 0;
		case 7:		*lp++ = 0;
		case 6:		*lp++ = 0;
		case 5:		*lp++ = 0;
		case 4:		*lp++ = 0;
		case 3:		*lp++ = 0;
		case 2:		*lp++ = 0;
		case 1:		*lp++ = 0;
			} while ( --i > 0 );
		}
	}

	return s;
}
