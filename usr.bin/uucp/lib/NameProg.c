/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Change name of program by overwriting args list.
**
**	RCSID $Id: NameProg.c,v 1.4 1994/01/31 01:25:56 donn Exp $
**
**	$Log: NameProg.c,v $
 * Revision 1.4  1994/01/31  01:25:56  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:57:32  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:41:56  vixie
 * Initial revision
 *
 * Revision 1.3  1993/12/19  23:18:24  donn
 * Use libutil setproctitle() routine.
 *
 * Revision 1.2  1993/02/28  15:28:45  pace
 * Add recent uunet changes
 *
 * Revision 1.1.1.1  1992/09/28  20:08:43  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/


#define	NO_VA_FUNC_DECLS

#include	"global.h"

#if defined(__bsdi__) && (_BSDI_VERSION >= 199312)

/* NameProg() is #defined to setproctitle() */

void SetNameProg(int argc, char **argv, char **envp) {}

#else

static char *	Argv0;
static int	ArgvLen;
static char *	LastEnd;

void
setproctitle(va_alist)
	va_dcl
{
	register va_list vp;
	register char *	cp;
	register char *	ep;
	register char *	s;

	if ( Argv0 == NULLSTR )
	{
		Debug((1, "No SetNameProg()"));
		return;
	}

	va_start(vp);
	s = newvprintf(vp);
	va_end(vp);

	ep = LastEnd;
	cp = strncpyend(Argv0, s, ArgvLen);
	LastEnd = cp;

	while ( cp < ep )
		*cp++ = '\0';

	Trace((1, "NameProg(%s)", s));
	free(s);
}

void
SetNameProg(
	int	argc,
	char *	argv[],
	char *	envp[]
)
{
	Argv0  = argv[0];
	*Argv0++ = '-';		/* Makes `ps' print " (uucico)" */

	LastEnd = argv[argc-1];

	while ( *envp != NULLSTR )
		LastEnd = *envp++;

	LastEnd += strlen(LastEnd);

	ArgvLen = LastEnd - Argv0;
}

#endif
