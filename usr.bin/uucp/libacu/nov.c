/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	RCSID $Id: nov.c,v 1.3 1994/01/31 01:26:12 donn Exp $
**
**	$Log: nov.c,v $
 * Revision 1.3  1994/01/31  01:26:12  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:58:01  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:42:24  vixie
 * Initial revision
 *
 * Revision 1.2  1993/02/28  15:30:59  pace
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

#ifdef	NOVATION

extern CallType	novcls(int);

/*
**	novopn(telno, flds, dev) connect to novation Smart-Cat
**	(similar to hayes smartmodem)
**	char *flds[], *dev[];
**
**	return codes:
**		>0  -  file number  -  ok
**		CF_DIAL,CF_DEVICE  -  failed
*/

CallType
novopn(
	char *	telno,
	char *	flds[],
	Device *dev
)
{
	int	dh		= -1;
	int	pulse		= 0;
	char	dcname[PATHNAMESIZE];

	sprintf(dcname, "/dev/%s", dev->D_line);
	Debug((4, "dc - %s", dcname));
	if ( strcmp(dev->D_calldev, "pulse") == 0 )
		pulse = 1;
	if ( setjmp(AlarmJmp) )
	{
		Debug((1, "timeout novation open %s", dcname));
		Log("TIMEOUT novation open");
		if ( dh >= 0 )
			close_dev(dh);
		rmlock(dev->D_line);
		return CF_DIAL;
	}
	signal(SIGALRM, Timeout);
	getnextfd();
	alarm(10);
	dh = open_dev(dcname);
	alarm(0);

	/* modem is open */
	NextFd = -1;
	if ( dh >= 0 )
	{
		SetupTty(dh, dev->D_speed);
		/* set guard time small so line is in transparant mode */
		slow_write(dh, "\rATS12=1\r");
		if ( expect("OK", dh) != 0 )
		{
			Log("%s NOV no line", FAILED);
			strcpy(devSel, dev->D_line);
			novcls(dh);
			return CF_DIAL;
		}

		if ( pulse )
			slow_write(dh, "ATDP");
		else
			slow_write(dh, "ATDT");
		slow_write(dh, telno);
		slow_write(dh, "\r");

		if ( expect("CONNECT", dh) != 0 )
		{
			Log("%s NOV no carrier", FAILED);
			strcpy(devSel, dev->D_line);
			novcls(dh);
			return CF_DIAL;
		}

	}
	if ( dh < 0 )
	{
		Debug((4, "novation failed"));
		rmlock(dev->D_line);
	}
	Debug((4, "novation ok"));
	return dh;
}

CallType
novcls(
	int		fd
)
{
	char		dcname[PATHNAMESIZE];

	if ( fd > 0 )
	{
		sprintf(dcname, "/dev/%s", devSel);
		Debug((4, "Hanging up fd = %d", fd));
		drop_dtr(fd);
		sleep(2);
		slow_write(fd, "\rATZ\r");
		close_dev(fd);
		rmlock(devSel);
	}
}
#endif	/* NOVATION */
