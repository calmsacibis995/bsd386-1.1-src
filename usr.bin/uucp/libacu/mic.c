/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	RCSID $Id: mic.c,v 1.3 1994/01/31 01:26:11 donn Exp $
**
**	$Log: mic.c,v $
 * Revision 1.3  1994/01/31  01:26:11  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:58:01  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:42:24  vixie
 * Initial revision
 *
 * Revision 1.2  1993/02/28  15:30:58  pace
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

#ifdef MICOM

/*
**	micopn: establish connection through a micom.
**	Returns descriptor open to tty for reading and writing.
**	Negative values (-1...-7) denote errors in connmsg.
**	Be sure to disconnect tty when done, via HUPCL or stty 0.
*/

CallType
micopn(
	register char *	flds[]
)
{
	register ConDev *cd;
	int		dh;
	int		ok = 0;
	int		speed;
	char *		data;
	char *		bufp;
	char *		copy;
	Device		dev;
	char		dcname[PATHNAMESIZE];

	if ( (data = ReadFile(DEVFILE)) == NULLSTRSTR )
	{
		SysError(CouldNot, ReadStr, DEVFILE);
		finish(EX_OSERR);
	}

	copy = Malloc(RdFileSize);

	signal(SIGALRM, Timeout);
	dh = -1;

	/*
	**	XXX This loop should be inside-out -
	**	to do just one pass of DEVFILE data.
	*/

	for ( cd = condevs ; ((cd->CU_meth != NULLSTR)&&(dh < 0)) ; cd++ )
	{
		if ( strccmp(flds[F_LINE], cd->CU_meth) == STREQUAL )
		{
			bufp = strcpy(copy, data);

			while ( NextDevice(&bufp, &dev) )
			{
				if ( strcmp(flds[F_CLASS], dev.D_class) != STREQUAL )
					continue;
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
					Log("micom open TIMEOUT %s", dev.D_line);
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
		}
	}

	free(copy);
	free(data);

	if ( dh < 0 )
		return CF_NODEV;

	speed = atoi(fdig(flds[F_CLASS]));
	SetupTty(dh, speed);
	sleep(1);

	/* negotiate with micom */
	if ( speed != 4800 )	/* damn their eyes! */
		write(dh, "\r", 1);
	else
		write(dh, " ", 1);

	ok = expect("SELECTION", dh);
	if ( ok == 0 )
	{
		write(dh, flds[F_PHONE], strlen(flds[F_PHONE]));
		sleep(1);
		write(dh, "\r", 1);
		ok = expect("GO", dh);
	}

	if ( ok != 0 )
	{
		if ( dh > 2 )
			close_dev(dh);
		Debug((4, "micom failed"));
		rmlock(dev.D_line);
		return(CF_DIAL);
	}
	else
		Debug((4, "micom ok"));

	CU_end = cd->CU_clos;
	strcat(devSel, dev.D_line);	/* for later unlock */
	return dh;
}

CallType
miccls(fd)
register int fd;
{

	if ( fd > 0 )
	{
		close_dev(fd);
		rmlock(devSel);
	}
}
#endif	/* MICOM */
