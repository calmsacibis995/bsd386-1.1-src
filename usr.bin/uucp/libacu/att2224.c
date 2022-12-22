/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	RCSID $Id: att2224.c,v 1.3 1994/01/31 01:25:58 donn Exp $
**
**	$Log: att2224.c,v $
 * Revision 1.3  1994/01/31  01:25:58  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:58:01  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:42:24  vixie
 * Initial revision
 *
 * Revision 1.2  1993/02/28  15:30:46  pace
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

#ifdef	ATT2224

CallType
attopn(telno, flds, dev)
char *telno, *flds[];
Device *dev;
{
	char dcname[PATHNAMESIZE], phone[MAXPH+10], c = 0;
	int dnf, failret = 0, timelim;

	sprintf(dcname, "/dev/%s", dev->D_line);

	if ( setjmp(AlarmJmp) )
	{
		rmlock(dev->D_line);
		Log("NO DEVICE");
		Debug((4, "Open timed out %s", dcname));
		alarm (0);
		return CF_NODEV;
	}

	signal(SIGALRM, Timeout);
	getnextfd();
	alarm(10);

	if ( (dnf = open_dev(dcname)) <= 0 )
	{
		rmlock(dev->D_line);
		Log("NO DEVICE");
		Debug((4, "Can't open %s", dcname));
		alarm (0);
		return CF_NODEV;
	}

	alarm(0);
	NextFd = -1;
	SetupTty(dnf, dev->D_speed);
	Debug((4, "modem port - %s", dcname));

	if ( setjmp(AlarmJmp) )
	{
		rmlock(dev->D_line);
		Log("%s ACU WRITE", FAILED);
		return CF_DIAL;
	}
	signal(SIGALRM, Timeout);
	alarm(10);
	do
	{
		slow_write(dnf, "\r");		/* wake up modem */
	}
	while
		( expect(":~3", dnf) );
	alarm(0);

	sprintf (phone, "atzt%s\r", telno);
	slow_write (dnf, phone);		/* type telno string to modem */

	if ( expect(phone, dnf) != SUCCESS )
	{
		rmlock(dev->D_line);
		Log("%s ACU READ", FAILED);
		return CF_DIAL;
	}

	if ( setjmp(AlarmJmp) )
	{
		rmlock(dev->D_line);
		Log("%s NO ANSWER", FAILED);
		alarm (0);
		return CF_DIAL;
	}
	timelim = strlen(telno) * 4;
	signal(SIGALRM, Timeout);
	alarm(timelim > 30 ? timelim : 30);

readchar:
	if ( (read(dnf, &c, 1)) != 1 )
	{
		rmlock(dev->D_line);
		Log("%s ACU READ", FAILED);
		return CF_DIAL;
	}

	switch ( c )
	{
		case 'D':	/* no dial tone */
			Log("%s NO DIAL TONE", FAILED);
			failret++;
			break;
		case 'B': 	/* line busy */
			Log("%s LINE BUSY", FAILED);
			failret++;
			break;
		case 'N': 	/* no answer */
			Log("%s NO ANSWER", FAILED);
			failret++;
			break;
		case 'H':	/* handshake failed */
			Log("%s MODEM HANDSHAKE", FAILED);
			failret++;
			break;
		case '3':	/* 2400 baud */
			Debug((4, "Baudrate set to 2400 baud"));
			SetupTty(dnf, 2400);
			break;
		case '2':	/* 1200 baud */
			Debug((4, "Baudrate set to 1200 baud"));
			SetupTty(dnf, 1200);
			break;
		case '1':	/* 300 baud */
			Debug((4, "Baudrate set to 300 baud"));
			SetupTty(dnf, 300);
			break;
		default:	/* Not one of the above, so must be garbage */
			goto readchar;
		}
	if ( failret )
	{
		alarm (0);
		rmlock(dev->D_line);
		return CF_DIAL;
	}
	alarm (0);
	return dnf;
}

CallType
attcls(fd)
int fd;
{
	char dcname[PATHNAMESIZE];

	if ( fd > 0 )
	{
		sprintf(dcname, "/dev/%s", devSel);
		Debug((4, "Hanging up fd = %d", fd));
		drop_dtr(fd);
		close_dev(fd);
		rmlock(devSel);
	}
}
#endif	/* ATT2224 */
