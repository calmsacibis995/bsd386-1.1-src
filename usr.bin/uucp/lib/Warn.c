/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Warning on ErrorFd
**
**	RCSID $Id: Warn.c,v 1.1.1.1 1992/09/28 20:08:37 trent Exp $
**
**	$Log: Warn.c,v $
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
Warn(va_alist)
	va_dcl
{
	register va_list vp;
	static char	type[]	= english("warning");

	va_start(vp);
	MesgV(type, vp);

	putc('\n', ErrorFd);
	(void)fflush(ErrorFd);

	if ( ErrorLogV(type, vp) )
		ErrorLogN(NULLSTR);

	va_end(vp);
}
