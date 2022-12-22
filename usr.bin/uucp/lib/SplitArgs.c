/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Split arguments into separate char pointers on white space.
**	White space, and quotes may be quoted (single or double) or escaped.
**	Return number of arguments setup.
**
**	RCSID $Id: SplitArgs.c,v 1.1.1.1 1992/09/28 20:08:40 trent Exp $
**
**	$Log: SplitArgs.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:40  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include	"global.h"



int
SplitArgs(
	VarArgs	*	to,
	register char *	arg
)
{
	register int	c;
	register int	quote;
	register char *	cp;
	register int	count;
	char *		temp;
	bool		hadquote;

	Trace((2, "SplitArgs(%#lx, %s)", (Ulong)to, arg));

	count = NARGS(to);

	if ( arg == NULLSTR )
		return count;

	for ( temp = Malloc(strlen(arg)+2), c = *arg++ ; c != '\0' ; )
	{
		cp = temp;
		quote = '\0';
		hadquote = false;

		while ( c == ' ' || c == '\t' || c == '\n' )
			c = *arg++;

		for ( ; c != '\0' ; c = *arg++ )
		{
			if ( c == quote )
			{
				quote = '\0';
				continue;
			}

			switch ( c )
			{
			case ' ': case '\t': case '\n':
					if ( quote != '\0' )
						break;
					goto break2;

			case '\'': case '"':
					if ( quote != '\0' )
						break;
					hadquote = true;
					quote = c;
					continue;

			case '\\':	switch ( c = *arg++ )
					{
					case '\0':	arg--;
					default:	*cp++ = '\\';
					case '\\':	*cp++ = c;
							continue;

					case 'r':	*cp++ = '\r';
							continue;
					case 'n':	*cp++ = '\n';
							continue;
					case 't':	*cp++ = '\t';
							continue;
					case 's':	*cp++ = ' ';
							continue;
					case 'b':	*cp++ = '\b';
							continue;

					case '\'': case '"':
							if ( quote != '\0' && c != quote )
								*cp++ = '\\';
					case ' ': case '\t': case '\n':
							break;
					}
			}

			*cp++ = c;
		}

break2:
		if ( cp != temp || hadquote )
		{
			if ( ++count <= MAXVARARGS )
				NEXTARG(to) = newnstr(temp, cp-temp);

			DODEBUG(*cp = '\0'; Trace((3, "SplitArgs => %s", temp)));
		}
	}

	free(temp);
	return count;
}
