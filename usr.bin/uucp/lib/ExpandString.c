/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Expand a string to printable characters.
**
**	RCSID $Id: ExpandString.c,v 1.1.1.1 1992/09/28 20:08:37 trent Exp $
**
**	$Log: ExpandString.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:37  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include	"global.h"


static char *	Buf;
static int	BufLen;
#define	MAXLEN	511



char *
ExpandString(string, size)
	char *		string;
	int		size;
{
	register int	c;
	register char *	cp;
	register char *	ep;
	register char *	bp;
	register char *	bep;

	if ( (cp = string) == NULLSTR )
		return EmptyStr;

	if ( (c = size) < 0 )
		c = strlen(cp);

	if ( c == 0 )
		return EmptyStr;

	if ( c > MAXLEN )
	{
		Warn("ExpandString size %d truncated to %d", c, MAXLEN);
		c = MAXLEN;
	}

	size = c;

	c = c * 4 + 1;

	if ( BufLen < c )
	{
		c &= ~63;
		c += 64;

		BufLen = c;

		if ( Buf != NULLSTR )
			free(Buf);

		Buf = Malloc(BufLen);
	}

	bp = Buf;
	bep = &bp[BufLen-5];

	for ( ep = &cp[size] ; cp < ep ; )
	{
		if ( ((c = *cp++) < ' ' /*&& c != '\n'*/) || c >= '\177' || c == '\\' )
		{
			(void)sprintf(bp, "%c%03o", '\\', c&0xff);
			bp += 4;
		}
		else
			*bp++ = c;

		if ( bp >= bep )
			break;
	}

	*bp = '\0';

	return Buf;
}
