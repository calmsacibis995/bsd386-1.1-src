/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	`do_uucp' - real subroutine to execute commands on remote.
**
**	RCSID $Id: do_uucp.c,v 1.2 1993/02/28 15:37:12 pace Exp $
**
**	$Log: do_uucp.c,v $
 * Revision 1.2  1993/02/28  15:37:12  pace
 * Improve error messages.
 *
 * Revision 1.1.1.1  1992/09/28  20:08:57  trent
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
#define	PARAMS
#define	SIGNALS
#define	STDIO
#define	SYSEXITS
#define	SYS_STAT

#include	"global.h"
#include	"uucp.h"

/*
**	Defaults:
*/

#define	MAXFILESPERCMD	15		/* Max. files/command file */
#define	MAXBYTESPERCMD	50000		/* Max. bytes/command file */
#define	FileMetaChars	"*?["		/* Shell meta-chars for filename expansion */

/*
**	Created files.
*/

static char *	CmdFile;		/* Remote commands file */
static char *	CmdPath;		/* Remote commands path */

/*
**	Environment for commands.
*/

static char	Path[PATHNAMESIZE]	= "PATH=/bin:/usr/bin:/usr/ucb";
#define		PATHP				5

static char *	Xenvs[] = {
	Path,
	NULLSTR
};

/*
**	Miscellaneous definitions.
*/

static Ulong	DataSize;		/* Total size of files */
static char *	FromNode;		/* Source node for copy() */
bool		LocalOnly;		/* Parameter for Uucico() */
static bool	NoClean;		/* Set to prevent unlinks on error exit */
static char *	ForeignNode;		/* Foreign node if only one */
static int	ForeignNodes;		/* Foreign node count */
bool		IgnoreTimeToCall;	/* Parameter for Uucico() */
bool		ReverseRole;		/* Parameter for Uucico() */
static bool	SendCmds;		/* Commands in TempCfile */
static struct stat Statb;		/* Global stat buffer filled by stat_file() */
static char *	ToFile;			/* Destination file name */
static char *	ToNode;			/* Destination node for ToFile */
int		TurnTime;		/* Parameter for Uucico() */

bool		UUCPReRun;		/* Set true if will be called > once */

char *		SourceAddress;		/* Used by ExpandArgs() */
char *		SenderName;		/* Used by ExpandArgs() */

static char	BadSysName[]	= "Bad system name: %s";
static char	NoPermission[]	= "Permission denied [%s (%s, %s)].\n";
static char	NoAccess[]	= "Can't %s file (%s) mode (%o).\n";

static bool	check_read(char *);	/* Check read permission */
static bool	check_write(char *);	/* Check write permission */
static bool	copy(char *, int);	/* Copy file/name to command */
static void	make_cmd_file(char *);	/* Create command files */
static void	new_node(char *);	/* Register new node for transfer */
static bool	stat_file(char *);	/* Check stat call on file */
static int	Uux(char *, char *, char *);

#define	Fprintf	(void)fprintf

#if	DEBUG
extern int	WorkFiles;
#endif	/* DEBUG */

/*
**	The real thing.
*/

void
do_uucp()
{
	char *	cp;
	bool	cache_sys;

	Trace((2, "do_uucp()"));

	if ( Debugflag )
		MesgN("START", NULLSTR);

	DataSize = 0;
	ForeignNode = NULLSTR;
	ForeignNodes = 0;
	NoClean = false;
	SendCmds = false;
#	if	DEBUG
	WorkFiles = 0;
#	endif	/* DEBUG */

	if ( (cp = getenv(Path)) != NULLSTR )
		(void)strcpy(&Path[PATHP], cp);
	NewEnvs = Xenvs;

	if ( UUCPReRun )
		cache_sys = true;
	else
	{
		register int	i;
		register int	n;

		cache_sys = false;

		for ( n = 0, i = FileCount ; --i >= 0 ; )
			if ( strchr(Files[i], '!') != NULLSTR )
				if ( ++n > 1 )
				{
					cache_sys = true;
					break;
				}
	}

	if ( cache_sys )
		LoadSys(NOSYS);

	FreeStr(&ToNode);

	/*
	**	Find `to' nodename.
	*/

	ToFile = Files[--FileCount];

	if ( !SplitSysName(ToFile, &ToNode, ToFile) )
		ToNode = newstr(NODENAME);	/* Default is here */
	else
	if ( !FindSysEntry(&ToNode, NOSYS, NEEDALIAS) )
	{
		ErrVal = EX_NOHOST;
		if ( !LOG_BAD_SYS_NAME )
			NoLog = true;
		Error(BadSysName, ToNode);
	}

	LogNode = ToNode;

	/*
	**	No multi-hop...
	*/

	if ( strchr(ToFile, '!') != NULLSTR )
	{
		fprintf
		(
			stderr, "\
`uucp' handles only adjacent sites.\n\
Try `uusend' for multi-hop delivery.\n"
		);

		finish(EX_USAGE);
	}

	/*
	**	Files processing.
	*/

	while ( --FileCount >= 0 )
	{
		char *	fromfile = Files[FileCount];
		int	type;

		FreeStr(&FromNode);

		if ( !SplitSysName(fromfile, &FromNode, fromfile) )
		{
			FromNode = newstr(NODENAME);	/* Default is here */
			type = 0;
		}
		else
		if ( !FindSysEntry(&FromNode, NOSYS, NEEDALIAS) )
		{
			ErrVal = EX_NOHOST;
			if ( !LOG_BAD_SYS_NAME )
				NoLog = true;
			Error(BadSysName, FromNode);
		}
		else
			type = 1;

		if ( !copy(fromfile, type) )
			finish(EX_DATAERR);
	}

	(void)WriteCmds();	/* Write out all commands generated so far */

	/*
	**	Start I/O.
	*/

	if ( ForeignNodes && StartJob )
		Uucico(ForeignNode, NOWAIT);

	if ( UUCPReRun )
		WfFree();	/* ...file + ...path */

	Trace((2, "Used %d work files", WorkFiles));
	if ( Debugflag )
		MesgN("FINISH", NULLSTR);
}

/*
**	Check directory write permission.
*/

static bool
check_dir(
	char *		file
)
{
	char *		cp;
	int		r;
	struct stat	statb;

	if ( (cp = strrchr(file, '/')) == NULLSTR )
		return false;

	*cp = '\0';
	r = stat(file, &statb);
	*cp = '/';

	if ( r != SYSERROR )
	{
		if ( statb.st_mode & 02 )
			return true;
		return false;
	}

	if ( !CreateDirs )
		return false;

	return MkDirs(file, NetUid, NetGid);
}

/*
**	Check file read permission.
*/

static bool
check_read(
	char *	file
)
{
	if ( Statb.st_mode & 04 )
		return true;

	Fprintf(stderr, NoAccess, ReadStr, file, Statb.st_mode);
	return false;
}

/*
**	Check file write permission.
*/

static bool
check_write(
	char *	file
)
{
	if ( Statb.st_mode & 02 )
		return true;

	Fprintf(stderr, NoAccess, WriteStr, file, Statb.st_mode);
	return false;
}

/*
**	Generate file copy.
*/

static bool
copy(
	char *		file,
	int		type
)
{
	register char *	cp;
	char		f1[PATHNAMESIZE];
	char		f2[PATHNAMESIZE];
	char		*f;
	struct stat	statb;

	Trace((1, 
		"copy(%s, %d)\n\t\t[ToFile=%s, ToNode=%s, FromNode=%s]",
		file, type, ToFile, ToNode, FromNode
	));

	if ( type && strpbrk(file, FileMetaChars) != NULLSTR )
		type = 4;
	else
	if ( strcmp(ToNode, NODENAME) != STREQUAL )
		type += 2;

	(void)strcpy(f1, file);
	(void)strcpy(f2, ToFile);

	switch ( type )
	{
	case 0:	/** Both files here **/
		Debug((4, "all work here %s %s", f1, f2));

		(void)ExpFn(f1);
		if ( !stat_file(f1) )
			return false;
		if ( !check_read(f1) )
			return false;
		statb = Statb;

		(void)ExpFn(f2);
		if ( stat_file(f2) )
		{
			if ( Statb.st_ino == statb.st_ino && Statb.st_dev == statb.st_dev )
			{
				Fprintf(stderr, "%s %s - same file; can't copy.\n", f1, f2);
				return false;
			}
			if ( !check_write(f2) )
				return false;
		}
		else
		if ( !check_dir(f2) )
		{
			Fprintf(stderr, NoPermission,
				"bad dir", "destination", f2);
			finish(EX_NOPERM);
		}
		if
		(
			((f = f1) && (!CheckUserPath(UserName, NULLSTR, f1)))
			||
			((f = f2) && (!CheckUserPath(UserName, NULLSTR, f2)))
		)
		{
			Fprintf(stderr, NoPermission,
				"user path", UserName, f);
			finish(EX_NOPERM);
		}
		CopyFileToFile(f1, f2);
		if ( FILE_MODE != 0666 )
			(void)chmod(f2, 0666);
		Log("DONE WORK HERE");
		break;

	case 1:	/** Receive file **/
		Debug((4, "receive file %s %s", f1, f2));
		if ( f1[0] != '~' )
			ExpFn(f1);
		ExpFn(f2);
		if ( !CheckUserPath(UserName, NULLSTR, f2) )
		{
			Fprintf(stderr, NoPermission,
				"user path", UserName, f2);
			return false;
		}
		if ( RmtNode != NULLSTR )
		{
			if ( Uux(NULLSTR, f1, f2) != EX_OK )
				return false;
			return true;
		}
		make_cmd_file(FromNode);
		QueueCmd(CmdPath, "R %s %s %s -%s\n", f1, f2, UserName, Optns);
		new_node(FromNode);
		break;

	case 2:	/** Send file **/
		Debug((4, "send file %s %s", f1, f2));
		ExpFn(f1);
		if ( !stat_file(f1) )
			return false;
		if ( !check_read(f1) )
			return false;
		if ( (Statb.st_mode & S_IFMT) == S_IFDIR )
		{
			Fprintf(stderr, "Directory name illegal - %s.\n", f1);
			return false;
		}
		if ( !CheckUserPath(UserName, NULLSTR, f1) )
		{
			Fprintf(stderr, NoPermission,
				"user path", UserName, f1);
			return false;
		}
		if ( f2[0] != '~' )
			ExpFn(f2);
		if ( RmtUser != NULLSTR && Optns[LastOptn] == '\0' )
		{
			Optns[LastOptn] = 'n';
			Optns[LastOptn+1] = '\0';
		}
		if ( RmtNode != NULLSTR )
		{
			if ( Uux(RmtUser, f1, f2) != EX_OK )
				return false;
			return true;
		}
		make_cmd_file(ToNode);
		if ( CopyFiles )
		{
			char *	fp;

			cp = WfName(DATA_TYPE, Grade, NODENAME);
			fp = WfPath(cp, DATA_TYPE, ToNode);

			if ( !CopyFileToFile(f1, fp) )
			{
				Fprintf(stderr, "Can't copy %s.\n", f1);
				return false;
			}
		}
		else
			cp = DUMMY_NAME;
		if ( (DataSize += Statb.st_size) < MinInput )
		{
			ErrVal = EX_NOINPUT;
			Error("data size (%lu) < minimum requested (%lu)", DataSize, MinInput);
		}
		QueueCmd
		(
			CmdPath,
			"S %s %s %s -%s %s %o %s\n",
			f1,
			f2,
			UserName,
			Optns,
			cp,
			Statb.st_mode & 0777,
			(RmtUser==NULLSTR)?EmptyStr:RmtUser
		);
		new_node(ToNode);
		break;

	case 3:
	case 4:	/** Send uucp command for execution on `FromNode' **/
		Debug((4, "send uucp command %s %s", f1, f2));
		if ( strcmp(ToNode, NODENAME) == STREQUAL )
		{
			ExpFn(f2);
			if ( !CheckUserPath(UserName, NULLSTR, f2) )
			{
				Fprintf(stderr, NoPermission,
					"user path", UserName, f2);
				return false;
			}
		}
		if ( RmtNode != NULLSTR )
		{
			if ( Uux(NULLSTR, f1, f2) != EX_OK )
				return false;
			return true;
		}
		make_cmd_file(FromNode);
		QueueCmd
		(
			CmdPath,
			"X %s %s!%s %s -%s\n",
			f1,
			ToNode,
			f2,
			UserName,
			Optns
		);
		new_node(FromNode);
		break;
	}

	return true;
}

/*
**	Cleanup for error routines.
*/

void
finish(int error)
{
	/** Unlink any files here **/

	if ( error && !ExInChild && !NoClean )
		UnlinkAllWf();

	if ( error && Debugflag )
		MesgN("ERROR", "code %d", error);

	(void)exit(error);
}

/*
**	Create command files (make a new one every MAX limit).
*/

static void
make_cmd_file(
	char *		node
)
{
	static char *	last_node;
	static int	filecount;

	if
	(
		++filecount <= MAXFILESPERCMD
		&&
		DataSize <= MAXBYTESPERCMD
		&&
		CmdPath != NULLSTR
		&&
		strcmp(node, last_node) == STREQUAL
	)
		return;

	last_node = node;
	filecount = 1;
	DataSize = 0;

	CmdPath = UniqueWf(&CmdFile, CMD_TYPE, Grade, node, node);

	Log("QUE'D %s", CmdFile);
}

/*
**	Keep track of foreign nodes.
*/

static void
new_node(
	char *	node
)
{
	if ( ForeignNodes > 1 )
		return;

	if ( ForeignNodes == 0 )
	{
		ForeignNode = node;
		ForeignNodes = 1;
	}
	else
	if ( strcmp(node, ForeignNode) != STREQUAL )
	{
		ForeignNode = NULLSTR;
		ForeignNodes = 2;
	}
}

/*
**	Stat file, results to Statb.
*/

static bool
stat_file(
	char *	file
)
{
	Trace((2, "stat_file(%s)", file));

	while ( stat(file, &Statb) == SYSERROR )
		if ( !SysWarn(CouldNot, StatStr, file) )
			return false;

	return true;
}

/*
**	Execute UUX for remote file copy.
*/

static int
Uux(
	char *		rusr,
	char *		f1,
	char *		f2
)
{
	register int	n;
	register int	i;
	ExBuf		args;
	static char	bang[]	= "%s!%s";
	static char	pbang[]	= "(%s!%s)";

	if ( UUX == NULLSTR )
		return EX_OK;

	FIRSTARG(&args.ex_cmd) = UUX;

	if ( !StartJob )
		NEXTARG(&args.ex_cmd) = "-r";

	i = NARGS(&args.ex_cmd);	/* These should not be free'd */

	if ( NewParamsFile )
		NEXTARG(&args.ex_cmd) = concat("-P", PARAMSFILE, NULLSTR);

#	if	DEBUG
	if ( Traceflag > 0 )
		NEXTARG(&args.ex_cmd) = newprintf("-T%d", Traceflag);
#	endif	/* DEBUG */

	if ( Debugflag > 0 )
		NEXTARG(&args.ex_cmd) = newprintf("-x%d", Debugflag);

	ExpandArgs(&args.ex_cmd, UUXARGS);

	NEXTARG(&args.ex_cmd) = newprintf(bang, RmtNode, "uucp");

	if ( rusr != NULLSTR )
		NEXTARG(&args.ex_cmd) = newprintf("-n%s", rusr);

	NEXTARG(&args.ex_cmd) = newprintf(bang, FromNode, f1);
	NEXTARG(&args.ex_cmd) = newprintf(pbang, ToNode, f2);

	n = NARGS(&args.ex_cmd);	/* These should be free'd */

	if ( (f1 = Execute(&args, ExIgnSigs, ex_exec, SYSERROR)) != NULLSTR )
	{
		Warn(f1);
		free(f1);
	}

	while ( --n >= i )
		free(ARG(&args.ex_cmd, n));

	return ExStatus;
}
