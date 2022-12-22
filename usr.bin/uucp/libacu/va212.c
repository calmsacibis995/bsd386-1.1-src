/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	RCSID $Id: va212.c,v 1.3 1994/01/31 01:26:16 donn Exp $
**
**	$Log: va212.c,v $
 * Revision 1.3  1994/01/31  01:26:16  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:58:01  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:42:24  vixie
 * Initial revision
 *
 * Revision 1.2  1993/02/28  15:31:05  pace
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

#ifdef	VA212

CallType
va212opn(telno, flds, dev)
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
		Debug((1, "timeout va212 open"));
		Log("TIMEOUT va212 open");
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
		Debug((4, errno == 4 ? "%s: no carrier" : "%s: can't open", dcname));
		rmlock(dev->D_line);
		return errno == 4 ? CF_DIAL : CF_NODEV;
	}
	SetupTty(dh, dev->D_speed);

	/* translate - to K for Vadic */
	Debug((4, "calling %s -> ", telno));
	delay = 0;
	for ( i = 0 ; i < strlen(telno) ; ++i )
	{
		switch ( telno[i] )
		{
		case '=':	/* await dial tone */
		case '-':	/* delay */
		case '<':
			telno[i] = 'K';
			delay += 5;
			break;
		}
	}
	Debug((4, "%s", telno));
	for ( i = 0 ; i < TRYCALLS ; ++i ) {	/* make TRYCALLS tries */
		/* wake up Vadic */
		sendthem("\005\\d", dh);
		ok = expect("*", dh);
		if ( ok != 0 )
			continue;

		sendthem("D\\d", dh);	/* "D" (enter number) command */
		ok = expect("NUMBER?", dh);
		if ( ok == 0 )
			ok = expect("\n", dh);
		if ( ok != 0 )
			continue;

		/* send telno, send \r */
		sendthem(telno, dh);
		ok = expect(telno, dh);
		if ( ok == 0 )
			ok = expect("\n", dh);
		if ( ok != 0 )
			continue;

		sendthem(EmptyStr, dh); /* confirm number */
		ok = expect("DIALING...", dh);
		if ( ok == 0 )
		{
			ok = expect("\n", dh);
			ok = expect("ANSWER TONE", dh);
			if ( ok == 0 )
				ok = expect("\n", dh);
		}
		if ( ok == 0 )
			break;
	}

	if ( ok == 0 )
		ok = expect("ON LINE\r\n", dh);

	if ( ok != 0 )
	{
		sendthem("I\\d", dh);	/* back to idle */
		if ( dh > 2 )
			close_dev(dh);
		Debug((4, "vadDial failed"));
		rmlock(dev->D_line);
		return CF_DIAL;
	}
	Debug((4, "va212 ok"));
	return dh;
}

CallType
va212cls(fd)
{
	if ( fd > 0 )
	{
		close_dev(fd);
		sleep(5);
		rmlock(devSel);
	}
}
#endif	/* VA212 */
