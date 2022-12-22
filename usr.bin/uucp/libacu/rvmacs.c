/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	RCSID $Id: rvmacs.c,v 1.2 1993/02/28 15:31:02 pace Exp $
**
**	$Log: rvmacs.c,v $
 * Revision 1.2  1993/02/28  15:31:02  pace
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

#ifdef	RVMACS

/*
**	Racal-Vadic 'RV820' MACS system with 831 adaptor.
**	A typical 300 baud L-devices entry is
**	ACU tty10 tty11,48 300 rvmacs
**	where tty10 is the communication line (D_Line),
**	tty11 is the dialer line (D_calldev),
**	the '4' is the dialer address + modem type (viz. dialer 0, Bell 103),
**	the '8' is the communication port,
**	We assume the dialer speed is 1200 baud unless MULTISPEED is defined.
**	We extended the semantics of the L-devices entry to allow you
**	to set the speed at which the computer talks to the dialer:
**	ACU cul0 cua0,0<,2400 1200 rvmacs
**	This is interpreted as above, except that the number following the second
**	comma in the third field is taken to be the speed at which the computer
**	must communicate with the dialer.  (If omitted, it defaults to the value
**	in the fourth field.)  Note -- just after the call completes and you get
**	carrier, the line speed is reset to the speed indicated in the fourth field.
**	To get this ability, define "MULTISPEED", as below.
**
*/

#define MULTISPEED		/* for dialers which work at various speeds */

#define	STX	02	/* Access Adaptor */
#define	ETX	03	/* Transfer to Dialer */
#define	SI	017	/* Buffer Empty (end of phone number) */
#define	ABORT	01	/* Abort */

#define	pc(fd, x)	(c = x, write(fd, &c, 1))

CallType
rvmacsopn(ph, flds, dev)
char *ph, *flds[];
Device *dev;
{
	register int va, i, child;
	register char *p;
	char c, acu[PATHNAMESIZE], com[PATHNAMESIZE];
	int baudrate;
	int timelim;
	int pid, status;
	int zero = 0;
#ifdef MULTISPEED
	char *pp;
#else	/* MULTISPEED */
	struct sgttyb sg;
#endif	/* MULTISPEED */

	child = -1;
	sprintf(com, "/dev/%s", dev->D_line);
	sprintf(acu, "/dev/%s", dev->D_calldev);
	if ( (p = strchr(acu, ',')) == NULLSTR )
	{
		Debug((2, "No dialer/modem specification"));
		return CF_DIAL;
	}
	*p++ = '\0';
#ifdef MULTISPEED
	baudrate = dev->D_speed;
	if ( (pp = strchr(p, ',')) != NULLSTR )
	{
		baudrate = atoi(pp+1);
		Debug((5, "Using speed %d baud", baudrate));
	}
#endif	/* MULTISPEED */
	if ( setjmp(AlarmJmp) )
	{
		Log("TIMEOUT rvmacsopn");
		goto failret;
	}
	Debug((4, "STARTING CALL"));
	getnextfd();
	signal(SIGALRM, Timeout);
	timelim = 5 * strlen(ph);
	alarm(timelim < 45 ? 45 : timelim);

	if ( (va = open(acu, O_RDWR)) < 0 )
	{
		Log("CAN'T OPEN %s", acu);
		alarm(0);
		return CF_DIAL;
	}

	/* rti!trt: avoid passing acu file descriptor to children */
	NextFd = -1;
	fioclex(va);

	if ( (child = fork()) == 0 )
	{
		/* create child to do dialing */
		sleep(2);
#ifdef MULTISPEED
		SetupTty(va, baudrate);
#else	/* MULTISPEED */
		sg.sg_flags = RAW|ANYP;
		sg.sg_ispeed = sg.sg_ospeed = B1200;
		ioctl(va, TIOCSETP, &sg);
#endif	/* MULTISPEED */
		pc(va, ABORT);
		sleep(1);
		ioctl(va, TIOCFLUSH, &zero);
		pc(va, STX);	/* access adaptor */
		pc(va, *p++);	/* Send Dialer Address Digit */
		pc(va, *p);	/* Send Modem Address Digit */
		while ( *ph && *ph != '<' )
		{
			switch ( *ph )
			{
			case '_':
			case '-':
			case '=':
				pc(va, '=');
				break;
			default:
				if ( *ph >= '0' && *ph <= '9' )
					pc(va, *ph);
				break;
			}
			ph++;
		}
		pc(va, '<');	/* Transfer Control to Modem (sigh) */
		pc(va, SI);	/* Send Buffer Empty */
		pc(va, ETX);	/* Initiate Call */
		sleep(1);

		if ( read(va, &c, 1) != 1 )
		{
			close(va);
			Log("%s ACU READ", FAILED);
			exit(1);
		}
		if ( c == 'B' || c == 'G' )
		{
			char cc;
			pc(va, ABORT);
			read(va, &cc, 1);
		}
		Debug((4, "Dialer returned %c", c));
		close(va);
		exit(c != 'A');
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
	while ( (pid = wait(&status)) != child && pid != -1 )
		;
	alarm(0);
	if ( status )
	{
		close(i);
		close(va);		/* XXX */
		return CF_DIAL;
	}
	SetupTty(i, dev->D_speed);
	return i;

failret:
	alarm(0);
	close(va);
	if ( child != -1 )
		kill(child, SIGKILL);
	return CF_DIAL;
}

CallType
rvmacscls(
	register int	fd
)
{
	char		c;

	if ( fd < 0 )
		return FAIL;

	pc(fd, ABORT);
	(void)ioctl(fd, TIOCCDTR, (void *)0);
	(void)sleep(1);
	(void)ioctl(fd, TIOCNXCL, (void *)0);
	(void)close(fd);
	rmlock(devSel);

	return SUCCESS;
}
#endif	/* RVMACS */
