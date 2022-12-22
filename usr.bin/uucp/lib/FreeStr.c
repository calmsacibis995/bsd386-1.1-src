/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Free a string.
**
**	RCSID $Id: FreeStr.c,v 1.1.1.1 1992/09/28 20:08:38 trent Exp $
**
**	$Log: FreeStr.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:38  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/


#include	"global.h"



void
FreeStr(cpp)
	register char **cpp;
{
	register char *	cp;

	if ( (cp = *cpp) != NULLSTR )
	{
		*cpp = NULLSTR;
		free(cp);
	}
}
