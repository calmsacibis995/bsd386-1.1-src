/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	General error output routine:
**		precede (non-tty) output with datestamp;
**		assumes first arg is error decription,
**			second arg is printf format string,
**			other args are printf parameters.
**
**	RCSID $Id: MesgV.c,v 1.1.1.1 1992/09/28 20:08:38 trent Exp $
**
**	$Log: MesgV.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:38  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/


#define	STDIO

#include	"global.h"


FILE *		ErrorFd	= stderr;

extern char *	Name;

#define	Fprintf	(void)fprintf
#define	Fflush	(void)fflush



void
MesgV(err, vp)
	char *		err;
	register va_list vp;
{
#	if	NO_VFPRINTF
	register int	i;
	char *		a[10];
#	endif	/* NO_VFPRINTF */
	char *		fmt;

	Fflush(stdout);

	if ( !IsattyDone )
		(void)ErrorTty((int *)0);

	if ( !ErrIsatty )
	{
		char	buf[ISODATETIMESTRLEN];

		(void)SetTimes();
		ISODateTimeStr(Time, Timeusec, buf);
		(void)fseek(ErrorFd, (long)0, 2);
		Fprintf(ErrorFd, "%s ", buf);
	}

	Fprintf(ErrorFd, "%s: ", Name);

	if ( err == NULLSTR )
		return;

	(void)fputs(err, ErrorFd);

	if ( (fmt = va_arg(vp, char *)) == NULLSTR )
		return;

	(void)fputs(" -- ", ErrorFd);

#	if	NO_VFPRINTF

#	if	DOPRNT_BUG == 1
	if ( strchr(fmt, '%') == NULLSTR )
	{
		(void)fputs(fmt, ErrorFd);	/* To bypass an old '_doprnt' bug */
		return;
	}
#	endif	/* DOPRNT_BUG == 1 */

	for ( i = 0 ; i < 10 ; i++ )
		a[i] = va_arg(vp, char *);

	Fprintf(ErrorFd, fmt, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9]);

#	else	/* NO_VFPRINTF */

	(void)vfprintf(ErrorFd, fmt, vp);

#	endif	/* NO_VFPRINTF */
}
