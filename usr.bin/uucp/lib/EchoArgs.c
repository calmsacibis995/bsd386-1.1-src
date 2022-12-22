/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Echo the arguments.
**
**	RCSID $Id: EchoArgs.c,v 1.1.1.1 1992/09/28 20:08:41 trent Exp $
**
**	$Log: EchoArgs.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:41  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	NO_VA_FUNC_DECLS
#define	STDIO

#include	"global.h"



void
EchoArgs(argc, argv)
	register int	argc;
	register char *	argv[];
{
	register char *	cp;

	MesgV(NULLSTR, (void *)0);	/* To get date-stamp and ``Name'' */

	while ( --argc >= 0 )
		if ( (cp = *argv++) != NULLSTR )
			(void)fprintf(ErrorFd, " \"%s\"", ExpandString(cp, -1));
		else
			(void)fprintf(ErrorFd, " <null>");

	putc('\n', ErrorFd);
	(void)fflush(ErrorFd);
}
