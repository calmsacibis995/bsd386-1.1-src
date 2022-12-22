/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Report fatal error on ErrorFd and abort().
**
**	RCSID $Id: Fatal.c,v 1.1.1.1 1992/09/28 20:08:41 trent Exp $
**
**	$Log: Fatal.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:41  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	LOCKING
#define	NO_VA_FUNC_DECLS
#define	STDIO

#include	"global.h"



void
Fatal(va_alist)
	va_dcl
{
	register va_list vp;
	static char	type[]	= english("INTERNAL error");

	va_start(vp);
	FreeStr(&ErrString);
	ErrString = newvprintf(vp);
	MesgV(type, vp);

	putc('\n', ErrorFd);
	(void)fflush(ErrorFd);

	if ( ErrorLogV(type, vp) )
		ErrorLogN(NULLSTR);

	va_end(vp);

	(void)abort();
}
