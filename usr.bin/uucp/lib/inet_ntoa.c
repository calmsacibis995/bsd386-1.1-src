/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Convert IP number to ASCII.
**
**	RCSID $Id: inet_ntoa.c,v 1.1.1.1 1992/09/28 20:08:43 trent Exp $
**
**	$Log: inet_ntoa.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:43  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	TCP_IP

#include	"global.h"


#ifndef	SAFE_LIBC_INET_NTOA
/*
**	The SunOS 4.1.2 C-library version causes SIGSEGV.
*/

char *
inet_ntoa(
	struct in_addr	a
)
{
	static char	numb[16];

	(void)sprintf
		(
			numb, "%d.%d.%d.%d",
			a.S_un.S_un_b.s_b1,
			a.S_un.S_un_b.s_b2,
			a.S_un.S_un_b.s_b3,
			a.S_un.S_un_b.s_b4
		);

	return numb;
}
#endif	/* !SAFE_LIBC_INET_NTOA */
