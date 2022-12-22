/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Return pointer to newly allocated string containing 's'.
**
**	RCSID $Id: newstr.c,v 1.1.1.1 1992/09/28 20:08:49 trent Exp $
**
**	$Log: newstr.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:49  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:48:05  piers
 * Initial revision
 *
*/

#include	"global.h"

#if	0
char		EmptyStr[]	= "";
#endif	/* 0 */

#ifndef	NULLSTR
#define	NULLSTR	(char *)0
#endif	/* NULLSTR */

char *
newstr(s)
	char *	s;
{
	if ( s == NULLSTR )
		s = EmptyStr;

	return strcpy(Malloc(strlen(s)+1), s);
}
