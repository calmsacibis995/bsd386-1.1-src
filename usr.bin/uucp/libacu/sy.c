/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	RCSID $Id: sy.c,v 1.3 1994/01/31 01:26:14 donn Exp $
**
**	$Log: sy.c,v $
 * Revision 1.3  1994/01/31  01:26:14  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:58:01  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:42:24  vixie
 * Initial revision
 *
 * Revision 1.2  1993/02/28  15:31:04  pace
 * Add hasv dialer; change dialers to do non-blocking open when appropriate
 *
 * Revision 1.1.1.1  1992/09/28  20:08:47  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include "condevs.h"

#ifdef SYTEK

/*
**	sykopn: establish connection through a sytek port.
**	Returns descriptor open to tty for reading and writing.
**	Negative values (-1...-7) denote errors in connmsg.
**	Will try to change baud rate of local port to match
**	that of the remote side.
*/

char	sykspeed[50];	/* speed to reset to on close */

CallType
sykopn(
	register char *	flds[]
)
{
	int		dh;
	int		ok = 0;
	int		speed;
	Device		dev;
	char		dcname[PATHNAMESIZE];
	char		speedbuf[50];

	if ( (data = ReadFile(DEVFILE)) == NULLSTRSTR )
	{
		SysError(CouldNot, ReadStr, DEVFILE);
		finish(EX_OSERR);
	}

	bufp = data;

	signal(SIGALRM, Timeout);
	dh = -1;
	while ( NextDevice(&bufp, &dev) )
	{
/*		Ignore speed.
**
**		if ( strcmp(flds[F_CLASS], dev.D_class) != STREQUAL )
**			continue;
*/
		if ( strccmp(flds[F_LINE], dev.D_type) != STREQUAL )
			continue;
		if ( !mklock(dev.D_line) )
			continue;

		sprintf(dcname, "/dev/%s", dev.D_line);
		getnextfd();
		alarm(10);
		if ( setjmp(AlarmJmp) )
		{
			rmlock(dev.D_line);
			Log("sytek open TIMEOUT %s", dev.D_line);
			dh = -1;
			break;
		}
		dh = open_dev(dcname);
		alarm(0);
		NextFd = -1;
		if ( dh > 0 )
		{
			break;
		}
		devSel[0] = '\0';
		rmlock(dev.D_line);
	}

	free(data);

	if ( dh < 0 )
		return(CF_NODEV);

	speed = atoi(fdig(dev.D_class));
	SetupTty(dh, speed);
	sleep(1);

	/* negotiate with sytek */
	SendBRK(dh, 3);

	ok = expect("#", dh);
	if ( ok != 0 )
	{
		if ( atoi(fdig(dev.D_class)) == 9600 )
		{
			SetupTty(dh, 2400);
			speed = 2400;
		}
		else
		{
			SetupTty(dh, 9600);
			speed = 9600;
		}
		sleep(1);
		SendBRK(dh, 3);
		ok = expect("#", dh);
		if ( ok )
		{
			close_dev(dh);
			Debug((4, "sytek BREAK failed"));
			rmlock(dev.D_line);
			return(CF_DIAL);
		}
	}
	write(dh, "done \r", 6);
	ok = expect("#", dh);
	if ( speed != atoi(fdig(flds[F_CLASS])) )
	{
		Debug((4, "changing speed"));
		sprintf(speedbuf, "baud %s\r", fdig(flds[F_CLASS]));
		write(dh, speedbuf, strlen(speedbuf));
		sleep(1);
		speed = atoi(fdig(flds[F_CLASS]));
		SetupTty(dh, speed);
		SendBRK(dh, 3);
		ok = expect("#", dh);
		Debug((4, "speed set %s", ok ? "failed" : flds[F_CLASS]));
	}
	strcpy(sykspeed, dev.D_class);
	write(dh, "command break\r", 14);
	ok = expect("#", dh);
	if ( ok == 0 )
	{
		Debug((4, "sytek dial %s", flds[F_PHONE]));
		write(dh, "call ", 5);
		write(dh, flds[F_PHONE], strlen(flds[F_PHONE]));
		write(dh, "\r", 1);
		ok = expect("COMPLETED TO ", dh);
	}

	if ( ok != 0 )
	{
		close_dev(dh);
		Debug((4, "sytek failed"));
		rmlock(dev.D_line);
		return(CF_DIAL);
	} else
		Debug((4, "sytek ok"));

	CU_end = sykcls;
	strcpy(devSel, dev.D_line);	/* for later unlock */
	return(dh);

}

CallType
sykcls(fd)
register int fd;
{
	register int ok, speed;


	if ( fd > 0 )
	{
		SendBRK(fd, 3);
		ok = expect("#", fd);
		if ( ok != 0 )
		{
			SendBRK(fd, 3);
			ok = expect("#", fd);
		}
		if ( ok == 0 )
		{
			write(fd, "done 1\r", 7);
			ok = expect("#", fd);
			Debug((4, "reset baud to %s", sykspeed));
			write(fd, "baud ", 5);
			write(fd, sykspeed, strlen(sykspeed));
			write(fd, "\r", 1);
			sleep(1);
		}
		close_dev(fd);
		rmlock(devSel);
	}
}
#endif	/* SYTEK */
