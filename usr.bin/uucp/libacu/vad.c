/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	RCSID $Id: vad.c,v 1.3 1994/01/31 01:26:18 donn Exp $
**
**	$Log: vad.c,v $
 * Revision 1.3  1994/01/31  01:26:18  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:58:01  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:42:24  vixie
 * Initial revision
 *
 * Revision 1.2  1993/02/28  15:31:09  pace
 * Add hasv dialer; change dialers to do non-blocking open when appropriate
 *
 * Revision 1.1.1.1  1992/09/28  20:08:48  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include "condevs.h"

#ifdef	VADIC

/*
**	vadopn: establish dial-out connection through a Racal-Vadic 3450.
**	Returns descriptor open to tty for reading and writing.
**	Negative values (-1...-7) denote errors in connmsg.
**	Be sure to disconnect tty when done, via HUPCL or stty 0.
*/


CallType
vadopn(telno, flds, dev)
char *telno;
char *flds[];
Device *dev;
{
	int	dh = -1;
	int	i, ok, er = 0, delay;
	extern errno;
	char dcname[PATHNAMESIZE];

	sprintf(dcname, "/dev/%s", dev->D_line);
	if ( setjmp(AlarmJmp) )
	{
		Debug((1, "timeout vadic open"));
		Log("TIMEOUT vadic open");
		if ( dh >= 0 )
			close_dev(dh);
		rmlock(dev->D_line);
		return CF_NODEV;
	}
	signal(SIGALRM, Timeout);
	getnextfd();
	alarm(10);
	dh = open_dev(dcname);
	alarm(0);

	/* modem is open */
	NextFd = -1;
	if ( dh < 0 )
	{
		rmlock(dev->D_line);
		return CF_NODEV;
	}
	SetupTty(dh, dev->D_speed);

	Debug((4, "calling %s -> ", telno));
	if ( dochat(dev, flds, dh) )
	{
		close_dev(dh);
		return CF_CHAT;
	}
	delay = 0;
	for ( i = 0 ; i < strlen(telno) ; ++i )
	{
		switch ( telno[i] )
		{
		case '=':	/* await dial tone */
		case '-':
		case ',':
		case '<':
		case 'K':
			telno[i] = 'K';
			delay += 5;
			break;
		}
	}
	Debug((4, "%s", telno));
	for ( i = 0 ; i < 5 ; ++i ) {	/* make 5 tries */
		/* wake up Vadic */
		write(dh, "\005", 1);
		sleep(1);
		write(dh, "\r", 1);
		ok = expect("*~5", dh);
		if ( ok != 0 )
			continue;

		write(dh, "D\r", 2); /* "D" (enter number) command */
		ok = expect("NUMBER?\r\n~5", dh);
		if ( ok != 0 )
			continue;

		/* send telno, send \r */
		write(dh, telno, strlen(telno));
		sleep(1);
		write(dh, "\r", 1);
		ok = expect(telno, dh);
		if ( ok == 0 )
			ok = expect("\r\n", dh);
		if ( ok != 0 )
			continue;

		write(dh, "\r", 1); /* confirm number */
		ok = expect("DIALING: ", dh);
		if ( ok == 0 )
			break;
	}

	if ( ok == 0 )
	{
		sleep(10 + delay);	/* give vadic some time */
		ok = expect("ON LINE\r\n", dh);
	}

	if ( ok != 0 )
	{
		if ( dh > 2 )
			close_dev(dh);
		Debug((4, "vadDial failed"));
		rmlock(dev->D_line);
		return CF_DIAL;
	}
	Debug((4, "vadic ok"));
	return dh;
}

CallType
vadcls(fd)
{
	if ( fd > 0 )
	{
		close_dev(fd);
		sleep(5);
		rmlock(devSel);
	}
}
#endif	/* VADIC */
