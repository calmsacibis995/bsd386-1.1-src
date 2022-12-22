/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	RCSID $Id: cds224.c,v 1.3 1994/01/31 01:25:59 donn Exp $
**
**	$Log: cds224.c,v $
 * Revision 1.3  1994/01/31  01:25:59  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:58:01  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:42:24  vixie
 * Initial revision
 *
 * Revision 1.2  1993/02/28  15:30:47  pace
 * Add hasv dialer; change dialers to do non-blocking open when appropriate
 *
 * Revision 1.1.1.1  1992/09/28  20:08:45  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include "condevs.h"

#ifdef	CDS224

/*
**	conopn: establish dial-out connection through a Concord CDS 224.
**	Returns descriptor open to tty for reading and writing.
**	Negative values (-1...-7) denote errors in connmsg.
**	Be sure to disconnect tty when done, via HUPCL or stty 0.
*/

#define TRYS 5	/* number of trys */

CallType
cdscls224(fd)
{
	if ( fd > 0 )
	{
		close_dev(fd);
		sleep(5);
		rmlock(devSel);
	}
}

CallType
cdsopn224(telno, flds, dev)
char *telno;
char *flds[];
Device *dev;
{
	int	dh = -1;
	int	i, ok, er = 0, delay;
	char	dcname[PATHNAMESIZE];

	sprintf(dcname, "/dev/%s", dev->D_line);
	if ( setjmp(AlarmJmp) )
	{
		Debug((1, "timeout concord open"));
		Log("TIMEOUT concord open");
		if ( dh >= 0 )
			cdscls224(dh);
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
		cdscls224(dh);
		return CF_CHAT;
	}

	for ( i = 0 ; i < TRYS ; ++i )
	{
		/* wake up Concord */
		(void)write(dh, "\r\r", 2);
		if ( (ok = expect("CDS >", dh)) != SUCCESS )
			continue;

		(void)write(dh, "\r", 2);
		if ( (ok = expect("CDS >", dh)) != SUCCESS )
			continue;

		/* send telno \r */
		(void)sprintf(dcname,"D%s\r",telno);
		(void)write(dh, dcname, strlen(dcname));

		if ( (ok = expect("DIALING ", dh)) == SUCCESS )
			break;
	}

	if ( ok == SUCCESS )
	{
		sleep(10);	/* give concord some time */
		ok = expect("INITIATING", dh);
	}

	if ( ok != SUCCESS )
	{
		if ( dh > 2 )
			close_dev(dh);
		Debug((4, "conDial failed"));
		rmlock(dev->D_line);
		return CF_DIAL;
	}
	Debug((4, "concord ok"));
	return dh;
}
#endif	/* CDS224 */
