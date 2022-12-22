/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Scan all work directories for work for `uucico'.
**
**	$Log: uusched.c,v $
 * Revision 1.3  1994/01/31  01:27:14  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:58:53  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:43:05  vixie
 * Initial revision
 *
 * Revision 1.2  1993/02/28  15:39:13  pace
 * Change install directory from /usr/sbin to /usr/libexec; don't mistake an
 * LCK file for a system name.
 *
 * Revision 1.1.1.1  1992/09/28  20:09:01  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1.1.1  1992/07/29  23:01:16  trent
 * UUCP from UUNET (as integrated by DOnn.)
 *
 * Revision 1.2  1992/04/17  21:36:58  piers
 * fix status file locking
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

static char	rcsid[]	= "$Id: uusched.c,v 1.3 1994/01/31 01:27:14 donn Exp $";

#define	ARGS
#define	ERRNO
#define	EXECUTE
#define	FILES
#define	FILE_CONTROL
#define	NDIR
#define	SIGNALS
#define	STDIO
#define	SYSEXITS

#include	"global.h"

/*
**	Parameters set from arguments.
*/

int	Debugflag;			/* Debug level */
bool	IgnoreTimeToCall;		/* Call anyway */
char *	Name		= rcsid;	/* Program invoked name (rcsid => AVersion) */
int	Traceflag;			/* Trace level */

/*
**	Arguments.
*/

static Args	Usage[]	=
{
	Arg_0(0, getName),
	Arg_bool(i, &IgnoreTimeToCall, 0),
	Arg_string(P, 0, 0, "paramsfile", OPTARG),
	Arg_int(T, &Traceflag, getInt1, "trace", OPTARG|OPTVAL),
	Arg_int(x, &Debugflag, getInt1, "debug", OPTARG|OPTVAL),
	Arg_end
};

/**	Booleans are repeated because ignored parameters can't be elided	**/

static Args	PUsage[] =
{
	Arg_0(0, 0),
	Arg_bool(i, 0, 0),
	Arg_bool(?, 0, 0),
	Arg_bool(\043, 0, 0),
	Arg_string(P, 0, getPARAMSFILE, NULLSTR, OPTARG),
	Arg_ignnomatch,
	Arg_end
};

/*
**	Lookup table for node names.
*/

#define	HASH_SIZE	67

typedef struct Entry	Entry;
struct Entry
{
	Entry *	great;
	Entry *	less;
	char	name[NODENAMEMAXSIZE+1];
};

static Entry *	NodeTable[HASH_SIZE];

/*
**	Status variables.
*/

static char *	StatusFile;
static int	StatusFd	= SYSERROR;

/*
**	Status files each have 2 lines:
**		node "in"  line oktime nowtime retrytime state count text...
**		node "out" line oktime nowtime retrytime state count text...
*/

static char	ReadFmt[]	= "%*s %*s %*s %lu %*lu %lu %d %d %*s";

/*
**	Miscellaneous definitions.
*/

int		Debugflag;		/* Debug level */
char		DefMaxGrade = '\177';	/* Default job priority */
char *		Invoker;
char		MaxGrade = '\177';	/* Required job priority */
char *		SenderName;		/* Used by ExpandArgs() */
char *		SourceAddress;		/* Used by ExpandArgs() */
int		Uid;
char *		UserName;
bool		LocalOnly;		/* Parameter for Uucico() */
bool		ReverseRole;		/* Parameter for Uucico() */
int		TurnTime;		/* Parameter for Uucico() */


static StatusType	CallOK(char *);

static void	ChkDebug(void);
static void	catch(int);
static void	catch_sig(int);
static void	close_status(void);
static bool	enter(char *, bool);
static void	fioclex(int);
static void	ignore(int);
static bool	open_status(char *);
static void	test_nologin(void);
static void	uusched(void);



/*
**	Argument processing.
*/

int
main(
	int	argc,
	char *	argv[],
	char *	envp[]
)
{
	SetNameProg(argc, argv, envp);

	DoArgs(argc, argv, PUsage);	/* Possible change to PARAMSFILE */

	InitParams();

	(void)NodeName();

	/*
	**	Set up user id.
	*/

	Uid = getuid();

	if ( !GetUser(&Uid, &Invoker, (char **)0) )
		Invoker = UUCPUSER;

	UserName = Invoker;

	/*
	**	Process args.
	*/

	DoArgs(argc, argv, Usage);

	LogNode = Name = "uusched";	/* Override invoked name */

	if ( Debugflag || Traceflag )
		ChkDebug();

	ignore(SIGHUP);
	ignore(SIGINT);
	catch(SIGTERM);

	/*
	**	Search for uucico's to fire up.
	*/

	uusched();

	finish(EX_OK);	/* Does rmlock(NULLSTR) */
}

static void
catch(int sig)
{
	(void)signal(sig, catch_sig);
}

static void
catch_sig(int sig)
{
	ignore(sig);
	finish(EX_OK);
}

/*
**	Turn off debugging if not allowed.
*/

static void
ChkDebug()
{
	if ( access(SYSFILE, R_OK) != SYSERROR )
		return;

	Traceflag = Debugflag = 0;
	ErrVal = EX_NOPERM;
	Error("debug requires read access to %s", SYSFILE);
}

/*
**	Close status file.
*/

static void
close_status()
{
	FreeStr(&StatusFile);

	if ( StatusFd == SYSERROR )
		return;

	(void)close(StatusFd);
	StatusFd = SYSERROR;
}

/*
**	Lookup name in table, enter if `add', return true if new.
*/

static bool
enter(
	char *			name,
	bool			add
)
{
	register char		c;
	register char *		cp1;
	register char *		cp2;
	register Entry **	epp;
	register Entry *	ep;

	for
	(
		epp = &NodeTable[HashName(name, HASH_SIZE)] ;
		(ep = *epp) != (Entry *)0 ;
	)
	{
		for ( cp1 = name, cp2 = ep->name ; c = *cp1++ ; )
			if ( c -= *cp2++ )
				break;

		if ( c == 0 && (c = *cp2) == 0 )
		{
			Trace((4, "enter(%s) exists", name));
			return false;
		}

		if ( c > 0 )
			epp = &ep->great;
		else
			epp = &ep->less;
	}

	if ( add )
	{
		*epp = ep = Talloc0(Entry);

		(void)strcpy(ep->name, name);

		Trace((3, "enter(%s)", name));
	}

	return true;
}

/*
**	Clean up.
*/

void
finish(
	int	error
)
{
	if ( !ExInChild )
		rmlock(NULLSTR);

	exit(error);
}

/*
**	Arrange to close fd on exec(2).
*/

static void
fioclex(
	int	fd
)
{
#	ifdef	F_SETFD
	if ( fcntl(fd, F_SETFD, 1) == SYSERROR )
		Debug((2, CouldNot, "F_SETFD", ErrnoStr(errno)));
#	endif	/* F_SETFD */
}

/*
**	Ignore signal.
*/

static void
ignore(int sig)
{
	(void)signal(sig, SIG_IGN);
}

/*
**	Open status file, and lock.
*/

static bool
open_status(
	char *	name
)
{
	if
	(
		name == NULLSTR
		||
		name[0] == '\0'
		||
		strcmp(name, NODENAME) == STREQUAL
	)
		return false;

	if ( StatusFd != SYSERROR )
		close_status();

	StatusFile = concat(STATUSDIR, name, NULLSTR);

	while ( (StatusFd = open(StatusFile, O_CREAT|O_RDWR, 0664)) == SYSERROR )
		if ( !CheckDirs(StatusFile) )
			SysError(CouldNot, CreateStr, StatusFile);

	Trace((2, "open_status(%s) => %s", name, StatusFile));

	while ( flock(StatusFd, LOCK_EX|LOCK_NB) == SYSERROR )
	{
		if ( errno == EWOULDBLOCK )
		{
			close_status();
			return false;
		}

		SysError(CouldNot, LockStr, StatusFile);
	}

	fioclex(StatusFd);

	return true;
}

/*
**	Test if shutdown required.
*/

static void
test_nologin()
{
	if ( access(NOLOGIN, 0) == SYSERROR )
		return;

	if ( !Debugflag )
		finish(EX_UNAVAILABLE);

	Debug((1, "%s: continuing anyway DEBUGGING", NOLOGIN));
}

/*
**	Find node with messages to be sent, and invoke specific `uucico'.
*/

static void
uusched()
{
	register struct direct *dp2;
	register DIR *		dirp2;
	register struct direct *dp;
	register DIR *		dirp;
	register int		i;
	char *			dir;
	char			lockfile[PATHNAMESIZE];

	static char		scfmt[]		= "scanning for work: %s%s";

	Trace((1, "%s%s", Name, "()"));

	/** Establish lock **/

	Debug((1, "Max %ss => %d", Name, MAXUUSCHEDS));

	for ( i = MAXUUSCHEDS ; --i >= 0 ; )
	{
		(void)sprintf(lockfile, "%s.%d", X_UUSCHED, i+1);

		if ( mklock(lockfile) )
			break;
	}

	if ( i < 0 )
	{
		Debug((1, "Too many %ss already running", Name));
		finish(EX_OK);
	}

	/** Load SYSFILE **/

	LoadSys(NEEDSYS);		/* And accumulate system fields */

	/** Scan SPOOLALTDIRS for nodes **/

	for ( dir = NULLSTR ; (dir = ChNodeDir(NULLSTR, dir)) != NULLSTR ; )
	{
		static char	stfmt[]	= "started uucico -cr1 -s%s";

		if ( (dirp = opendir(".")) == (DIR *)0 )
		{
			(void)SysWarn(CouldNot, ReadStr, dir);
			continue;
		}

		setproctitle(scfmt, dir, EmptyStr);

		while ( (dp = readdir(dirp)) != (struct direct *)0 )
		{
			int	l;

			if ( dp->d_name[0] == '.' )
				continue;

			if ( strncmp(dp->d_name, LOCKPRE, strlen(LOCKPRE))
			     == STREQUAL
			    )
				continue;

			if ( (l = strlen(dp->d_name)) > NODENAMEMAXSIZE )
			{
				Debug((1, "name too long: %s", ExpandString(dp->d_name, l)));
				continue;
			}

			test_nologin();

			if ( SpoolAltDirs > 1 && !enter(dp->d_name, false) )
				continue;

			if ( (dirp2 = opendir(dp->d_name)) == NULL )
			{
				if ( errno != ENOENT )	/* Rmdir'd? */
					(void)SysWarn(CouldNot, ReadStr, dp->d_name);
				continue;
			}

			Debug((2, scfmt, dir, dp->d_name));

			while ( (dp2 = readdir(dirp2)) != (struct direct *)0 )
			{
				char *	name;

				if ( dp2->d_name[0] != CMD_TYPE )
					continue;

				if ( SpoolAltDirs > 1 )
					(void)enter(dp->d_name, true);

				name = dp->d_name;

				if ( !FindSysEntry(&name, NEEDSYS, NOALIAS) )
					break;		/* Unknown system! */

				if ( !IgnoreTimeToCall && CheckSysTime(name, Master) == CF_TIME )
					break;		/* Wrong time to call */

				if ( CallOK(dp->d_name) != SS_OK )
					break;

				setproctitle(stfmt, dp->d_name);

				Uucico(dp->d_name, WAIT);

				setproctitle(scfmt, dir, EmptyStr);
				break;
			}

			closedir(dirp2);
		}

		closedir(dirp);
	}
}

/*
**	Find out if call can be made.
*/

static StatusType
CallOK(
	char *	node
)
{
	int	count;
	char *	indata;
	Time_t	lasttime;
	int	mode;
	char *	outdata;
	Time_t	retrytime;

	Trace((1, "CallOK(%s)", node));

	mode = (int)SS_OK;

	if ( !open_status(node) )
		return SS_INPROGRESS;	/* LOCKED */

	while ( (indata = ReadFd(StatusFd)) != NULLSTR )
	{
		if
		(
			(outdata = strchr(indata, '\n')) == NULLSTR
			||
			strchr(++outdata, '\n') != NULLSTR
		)
		{
			DebugT(1, (1,
				"CallOK bad status format:\n%s",
				ExpandString(indata, RdFileSize)
			));
			(void)unlink(StatusFile);
			free(indata);
			break;
		}

		Trace((6, "scan line: %s", outdata));

		(void)sscanf(outdata, ReadFmt, &lasttime, &retrytime, &mode, &count);

		switch ( mode )
		{
		default:
			mode = (int)SS_OK;
			break;

		case SS_WRONGTIME:
		case SS_FAIL:
			mode = (int)SS_OK;

			SetTimes();
			if ( (Time - lasttime) >= retrytime )
				break;

			Debug((4, "NO CALL: RETRY TIME (%lu) NOT REACHED", retrytime));

			if ( Debugflag )
				Debug((4, "debugging: continuing anyway"));
			else
				mode = (int)SS_WRONGTIME;
			break;
		}

		free(indata);
		break;
	}

	close_status();

	Debug((2, "CallOK => %d", mode));
	return (StatusType)mode;
}
