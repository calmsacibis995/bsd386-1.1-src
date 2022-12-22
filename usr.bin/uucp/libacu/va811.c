/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	RCSID $Id: va811.c,v 1.3 1994/01/31 01:26:17 donn Exp $
**
**	$Log: va811.c,v $
 * Revision 1.3  1994/01/31  01:26:17  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:58:01  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:42:24  vixie
 * Initial revision
 *
 * Revision 1.2  1993/02/28  15:31:06  pace
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

#ifdef	VA811S

/*
**	Racal-Vadic VA811 dialer with 831 adaptor.
**	A typical 300 baud L-devices entry is
**	ACU /dev/tty10 unused 300 va811s
**	where tty10 is the communication line (D_Line),
**	and 300 is the line speed.
**	This is almost identical to RVMACS except that we don't need to
**	send addresses and modem types, and don't need the fork.
**	Joe Kelsey, fluke!joe, vax4.1526, Apr 11 1984.
*/


#define	STX	02	/* Access Adaptor */
#define	ETX	03	/* Transfer to Dialer */
#define	SI	017	/* Buffer Empty (end of phone number) */
#define	SOH	01	/* Abort */

CallType
va811opn(ph, flds, dev)
char *ph, *flds[];
Device *dev;
{
	register int	i;
	register int	tries;
	int		va;
	char		c;
	char		dcname[PATHNAMESIZE];
	char		vabuf[36];	/* STX, 31 phone digits, SI, ETX, NUL, \0 */

	va = 0;
	sprintf(dcname, "/dev/%s", dev->D_line);
	if ( setjmp(AlarmJmp) )
	{
		Debug((1, "timeout va811 open"));
		Log("TIMEOUT va811opn");
		if ( va >= 0 )
			close_dev(va);
		rmlock(dev->D_line);
		return CF_NODEV;
	}
	Debug((4, "va811: STARTING CALL"));
	getnextfd();
	signal(SIGALRM, Timeout);
	alarm(10);
	va = open_dev(dcname);
	alarm(0);

	/* line is open */
	NextFd = -1;
	if ( va < 0 )
	{
		Debug((4, errno == 4 ? "%s: no carrier" : "%s: can't open", dcname));
		rmlock(dev->D_line);
		Log("CAN'T OPEN %s", dcname);
		return(errno == 4 ? CF_DIAL : CF_NODEV);
	}
	SetupTty(va, dev->D_speed);

	/* first, reset everything */
	sendthem("\\01\\c", va);
	i = expect("B", va);
	if ( i != 0 )
	{
		Debug((4, "va811: NO RESPONSE"));
		Log("TIMEOUT va811 reset");
		close_dev(va);
		rmlock(dev->D_line);
		return CF_DIAL;
	}

	sprintf(vabuf, "%c%.31s%c%c\\c", STX, ph, SI, SOH);
	sendthem(vabuf, va);
	i = expect("B", va);

	if ( i != 0 )
	{
		Debug((4, "va811: STORE NUMBER"));
		Log("%s va811 STORE", FAILED);
		close_dev(va);
		rmlock(dev->D_line);
		return CF_DIAL;
	}

	for ( tries = 0 ; tries < TRYCALLS ; tries++ )
	{
		sprintf(vabuf, "%c%c\\c", STX, ETX);
		sendthem(vabuf, va);
		i = expect("A", va);
		if ( i != 0 )
		{
			Debug((4, "va811: RESETTING"));
			Log("%s va811 DIAL", FAILED);
			sendthem("\\01\\c", va);
			expect("B", va);
		}
		else
			break;
	}

	if ( tries >= TRYCALLS )
	{
		close_dev(va);
		rmlock(dev->D_line);
		return CF_DIAL;
	}

	Debug((4, "va811 ok"));
	return va;
}

CallType
va811cls(fd)
register int fd;
{
	Debug((2, "va811 close %d", fd));
	p_chwrite(fd, SOH);
/*	ioctl(fd, TIOCCDTR, NULLSTR);*/
	close_dev(fd);
	rmlock(devSel);
}
#endif	/* VA811S */
