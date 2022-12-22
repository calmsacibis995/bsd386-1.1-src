/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	RCSID $Id: dn.c,v 1.3 1994/01/31 01:26:05 donn Exp $
**
**	$Log: dn.c,v $
 * Revision 1.3  1994/01/31  01:26:05  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:58:01  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:42:24  vixie
 * Initial revision
 *
 * Revision 1.2  1993/02/28  15:30:51  pace
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

#ifdef	DN11

#define ACULAST "-<"

/***
**	dnopn(ph, flds, dev)	dial remote machine
**
**	return codes:
**		file descriptor  -  succeeded
**		FAIL  -  failed
*/

CallType
dnopn(ph, flds, dev)
char *ph;
char *flds[];
Device *dev;
{
	char dcname[PATHNAMESIZE], dnname[PATHNAMESIZE], phone[MAXPH+2], c = 0;
#ifdef USE_TERMIOS
	struct termios ttbuf;
#endif
	int dnf, dcf;
	int nw, lt, pid, status;
	unsigned timelim;
#ifdef TIOCFLUSH
	int zero = 0;
#endif

	sprintf(dnname, "/dev/%s", dev->D_calldev);
	errno = 0;

	if ( setjmp(AlarmJmp) )
	{
		Log("CAN'T OPEN %s", dnname);
		Debug((4, "%s Open timed out", dnname));
		return(CF_NODEV);
	}
	signal(SIGALRM, Timeout);
	getnextfd();
	alarm(10);
	dnf = open(dnname, O_WRONLY);
	alarm(0);
	NextFd = -1;
	if ( dnf < 0 && errno == EACCES )
	{
		Log("NO DEVICE: CAN'T OPEN %s", dnname);
		return CF_NODEV;
	}
	fioclex(dnf);

	sprintf(dcname, "/dev/%s", dev->D_line);
	sprintf(phone, "%s%s", ph, ACULAST);
	Debug((4, "dc - %s, ", dcname));
	Debug((4, "acu - %s", dnname));
	pid = 0;
	if ( setjmp(AlarmJmp) )
	{
		Log("TIMEOUT DIALUP DN write");
		if ( pid )
			kill(pid, 9);
		rmlock(dev->D_line);
		if ( dnf )
			close_dev(dnf);
		return CF_DIAL;
	}
	signal(SIGALRM, Timeout);
	timelim = 5 * strlen(phone);
	alarm(timelim < 30 ? 30 : timelim);
	if ( (pid = fork()) == 0 )
	{
		sleep(2);
#ifdef	TIOCFLUSH
		ioctl(dnf, TIOCFLUSH, &zero);
#endif	/* TIOCFLUSH */
		nw = write(dnf, phone, lt = strlen(phone));
		if ( nw != lt )
		{
			Log("%s DIALUP ACU write", FAILED);
			exit(1);
		}
		Debug((4, "ACU write ok"));
		exit(0);
	}
	/*  open line - will return on carrier */
	/* RT needs a sleep here because it returns immediately from open */

#if RT
	sleep(15);
#endif

	getnextfd();
	errno = 0;
	dcf = open_dev(dcname);
	NextFd = -1;
	if ( dcf < 0 && errno == EACCES )
		Log("CAN'T OPEN %s", dcname);
	Debug((4, "dcf is %d", dcf));
	if ( dcf < 0 )
	{
		Log("%s DIALUP LINE open", FAILED);
		alarm(0);
		kill(pid, 9);
		close_dev(dnf);
		rmlock(dev->D_line);
		return CF_DIAL;
	}
	while ( (nw = wait(&lt)) != pid && nw != -1 )
		;
#ifdef USE_TERMIOS
	tcgetattr(dcf, &ttbuf);
	if (!(ttbuf.c_cflag & HUPCL)) {
		ttbuf.c_cflag |= HUPCL;
		tcsetattr(dcf, TCSANOW, &ttbuf);
	}
#endif
	alarm(0);
	SetupTty(dcf, dev->D_speed);
	Debug((4, "Fork Stat %o", lt));
	if ( lt != 0 )
	{
		close_dev(dcf);
		if ( dnf )
			close_dev(dnf);
		rmlock(dev->D_line);
		return CF_DIAL;
	}
	return dcf;
}

/***
**	dncls()		close dn type call unit
**
**	return codes:	None
*/

CallType
dncls(fd)
register int fd;
{
	if ( fd > 0 )
	{
		close_dev(fd);
		sleep(5);
		rmlock(devSel);
	}
}

#endif	/* DN11 */
