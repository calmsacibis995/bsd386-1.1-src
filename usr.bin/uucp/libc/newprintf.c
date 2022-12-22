/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	`sprintf' into allocated string.
**
**	string has extra char on end.
**
**	RCSID $Id: newprintf.c,v 1.2 1993/02/28 15:31:37 pace Exp $
**
**	$Log: newprintf.c,v $
 * Revision 1.2  1993/02/28  15:31:37  pace
 * Increase buffer size - a recent uunet change.
 *
 * Revision 1.1.1.1  1992/09/28  20:08:49  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/


#define	NO_VA_FUNC_DECLS
#define	STDIO

#include	"global.h"



char *
newprintf(va_alist)
	va_dcl
{
	va_list	vp;
	char *	val;

	va_start(vp);
	val = newvprintf(vp);
	va_end(vp);

	return val;
}



char *
newvprintf(vp)
	register va_list vp;
{
	register int	i;
	char *		l;
	char *		fmt;
#	if	NO_VFPRINTF
	char *		a[12];
#	endif	/* NO_VFPRINTF */
	char		buf[10240];

	if ( (fmt = va_arg(vp, char *)) == NULLSTR )
		i = 0;
	else
	{
#		if	NO_VFPRINTF

		for ( i = 0 ; i < 12 ; i++ )
			a[i] = va_arg(vp, char *);

		l = (char *)sprintf(buf, fmt,
				a[0], a[1], a[2], a[3], a[4], a[5],
				a[6], a[7], a[8], a[9], a[10], a[11]
				);

#		else	/* NO_VFPRINTF */

		l = (char *)vsprintf(buf, fmt, vp);

#		endif	/* NO_VFPRINTF */

		if ( l == buf )
			i = strlen(buf);/* BSD4 tells us what we already know */
		else
			i = (int)l;	/* SYSV returns char count */
	}

	return newnstr(buf, i);
}
