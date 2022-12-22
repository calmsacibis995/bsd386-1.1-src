/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Sanitize environment.
**
**	RCSID $Id: StripEnv.c,v 1.1.1.1 1992/09/28 20:08:39 trent Exp $
**
**	$Log: StripEnv.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:39  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	SYSEXITS

#include	"global.h"


char		PATH[]	= "PATH=/bin:/usr/bin";
#define		PATHLEN		5

static char *	Envs[]	=
{
#	ifdef	APOLLO
	"NODEID=",
	"SYSTYPE=",
#	endif	/* APOLLO */
#	if	SYSV > 0
	"TZ=",
#	endif	/* SYSV > 0 */
	NULLSTR
};
#define	NENVS	ARR_SIZE(Envs)
#define	MAXENVS	(NENVS+4)

static char *	Env[MAXENVS];

char **		NewEnvs;	/* Global for importing new environ */



char **
StripEnv(
	char **		pathp
)
{
	register char *	cp;
	register char *	ep;
	register char **cpp;
	register char **epp;

	for ( epp = Env, cpp = Envs ; (cp = *cpp++) != NULLSTR ; )
		if ( (ep = getenv(cp)) != NULLSTR )
			*epp++ = ep - strlen(cp);

	if ( (cpp = NewEnvs) != (char **)0 )
	{
		*pathp = NULLSTR;

		while ( (cp = *cpp++) != NULLSTR )
		{
			if ( epp == &Env[MAXENVS-1] )
			{
				ErrVal = EX_USAGE;
				Error("Too many environment vars");
			}

			*epp++ = cp;

			if ( strncmp(cp, PATH, PATHLEN) == STREQUAL )
				*pathp = cp+PATHLEN;
		}
	}
	else
	{
		*epp++ = PATH;
		*pathp = PATH+PATHLEN;
	}

	*epp = NULLSTR;

	return Env;
}
