/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	RCSID $Id: pnet.c,v 1.1.1.1 1992/09/28 20:08:47 trent Exp $
**
**	$Log: pnet.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:47  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include "condevs.h"

#ifdef	PNET

/***
**	pnetopn(flds)
**
**	call remote machine via Purdue network
**	use dial string as host name, speed as socket number
**	- Steve Bellovin
*/

CallType
pnetopn(flds)
char *flds[];
{
	int fd;
	int socket;
	register char *cp;

	fd = pnetfile();
	Debug((4, "pnet fd - %d", fd));
	if ( fd < 0 )
	{
		Log("NO %s", "AVAILABLE DEVICE");
		return CF_NODEV;
	}
	socket = 0;
	for ( cp = flds[F_CLASS] ; *cp ; cp++ )
		socket = 10*socket + (*cp - '0');
	Debug((4, "socket - %d", socket));
	if ( setjmp(AlarmJmp) )
	{
		Debug((4, "pnet timeout  - %s", flds[F_PHONE]));
		return CF_DIAL;
	}
	signal(SIGALRM, Timeout);
	Debug((4, "host - %s", flds[F_PHONE]));
	alarm(15);
	if ( pnetscon(fd, flds[F_PHONE], socket) < 0 )
	{
		Debug((4, "pnet connect failed - %s", flds[F_PHONE]));
		alarm(0);
		return CF_DIAL;
	}
	alarm(0);
	return fd;
}
#endif	/* PNET */
