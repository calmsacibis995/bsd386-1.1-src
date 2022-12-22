/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Extract a number from a field encoded by EncodeNum.
**
**	NB: result is undefined if field not produced by EncodeNum.
**
**	RCSID $Id: DecodeNum.c,v 1.1.1.1 1992/09/28 20:08:40 trent Exp $
**
**	$Log: DecodeNum.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:40  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include	"global.h"



Ulong
DecodeNum(name, length)
	register char *	name;
	register int	length;
{
	register Ulong	number = 0;
	register char *	cp;

	if ( length > 0 )
	for ( ;; )
	{
		if ( (cp = strchr(ValidChars, *name++)) != NULLSTR )
			number += cp - ValidChars;

		if ( --length <= 0 )
			break;

		number *= VC_size;
	}

	return number;
}
