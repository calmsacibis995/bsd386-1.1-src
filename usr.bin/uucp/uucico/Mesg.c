/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Routines to send/receive intial handshake lines.
**
**	RCSID $Id: Mesg.c,v 1.1.1.1 1992/09/28 20:08:54 trent Exp $
**
**	$Log: Mesg.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:54  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	FILES
#define	SETJMP

#include	"global.h"
#include	"cico.h"


#define	SYNC	'\020'

static bool	SeenEnd;
static char	EndChar;

/*
**	Read line from remote.
*/

bool
InMesg(
	int		max,
	char *		amsg,
	register int	fn
)
{
	register char *	msg;
	register int	nchars;
	bool		foundsync;
	char		c;
	int		i = 0;

	Debug((5, "InMesg looking for SYNC<\\c"));

	for ( msg = amsg, nchars = 0, foundsync = false ;; )
	{
		if ( read(fn, &c, 1) != 1 )
		{
			Debug((1, " read FAILED"));
			return false;
		}

		DebugT(9, (9, "%s%s\\c", ExpandString(&c, 1), (((++i)%8)==0)?">\n\t<":EmptyStr));

		c &= 0177;

		if ( c == SYNC )
		{
			Debug((5, ">got sync<\\c"));
			foundsync = true;
			continue;
		}
		else
		if ( !foundsync )
			continue;

		if ( c == '\n' || c == '\0' )
		{
			if ( !SeenEnd )
			{
				EndChar = c;
				SeenEnd = true;
				Debug((9, ">\n\tUsing \\%#o as End of message char\n\t<\\c", EndChar));
			}

			break;
		}

		if ( ++nchars <= max )
			*msg++ = c;
	}
	*msg = '\0';

	Debug((5, ">\n\tgot %d characters\n\\c", msg-amsg));
	DebugT(4, (4, "InMesg <%s>", ExpandString(amsg, msg-amsg)));

	if ( nchars > max )
	{
		Debug((1, "buffer overrun in InMesg (%d>%d)", nchars, max));
		return false;
	}

	return foundsync;
}


/*
**	Write line to remote.
*/

int
OutMesg(
	char		type,
	register char *	msg,
	int		fn
)
{
	register char *	cp;
	int		n;
	char		buf[MAX_MESG_CHARS+4];

	cp = buf;
	*cp++ = SYNC;
	*cp++ = type;
	cp = strncpyend(cp, msg, MAX_MESG_CHARS);

	if ( SeenEnd ) 
		*cp = EndChar;
	else
		*++cp = '\n';

	cp++;	/* Include EndChar/'\0' */

	if ( (n = cp - buf) > MAX_MESG_CHARS && strlen(msg) > MAX_MESG_CHARS )
		Warn("OutMesg too large: %.32s...", msg);

	DebugT(4, (4, "OutMesg <%s>", ExpandString(buf, n)));

	return write(fn, buf, n);
}
