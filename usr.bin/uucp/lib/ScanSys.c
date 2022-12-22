/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Scan ALIASFILE, then SYSFILE, for match.
**
**	RCSID $Id: ScanSys.c,v 1.1.1.1 1992/09/28 20:08:43 trent Exp $
**
**	$Log: ScanSys.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:43  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	FILES

#include "global.h"


/*
**	Variables.
*/

char *		SysDetails;	/* SYSFILE params for current node if needed */

char		SS_Space[]	= " \t";
#ifdef	DEBUG
char		SS_AliasStr[]	= "\tALIAS %s TO %s";
char		SS_FoundStr[]	= "FOUND %s";
char		SS_GotStr[]	= "\tgot %s %s";
char		SS_Need[]	= "need";
char		SS_No[]		= "NO";
#endif	/* DEBUG */


/*
**	Return next node entry from file.
**
**	Value is node, pointer to params passed back via `vp'.
**
**	node[<space><param>]*
*/

char *
GetSysNode(
	char **		vp,	/* Pass back params */
	char **		bufp	/* Pointer to input */
)
{
	register char *	cp;
	register char *	dp;

	if ( (dp = GetLine(bufp)) == NULLSTR )
		return dp;

	if ( (cp = strpbrk(dp, SS_Space)) != NULLSTR )
	{
		*cp++ = '\0';
		cp += strspn(cp, SS_Space);

		if ( *cp == '\0' )
			cp = NULLSTR;
	}

	*vp = cp;
	return dp;
}

/*
**	Scan ALIASFILE (if `needaliases'), then SYSFILE, for match.
**
**	(This is faster than LoadSys() if only one lookup.)
*/

bool
ScanSys(
	char **		sysname,	/* Must have been Malloc'd if `needaliases' */
	Nst		needsys,	/* Need `SysDetails' set */
	Nat		needaliases	/* Need ALISAFILE details checked */
)
{
	register char *	cp;
	register char *	p;
	register char *	q;
	bool		ret;
	char *		val;
	char *		bufp;
	char *		data;

	Trace((2,
		"ScanSys(%s, %ssys, %saliases)",
		*sysname,
		(needsys==NOSYS)?SS_No:SS_Need,
		(needaliases==NOALIAS)?SS_No:SS_Need
	));

	FreeStr(&SysDetails);

	if ( needaliases == NEEDALIAS && (cp = ReadFile(ALIASFILE)) != NULLSTR )
	{
		bufp = data = cp;

		while ( (cp = GetSysNode(&val, &bufp)) != NULLSTR )
		{
			TraceT(8, (8, SS_GotStr, cp, (val==NULLSTR)?EmptyStr:val));

			if ( strcmp(*sysname, cp) == STREQUAL )
			{
found:				if ( needsys == NOSYS )
				{
					free(data);
					Trace((2, SS_FoundStr, cp));
					return true;
				}

				break;
			}

			if ( (p = val) != NULLSTR )
			for ( ;; )
			{
				if ( (q = strpbrk(p, SS_Space)) != NULLSTR )
					*q++ = '\0';

				if ( strcmp(*sysname, p) == STREQUAL )
				{
					Trace((2, SS_AliasStr, p, cp));
					FreeStr(sysname);
					*sysname = newstr(cp);
					goto found;
				}

				if ( (p = q) == NULLSTR )
					break;

				p += strspn(p, SS_Space);
			}
		}

		free(data);
	}

	while ( (cp = ReadFile(SYSFILE)) == NULLSTR )
		SysError(CouldNot, ReadStr, SYSFILE);

	bufp = data = cp;
	ret = false;

	while ( (cp = GetSysNode(&val, &bufp)) != NULLSTR )
	{
		TraceT(8, (8, SS_GotStr, cp, (val==NULLSTR)?EmptyStr:val));

		if ( strcmp(*sysname, cp) != STREQUAL )
			continue;

		Trace((2, SS_FoundStr, cp));
		ret = true;

		if ( needsys == NOSYS )
			break;

		if ( (cp = SysDetails) != NULLSTR )
		{
			Trace((3, "%s: multiple entry", *sysname));

			if ( val != NULLSTR )
			{
				SysDetails = concat(cp, "\n", val, NULLSTR);
				free(cp);
			}
		}
		else
		if ( val != NULLSTR )
			SysDetails = newstr(val);
	}

	free(data);
	return ret;
}
