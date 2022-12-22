/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	RCSID $Id: bsdtcp.c,v 1.1.1.1 1992/09/28 20:08:45 trent Exp $
**
**	$Log: bsdtcp.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:45  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.2  1992/09/10  14:50:58  ziegast
 * Added condition for 4.4BSD connect (one fewer argument)
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	TCP_IP

#include "condevs.h"

#ifdef	BSDTCP

#ifndef	MAXHOSTNAMELEN
#define	MAXHOSTNAMELEN 64
#endif	/* MAXHOSTNAMELEN */

/*
**	bsdtcpcls -- close tcp connection
*/

CallType
bsdtcpcls(
	int	fd
)
{
	Debug((4, "TCP CLOSE called"));

	if ( fd == SYSERROR )
		return;

	(void)close(fd);

	Debug((4, "closed fd %d", fd));
}

/*
**	bsdtcpopn -- make a tcp connection
**
**	return codes:
**		>0 - file number - ok
**		FAIL - failed
*/

CallType
bsdtcpopn(
	register char *		flds[]
)
{
	struct servent *	sp;
	struct hostent *	hp;
	struct sockaddr_in	hisctladdr;
	Ushort			port;

	static int		s;
	static char		terr[]	= "%s() failed: errno %d";

	s = SYSERROR;

	if ( (sp = getservbyname(flds[F_CLASS], "tcp")) == (struct servent *)0 )
	{
		if ( (port = htons(atoi(flds[F_CLASS]))) == 0 )
		{
			(void)strcpy(FailMsg, "UNKNOWN PORT NUMBER");
			Log("%s %s", FailMsg, flds[F_CLASS]);
			return CF_SYSTEM;
		}
	}
	else
		port = sp->s_port;

	Debug((4, "bsdtcpopn host %s, service %s, port %d", flds[F_PHONE], flds[F_CLASS], ntohs(port)));

	bzero((char *)&hisctladdr, sizeof(hisctladdr));

	if ( (hp = gethostbyname(flds[F_PHONE])) == (struct hostent *)0 )
	{
		(void)strcpy(FailMsg, "UNKNOWN HOST");
		Log("%s %s", FailMsg, flds[F_PHONE]);
		return CF_DIAL;
	}

	FreeStr(&IPHostName);
	IPHostName = newnstr(hp->h_name, MAXHOSTNAMELEN);

	if ( setjmp(AlarmJmp) )
	{
		(void)strcpy(FailMsg, "TIMEOUT");
		Log("%s tcpopen", FailMsg);
		bsdtcpcls(s);
		return CF_DIAL;
	}

	(void)signal(SIGALRM, Timeout);
	(void)alarm(MAXMSGTIME*2);

	for ( ;; )
	{
		hisctladdr.sin_family = hp->h_addrtype;

		if ( (s = socket(hp->h_addrtype, SOCK_STREAM, 0)) == SYSERROR )
		{
			Debug((5, terr, "socket", errno));
			break;
		}

		bcopy(hp->h_addr_list[0], (char *)&hisctladdr.sin_addr, hp->h_length);
		hisctladdr.sin_port = port;
#	if	BSD4 >= 4
		if ( connect(s, (struct sockaddr *)&hisctladdr, sizeof (hisctladdr)) == SYSERROR )
#	else	/* BSD4 < 4 */
		if ( connect(s, (struct sockaddr *)&hisctladdr, sizeof (hisctladdr), 0) == SYSERROR )
#	endif	/* BSD4 >= 4 */
		{
			Debug((5, terr, "connect", errno));
			break;
		}

		(void)alarm(0);
		CU_end = bsdtcpcls;
		return s;
	}

	(void)alarm(0);
	(void)strcpy(FailMsg, ErrnoStr(errno));
	Debug((2, "tcpopen failed: %s", FailMsg));
	Log("%s %s", FAILED, FailMsg);
	bsdtcpcls(s);
	return CF_DIAL;
}
#endif	/* BSDTCP */
