/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	UUCP lock file management.
**
**	RCSID $Id: uucplock.c,v 1.2 1994/01/31 01:26:21 donn Exp $
**
**	$Log: uucplock.c,v $
 * Revision 1.2  1994/01/31  01:26:21  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:58:12  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:42:33  vixie
 * Initial revision
 *
 * Revision 1.1.1.1  1992/09/28  20:08:50  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

/*
**	TODO: add nodename into lockfile --
**	NFS operations make PID meaningless.
*/

#define	ERRNO
#define	FILE_CONTROL
#define	SYSEXITS
#define	SYS_STAT

#include	"global.h"

#if	SYSMKDEV == 1
#include	<sys/mkdev.h>
#else	/* SYSMKDEV == 1 */
#ifndef	major
#include	<sys/sysmacros.h>
#endif	/* major */
#endif	/* SYSMKDEV == 1 */

#define MAXLOCKS	64	/* Maximum number of lock files */
#define	MAXTRIES	4	/* Maximum attempts to get an existing lock */
#define	PIDSTRSIZE	11	/* If lockfile contains ASCII */
#define	DEADBADLOCKTIME	(60*3)	/* Any abnormal lockfile older than this is dead */


static char *	Lockfiles[MAXLOCKS];
static char	Lckdevfmt[]	= "%s%s%3.3lu.%3.3lu.%3.3lu";	/* LOCKDIR, LOCKPRE, maj, maj, min */
static char	Lckfmt[]	= "%s%s%s";			/* LOCKDIR, LOCKPRE, tty */
static int	Nlocks;

static bool	createlock(char *);
static void	keepfile(char *);
static char *	name_to_lock(char *, char *);
static bool	trylock(char *, char *);



/*
**	Exported functions:
*/


/*
**	Make a lockfile.
*/

bool
mklock(
	char *	name
)
{
	char	ln[PATHNAMESIZE];

	setproctitle("%s %s", Name, name);

	return createlock(name_to_lock(name, ln));
}


/*
**	Remove lock file(s).
*/

void
rmlock(
	register char *	name	/* == NULLSTR means _all_ */
)
{
	register int	i;
	register char *	cp;
	char		ln[PATHNAMESIZE];

	if ( name != NULLSTR )
	{
		Trace((1, "rmlock(%s) [%d]", name, Nlocks));
		(void)name_to_lock(name, ln);
	}
	else
		Trace((1, "rmlock(NULLSTR) [%d]", Nlocks));

	for ( i = Nlocks ; --i >= 0 ; )
		if
		(
			(cp = Lockfiles[i]) != NULLSTR
			&&
			(
				name == NULLSTR
				||
				strcmp(ln, cp) == STREQUAL
			)
		)
		{
			Trace((2, "rmlock() unlink %s", cp));
			(void)unlink(cp);
			FreeStr(&Lockfiles[i]);

			if ( (i+1) == Nlocks )
				--Nlocks;
		}
}


/*
**      Postpone timeout for lock files.
*/

void
touchlock()
{
	register int	i;

	for ( i = Nlocks ; --i >= 0 ; )
	{
		if ( Lockfiles[i] == NULLSTR )
			continue;

		if ( utimes(Lockfiles[i], (struct timeval *)0) == SYSERROR )
			SysWarn(CouldNot, "utimes", Lockfiles[i]);
	}
}



/*
**	Own functions:
*/


/*
**      Create a lock file.
*/

static bool
createlock(
	char *		file
)
{
	int		retry;
	int		backoff;
	struct stat	statb;
	static int	masks[MAXTRIES] = { 037, 017, 07, 03 };
	extern long	random(void);
#	ifndef	O_CREAT
	static char	tempfile[PATHNAMESIZE];

	if ( tempfile[0] == '\0' )
		(void)sprintf(tempfile, "%sLTMP.%d", LOCKDIR, Pid);
#	else	/* !O_CREAT */
#	define	tempfile	file
#	endif	/* !O_CREAT */

	Trace((2, "createlock(%s)", file));

	SetTimes();
	srandom((int)Time);	/* For binary exponential backoff */

	for
	(
		backoff = 1, retry = MAXTRIES ;
		!trylock(tempfile, file) && --retry >= 0 ;
		backoff += random() & masks[retry]
	)
	{
		char	pidbuf[PIDSTRSIZE+1];	/* "nnnnnnnnnn\n\0" */
		int	size;
		char *	buf;
		int	pid;
		int	fd;
		int	n;

		/* Lock file exists */

		if ( (fd = open(file, O_RDONLY)) == SYSERROR )
		{
			if ( errno == ENOENT )
				continue;	/* No it doesn't */

			retry = -1;		/* Lock can't be broken */
			break;
		}

		if ( LOCKPIDISSTR )
		{
			size = PIDSTRSIZE;
			buf = pidbuf;
		}
		else
		{
			size = sizeof pid;
			buf = (char *)&pid;
		}

		n = read(fd, buf, size);

		(void)close(fd);

		if ( n != size )
		{
			if
			(
				stat(file, &statb) == SYSERROR
				||
				statb.st_mtime > (Time - DEADBADLOCKTIME)
			)
			{
				setproctitle("sleeping on lock: %s", file);
				(void)sleep(backoff);	/* Race? */
				Time += backoff;
				continue;
			}
		}
		else
		{
			if ( LOCKPIDISSTR )
			{
				pidbuf[PIDSTRSIZE] = '\0';
				pid = atoi(pidbuf);
			}

			if
			(
				pid != 0
				&&
				(
					kill(pid, 0) != SYSERROR
					||
					errno != ESRCH
				)
			)
				return false;		/* Active lock */
		}

		Debug((1, "unlink dead lock: %s", file));

		if ( unlink(file) == SYSERROR && errno != ENOENT )
		{
			retry = -1;		/* Lock can't be broken */
			break;
		}
	}

	if ( retry < 0 )
	{
		(void)SysWarn("dead lock %s", file);
		return false;
	}

	keepfile(file);
	return true;

#	ifdef	O_CREAT
#	undef	tempfile
#	endif	/* O_CREAT */
}


/*
**      Put name in array of lock files.
*/

static void
keepfile(
	char *	name
)
{
	int	i;

	Trace((1, "keepfile(%s)", name));

	for ( i = 0 ; i < Nlocks ; i++ )
		if ( Lockfiles[i] == NULLSTR )
			break;

	if ( i == Nlocks )
	{
		if ( i >= MAXLOCKS )
		{
			ErrVal = EX_TEMPFAIL;
			Error("Too many locks");
		}

		++Nlocks;
	}

	Lockfiles[i] = newstr(name);
}


/*
**	Convert name to lockfile path.
*/

static char *
name_to_lock(
	char *		name,
	char *		lock
)
{
	char *		cp;
	struct stat	statb;

	if ( (cp = strrchr(name, '/')) != NULLSTR )
		++cp;
	else
		cp = name;

	if ( !LOCKNAMEISDEV || stat(name, &statb) == SYSERROR )
		(void)sprintf(lock, Lckfmt, LOCKDIR, LOCKPRE, cp);
	else
		(void)sprintf
		(
			lock,
			Lckdevfmt,
			LOCKDIR,
			LOCKPRE,
			(unsigned long)major(statb.st_dev),
			(unsigned long)major(statb.st_rdev),
			(unsigned long)minor(statb.st_rdev)
		);

	Trace((3, "name_to_lock(%s) => %s", name, lock));
	return lock;
}


/*
**	Try making a lock.
*/

static bool
trylock(
	char *	tempfile,
	char *	file
)
{
	int	fd;
	bool	val;
#	if	DEBUG
	static char	errfmt[]	= "trylock(%s) %s";
#	endif	/* DEBUG */

#	ifdef	O_CREAT
	if ( (fd = open(file, O_CREAT|O_EXCL|O_WRONLY, LOCK_MODE)) == SYSERROR )
#	else	/* O_CREAT */
	if ( (fd = creat(tempfile, LOCK_MODE)) == SYSERROR )
#	endif	/* O_CREAT */
	{
		Trace((3, errfmt, tempfile, "FAIL"));
		return false;
	}

	val = true;

	if ( LOCKPIDISSTR )
	{
		char	pidbuf[PIDSTRSIZE+1];	/* "nnnnnnnnnn\n\0" */

		(void)sprintf(pidbuf, "%*d\n", PIDSTRSIZE-1, Pid);

		if ( write(fd, pidbuf, PIDSTRSIZE) != PIDSTRSIZE )
			val = false;
	}
	else
		if ( write(fd, (char *)&Pid, sizeof(Pid)) != sizeof(Pid) )
			val = false;

	(void)close(fd);

#	ifdef	O_CREAT
	if ( !val )
		(void)unlink(file);
#	else	/* O_CREAT */
	if ( val && link(tempfile, file) == SYSERROR )
		val = false;
	(void)unlink(tempfile);
#	endif	/* O_CREAT */

	Trace((3, errfmt, file, val?"SUCCEED":"FAIL"));

	return val;
}
