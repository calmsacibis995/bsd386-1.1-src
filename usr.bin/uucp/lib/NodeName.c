/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Routine to obtain local CPU ``node'' name.
**
**	RCSID $Id: NodeName.c,v 1.1.1.1 1992/09/28 20:08:39 trent Exp $
**
**	$Log: NodeName.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:39  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	FILE_CONTROL
#define	STAT_CALL

#include	"global.h"

int		NodeNameLen;	/* Length of NODENAME in `nodename' */
static char	nodename[NODENAMEMAXSIZE+1];
char *		RealNodeName;	/* Real nodename */


#if	SYSV > 0

#include	<sys/utsname.h>

char *
NodeName()
{
	struct utsname	SysNames;

	if ( RealNodeName != NULLSTR )
		return NODENAME;

	SysNames.nodename[0] = '\0';

	(void)uname(&SysNames);

	RealNodeName = newstr(SysNames.nodename);

	if ( NODENAME == NULLSTR || NODENAME[0] == '\0' )
	{
		if ( SysNames.nodename[0] == '\0' )
		{
			Warn(english("null system nodename"));
			NODENAME = strcpy(nodename, NODEUNKNOWN);
		}
		else
		{
			char *	cp;

			if ( (cp = strchr(SysNames.nodename, '.')) != NULLSTR )
				*cp = '\0';
			NODENAME = strncpy(nodename, SysNames.nodename, NODENAMEMAXSIZE);
			nodename[NODENAMEMAXSIZE] = '\0';
		}

		Trace((1, "NodeName() => %s", NODENAME));
	}

	NodeNameLen = strlen(NODENAME);
	return NODENAME;
}

#else	/* SYSV > 0 */



#if	BSD4 >= 2

char *
NodeName()
{
	char	temp[65];

	if ( RealNodeName != NULLSTR )
		return NODENAME;

	(void)gethostname(temp, sizeof(temp)-1);
	temp[sizeof(temp)-1] = '\0';
	RealNodeName = newstr(temp);

	if ( NODENAME == NULLSTR || NODENAME[0] == '\0' )
	{
		if ( temp[0] == '\0' )
		{
			Warn(english("null system nodename"));
			NODENAME = strcpy(nodename, NODEUNKNOWN);
		}
		else
		{
			char *	cp;

			if ( (cp = strchr(temp, '.')) != NULLSTR )
				*cp = '\0';
			NODENAME = strncpy(nodename, temp, NODENAMEMAXSIZE);
			nodename[NODENAMEMAXSIZE] = '\0';
		}

		Trace((1, "NodeName() => %s", NODENAME));
	}

	NodeNameLen = strlen(NODENAME);
	return NODENAME;
}

#else	/* BSD4 >= 2 */


char *
NodeName()
{
	if ( RealNodeName != NULLSTR )
		return NODENAME;

#	ifdef	NODENAMEFILE
	if ( (RealNodeName = ReadFile(NODENAMEFILE)) == NULLSTR )
		(void)SysWarn
			(
				CouldNot, ReadStr,
				NODENAMEFILE==NULLSTR?"<NODENAMEFILE>":NODENAMEFILE
			);
	else
	if ( RealNodeName[0] == '\0' )
	{
		free(RealNodeName);
		RealNodeName = NULLSTR;
	}
#	endif	/* NODENAMEFILE */

	if ( RealNodeName == NULLSTR )
		if ( NODENAME == NULLSTR || NODENAME[0] == '\0' )
		{
			Warn(english("null nodename"));
			RealNodeName = NODEUNKNOWN;
		}
		else
			RealNodeName = NODENAME;
	
	if ( NODENAME == NULLSTR || NODENAME[0] == '\0' )
	{
		char *	cp;

		if ( (cp = strchr(RealNodeName, '.')) != NULLSTR )
			*cp = '\0';
		NODENAME = strncpy(nodename, RealNodeName, NODENAMEMAXSIZE);
		nodename[NODENAMEMAXSIZE] = '\0';
		if ( cp != NULLSTR )
			cp = '.';
		Trace((1, "NodeName() => %s", NODENAME));
	}

	NodeNameLen = strlen(NODENAME);
	return NODENAME;
}

#endif	/* BSD4 >= 2 */
#endif	/* SYSV > 0 */
