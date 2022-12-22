/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	RCSID $Id: vmacs.c,v 1.2 1993/02/28 15:31:12 pace Exp $
**
**	$Log: vmacs.c,v $
 * Revision 1.2  1993/02/28  15:31:12  pace
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

#ifdef	VMACS

/*
**	A typical 300 baud L-devices entry is
**	ACU /dev/tty10 /dev/tty11,48,1200 300 vmacs
**	where tty10 is the communication line (D_Line),
**	tty11 is the dialer line (D_calldev),
**	the '4' is the dialer address + modem type (viz. dialer 0, Bell 103),
**	and the '8' is the communication port
**	(Note: Based on RVMACS version for 820 dialer.  This version
**	 developed by Doug Kingston @ BRL, 13 December 83.)
*/


#define	SOH	01	/* Abort */
#define	STX	02	/* Access Adaptor */
#define	ETX	03	/* Transfer to Dialer */
#define	SI	017	/* Buffer Empty (end of phone number) */

CallType
vmacsopn(ph, flds, dev)
char *ph, *flds[];
Device *dev;
{
	register int va, i, child;
	register char *p;
	char c, acu[PATHNAMESIZE], com[PATHNAMESIZE];
	char	modem, dialer;
	int	dialspeed;
	char c_STX = STX;
	char c_ETX = ETX;
	char c_SI = SI;
	char c_SOH = SOH;

	/* create child to open comm line */
	child = -1;
	sprintf(com, "/dev/%s", dev->D_line);
	if ( (child = fork()) == 0 )
	{
		signal(SIGINT, SIG_DFL);
		open(com, 0);			/* XXX - 0? --vix */
		Debug((5, "%s Opened.", com));
		sleep(5);
		exit(1);
	}

	if ( (p = strchr(dev->D_calldev, ',')) == NULLSTR )
	{
		Debug((2, "No dialer/modem specification"));
		goto failret;
	}
	*p++ = '\0';
	if ( *p < '0' || *p > '7' )
	{
		Log("Bad dialer address/modem type %s", p);
		goto failret;
	}
	dialer = *p++;
	if ( *p < '0' || *p > '>' )
	{
		Log("Bad modem address %s", p);
		goto failret;
	}
	modem = *p++;
	if ( *p++ == ',' )
		dialspeed = atoi (p);
	else
		dialspeed = dev->D_speed;
	if ( setjmp(AlarmJmp) )
	{
		Log("TIMEOUT vmacsopn");
		i = CF_DIAL;
		goto ret;
	}
	Debug((4, "STARTING CALL"));
	sprintf(acu, "/dev/%s", dev->D_calldev);
	getnextfd();
	signal(SIGALRM, Timeout);
	alarm(60);
	if ( (va = open(acu, O_RDWR)) < 0 )
	{
		Log("CAN'T OPEN %s", acu);
		i = CF_NODEV;
		goto ret;
	}
	Debug((5, "ACU %s opened.", acu));
	NextFd = -1;
	SetupTty(va, dialspeed);

	write(va, &c_SOH, 1);		/* abort, reset the dialer */
	do {
		if ( read (va, &c, 1) != 1 )
		{
			Log("%s MACS initialization", FAILED);
			goto failret;
		}
	} while ( (c&0177) != 'B' );
	Debug((5, "ACU initialized"));

	write(va, &c_STX, 1);		/* start text, access adaptor */
	write(va, &dialer, 1);		/* send dialer address digit */
	write(va, &modem, 1);		/* send modem address digit */
	for ( p=ph ; *p ; p++ )
	{
		if ( *p == '=' || (*p >= '0' && *p <= '9') )
			write(va, p, 1);
	}
	write(va, &c_SI, 1);		/* send buffer empty */
	write(va, &c_ETX, 1);		/* end of text, initiate call */

	if ( read(va, &c, 1) != 1 )
	{
		Log("%s ACU READ", FAILED);
		goto failret;
	}
	switch ( c )
	{
	case 'A':
		/* Fine! */
		Debug((5, "Call connected"));
		break;
	case 'B':
		Debug((2, "Dialer Timeout or Abort"));
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
		Debug((2, "Busy signal"));
		goto failret;
	default:
		Debug((2, "Unknown MACS return code '%c'", i));
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
	SetupTty(i, dev->D_speed);
	goto ret;
failret:
	i = CF_DIAL;
ret:
	alarm(0);
	if ( child > 1 )
		kill(child, SIGKILL);
	close(va);
	sleep(2);
	return i;
}

CallType
vmacscls(fd)
register int fd;
{
	char c_SOH = SOH;

	Debug((2, "MACS close %d", fd));
	write(fd, &c_SOH, 1);
/*	ioctl(fd, TIOCCDTR, NULLSTR);*/
	close(fd);
}
#endif	/* VMACS */
