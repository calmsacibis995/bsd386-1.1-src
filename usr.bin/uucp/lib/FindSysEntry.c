/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Verify system name exists, accumulate details.
**
**	RCSID $Id: FindSysEntry.c,v 1.1.1.1 1992/09/28 20:08:43 trent Exp $
**
**	$Log: FindSysEntry.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:43  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	FILES

#include "global.h"


#ifdef	DEBUG
extern char	SS_AliasStr[];
extern char	SS_FoundStr[];
extern char	SS_Need[];
extern char	SS_No[];
#endif	/* DEBUG */


bool
FindSysEntry(
	char **		sysname,	/* Must have been Malloc'd if `needaliases' */
	Nst		needsys,	/* Need `SysDetails' set */
	Nat		needaliases	/* Need ALISAFILE details checked */
)
{
	register char *	cp;
	register Sysl *	sp;

	FreeStr(&SysDetails);

	if ( sysname == (char **)0 || (cp = *sysname) == NULLSTR )
	{
		Trace((1, "FindSysEntry(<null>)"));
		return (needaliases==NEEDALIAS)?true:false;
	}

	Trace((1,
		"FindSysEntry(%s, %ssys, %saliases)",
		cp,
		(needsys==NOSYS)?SS_No:SS_Need,
		(needaliases==NOALIAS)?SS_No:SS_Need
	));

	if ( *cp == '\0' || strncmp(cp, NODENAME, NODENAMEMAXSIZE) == STREQUAL )
		return (needaliases==NEEDALIAS)?true:false;

	if ( SysHead.next[0] == (Sysl *)0 )
		return ScanSys(sysname, needsys, needaliases);

	while ( (sp = LookupSys(cp)) != (Sysl *)0 )
	{
		if ( sp->type == alias_t )
		{
			if ( needaliases != NEEDALIAS )
				return false;

			Trace((2, SS_AliasStr, cp, sp->val));

			FreeStr(sysname);

			*sysname = newstr(cp = sp->val);

			if ( needsys == NEEDSYS )
				continue;

			Trace((2, SS_FoundStr, cp));
			return true;
		}

		if ( needsys == NEEDSYS )
			SysDetails = newstr(sp->val);

		Trace((2, SS_FoundStr, cp));
		return true;
	}

	return false;
}
