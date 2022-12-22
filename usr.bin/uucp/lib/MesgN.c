/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Error message output routine:
**		precede (non-tty) output with datestamp;
**		assumes first arg is error decription,
**			second arg is printf format string,
**			other args are printf parameters;
**		appends '\n'.
**
**	RCSID $Id: MesgN.c,v 1.1.1.1 1992/09/28 20:08:37 trent Exp $
**
**	$Log: MesgN.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:37  trent
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
MesgN(va_alist)
	va_dcl
{
	register va_list vp;
	register char *	err;

	va_start(vp);
	err = va_arg(vp, char *);
	MesgV(err, vp);
	va_end(vp);

	putc('\n', ErrorFd);
	(void)fflush(ErrorFd);
}
