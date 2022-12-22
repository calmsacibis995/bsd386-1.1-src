/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Split name into system and file part.
**
**	Return true if split.
**
**	RCSID $Id: SplitSysName.c,v 1.1.1.1 1992/09/28 20:08:38 trent Exp $
**
**	$Log: SplitSysName.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:38  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include	"global.h"

bool
SplitSysName(name, sys, rest)
	register char *	name;
	char **		sys;	/* system name if `true' */
	register char *	rest;	/* non-system part of name */
{
	register char *	cp;
	DODEBUG(static char	ds[] = "SplitSysName => \"%s\" ! \"%s\"");

	Trace((2, "SplitSysName(%s)", name));

	FreeStr(sys);

	if ( *name == '(' && (cp = strchr(name+1, ')')) != NULLSTR )
	{
		name++;
		(void)strncpy(rest, name, cp-name);
		rest[cp-name] = '\0';

		Trace((1, ds, NullStr, rest));
		return false;
	}

	if ( (cp = strchr(name, '!')) == NULLSTR )
	{
		(void)strcpy(rest, name);

		Trace((1, ds, NullStr, rest));
		return false;
	}

	if ( cp == name )
	{
		++cp;
		*sys = newstr(NODENAME);
	}
	else
	{
		*cp++ = '\0';
		*sys = newnstr(name, NODENAMEMAXSIZE);
	}

	(void)strcpy(rest, cp);

	Trace((1, ds, *sys, rest));
	return true;
}
