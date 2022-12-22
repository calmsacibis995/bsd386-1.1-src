/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Routines to establish virtual circuits.
**
**	RCSID $Id: Diallers.c,v 1.6 1994/02/07 20:38:18 polk Exp $
**
**	$Log: Diallers.c,v $
 * Revision 1.6  1994/02/07  20:38:18  polk
 * patch from vixie
 *
 * Revision 1.5  1994/01/31  04:46:06  donn
 * Don't try to clear DTR if we don't have TIOCCDTR.
 *
 * Revision 1.4  1994/01/31  01:26:39  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:58:30  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:42:44  vixie
 * Initial revision
 *
 * Revision 1.3  1993/02/28  15:36:31  pace
 * Add recent uunet changes; add P_HWFLOW_ON, etc; add hayesv dialer
 *
 * Revision 1.2  1992/11/20  18:08:46  trent
 * set clocal for bidirectional support.
 *
 * Revision 1.1.1.1  1992/09/28  20:08:55  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	ERRNO
#define	EXECUTE
#define	FILES
#define	FILE_CONTROL
#define	SETJMP
#define	SIGNALS
#define	STDIO
#define	SYSEXITS
#define	TERMIOCTL

#include	"global.h"
#include	"cico.h"
#include	"devices.h"

/*
**	These are various dialers to establish a virtual circuit.
**
**	The dialers were supplied by many people, to whom we are grateful.
*/

static CallType	ACU_Open(char **);

ConDev	condevs[] = {
	{ "DIR", "direct", diropn, nullopn, dircls },
#ifdef DATAKIT
	{ "DK", "datakit", dkopn, nullopn, nullcls },
#endif	/* DATAKIT */
#ifdef PNET
	{ "PNET", "pnet", pnetopn, nullopn, nullcls },
#endif	/* PNET */
#ifdef	UNETTCP
	{ "TCP", "TCP", unetopn, nullopn, unetcls },
#endif	/* UNETTCP */
#ifdef BSDTCP
	{ "TCP", "TCP", bsdtcpopn, nullopn, bsdtcpcls },
#endif	/* BSDTCP */
#ifdef MICOM
	{ "MICOM", "micom", micopn, nullopn, miccls },
#endif	/* MICOM */
#ifdef DN11
	{ "ACU", "dn11", ACU_Open, dnopn, dncls },
#endif	/* DN11 */
#ifdef HAYES
	{ "ACU", "hayes", ACU_Open, hyspopn, hyscls },
	{ "ACU", "hayespulse", ACU_Open, hyspopn, hyscls },
	{ "ACU", "hayestone", ACU_Open, hystopn, hyscls },
	{ "WATS", "hayestone", ACU_Open, hystopn, hyscls },
#endif	/* HAYES */
#ifdef HAYES2400
	{ "ACU", "hayes2400", ACU_Open, hyspopn24, hyscls24 },
	{ "ACU", "hayes2400pulse", ACU_Open, hyspopn24, hyscls24 },
	{ "ACU", "hayes2400tone", ACU_Open, hystopn24, hyscls24 },
#endif	/* HAYES2400 */
#ifdef HAYESQ	/* a version of hayes that doesn't use result codes */
	{ "ACU", "hayesq", ACU_Open, hysqpopn, hysqcls },
	{ "ACU", "hayesqpulse", ACU_Open, hysqpopn, hysqcls },
	{ "ACU", "hayesqtone", ACU_Open, hysqtopn, hysqcls },
#endif	/* HAYESQ */
#ifdef HAYESV	/* a very aggressive version for use with recalcitrant modems */
	{ "ACU", "hayesv", ACU_Open, hysvpopn, hysvcls },
	{ "ACU", "hayesvpulse", ACU_Open, hysvpopn, hysvcls },
	{ "ACU", "hayesvtone", ACU_Open, hysvtopn, hysvcls },
#endif	/* HAYESV */
#ifdef CDS224
	{ "ACU", "cds224", ACU_Open, cdsopn224, cdscls224},
#endif	/* CDS224 */
#ifdef NOVATION
	{ "ACU", "novation", ACU_Open, novopn, novcls},
#endif	/* NOVATION */
#ifdef DF02
	{ "ACU", "DF02", ACU_Open, df2opn, df2cls },
#endif	/* DF02 */
#ifdef DF112
	{ "ACU", "DF112P", ACU_Open, df12popn, df12cls },
	{ "ACU", "DF112T", ACU_Open, df12topn, df12cls },
#endif	/* DF112 */
#ifdef VENTEL
	{ "ACU", "ventel", ACU_Open, ventopn, ventcls },
#endif	/* VENTEL */
#ifdef PENRIL
	{ "ACU", "penril", ACU_Open, penopn, pencls },
#endif	/* PENRIL */
#ifdef VADIC
	{ "ACU", "vadic", ACU_Open, vadopn, vadcls },
#endif	/* VADIC */
#ifdef VA212
	{ "ACU", "va212", ACU_Open, va212opn, va212cls },
#endif	/* VA212 */
#ifdef VA811S
	{ "ACU", "va811s", ACU_Open, va811opn, va811cls },
#endif	/* VA811S */
#ifdef VA820
	{ "ACU", "va820", ACU_Open, va820opn, va820cls },
	{ "WATS", "va820", ACU_Open, va820opn, va820cls },
#endif	/* VA820 */
#ifdef RVMACS
	{ "ACU", "rvmacs", ACU_Open, rvmacsopn, rvmacscls },
#endif	/* RVMACS */
#ifdef VMACS
	{ "ACU", "vmacs", ACU_Open, vmacsopn, vmacscls },
#endif	/* VMACS */
#ifdef SYTEK
	{ "SYTEK", "sytek", sykopn, nullopn, sykcls },
#endif	/* SYTEK */
#ifdef ATT2224
	{ "ACU", "att", ACU_Open, attopn, attcls },
#endif	/* ATT2224 */

	/* Insert new entries before this line */

	{ NULLSTR, NULLSTR, (CallType(*)())0, (CallType(*)())0, (CallType(*)())0 }
};

/*
**	Miscellaneous.
*/

char		devSel[PATHNAMESIZE];	/* used for later unlock() */
static char	LockACUdevice[LOCKNAMESIZE+1];

static CallType	disable(char *);
static CallType	enable(char *, char *);
static void	setup(void);

/*
**	Custom open that respects kernel/getty bidirectional protocol
*/

int
open_dev(char *name)
{
	int fd;

	fd = open(name, O_RDWR|O_NONBLOCK);

	if (fd >= 0) {
		DoTermios(fd, UU_CLOCAL, 1);

		/* turn off O_NONBLOCK */
		fcntl(fd, F_SETFL,
		      fcntl(fd, F_GETFL, 0) & ~(O_NDELAY|O_APPEND|O_ASYNC)
		      );
	}

	return fd;
}

/*
**	Custom close that flushes output on the tty to avoid hanging in exit
*/

int
close_dev(int fd)
{
#ifdef USE_TERMIOS
	tcflush(fd, TCIOFLUSH);
#else
	/* XXX - some ioctl, i guess */
#endif
	return close(fd);
}

/*
**	Open an ACU and dial the number.  The condevs table
**	will be searched until a dialing unit is found that is free.
*/

static CallType
ACU_Open(
	register char *		flds[]
)
{
	register ConDev *	cd;
	register int		fd;
	register int		acustatus;
	Device			dev;
	int			retval = CF_NODEV;
	char *			line;
	char *			bufp;
	char *			data;
	char			phone[MAXPH+1];
	char			nobrand[MAXPH];

	ExpandTelno(flds[F_PHONE], phone);

	if ( strccmp(flds[F_LINE], "LOCAL") == STREQUAL )
		line = "ACU";
	else
		line = flds[F_LINE];

	devSel[0] = '\0';
	nobrand[0] = '\0';

	Debug((4, "Dialing %s", phone));

	if ( (data = ReadFile(DEVFILE)) == NULLSTR )
	{
		SysError(CouldNot, ReadStr, DEVFILE);
		finish(EX_OSERR);
	}

	bufp = data;

	/*
	**	For each ACU L.sys line, try at most twice
	**	(TRYCALLS) to establish carrier.  The old way tried every
	**	available dialer, which on big sites takes forever!
	**	Sites with a single auto-dialer get one try.
	**	Sites with multiple dialers get a try on each of two
	**	different dialers.
	**
	**	To try 'harder' to connect to a remote site,
	**	use multiple L.sys entries.
	*/

	acustatus = 0;	/* none found, none locked */

	while ( acustatus <= TRYCALLS && NextDevice(&bufp, &dev) )
	{
		if ( strcmp(flds[F_CLASS], dev.D_class) != STREQUAL )
			continue;
		if ( strccmp(line, dev.D_type) != STREQUAL )
			continue;
		if ( dev.D_brand[0] == '\0' )
		{
			Log("No 'brand' name on ACU open");
			continue;
		}

		for ( cd = condevs ; cd->CU_meth != NULLSTR ; cd++ )
		{
			if ( strccmp(line, cd->CU_meth) == STREQUAL )
			{
				if ( strccmp(dev.D_brand, cd->CU_brand) == STREQUAL )
				{
					nobrand[0] = '\0';
					break;
				}

				(void)strncpy(nobrand, dev.D_brand, sizeof nobrand);
			}
		}

		if ( cd->CU_meth == NULLSTR ) {
			Log("Bad ACU 'brand' in L-devices");
			continue;
		}

		if ( acustatus < 1 )
			acustatus = 1;	/* has been found */

		if ( !mklock(dev.D_line) )
			continue;

		if 
		(
			DIALINOUT != NULLSTR
			&&
			(ALLACUINOUT || strccmp("inout", dev.D_calldev) == STREQUAL)
		)
		{
			if ( disable(dev.D_line) == FAIL )
			{
				rmlock(dev.D_line);
				continue;
			}
		}
		else
			Reenable();

		Debug((4, "Using %s", cd->CU_brand));

		if ( (fd = (*(cd->CU_open))(phone, flds, &dev)) != SYSERROR )
		{
			CU_end = cd->CU_clos;
			(void)strcpy(devSel, dev.D_line);
			free(data);
			return fd;
		}

		rmlock(dev.D_line);
		retval = CF_DIAL;
		acustatus++;
	}

	free(data);

	if ( acustatus == 0 )
	{
		if ( nobrand[0] )
			Log("unsupported ACU type %s", nobrand);
		else
			Log("No appropriate ACU L-devices");
	}
	else
	if ( acustatus == 1 )
		Log("NO DEVICE");

	return retval;
}

/*
**	Connect to hardware line.
*/

CallType
diropn(
	register char *	flds[]
)
{
	register int	dcr;
	register int	status;
	char		*data;
	char		*bufp;
	Device		dev;
	char		dcname[PATHNAMESIZE];

	if ((data = ReadFile(DEVFILE)) == NULLSTR) {
		SysError(CouldNot, ReadStr, DEVFILE);
		finish(EX_OSERR);
	}

	devSel[0] = '\0';
	bufp = data;
	status = 0;

	while (NextDevice(&bufp, &dev)) {
		if (strcmp(flds[F_CLASS], dev.D_class) != STREQUAL)
			continue;
		if (strcmp(flds[F_PHONE], dev.D_line) != STREQUAL)
			continue;
		if (mklock(dev.D_line)) {
			status++;
			break;
		}
	}

	free(data);

	if (!status) {
		Log("No appropriate DIR L-devices");
		return (CF_NODEV);
	}

	(void)sprintf(dcname, "/dev/%s", dev.D_line);

	if (setjmp(AlarmJmp)) {
		Debug((4, "Open timed out"));
		rmlock(dev.D_line);
		return CF_DIAL;
	}

	(void)signal(SIGALRM, Timeout);

	(void)alarm(MAXMSGTIME*4);

	getnextfd();
	errno = 0;

        Debug((4, "Opening %s", dcname));

	dcr = open_dev(dcname);

	NextFd = SYSERROR;
	(void)alarm(0);

	if (dcr < 0) {
		if (errno == EACCES)
			Log(CouldNot, OpenStr, dev.D_line);
		Debug((4, "OPEN FAILED: %s", ErrnoStr(errno)));
		rmlock(dev.D_line);
		return (CF_DIAL);
	}

	(void)fflush(stdout);

	if (!SetupTty(dcr, dev.D_speed)) {
		Debug((4, "SetupTty FAILED"));
		return (CF_DIAL);
	}

	if (dochat(&dev, flds, dcr)) {
		Log(dcname, "INITIAL CHAT FAILED");
		close_dev(dcr);   dcr = -1;
		return (CF_DIAL);
	}

	(void)strcpy(devSel, dev.D_line);	/* for later unlock */
	CU_end = dircls;

	return (dcr);
}

CallType
dircls(
	int	fd
)
{
	if ( fd < 0 )
		return;

	(void)close_dev(fd);
	rmlock(devSel);
}

/*
**	Allow a single line to be use for dialin/dialout.
*/

static CallType
disable(
	char *		dev
)
{
	register char *	rdev;

	/** Strip off directory prefixes **/

	if ( (rdev = strrchr(dev, '/')) != NULLSTR )
		++rdev;
	else
		rdev = dev;

	if ( LockACUdevice[0] )
	{
		if ( strcmp(LockACUdevice, rdev) == STREQUAL )
			return SUCCESS;	/* already disabled */

		rmlock(LockACUdevice);
		Reenable();		/* else, reenable the old one */
	}

	Debug((4, "Disable %s", rdev));

	if ( enable("disable", rdev) == FAIL )
		return FAIL;

	strcpy(LockACUdevice, rdev);
	return SUCCESS;
}

static CallType
enable(
	char *	type,
	char *	dev
)
{
	ExBuf	args;
	char *	errs;

	FIRSTARG(&args.ex_cmd) = DIALINOUT;
	NEXTARG(&args.ex_cmd) = type;
	NEXTARG(&args.ex_cmd) = dev;
	NEXTARG(&args.ex_cmd) = RemoteNode;

	if ( (errs = Execute(&args, setup, ex_exec, SYSERROR)) != NULLSTR )
	{
		Warn(errs);
		free(errs);
		return FAIL;
	}

	return SUCCESS;
}

/*
**	Find first digit in string.
*/

char *
fdig(
	char *		str
)
{
	register char *	cp;
	register int	c;

	for ( cp = str ; c = *cp++ ; )
		if ( c >= '0' && c <= '9' )
			break;

	return --cp;
}

/*
**	Read and decode a line from device file.
*/

bool
NextDevice(
	char **			bufp,
	register Device *	dev
)
{
	register int		na;
	register char *		cp;

	if ( (cp = GetLine(bufp)) == NULLSTR )
		return false;

	(void)strncpy(dev->D_buf, cp, MAXDEVCHARS-1);
	dev->D_buf[MAXDEVCHARS-1] = '\0';

	na = SplitSpace(dev->D_arg, dev->D_buf, MAXDEVARGS);

	if ( na < 4 )
	{
		ErrVal = EX_DATAERR;
		Error("%s: invalid device entry", dev->D_buf);
	}

	if ( na == 4 )
	{
		dev->D_brand = EmptyStr;
		na++;
	}

	if ( strcmp(dev->D_class, "FAST") == 0 )
		dev->D_speed = 19200;
	else
	if ( strcmp(dev->D_class, "V32") == 0 )
		dev->D_speed = 19200;
	else
		dev->D_speed = atoi(fdig(dev->D_class));

	dev->D_nargs = na;

	return true;
}

/*
**	Null device (returns CF_NODEV)
*/

CallType
nodev()
{
	return CF_NODEV;
}

/*
**	Null device (returns CF_DIAL)
*/

CallType
nullopn(char *ph, char **flds, Device *dev)
{
	return CF_DIAL;
}

CallType
nullcls(int fd)
{
	return CF_DIAL;
}

/*
**	Allow a single line to be use for dialin/dialout.
*/

void
Reenable()
{
	if ( DIALINOUT == NULLSTR || LockACUdevice[0] == '\0' )
		return;
	Debug((4, "Reenable %s", LockACUdevice));
	(void)enable("enable", LockACUdevice);
	LockACUdevice[0] = '\0';
}

/*
**	Reset UID in child process.
**
**	(For chown(uid()) in ACU program.)
*/

static void
setup()
{
	(void)setuid(geteuid());
}

/*
**	Set speed/echo/mode... (called from ACU setup).
**
**	( XXX Check this against SetRaw().)
*/

bool
SetupTty(
	int		tty,
	int		spwant
)
{
#ifdef USE_TERMIOS
	struct termios	ttbuf;
	int		save_cflag;
#else
	struct sgttyb	ttbuf;
#endif
	register Speed	*ps;
	long		speed = -1;

	if (!Is_a_tty) {
		Baud = DFLT_BAUD;
		return (true);
	}

	for (ps = Speeds; ps->speed; ps++)
		if (ps->baud == spwant)
			speed = ps->speed;

	if (speed < 0) {
#ifdef USE_TERMIOS
		speed = spwant;
		Debug((1, "guessing the baud rate as %ld", spwant));
#else
		ErrVal = EX_DATAERR;
		Error("unrecognized speed: %d", speed);
#endif
	}

	Debug((1, "SetupTty baud/no. => %d/%d", spwant, speed));

#ifdef USE_TERMIOS

	if (tcgetattr(tty, &ttbuf) == SYSERROR)
		return (false);

	save_cflag = ttbuf.c_cflag & CLOCAL;
	ttbuf.c_iflag = 0;
	ttbuf.c_oflag = 0;
	ttbuf.c_lflag = 0;
	ttbuf.c_cflag = (CS8|HUPCL|CREAD|save_cflag);
	cfsetispeed(&ttbuf, speed);
	cfsetospeed(&ttbuf, speed);
	ttbuf.c_cc[VMIN] = 6;
	ttbuf.c_cc[VTIME] = 1;

	if (tcsetattr(tty, TCSANOW, &ttbuf) == SYSERROR)
		return (false);

#else	/* USE_TERMIOS */

	if (ioctl(tty, TIOCGETP, &ttbuf) == SYSERROR)
		return false;

	ttbuf.sg_flags = (ANYP|RAW);
	ttbuf.sg_ispeed = ttbuf.sg_ospeed = speed;

	if (ioctl(tty, TIOCSETP, &ttbuf) == SYSERROR)
		return false;

	if (ioctl(tty, TIOCHPCL, (void *)0) == SYSERROR)
		Debug((1, CouldNot, "TIOCHPCL", ErrnoStr(errno)));

#endif	/* USE_TERMIOS */

#ifdef TIOCEXCL
	if (ioctl(tty, TIOCEXCL, (void *)0) == SYSERROR)
		Debug((1, CouldNot, "TIOCEXCL", ErrnoStr(errno)));
#endif

	Baud = spwant;
	return (true);
}

/*
**	Delay execution for (numerator/denominator) seconds.
*/

#if	BSD4 >= 2
#include <sys/time.h>

static void
intervaldelay(
	int		num,
	int		denom
)
{
	struct timeval	tv;

	tv.tv_sec = num / denom;
	tv.tv_usec = (num * 1000000L / denom ) % 1000000L;

	(void)select(0, (fd_set *)0, (fd_set *)0, (fd_set *)0, &tv);
}

#define UUDELAY(num,denom)	intervaldelay(num,denom)

#else	/* BSD4 >= 2 */

#ifdef	FASTTIMER

/*
**	Sleep in increments of 60ths of second.
*/

static void
nap(
	int		time;
)
{
	static int	fd;

	if ( fd == 0 )
		fd = open(FASTTIMER, O_RDONLY);

	(void)read(fd, 0, time);
}

#define UUDELAY(num,denom)	nap(60*num/denom)

#endif	/* FASTTIMER */

#ifdef	FTIME

/*
**	NB: also a `busy loop'.
*/

static void
ftimedelay(
	register int		n
)
{
	static struct timeb	loctime;
	register int		i = loctime.millitm;

	ftime(&loctime);

	while ( abs((int)(loctime.millitm - i)) < n )
		ftime(&loctime);
}

#define UUDELAY(num,denom)	ftimedelay(1000*num/denom)

#endif	/* FTIME */

#ifdef	BUSYLOOP

/*
**	Assume 10 MIPS and poor C optimiser
*/

#define CPUSPEED	10000000
#define	LOOP(n)		{ register long N = (n); while ( --N > 0 ); }

static void
busyloop(
	int	n
)
{
	LOOP(n);
}

#define UUDELAY(num,denom)	busyloop(CPUSPEED*num/denom)

#endif	/* BUSYLOOP */
#endif	/* BSD4 >= 2 */

/*
**	External interface to our UUDELAY macro
*/

void
Delay(
      int num,
      int denom
)
{
	UUDELAY(num, denom);
}

/*
**	Send a BREAK.
*/

void
SendBRK(
	register int	fn,
	register int	null_count
)
{
#ifdef USE_TERMIOS

	if (tcsendbreak(fn, 0) == SYSERROR)
		Debug((5, "tcsendbreak %s", ErrnoStr(errno)));

#else	/* USE_TERMIOS */

#ifdef	TIOCSBRK

	if ( ioctl(fn, TIOCSBRK, (void *)0) == SYSERROR )
		Debug((5, "TIOCSBRK %s", ErrnoStr(errno)));

	UUDELAY(null_count, 10);

	if ( ioctl(fn, TIOCCBRK, (void *)0) == SYSERROR )
		Debug((5, "TIOCCBRK %s", ErrnoStr(errno)));

#else	/* TIOCSBRK */

	register int	save_speed;
	struct sgttyb	ttbuf;
	static char	nulls[20];

	if ( ioctl(fn, TIOCGETP, &ttbuf) == SYSERROR )
		Debug((5, "TIOCGETP %s", ErrnoStr(errno)));

	save_speed = ttbuf.sg_ospeed;
	ttbuf.sg_ospeed = B150;

	if ( ioctl(fn, TIOCSETP, &ttbuf) == SYSERROR )
		Debug((5, "TIOCSETP %s", ErrnoStr(errno)));

	if ( null_count > 20 )
		null_count = 20;
	if ( write(fn, nulls, null_count) != null_count )
	{
badbreak:	(void)alarm(0);
		Log("BAD WRITE for BRK %s", ErrnoStr(errno));
		longjmp(AlarmJmp, 3);
	}

	ttbuf.sg_ospeed = save_speed;

	if ( ioctl(fn, TIOCSETP, &ttbuf) == SYSERROR )
		Debug((5, "TIOCSETP %s", ErrnoStr(errno)));

	if ( write(fn, "@", 1) != 1 )
		goto badbreak;

	Debug((4, "sent BREAK null_count - %d", null_count));

#endif	/* TIOCSBRK */

#endif	/* USE_TERMIOS */
}

/*
**	Slow write for slow modems.
*/

void
slow_write(
	int		fd,
	register char *	str
)
{
	Debug((6, "slow_write %s", str));

	while ( *str )
	{
		UUDELAY(1, 10);	/* Delay 1/10 second */
		(void)write(fd, str, 1);
		str++;
	}
}

/*
**	cycle DTR down for two seconds
*/

void
drop_dtr(int fd)
{
#ifdef TIOCCDTR
	Debug((8, "cycling DTR..."));
	ioctl(fd, TIOCCDTR, 0);
	UUDELAY(2, 1);
	ioctl(fd, TIOCSDTR, 0);
	UUDELAY(1, 10);
#endif /* TIOCCDTR */
}
