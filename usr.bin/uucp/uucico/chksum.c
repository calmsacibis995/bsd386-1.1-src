/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	`chksum' is new portable version
**	of original published UUCP checksum algorithm.
**
**	RCSID $Id: chksum.c,v 1.1.1.1 1992/09/28 20:08:56 trent Exp $
**
**	$Log: chksum.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:56  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:48:05  piers
 * Initial revision
 *
*/

#include	"global.h"

Uint
chksum(
	register Uchar *s,
	register int	n
)
{
	register Uint	sum;
	register Uint	tmp;
	register Uint	chk;

	sum = 0xffff;
	chk = 0;

	do
	{
		/** Rotate left, copying bit 15 to bit 0 **/
#		if	MAX_UINT == 0xffff
		if ( sum & 0x8000 )
		{
			sum <<= 1;
			sum++;
		}
		else
			sum <<= 1;
#		else	/* MAX_UINT == 0xffff */
		sum <<= 1;
		if ( sum & 0x10000 )
			sum ^= 0x10001;
#		endif	/* MAX_UINT == 0xffff */

		tmp = sum;
		sum += *s++;

#		if	MAX_UINT > 0xffff
		sum &= 0xffff;
#		endif	/* MAX_UINT > 0xffff */

		chk += sum ^ n;
		if ( sum <= tmp )
			sum ^= chk & 0xffff;
	}
	while ( --n > 0 );

	return sum;
}
