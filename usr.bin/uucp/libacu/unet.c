/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	RCSID $Id: unet.c,v 1.1.1.1 1992/09/28 20:08:47 trent Exp $
**
**	$Log: unet.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:47  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include "condevs.h"

#ifdef UNETTCP

/*
**	unetopn -- make UNET (tcp-ip) connection
**
**	return codes:
**		>0 - file number - ok
**		FAIL - failed
*/


/* Default port of uucico server */
#define	DFLTPORT	33

CallType
unetopn(flds)
register char *flds[];
{
	register int ret, port;
	int unetcls();

	port = atoi(flds[F_PHONE]);
	if ( port <= 0 || port > 255 )
		port = DFLTPORT;
	Debug((4, "unetopn host %s, ", flds[F_NAME]));
	Debug((4, "port %d", port));
	if ( setjmp(AlarmJmp) )
	{
		Log("TIMEOUT %s", "tcpopen");
		endhnent();	/* see below */
		return CF_DIAL;
	}
	signal(SIGALRM, Timeout);
	alarm(30);
	ret = tcpopen(flds[F_NAME], port, 0, TO_ACTIVE, "rw");
	alarm(0);
	endhnent();	/* wave magic wand at 3com and incant "eat it, bruce" */
	if ( ret < 0 )
	{
		Debug((5, "tcpopen failed: errno %d", errno));
		Log("%s tcpopen", FAILED);
		return CF_DIAL;
	}
	CU_end = unetcls;
	return ret;
}

/*
**	unetcls -- close UNET connection.
*/

CallType
unetcls(fd)
register int fd;
{
	Debug((4, "UNET CLOSE called", 0));
	if ( fd > 0 )
	{
#ifdef notdef
		/* disable this until a timeout is put in */
		if ( ioctl(fd, UIOCCLOSE, STBNULLSTR) )
			Log("%s UNET CLOSE", FAILED);
#endif	/* notdef */
		close(fd);
		Debug((4, "closed fd %d", fd));
	}
}
#endif	/* UNETTCP */
