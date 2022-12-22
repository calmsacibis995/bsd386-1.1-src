/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Debug on DebugFd.
**
**	RCSID $Id: Debug.c,v 1.1.1.1 1992/09/28 20:08:43 trent Exp $
**
**	$Log: Debug.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:43  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	NO_VA_FUNC_DECLS
#define	STDIO

#include	"global.h"


FILE *	DebugFd	= stderr;

#define	Fflush	(void)fflush
#define	Fprintf	(void)fprintf



void
_Debug(va_alist)
	va_dcl
{
	register va_list vp;
	register int	l;
	register int	i;
#	if	NO_VFPRINTF
	char *		a[10];
#	endif	/* NO_VFPRINTF */
	char		cfmt[128];
	char *		fmt;
	static bool	indebug;

	va_start(vp);
	l = va_arg(vp, int);

	if ( Debugflag < l || indebug )
	{
		va_end(vp);
		return;
	}

	indebug = true;

	Fflush(stdout);
	Fflush(stderr);

	(void)fseek(DebugFd, (long)0, 2);

#	if	NO_VFPRINTF

	for ( l = 0 ; l < 10 ; l++ )
		a[l] = va_arg(vp, char *);

	fmt = a[0];

#	else	/* NO_VFPRINTF */

	fmt = va_arg(vp, char *);

#	endif	/* NO_VFPRINTF */

	i = strlen(fmt);

	if ( fmt[--i] == 'c' && fmt[--i] == '\\' )
	{
		fmt = strncpy(cfmt, fmt, i);
		fmt[i] = '\0';
	}
	else
	{
		i = -1;
/*
**		if ( l > 0 && fmt[0] != '\n' && fmt[0] != '\t' )
**		{
**			putc('\t', DebugFd);
**
**			while ( --l > 0 )
**				putc(' ', DebugFd);
**		}
*/	}

#	if	NO_VFPRINTF

	Fprintf(DebugFd, fmt, a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9]);

#	else	/* NO_VFPRINTF */

	(void)vfprintf(DebugFd, fmt, vp);

#	endif	/* NO_VFPRINTF */

	va_end(vp);

	if ( i < 0 )
		putc('\n', DebugFd);

	Fflush(DebugFd);

	indebug = false;
}
