/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Clear guaranteed memory.
**
**	RCSID $Id: Malloc0.c,v 1.1.1.1 1992/09/28 20:08:40 trent Exp $
**
**	$Log: Malloc0.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:40  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include	"global.h"

char *
Malloc0(size)
	int		size;
{
	return strclr(Malloc(size), size);
}
