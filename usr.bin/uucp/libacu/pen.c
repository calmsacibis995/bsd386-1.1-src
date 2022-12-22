/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	RCSID $Id: pen.c,v 1.3 1994/01/31 01:26:13 donn Exp $
**
**	$Log: pen.c,v $
 * Revision 1.3  1994/01/31  01:26:13  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:58:01  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:42:24  vixie
 * Initial revision
 *
 * Revision 1.2  1993/02/28  15:31:01  pace
 * Add hasv dialer; change dialers to do non-blocking open when appropriate
 *
 * Revision 1.1.1.1  1992/09/28  20:08:47  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

/*
**	Speaker's quick and dirty penril hack.  STA 4/1/85.
*/

#include "condevs.h"

#ifdef PENRIL

CallType
penopn(telno, flds, dev)
char *flds[], *telno;
Device *dev;
{
	int	dh;
	int	i, ok = -1;
	char dcname[PATHNAMESIZE];

	sprintf(dcname, "/dev/%s", dev->D_line);
	if ( setjmp(AlarmJmp) )
	{
		Debug((1, "timeout penril open"));
		Log("TIMEOUT penril open");
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
	NextFd = -1;
	if ( dh < 0 )
	{
		Debug((4,"%s", errno == 4 ? "no carrier" : "can't open modem"));
		rmlock(dev->D_line);
		return errno == 4 ? CF_DIAL : CF_NODEV;
	}

	/* modem is open */
	SetupTty(dh, dev->D_speed);

	/* translate - to P and = to W for Penril */
	Debug((4, "calling %s -> ", telno));
	for ( i = 0 ; i < strlen(telno) ; ++i )
	{
		switch ( telno[i] )
		{
		case '-':	/* delay */
			telno[i] = 'P';
			break;
		case '=':	/* await dial tone */
			telno[i] = 'W';
			break;
		case '<':
			telno[i] = 'P';
			break;
		}
	}
	Debug((4, "%s", telno));
	sleep(1);
	for ( i = 0 ; i < 5 ; ++i ) {	/* make up to 5 tries */
		slow_write(dh, "\r");/* awake, thou lowly Penril! */

		ok = expect(">", dh);
		if ( ok != 0 )
			continue;
		slow_write(dh, "K");	/* "K" (enter number) command */
		ok = expect("NO.: ", dh);
		if ( ok == 0 )
			break;
	}

	if ( ok == 0 )
	{
		slow_write(dh, telno); /* send telno, send \r */
		slow_write(dh, "\r");
		ok = expect("OK", dh);
	}
	if ( ok != 0 )
	{
		if ( dh > 2 )
			close_dev(dh);
		Debug((4, "penDial failed"));
		return CF_DIAL;
	}
	else
		Debug((4, "penDial ok"));
	return dh;
}

CallType
pencls(fd)
int fd;
{
	if ( fd > 0 )
	{
		close_dev(fd);
		sleep(5);
		rmlock(devSel);
	}
}
#endif	/* PENRIL */
