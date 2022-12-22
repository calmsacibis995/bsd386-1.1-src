/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	`do_uux' - real subroutine to execute commands on remote.
**
**	RCSID $Id: do_uux.c,v 1.1.1.1 1992/09/28 20:09:02 trent Exp $
**
**	$Log: do_uux.c,v $
 * Revision 1.1.1.1  1992/09/28  20:09:02  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	ARGS
#define	ERRNO
#define	EXECUTE
#define	FILES
#define	FILE_CONTROL
#define	SETJMP
#define	STDIO
#define	SYSEXITS
#define	SYS_STAT

#include	"global.h"
#include	"uux.h"

/*
**	Command format strings.
*/

static char	CntlCmdFmt[]	= "%c\n";
static char	Fil2CmdFmt[]	= "%c %s %s\n";
static char	FileCmdFmt[]	= "%c %s\n";
static char	RecvCmdFmt[]	= "R %s %s %s - \n";
static char	SendCmdFmt[]	= "S %s %s %s -%s %s 0666\n";

/*
**	Created files.
*/

static char *	RemoteXfile;		/* Remote commands file */
static char *	RemoteXpath;		/* Remote commands path */
static char *	TempCfile;		/* Local commands accumulation file */
static char *	TempCpath;		/* Local commands accumulation path */

/*
**	Miscellaneous definitions.
*/

static Ulong	DataSize;		/* Total size of files */
static int	FileMode;		/* st_mode from last get_size() */
bool		IgnoreTimeToCall;	/* Parameter for Uucico() */
static char *	ParsedString;		/* Collected commands after processing */
static bool	ReceiveCmds;		/* C. files for receiving */
bool		ReverseRole;		/* Parameter for Uucico() */
static bool	SendCmds;		/* Commands in TempCfile */
int		TurnTime;		/* Parameter for Uucico() */

bool		NoClean;		/* Set to prevent unlinks on error exit */
bool		UUXReRun;		/* Set true if will be called > once */
char *		XqtNode;		/* Nodename of command */

char *		SourceAddress;		/* Used by ExpandArgs() */
char *		SenderName;		/* Used by ExpandArgs() */

static char	BadSysName[]	= "Bad system name: %s";

static void	start_cmds();		/* Build remote exec commands */
static void	pipe_cmds();		/* Build commands for pipe input */
static void	parse();		/* Parse command args */
static void	get_size(char *);	/* Stat file for size */
static char *	basename(char *);	/* `basename path` */

#if	DEBUG
extern int	WorkFiles;
#endif	/* DEBUG */

/*
**	The real thing.
*/

void
do_uux()
{
	register char *	arg;
	register char *	cp;
	static char	xqtqed[] = "XQT QUE'D %s";

	Trace((2, "do_uux()"));

	if ( Debugflag )
		MesgN("START", NULLSTR);

	DataSize = 0;
	NoClean = false;
	ReceiveCmds = false;
	SendCmds = false;
	FreeStr(&XqtNode);
#	if	DEBUG
	WorkFiles = 0;
#	endif	/* DEBUG */

	/*
	**	Find remote execute nodename.
	*/

	arg = Malloc(CmdLen);

	for ( cp = CmdString ; (cp = NextArg(cp, arg)) != NULLSTR ; )
	{
		/** 1st non-i/o arg. is command **/

		if ( arg[0] != '<' && arg[0] != '>' )
		{
			(void)SplitSysName(arg, &XqtNode, arg);
			break;
		}

		/** Throw away filename **/

		if ( (cp = NextArg(cp, NULLSTR)) == NULLSTR )
			break;
	}

	free(arg);

	if ( ACCESS_REMOTE_FILES || UUXReRun )
		LoadSys(NOSYS);

	if ( XqtNode == NULLSTR )
		XqtNode = newstr(NODENAME);	/* Default is here */
	else
	if ( !FindSysEntry(&XqtNode, NOSYS, NEEDALIAS) )
	{
		ErrVal = EX_NOHOST;
		if ( !LOG_BAD_SYS_NAME )
			NoLog = true;
		Error(BadSysName, XqtNode);
	}

	Debug((1, "NODE %s", XqtNode);
	LogNode = XqtNode;

	/*
	**	Build command file names.
	*/

	RemoteXpath = UniqueWf(&RemoteXfile, DATA_TYPE, 'X', NODENAME, XqtNode);

	TempCfile = WfName(DATA_TYPE, 'T', NODENAME);
	TempCpath = WfPath(TempCfile, DATA_TYPE, XqtNode);

	/*
	**	Command processing.
	*/

	start_cmds();

	if ( PipeIn )
		pipe_cmds();

	parse();

	if ( DataSize < MinInput )
	{
		ErrVal = EX_NOINPUT;
		Error("data size (%lu) < minimum requested (%lu)", DataSize, MinInput);
	}

	/*
	**	Change job priority.
	*/

	if ( DataSize > 0 && GradeDelta != 0 && GradeDelta < MAX_ULONG )
	{
		int	delta = DataSize/GradeDelta;

		if
		(
			(cp = strchr(ValidChars, Grade)) == NULLSTR
			||
			&cp[delta] >= &ValidChars[VC_size]
		)
			Grade = 'z';
		else
			Grade = cp[delta];

		Debug((1, "Grade => %c", Grade));
	}

	/*
	**	Start processing.
	*/

	arg = WfName(EXEC_TYPE, Grade, NODENAME);
	cp = WfPath(arg, EXEC_TYPE, XqtNode);

	if ( strcmp(XqtNode, NODENAME) == STREQUAL )
	{
		(void)WriteCmds();	/* Write out all commands generated so far */
		MoveCp(RemoteXpath, cp);

		NoClean = true;
		Log(xqtqed, ParsedString);

		if ( StartJob )
			if ( ReceiveCmds )
			{
				Uucico(XqtNode, NOWAIT);
				StartJob  = false;	/* Avoid 2nd uucico below */
			}
			else
				Uuxqt(XqtNode);
	}
	else
	{
		QueueCmd(TempCpath, SendCmdFmt, RemoteXfile, arg, Invoker, EmptyStr, RemoteXfile);
		SendCmds = true;

		(void)WriteCmds();	/* Write out all commands generated so far */

		NoClean = true;
		Log(xqtqed, ParsedString);
	}

	if ( SendCmds )
	{
		arg = WfName(CMD_TYPE, Grade, XqtNode);
		cp = WfPath(arg, CMD_TYPE, XqtNode);

		MoveCp(TempCpath, cp);

		if ( StartJob )
			Uucico(XqtNode, NOWAIT);
	}

	if ( UUXReRun )
	{
		WfFree();	/* ...file + ...path */
		free(ParsedString);
	}

	Trace((2, "Used %d work files", WorkFiles));
	if ( Debugflag )
		MesgN("FINISH", XqtNode));
}

/*
**	Cleanup for error routines.
*/

void
finish(int error)
{
	/** Unlink any files here **/

	if ( error && !ExInChild )
	{
		if ( !NoClean )
			UnlinkAllWf();

		if ( ErrFlag == ert_here )
			longjmp(ErrBuf, error);
	}

	if ( Debugflag || (error && !ExplainArgs && !ExplVersion) )
		MesgN(error?"ERROR":"exit", "node %s, code %d", XqtNode, error);

	(void)exit(error);
}

static void
start_cmds()
{
	char *	cp;

	Trace((2, "start_cmds()"));

	QueueCmd(RemoteXpath, Fil2CmdFmt, X_USER, Invoker, NODENAME);

	if ( NoMail )
		QueueCmd(RemoteXpath, CntlCmdFmt, X_NONOTI);

	if ( NotifyOnFail )
		QueueCmd(RemoteXpath, CntlCmdFmt, X_NONZERO);

	if ( UserName == NULLSTR || UserName[0] == '\0' )
		UserName = Invoker;

	QueueCmd(RemoteXpath, FileCmdFmt, X_RETURNTO, UserName);
}

static void
pipe_cmds()
{
	char *	datafile;
	char *	tempfile;
	bool	freetemp;
	int	fd;
	Ulong	size = MAX_ULONG;

	Trace((2, "pipe_cmds()"));

	datafile = WfName(DATA_TYPE, 'B', NODENAME);
	tempfile = WfPath(datafile, DATA_TYPE, XqtNode);

	fd = CreateWf(tempfile);

	if ( !CopyFdToFd(fileno(stdin), "stdin", fd, tempfile, &size) )
		finish(EX_IOERR);

	(void)close(fd);

	DataSize += size;
	Trace((2, "size = %lu", DataSize));

	if ( strcmp(NODENAME, XqtNode) == STREQUAL )
	{
		tempfile = datafile;
		freetemp = false;
	}
	else
	{
		tempfile = WfName(DATA_TYPE, 'S', NODENAME);
		QueueCmd(TempCpath, SendCmdFmt, datafile, tempfile, Invoker, EmptyStr, datafile);
		SendCmds = true;
		freetemp = true;
	}

	QueueCmd(RemoteXpath, FileCmdFmt, X_RQDFILE, tempfile);
	QueueCmd(RemoteXpath, FileCmdFmt, X_STDIN, tempfile);

	if ( freetemp )
		free(tempfile);
}

/*
**	Parse command.
*/

static void
parse()
{
	register char *	cp;
	register char *	p;
	char *		arg;
	char *		sysname;
	char *		filename;
	char		redirectchar;
	bool		hassysname;

	static char	allfileslocal[]	= "all files must be local or on exec node";
	static char	multihopmsg[]	= "\n\
uux handles only adjacent sites.\n\
try uusend for multi-hop delivery.";

	Trace((2, "parse()"));

	ParsedString = p = Malloc(CmdLen+1024);

	arg = Malloc(CmdLen);
	filename = Malloc(256);	/* Allow for expansion */
	sysname = NULLSTR;

	for ( cp = CmdString, redirectchar = '\0' ; (cp = NextArg(cp, arg)) != NULLSTR ; )
	{
		Debug((4, "prm - %s", arg));

		if ( arg[0] == '<' || arg[0] == '>' )
		{
			redirectchar = arg[0];
			continue;
		}

		if ( arg[0] == ';' )
		{
			p = strcpyend(p, arg);
			*p++ = ' ';
			redirectchar = '\0';
			continue;
		}

		if ( arg[0] == '|' )
		{
			if ( p != ParsedString )
			{
				p = strcpyend(p, arg);
				*p++ = ' ';
			}
			redirectchar = '\0';
			continue;
		}

		/** Command / file / option  **/

		if ( !(hassysname = SplitSysName(arg, &sysname, filename)) )
			sysname = newstr(NODENAME);

		Debug((4, "sysname - %s, rest - %s, return - %d", sysname, filename, (int)hassysname));

		if ( p == ParsedString && redirectchar == '\0' )
		{
			/** Command **/

			if ( strchr(filename, '!') != NULLSTR )
			{
				ErrVal = EX_USAGE;
				Error(multihopmsg);
			}

			p = strcpyend(p, filename);
			*p++ = ' ';
			continue;
		}

		if ( redirectchar == '>' )
		{
			/** File **/

			if ( filename[0] != '~' )
				ExpFn(filename);

			QueueCmd(RemoteXpath, Fil2CmdFmt, X_STDOUT, filename, sysname);
			redirectchar = '\0';
			continue;
		}

		if ( !hassysname && redirectchar == '\0' )
		{
			/** Option **/

			p = strcpyend(p, filename);
			*p++ = ' ';
			continue;
		}

		if
		(
			strcmp(XqtNode, NODENAME) == STREQUAL
			&&
			strcmp(XqtNode, sysname) == STREQUAL
		)
		{
			ExpFn(filename);
			get_size(filename);

			if ( redirectchar == '<' )
			{
				QueueCmd(RemoteXpath, FileCmdFmt, X_STDIN, filename);
				redirectchar = '\0';
			}
			else
			{
				p = strcpyend(p, filename);	/* Option */
				*p++ = ' ';
			}

			continue;
		}

		if ( strcmp(sysname, NODENAME) == STREQUAL )
		{
			bool	notlinked;
			char *	datafile;	/* Last component of work file */
			char *	datapath;	/* Full pathname for `datafile' */

			/** Send local file **/

			ExpFn(filename);
			get_size(filename);	/* Sets FileMode */

			if
			(
				(FileMode & 04) == 0
				||
				!CheckUserPath(Invoker, NULLSTR, filename)
			)
			{
				ErrVal = EX_NOPERM;
				Error("%s: permission denied", filename);
			}

			datafile = WfName(DATA_TYPE, 'A', NODENAME);
			datapath = WfPath(datafile, DATA_TYPE, sysname);

			notlinked = false;

			if ( LinkFiles )
			{
				if ( link(filename, datapath) == SYSERROR )
					notlinked = true;
				else
					QueueCmd(TempCpath, SendCmdFmt, filename, datafile, Invoker, EmptyStr, datafile);
			}

			if ( CopyFiles || notlinked )
			{
				if ( !CopyFileToFile(filename, datapath) )
					finish(EX_NOINPUT);

				QueueCmd(TempCpath, SendCmdFmt, filename, datafile, Invoker, EmptyStr, datafile);
			}

			if ( !CopyFiles && !LinkFiles )
				QueueCmd(TempCpath, SendCmdFmt, filename, datafile, Invoker, "c", DUMMY_NAME);

			SendCmds = true;

			if ( redirectchar == '<' )
			{
				QueueCmd(RemoteXpath, FileCmdFmt, X_STDIN, datafile);
				QueueCmd(RemoteXpath, FileCmdFmt, X_RQDFILE, datafile);
				redirectchar = '\0';
			}
			else
			{
				char *	cp = basename(filename);

				QueueCmd(RemoteXpath, Fil2CmdFmt, X_RQDFILE, datafile, cp);

				p = strcpyend(p, cp);
				*p++ = ' ';
			}

			continue;
		}

		if ( strcmp(NODENAME, XqtNode) == STREQUAL )
		{
			char *	cmdsfile;	/* Last component of commands file */
			char *	cmdspath;	/* Full pathname of commands file */
			char *	datafile;	/* Last component of data file */

			/** Local receive **/

			if ( !ACCESS_REMOTE_FILES )
			{
				ErrVal = EX_USAGE;
				Error(allfileslocal);
			}

			if ( !FindSysEntry(&sysname, NOSYS, NEEDALIAS) )
			{
				ErrVal = EX_NOHOST;
				if ( !LOG_BAD_SYS_NAME )
					NoLog = true;
				Error(BadSysName, sysname);
			}

			cmdsfile = WfName(CMD_TYPE, 'R', sysname);
			cmdspath = WfPath(cmdsfile, CMD_TYPE, sysname);

			datafile = newstr(cmdsfile);
			datafile[0] = DATA_TYPE;

			ExpFn(filename);

			QueueCmd(cmdspath, RecvCmdFmt, filename, datafile, Invoker);

			ReceiveCmds = true;

			if ( redirectchar == '<' )
			{
				QueueCmd(RemoteXpath, FileCmdFmt, X_RQDFILE, datafile);
				QueueCmd(RemoteXpath, FileCmdFmt, X_STDIN, datafile);
				redirectchar = '\0';
			}
			else
			{
				char *	cp = basename(filename);

				QueueCmd(RemoteXpath, Fil2CmdFmt, X_RQDFILE, datafile, cp);

				p = strcpyend(p, cp);
				*p++ = ' ';
			}

			free(datafile);
			continue;
		}

		if ( strcmp(sysname, XqtNode) != STREQUAL )
		{
			char *	cmdsfile;	/* Last component of commands file */
			char *	cmdspath;	/* Full pathname of commands file */
			char *	datafile;	/* Last component of data file */
			char *	tempfile;

			/** Remote receive **/

			if ( !ACCESS_REMOTE_FILES )
			{
				ErrVal = EX_USAGE;
				Error(allfileslocal);
			}

			if ( !FindSysEntry(&sysname, NOSYS, NEEDALIAS) )
			{
				ErrVal = EX_NOHOST;
				if ( !LOG_BAD_SYS_NAME )
					NoLog = true;
				Error(BadSysName, sysname);
			}

			cmdsfile = WfName(DATA_TYPE, 'R', sysname);
			cmdspath = WfPath(cmdsfile, DATA_TYPE, sysname);

			datafile = newstr(cmdsfile);
			datafile[0] = CMD_TYPE;

			tempfile = WfName(DATA_TYPE, 'T', NODENAME);

			QueueCmd(cmdspath, RecvCmdFmt, filename, tempfile, Invoker);
			QueueCmd(TempCpath, SendCmdFmt, cmdsfile, datafile, Invoker, EmptyStr, cmdsfile);
			SendCmds = true;

			if ( redirectchar == '<' )
			{
				QueueCmd(RemoteXpath, FileCmdFmt, X_RQDFILE, tempfile);
				QueueCmd(RemoteXpath, FileCmdFmt, X_STDIN, tempfile);
				redirectchar = '\0';
			}
			else
			{
				char *	cp = basename(filename);

				QueueCmd(RemoteXpath, Fil2CmdFmt, X_RQDFILE, tempfile, cp);

				p = strcpyend(p, cp);
				*p++ = ' ';
			}

			free(tempfile);
			free(datafile);
			continue;
		}

		/** File on XqtNode **/

		if ( filename[0] != '~' )
			ExpFn(filename);

		if ( redirectchar == '<' )
		{
			QueueCmd(RemoteXpath, FileCmdFmt, X_STDIN, filename);
			redirectchar = '\0';
		}
		else
		{
			p = strcpyend(p, cp);
			*p++ = ' ';
		}
	}

	if ( p > ParsedString )
		--p;
	*p = '\0';
	Trace((1, "parse() => %s", ParsedString));

	QueueCmd(RemoteXpath, FileCmdFmt, X_CMD, ParsedString);

	FreeStr(&sysname);
	free(filename);
	free(arg);
}

static void
get_size(filename)
	char *	filename;
{
	struct stat	statb;

	Trace((2, "get_size(%s)", filename));

	if ( filename[0] == '\0' )
		return;

	if ( stat(filename, &statb) == SYSERROR )
	{
		Debug((1, "Could not stat \"%s\": %s", filename, ErrnoStr(errno)));
		return;
	}

	FileMode = statb.st_mode;	/* Export something useful */
	DataSize += statb.st_size;
	Debug((4, "size = %lu", DataSize));
}

static char *
basename(path)
	char *		path;
{
	register char *	cp;

	if ( (cp = strrchr(path, '/')) != NULLSTR )
		return ++cp;

	return path;
}
