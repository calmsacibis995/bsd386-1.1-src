/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	RCSID $Id: hys24.c,v 1.3 1994/01/31 01:26:07 donn Exp $
**
**	$Log: hys24.c,v $
 * Revision 1.3  1994/01/31  01:26:07  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:58:01  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:42:24  vixie
 * Initial revision
 *
 * Revision 1.2  1993/02/28  15:30:54  pace
 * Add hasv dialer; change dialers to do non-blocking open when appropriate
 *
 * Revision 1.1.1.1  1992/09/28  20:08:46  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	FILE_CONTROL

#include "condevs.h"

#ifdef	HAYES2400


extern CallType	hysopn24(char *, char **, Device *, int);
extern CallType	hyscls24(int, int);

/*
**	hyspopn24(telno, flds, dev) connect to hayes smartmodem (pulse call)
**	hystopn24(telno, flds, dev) connect to hayes smartmodem (tone call)
**
**	return codes: >0  -  file number  -  ok CF_DIAL,CF_DEVICE  -  failed
*/

CallType
hyspopn24(telno, flds, dev)
char *telno, *flds[];
Device *dev;
{
	return hysopn24(telno, flds, dev, 0);
}

CallType
hystopn24(telno, flds, dev)
char *telno, *flds[];
Device *dev;
{
	return hysopn24(telno, flds, dev, 1);
}

CallType
hysopn24(telno, flds, dev, toneflag)
char *telno;
char *flds[];
Device *dev;
int toneflag;
{
	int dh = -1;
	int result, ix, speed;
	char *ii;
	extern errno;
	char dcname[PATHNAMESIZE];
	char resultbuf[16];

	sprintf(dcname, "/dev/%s", dev->D_line);
	Debug((4, "dc - %s", dcname));
	if ( setjmp(AlarmJmp) )
	{
		Log("TIMEOUT %s", dcname);
		if ( dh >= 0 )
			hyscls24(dh, 0);
		return CF_DIAL;
	}
	signal(SIGALRM, Timeout);
	getnextfd();
	alarm(10);
	dh = open_dev(dcname);
	alarm(0);

	for ( ii = telno ; *ii ; ii++ )
		if ( *ii == '=' )
			*ii = ',';

	/* modem is open */
	NextFd = -1;
	if ( dh >= 0 )
	{
		ioctl(dh, TIOCHPCL, 0);
		SetupTty(dh, dev->D_speed);
		if ( dochat(dev, flds, dh) )
		{
			hyscls24(dh, 0);
			return CF_CHAT;
		}
		hyscls24(dh, 1);/* make sure the line is reset */
		write(dh, "AT&F&D3&C1E0M0X3QV0Y\r", 21);
		if ( expect("0\r", dh) != 0 )
		{
			Log("HSM not responding OK %s", dcname);
			hyscls24(dh, 0);
			return CF_DIAL;
		}
		if ( toneflag )
			write(dh, "\rATDT", 5);
		else
			write(dh, "\rATDP", 5);
		write(dh, telno, strlen(telno));
		write(dh, "\r", 1);

		if ( setjmp(AlarmJmp) )
		{
			Log("Modem Hung %s", dcname);
			if ( dh >= 0 )
				hyscls24(dh, 0);
			return CF_DIAL;
		}
		signal(SIGALRM, Timeout);
		alarm(120);
		do {
			for ( ix = 0 ; ix < 16 ; ix++ )
			{
				read(dh, resultbuf + ix, 1);
				Debug((6, "character read = 0x%X ", resultbuf[ix]));
				if ( (0x7f & resultbuf[ix]) == 0xd )
					break;
			}

			result = atol(resultbuf);
			switch ( result )
			{
			case 0:
				Log("%s HSM Spurious OK response", FAILED);
				speed = 0;
				break;
			case 1:
				Log("%s HSM connected at 300 baud!", FAILED);
				speed = -1;
				break;
			case 2:
				speed = 0;
				Debug((4, "Ringing"));
				break;
			case 3:
				Log("%s HSM No Carrier", FAILED);
				speed = -1;
				break;
			case 4:
				Log("%s HSM Error", FAILED);
				speed = -1;
				break;
			case 5:
				speed = 1200;
				break;
			case 6:
				Log("%s HSM No dialtone", FAILED);
				speed = -1;
				break;
			case 7:
				Log("%s HSM detected BUSY", FAILED);
				speed = -1;
				break;
			case 8:
				Log("%s HSM No quiet answer", FAILED);
				speed = -1;
				break;
			case 10:
				speed = 2400;
				break;
			default:
				Log("%s HSM Unknown response", FAILED);
				speed = -1;
				break;
			}

		} while ( speed == 0 );

		alarm(0);

		if ( speed < 0 )
		{
			strcpy(devSel, dev->D_line);
			hyscls24(dh, 0);
			return CF_DIAL;
		} else if ( speed != dev->D_speed )
		{
			Debug((4, "changing line speed to %d baud", speed));
			SetupTty(dh, speed);
		}
	}
	if ( dh < 0 )
	{
		Log("CAN'T OPEN %s", dcname);
		return dh;
	}
	Debug((4, "hayes ok"));
	return dh;
}

CallType
hyscls24(fd, flag)
int fd, flag;
{
	char dcname[PATHNAMESIZE];
	int fff = FREAD;

	if ( fd > 0 )
	{
		sprintf(dcname, "/dev/%s", devSel);
		if ( flag )
			Debug((4, "Resetting fd = %d", fd));
		else
			Debug((4, "Hanging up fd = %d", fd));
		/*
		 * Since we have a getty sleeping on this line, when it wakes
		 * up it sends all kinds of garbage to the modem.
		 * Unfortunatly, the modem likes to execute the previous
		 * command when it sees the garbage.  The previous command
		 * was to dial the phone, so let's make the last command
		 * reset the modem.
		 */
		if ( !flag )
			SetupTty(fd, 2400);
		write(fd, "\r", 1);
		sleep(2);
		write(fd, "+++", 3);
		sleep(3);
		write(fd, "\rATH\rATZ\r", 9);
		sleep(2);
		ioctl(fd, TIOCFLUSH, &fff);

		if ( !flag )
		{
			close_dev(fd);
			rmlock(devSel);
		}
	}
}

#endif	/* HAYES2400 */
