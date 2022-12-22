/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Trace on TraceFd.
**
**	RCSID $Id: Trace.c,v 1.1.1.1 1992/09/28 20:08:36 trent Exp $
**
**	$Log: Trace.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:36  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	NO_VA_FUNC_DECLS
#define	STDIO

#include	"global.h"


#ifdef	DEBUG

FILE *	TraceFd	= stderr;

#define	Fflush	(void)fflush
#define	Fprintf	(void)fprintf



void
_Trace(va_alist)
	va_dcl
{
	register va_list vp;
	register int	l;
#	if	NO_VFPRINTF
	register int	i;
	char *		a[10];
#	else	/* NO_VFPRINTF */
	char *		fmt;
#	endif	/* NO_VFPRINTF */
	static bool	intrace;

	va_start(vp);
	l = va_arg(vp, int);

	if ( Traceflag < l || intrace )
	{
		va_end(vp);
		return;
	}

	intrace = true;

#	if	NO_VFPRINTF
	for ( i = 0 ; i < 10 ; i++ )
		a[i] = va_arg(vp, char *);
#	endif	/* NO_VFPRINTF */

	Fflush(stdout);
	Fflush(stderr);

	(void)fseek(TraceFd, (long)0, 2);

	if ( l > 0 )
	{
		putc('\t', TraceFd);
		while ( --l > 0 )
			putc(' ', TraceFd);
	}

#	if	NO_VFPRINTF

	Fprintf(TraceFd, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9]);

#	else	/* NO_VFPRINTF */

	fmt = va_arg(vp, char *);
	(void)vfprintf(TraceFd, fmt, vp);

#	endif	/* NO_VFPRINTF */

	va_end(vp);

	putc('\n', TraceFd);
	Fflush(TraceFd);

	intrace = false;
}
#endif	DEBUG
