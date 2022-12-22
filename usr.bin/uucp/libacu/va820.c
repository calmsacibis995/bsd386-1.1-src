/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	RCSID $Id: va820.c,v 1.2 1993/02/28 15:31:08 pace Exp $
**
**	$Log: va820.c,v $
 * Revision 1.2  1993/02/28  15:31:08  pace
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

#ifdef	VA820

/*
**	Racal-Vadic 'RV820' with 831 adaptor.
**	BUGS:
**	dialer baud rate is hardcoded
*/

#define	MAXDIG 30	/* set by switches inside adapter */
char	c_abort	= '\001';
char	c_start	= '\002';
char	c_empty	= '\017';
char	c_end	= '\003';

CallType
va820opn(ph, flds, dev)
char *ph, *flds[];
Device *dev;
{
	register int va, i, child;
	char c, acu[PATHNAMESIZE], com[PATHNAMESIZE];
	char vadbuf[MAXDIG+2];
	int nw, lt;
	unsigned timelim;
	struct sgttyb sg;

	child = -1;
	if ( strlen(ph) > MAXDIG )
	{
		Debug((4, "BAD PHONE NUMBER %s", ph));
		Log("BAD PHONE NUMBER rvadopn");
		i = CF_DIAL;
		goto ret;
	}

	if ( setjmp(AlarmJmp) )
	{
		Log("TIMEOUT rvadopn");
		i = CF_DIAL;
		goto ret;
	}
	Debug((4, "ACU %s", dev->D_calldev));
	Debug((4, "LINE %s", dev->D_line));
	sprintf(acu, "/dev/%s", dev->D_calldev);
	getnextfd();
	signal(SIGALRM, Timeout);
	alarm(10);
	va = open(acu, O_RDWR);
	alarm(0);
	NextFd = -1;
	if ( va < 0 )
	{
		Debug((4, "ACU OPEN FAIL %d", errno));
		Log("CAN'T OPEN %s", acu);
		i = CF_NODEV;
		goto ret;
	}
	/*
	 * Set speed and modes on dialer and clear any
	 * previous requests
	 */
	Debug((4, "SETTING UP VA831 (%d)", va));
	ioctl(va, TIOCGETP, &sg);
	sg.sg_ispeed = sg.sg_ospeed = B1200;
	sg.sg_flags |= RAW;
	sg.sg_flags &= ~ECHO;
	ioctl(va, TIOCSETP, &sg);
	Debug((4, "CLEARING VA831"));
	if (  write(va, &c_abort, 1) != 1 )
	{
		Debug((4,"BAD VA831 WRITE %d", errno));
		Log("CAN'T CLEAR %s", acu);
		i = CF_DIAL;
		goto ret;
	}
	sleep(1);			/* XXX */
	read(va, &c, 1);
	if ( c != 'B' )
	{
		Debug((4,"BAD VA831 RESPONSE %c", c));
		Log("CAN'T CLEAR %s", acu);
		i = CF_DIAL;
		goto ret;
	}
	/*
	 * Build the dialing sequence for the adapter
	 */
	Debug((4, "DIALING %s", ph));
	sprintf(vadbuf, "%c%s<%c%c", c_start, ph, c_empty, c_end);
	timelim = 5 * strlen(ph);
	alarm(timelim < 30 ? 30 : timelim);
	nw = write(va, vadbuf, strlen(vadbuf));	/* Send Phone Number */
	if ( nw != strlen(vadbuf) )
	{
		Debug((4,"BAD VA831 WRITE %d", nw));
		Log("BAD WRITE %s", acu);
		goto failret;
	}

	sprintf(com, "/dev/%s", dev->D_line);

	/* create child to open comm line */
	if ( (child = fork()) == 0 )
	{
		signal(SIGINT, SIG_DFL);
		open(com, 0);			/* XXX - 0? --vix */
		sleep(5);
		_exit(1);
	}

	Debug((4, "WAITING FOR ANSWER"));
	if ( read(va, &c, 1) != 1 )
	{
		Log("%s ACU READ", FAILED);
		goto failret;
	}
	switch ( c )
	{
	case 'A':
		/* Fine! */
		break;
	case 'B':
		Debug((2, "Line Busy / No Answer"));
		goto failret;
	case 'D':
		Debug((2, "Dialer format error"));
		goto failret;
	case 'E':
		Debug((2, "Dialer parity error"));
		goto failret;
	case 'F':
		Debug((2, "Phone number too long"));
		goto failret;
	case 'G':
		Debug((2, "Modem Busy"));
		goto failret;
	default:
		Debug((2, "Unknown MACS return code '%c'", c&0177));
		goto failret;
	}
	/*
	 * open line - will return on carrier
	 */
	if ( (i = open(com, O_RDWR)) < 0 )
	{
		if ( errno == EIO )
			Log("LOST carrier");
		else
			Log("%s dialup open", FAILED);
		goto failret;
	}
	Debug((2, "RVADIC opened %d", i));
	SetupTty(i, dev->D_speed);
	goto ret;
failret:
	i = CF_DIAL;
ret:
	alarm(0);
	if ( child != -1 )
		kill(child, SIGKILL);
	close(va);
	while ( (nw = wait(&lt)) != child && nw != -1 )
		;
	return i;
}

CallType
va820cls(fd)
register int fd;
{

	Debug((2, "RVADIC close %d", fd));
	close(fd);
}
#endif	/* VA820 */
