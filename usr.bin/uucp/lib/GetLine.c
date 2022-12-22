/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Return pointer to null-teminated non-blank, non-comment, possibly continued line.
**
**	Following character sequences are processed:
**		Chars			Result
**		<\>[^<NL><#>]		nochange (escapes <"> <\>)
**		<">.*<">		nochange (escapes <#> <\> <NL>)
**		<\><NL>			removed (line continued)
**		<NL>[<sp><tab>]+	<NL> removed (line continued)
**		<\><#>			<\> removed
**		[<sp><tab>]*<#>.*<NL>	removed
**		^[<sp><tab>]*		removed
**
**	Data is copied in buffer.
**
**	RCSID $Id: GetLine.c,v 1.1.1.1 1992/09/28 20:08:40 trent Exp $
**
**	$Log: GetLine.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:40  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include	"global.h"



char *
GetLine(bufp)
	char **		bufp;
{
	register int	c;
	register char *	bp;
	register char *	cp;
	register char *	dp;
	register int	quote;

	if ( bufp == (char **)0 || (bp = *bufp) == NULLSTR )
		return NULLSTR;

	Trace((9, "GetLine(%.16s)", bp));

	for ( cp = dp = bp, c = *bp++ ; c != '\0' ; )
	{
		quote = '\0';

		while ( c == ' ' || c == '\t' || c == '\n' )
			c = *bp++;

		if ( c == '#' )
		{
			while ( (c = *bp++) != '\0' && c != '\n' );
			continue;
		}

		for ( ; c != '\0' ; c = *bp++ )
		{
			if ( c == quote )
			{
				quote = '\0';
				*cp++ = c;
				continue;
			}

			switch ( c )
			{
			case '#':	if ( quote != '\0' )
						break;
					while ( (c = *bp++) != '\0' && c != '\n' );
					if ( c == '\0' )
						goto break0;
					if ( bp[-2] == '\\' )
						continue;
					goto break2;

			case '\n':	if ( quote != '\0' )
						break;
					if ( (c = *bp) == '\t' || c == ' ' )
					{
						bp++;
						break;
					}
					goto break2;
#			ifdef	SINGLE_QUOTE
			case '\'':
#			endif	/* SINGLE_QUOTE */
			case '"':
					if ( quote != '\0' )
						break;
					quote = c;
					break;

			case '\\':	switch ( c = *bp++ )
					{
					case '#':	if ( quote != '\0' )
								break;
							*cp++ = c;
					case '\n':	continue;
					case '\0':	*cp++ = '\\';
							goto break0;
					}
					*cp++ = '\\';
					break;
			}

			*cp++ = c;
		}
break0:
		--bp;	/* Back over '\0' */
break2:
		if ( cp > dp )
		{
			*cp = '\0';
			TraceT(8, (8,
				"\tGetLine => %.16s%s",
				dp, ((cp-dp)>16)?" ...":EmptyStr
			));
			*bufp = bp;
			return dp;
		}
	}

	*bufp = --bp;
	return NULLSTR;
}
