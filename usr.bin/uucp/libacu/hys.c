/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	RCSID $Id: hys.c,v 1.3 1994/01/31 01:26:06 donn Exp $
**
**	$Log: hys.c,v $
 * Revision 1.3  1994/01/31  01:26:06  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:58:01  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:42:24  vixie
 * Initial revision
 *
 * Revision 1.2  1993/02/28  15:30:52  pace
 * Add hasv dialer; change dialers to do non-blocking open when appropriate
 *
 * Revision 1.1.1.1  1992/09/28  20:08:46  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.3  1992/09/10  14:54:48  ziegast
 * Unnecessary return deleted (per donn@bsdi.com)
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include "condevs.h"

#ifdef	HAYES

#ifdef	USR2400
#define DROPDTR
/*
**	The "correct" switch settings for a USR Courier 2400 are:
**		Dialin/out:	0 0 1 1 0 0 0 1 0 0
**		Dialout only:	0 0 1 1 1 1 0 1 0 0
**	where 0 = off and 1 = on
*/
#endif	/* USR2400 */

extern CallType	hyscls(int);
extern CallType	hysopn(char *, char **, Device *, int);

static void	SendMsg(int, char *);

/*
**	hyspopn(telno, flds, dev) connect to hayes smartmodem (pulse call)
**	hystopn(telno, flds, dev) connect to hayes smartmodem (tone call)
**	char *flds[], *dev[];
**
**	return codes:
**		>0  -  file number  -  ok
**		CF_DIAL,CF_DEVICE  -  failed
*/

CallType
hyspopn(
	char *	telno,
	char *	flds[],
	Device *dev
)
{
	return hysopn(telno, flds, dev, 0);
}

CallType
hystopn(
	char *	telno,
	char *	flds[],
	Device *dev
)
{
	return hysopn(telno, flds, dev, 1);
}

/* ARGSUSED */
CallType
hysopn(
	char *		telno,
	char *		flds[],
	Device *	dev,
	int		toneflag
)
{
	register char *	cp;
	int		fd		= -1;
	int		nrings		= 0;
	char		dcname[PATHNAMESIZE];
	char		cbuf[MAXPH];

	(void)sprintf(dcname, "/dev/%s", dev->D_line);

	Debug((4, "dc - %s", dcname));

	(void)strcpy(devSel, dev->D_line);

	if ( setjmp(AlarmJmp) )
	{
timeout:	Log("TIMEOUT %s", dcname);
		(void)hyscls(fd);
		return CF_DIAL;
	}

	(void)signal(SIGALRM, Timeout);

	getnextfd();

	(void)alarm(10);
	fd = open_dev(dcname);
	(void)alarm(0);

	NextFd = -1;

	if ( fd < 0 )
	{
		Log("CAN'T OPEN %s", dcname);
		rmlock(devSel);
		return CF_DIAL;
	}

	/** Modem is open **/

	if ( !SetupTty(fd, dev->D_speed) )
		return CF_DIAL;

	if ( dochat(dev, flds, fd) )
	{
		(void)hyscls(fd);
		return CF_CHAT;
	}

	SendMsg(fd, "ATV1E0\r");

	if ( expect("OK\r\n", fd) != 0 )
	{
		Log("modem seems dead %s", dcname);
		(void)hyscls(fd);
		return CF_DIAL;
	}

	SendMsg(fd, "ATH\r");

	(void)expect("OK\r\n", fd);

#	ifdef	USR2400
	SendMsg(fd, "ATX6S7=44\r");

	if ( expect("OK\r\n", fd) != 0 )
	{
		Log("modem seems dead %s", dcname);
		(void)hyscls(fd);
		return CF_DIAL;
	}
#	endif	/* USR2400 */

	if ( toneflag )
		SendMsg(fd, "ATDT");
	else
#	ifdef	USR2400
		SendMsg(fd, "\rATD");
#	else	/* USR2400 */
		SendMsg(fd, "ATDP");
#	endif	/* USR2400 */

	SendMsg(fd, telno);
	SendMsg(fd, "\r");

	if ( setjmp(AlarmJmp) )	/* Because expect() uses AlarmJmp */
		goto timeout;

	(void)signal(SIGALRM, Timeout);
	(void)alarm(3*MAXMSGTIME);

	do
	{
		cp = cbuf;

		while ( read(fd, cp ,1) == 1 )
			if ( *cp >= ' ' )
				break;

		while
		(
			++cp < &cbuf[MAXPH]
			&&
			read(fd, cp, 1) == 1
			&&
			*cp != '\n'
		)
			;

		*cp-- = '\0';
		if ( *cp == '\r' )
			*cp = '\0';

		Debug((4,"GOT: %s", cbuf));

		(void)alarm(3*MAXMSGTIME);
	} while
		(
			(
				strncmp(cbuf, "RING", 4) == STREQUAL
				||
				strncmp(cbuf, "RRING", 5) == STREQUAL
			)
			&&
			nrings++ < 5
		);

	(void)alarm(0);

	if ( strncmp(cbuf, "CONNECT", 7) != STREQUAL )
	{
		cp = strcpyend(FailMsg, cbuf);
		*cp++ = ' ';
		(void)strcpy(cp, dev->D_line);
		Log("%s %s", FAILED, FailMsg);
		(void)hyscls(fd);
		return CF_DIAL;
	}

#define	DONTRESETBAUDRATE
#ifndef	DONTRESETBAUDRATE
	i = atoi(&cbuf[8]);
	if ( i > 0 && i != dev->D_speed )
	{
		Debug((4,"Baudrate reset to %d", i));
		SetupTty(fd, i);
	}
#endif	/* DONTRESETBAUDRATE */

	Debug((4, "hayes ok"));
	return fd;
}

CallType
hyscls(
	int	fd
)
{
	char	dcname[PATHNAMESIZE];

	if ( fd < 0 )
	{
		rmlock(devSel);
		return;
	}

	(void)sprintf(dcname, "/dev/%s", devSel);
	Debug((4, "Hanging up fd = %d", fd));

#	ifdef	DROPDTR

	drop_dtr(fd);

#	else	/* DROPDTR */

	(void)sleep(3);
	SendMsg(fd, "+++");

#	endif	/* DROPDTR */

	(void)sleep(3);
	SendMsg(fd, "ATH\r");

	(void)sleep(1);
	SendMsg(fd, "ATZ\r");

	if ( expect("OK\r\n", fd) != 0 )
		Log("could not reset modem %s on close", dcname);

	(void)close_dev(fd);

	rmlock(devSel);
}

static void
SendMsg(
	int	fd,
	char *	str
)
{
	(void)write(fd, str, strlen(str));
	Debug((4, "sent \"%s\"", ExpandString(str, -1)));
}
#endif	/* HAYES */
