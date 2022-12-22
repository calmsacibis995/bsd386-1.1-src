/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Step through directories for command files for `node'.
**
**	Return malloc'd string for new directory path (ending in '/').
**
**	RCSID $Id: ChNodeDir.c,v 1.1.1.1 1992/09/28 20:08:37 trent Exp $
**
**	$Log: ChNodeDir.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:37  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	FILE_CONTROL

#include	"global.h"


char *
ChNodeDir(
	char *		node,
	char *		prev
)
{
	register char *	cp;
	register char **cpp;
	register bool	ok;
	char *		dir;
	DODEBUG(char *	np = (node==NULLSTR)?EmptyStr:node);

	Trace((3, "ChNodeDir(%s, %s)", np, (prev==NULLSTR)?NullStr:prev));
again:
	if ( prev == NULLSTR )
		ok = true;
	else
		ok = false;

	dir = NULLSTR;

	for ( cpp = SPOOLALTDIRS ; (cp = *cpp++) != NULLSTR ; )
	{
		Trace((5, "[%s]", cp));

		if ( ok )
		{
			dir = concat(cp, node, Slash, NULLSTR);
			break;
		}

		if ( strncmp(prev, cp, strlen(cp)) == STREQUAL )
			ok = true;
	}

	if ( prev != NULLSTR )
		free(prev);

	if ( (cp = dir) == NULLSTR )
	{
		Trace((1, "ChNodeDir(%s) => %s", np, NullStr));
		return cp;
	}

	while ( chdir(cp) == SYSERROR )
		if ( !CheckDirs(cp) )
		{
			if ( strncmp(cp, SPOOLDIR, SpoolDirLen) != STREQUAL )
			{
				/** Ignore non-existent non-SPOOLDIR **/

				prev = cp;
				goto again;
			}

			SysError(CouldNot, ChdirStr, cp);
		}

	Debug((1, "chdir => %s", cp));

	return cp;
}
