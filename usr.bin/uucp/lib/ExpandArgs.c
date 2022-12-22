/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Expand arguments containing any '&x' sequences;
**	change arguments containing any '%xstring%' sequences.
**
**	RCSID $Id: ExpandArgs.c,v 1.1.1.1 1992/09/28 20:08:41 trent Exp $
**
**	$Log: ExpandArgs.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:41  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include	"global.h"

static char *	selecta(int);
static int	selectp(int);
static int	selectx(int);



void
ExpandArgs(
	VarArgs	*	to,
	char **		app
)
{
	register char *	ap;
	register char *	dp;
	register char *	sp;
	register char *	cp;
	register int	size;
	char *		string;
	bool		hadsub;

	if ( app == (char **)0 )
		return;

	while ( (ap = *app++) != NULLSTR )
	{
		if ( *ap == '\0' )
			continue;

		dp = ap;
		size = 1;
		hadsub = false;

		while ( (dp = strchr(dp, '&')) != NULLSTR )
			if ( (sp = selecta(*++dp)) != NULLSTR )
			{
				size += strlen(sp);
				dp++;
			}

		size += strlen(ap);

		string = sp = Malloc(size);

		while ( *ap != '\0' && (dp = strpbrk(ap, "&%")) != NULLSTR )
		{
			hadsub = true;

			if ( size = dp-ap )
				sp = strncpy(sp, ap, size) + size;

			if ( *dp++ == '&' )
			{
				if ( (cp = selecta(*dp)) != NULLSTR )
				{
					sp = strcpyend(sp, cp);
					dp++;
				}
				else
				{
					*sp++ = '&';
					*sp++ = *dp++;
				}
			}
			else
			{
				register int	negate;
				register int	testex;

				testex = 0;
				size = *dp;

				if ( size == '+' || size == '-' )
				{
					testex = 1;
					negate = (size == '-');
					size = *++dp;
				}
				else
				if ( negate = (size == '!') )
					size = *++dp;

				cp = &dp[1];

				if
				(
					(ap = strchr(cp, '%')) != NULLSTR
					&&
					(size = testex?selectx(size):selectp(size)) != 2
				)
				{
					if ( size != negate )
					{
						while
						(
							(dp = strchr(cp, '&')) != NULLSTR
							&&
							dp < ap
						)
						{
							if ( size = dp-cp )
								sp = strncpy(sp, cp, size) + size;
							if ( (cp = selecta(*++dp)) != NULLSTR )
							{
								sp = strcpyend(sp, cp);
								dp++;
							}
							else
							{
								*sp++ = '&';
								*sp++ = *dp++;
							}
							cp = dp;
						}
						if ( size = ap-cp )
							sp = strncpy(sp, cp, size) + size;
					}
					dp = ap + 1;
				}
				else
				{
					*sp++ = '%';
					if ( testex )
					{
						if ( negate )
							*sp++ = '-';
						else
							*sp++ = '+';
					}
					else
					if ( negate )
						*sp++ = '!';
				}
			}

			ap = dp;
		}

		(void)strcpy(sp, ap);

		if ( !hadsub || strlen(string) > 0 )
			NEXTARG(to) = string;
		else
			free(string);
	}
}



/*
**	String test: NULLSTR - nomatch.
**
**	(Include changes in `selectx()' below.)
*/

static char *
selecta(
	int	c
)
{
	char *	cp;

	switch ( c )
	{
	case 'F':	cp = SenderName; break;
	case 'H':	cp = NODENAME; break;
	case 'O':	cp = SourceAddress; break;
	case 'U':	cp = UserName; break;
	case '&':	cp = "&"; break;
	case '%':	cp = "%"; break;
	default:	return NULLSTR;
	}

	return (cp == NULLSTR) ? EmptyStr : cp;
}



/*
**	Boolean test: 0 - false; 1 - true; 2 - nomatch.
*/

static int
selectp(
	int	c
)
{
	switch ( c )
	{
	case 'H':	if ( SourceAddress == NULLSTR || NODENAME == NULLSTR )
				return 0;
			return strcmp(SourceAddress, NODENAME) == STREQUAL;
	}

	return 2;
}



/*
**	Boolean null string test: 0 - false; 1 - true; 2 - nomatch.
*/

static int
selectx(
	int	c
)
{
	switch ( c )
	{
	case 'F':	return SenderName != NULLSTR;
	case 'H':	return NODENAME != NULLSTR;
	case 'O':	return SourceAddress != NULLSTR;
	case 'U':	return UserName != NULLSTR;
	case '&':	return 1;
	case '%':	return 1;
	}

	return 2;
}
