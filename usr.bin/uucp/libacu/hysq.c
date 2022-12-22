/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	RCSID $Id: hysq.c,v 1.3 1994/01/31 01:26:08 donn Exp $
**
**	$Log: hysq.c,v $
 * Revision 1.3  1994/01/31  01:26:08  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:58:01  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:42:24  vixie
 * Initial revision
 *
 * Revision 1.2  1993/02/28  15:30:55  pace
 * Add hasv dialer; change dialers to do non-blocking open when appropriate
 *
 * Revision 1.1.1.1  1992/09/28  20:08:46  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include "condevs.h"

#ifdef HAYESQ
/*
**	New dialout routine to work with Hayes' SMART MODEM
**	13-JUL-82, Mike Mitchell
**	Modified 23-MAR-83 to work with Tom Truscott's (rti!trt)
**	version of UUCP	(ncsu!mcm)
**
**	The modem should be set to NOT send any result codes to
**	the system ( switch 3 up, 4 down ). This end will figure out
**	what is wrong.
**
**	I had lots of problems with the modem sending
**	result codes since I am using the same modem for both incomming and
**	outgoing calls.  I'd occasionally miss the result code (getty would
**	grab it), and the connect would fail.  Worse yet, the getty would
**	think the result code was a user name, and send garbage to it while
**	it was in the command state.  I turned off ALL result codes, and hope
**	for the best.  99% of the time the modem is in the correct state.
**	Occassionally it doesn't connect, or the phone was busy, etc., and
**	uucico sits there trying to log in.  It eventually times out, calling
**	clsacu() in the process, so it resets itself for the next attempt.
*/

/*
**	NOTE: this version is not for the faint-hearted.
**	Someday it would be nice to have a single version of hayes dialer
**	with a way to specify the switch settings that are on the dialer
**	as well as tone/pulse.
**	In the meantime, using HAYES rather than HAYESQ is probably best.
*/

static CallType	hysqopn(char *, char **, Device *, int);

CallType
hysqpopn(telno, flds, dev)
char *telno, *flds[];
Device *dev;
{
	return hysqopn(telno, flds, dev, 0);
}

CallType
hysqtopn(telno, flds, dev)
char *telno, *flds[];
Device *dev;
{
	return hysqopn(telno, flds, dev, 1);
}

static CallType
hysqopn(telno, flds, dev, toneflag)
char *telno, *flds[];
Device *dev;
int toneflag;
{
	char dcname[PATHNAMESIZE], phone[MAXPH+10], c = 0;
	int status, dnf;
	unsigned timelim;

	signal(SIGALRM, Timeout);
	sprintf(dcname, "/dev/%s", dev->D_line);

	getnextfd();
	if ( setjmp(AlarmJmp) )
	{
		Log("NO DEVICE");
		Debug((4, "Open timed out %s", dcname));
		return CF_NODEV;
	}
	alarm(10);

	if ( (dnf = open_dev(dcname)) <= 0 )
	{
		Log("NO DEVICE");
		Debug((4, "Can't open %s", dcname));
		return CF_NODEV;
	}

	alarm(0);
	NextFd = -1;
	SetupTty(dnf, dev->D_speed);
	Debug((4, "Hayes port - %s, ", dcname));

	if ( toneflag )
		sprintf(phone, "\rATDT%s\r", telno);
	else
		sprintf(phone, "\rATDP%s\r", telno);

	write(dnf, phone, strlen(phone));

	/* calculate delay time for the other system to answer the phone.
	 * Default is 15 seconds, add 2 seconds for each comma in the phone
	 * number.
	 */
	timelim = 150;
	while ( *telno )
	{
		c = *telno++;
		if ( c == ',' )
			timelim += 20;
		else if ( toneflag )
			timelim += 2;	/* .2 seconds per tone */
		else {
			if ( c == '0' ) timelim += 10;   /* .1 sec per digit */
			else if ( c > '0' && c <= '9' )
				timelim += (c - '0');
		}
	}
	alarm(timelim/10 + 1);
	if ( setjmp(AlarmJmp) == 0 )
	{
		read(dnf, &c, 1);
		alarm(0);
	}

	return dnf;
}

CallType
hysqcls(fd)
int fd;
{
	char dcname[PATHNAMESIZE];

	if ( fd > 0 )
	{
		sprintf(dcname, "/dev/%s", devSel);
		Debug((4, "Hanging up fd = %d", fd));
		drop_dtr(fd);
		sleep(2);
		write(fd, "\rATZ\r", 5);
		close_dev(fd);
		rmlock(devSel);
	}
}
#endif	/* HAYESQ */
