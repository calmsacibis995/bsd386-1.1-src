/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	`uux_args' - process args for `uux'.
**
**	$Log: uux_args.c,v $
 * Revision 1.1.1.1  1992/09/28  20:09:02  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

static char rcsid[]	= "$Id: uux_args.c,v 1.1.1.1 1992/09/28 20:09:02 trent Exp $";

#define	ARGS
#define	FILE_CONTROL
#define	SIGNALS
#define	SYSEXITS
#define	SYSLOG

#include	"global.h"
#include	"uux.h"

/*
**	Parameters set from arguments.
*/

bool	CopyFiles;			/* Copy original files to spool */
int	Debugflag;			/* Debug level */
char	Grade		= 'A';		/* Job priority */
Ulong	GradeDelta	= MAX_ULONG;	/* Files larger than this degrade job priority */
bool	LinkFiles;			/* Make *real* link from spool to original files */
bool	LocalOnly;			/* Start uucico with `-L' (for local only) */
Ulong	MinInput;			/* Abort if less than this */
char *	Name		= rcsid;	/* Program invoked name (rcsid => AVersion) */
bool	NoMail;				/* Send no mail notification */
bool	NotifyOnFail;			/* Mail user only if command fails */
bool	PipeIn;				/* Read data from `stdin' (also `-') */
bool	StartJob	= true;		/* Start uucico after queuing job */
int	Traceflag;			/* Trace level */
char *	UserName;			/* Invoker, or arg */

/*
**	Arguments.
*/

static AFuncv	explainArgs(PassVal, Pointer, char *);	/* Explain arguments */
static AFuncv	explainVn(PassVal, Pointer, char *);	/* Explain version */
static AFuncv	getCmd(PassVal, Pointer, char *);	/* Collect command strings */
static AFuncv	getDebug(PassVal, Pointer, char *);	/* Set debug level */
static AFuncv	getGrade(PassVal, Pointer, char *);	/* Collect optional delta */
static AFuncv	getSPOOLDIR(PassVal, Pointer, char *);	/* Check SPOOLDIR */
static AFuncv	getUname(PassVal, Pointer, char *);	/* Check user name */
static AFuncv	setfalse(PassVal, Pointer, char *);	/* Set boolean false */

static Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_bool(c, &CopyFiles, setfalse),
	Arg_bool(C, &CopyFiles, 0),
	Arg_bool(l, &LinkFiles, 0),
	Arg_bool(L, &LocalOnly, 0),
	Arg_bool(n, &NoMail, 0),
	Arg_bool(p, &PipeIn, 0),
	Arg_bool(r, &StartJob, setfalse),
	Arg_bool(z, &NotifyOnFail, 0),
	Arg_bool(?, 0, explainArgs),
	Arg_bool(\043, 0, explainVn),
	Arg_string(a, &UserName, getUname, "name[!address]", OPTARG),
	Arg_string(d, 0, getSPOOLDIR, "spooldir", OPTARG),
	Arg_char(g, &Grade, getGrade, "grade[delta]", OPTARG),
	Arg_long(m, &MinInput, 0, "mininput", OPTARG),
	Arg_long(M, &MinInput, 0, "mininput", OPTARG),
	Arg_string(P, 0, 0, "paramsfile", OPTARG),
	Arg_string(S, 0, getSPOOLDIR, "spooldir", OPTARG),
	Arg_int(T, &Traceflag, getDebug, "trace", OPTARG|OPTVAL),
	Arg_int(x, &Debugflag, getDebug, "debug", OPTARG|OPTVAL),
	Arg_minus(&PipeIn, 0),
	Arg_any(0, getCmd, "command", MANY),
	Arg_end
};

/**	Booleans are repeated because ignored parameters can't be elided	**/

static Args	PUsage[] =
{
	Arg_0(0, 0),
	Arg_bool(c, 0, 0),
	Arg_bool(C, 0, 0),
	Arg_bool(l, 0, 0),
	Arg_bool(L, 0, 0),
	Arg_bool(n, 0, 0),
	Arg_bool(p, 0, 0),
	Arg_bool(r, 0, 0),
	Arg_bool(z, 0, 0),
	Arg_bool(?, 0, 0),
	Arg_bool(\043, 0, 0),
	Arg_string(P, 0, getPARAMSFILE, NULLSTR, OPTARG),
	Arg_ignnomatch,
	Arg_end
};

/*
**	Command concatenation.
*/

#define	MAXCMDS	512		/* Too miserly? */
static int	CmdCount;
	int	CmdLen;		/* Length of CmdString */
	char *	CmdString;	/* The collected command */
static char **	Cmds;		/* Array of pointers */
static char **	CmdPtr;		/* Next to be collected */
static void	ConcatCmds();	/* Collect Cmds -> CmdString */

/*
**	Miscellaneous definitions.
*/

char *	HomeDir;		/* Invoker's path */
char *	Invoker;		/* Invoker's name */

/*
**	Argument processing.
*/

void
uux_args(
	int		argc,	/* If 0, split `args' */
	char *		argv[],
	char *		args
)
{
	register Args *	up;

	/*
	**	Pre-process args.
	*/

	if ( (CmdPtr = Cmds) == (char **)0 )
		CmdPtr = Cmds = (char **)Malloc(MAXCMDS*sizeof(char *));
	CmdCount = CmdLen = 0;

	if ( argc == 0 )
	{
		if ( (argc = SplitSpace(Cmds, args, MAXCMDS)) > MAXCMDS )
		{
			ErrVal = EX_USAGE;
			Error("too many commands");
		}

		argv = Cmds;
	}

	ArgsIgnFlag = false;

	for ( up = PUsage ; up->type ; up++ )
		up->opt &= ~(REJECT|FOUND);

	DoArgs(argc, argv, PUsage);	/* Possible change to PARAMSFILE */

	/*
	**	Reset variables.
	*/

	if ( UUXReRun )
		MaxWorkFiles = 64;
	else
		MaxWorkFiles = 8;	/* The most common number needed by uux */

	CopyFiles	= false;
	Grade		= 'A';
	GradeDelta	= MAX_ULONG;
	LinkFiles	= false;
	LocalOnly	= false;
	MinInput	= 0;
	NoMail		= false;
	NotifyOnFail	= false;
	PipeIn		= false;
	StartJob	= true;

	FreeStr(&CmdString);
	FreeStr(&Invoker);
	FreeStr(&HomeDir);

	InitParams();

	(void)NodeName();

	/*
	**	Set up user id.
	*/

	GetNetUid();	/* Sets R_uid */

	if ( !GetUser(&R_uid, &Invoker, &HomeDir) )
	{
		Warn("Could not find name for uid %d, using %s", R_uid, UUCPUSER);
		Invoker = newstr(UUCPUSER);
		HomeDir = newstr(PUBDIR);
	}

	UserName = Invoker;

	/*
	**	Process args.
	*/

	for ( up = Usage ; up->type ; up++ )
		up->opt &= ~(REJECT|FOUND);

	DoArgs(argc, argv, Usage);

	if ( Traceflag > 0 && Debugflag == 0 )
		Debugflag = Traceflag;

	ArgsIgnFlag = false;

	if ( LinkFiles )
		CopyFiles = false;
	else
	if ( CopyFiles )
		LinkFiles = false;

	ConcatCmds();

	LogNode = Name = "uux";		/* Override invoked name */

	OpenLog(Name, LOG_PID, LOG_UUCP);

	(void)signal(SIGHUP, finish);
	(void)signal(SIGINT, finish);
}

/*
**	Needed because Arg_any accepts "-?".
*/

static AFuncv
explainArgs(PassVal val, Pointer ptr, char * str)
{
	ExplainArgs = true;
	usagerror(ArgsUsage(Usage));
	return ACCEPTARG;
}

/*
**	Needed because Arg_any accepts "-#".
*/

static AFuncv
explainVn(PassVal val, Pointer ptr, char * str)
{
	ExplVersion = true;
	usagerror(ArgsUsage(Usage));
	return ACCEPTARG;
}

/*
**	Collect command strings.
*/

static AFuncv
getCmd(PassVal val, Pointer ptr, char * str)
{
	ArgsIgnFlag = true;	/* All succeeding flags are for command */

	if ( ++CmdCount > MAXCMDS )
	{
		NoArgsUsage = true;
		return (AFuncv)"too many args";
	}

	*CmdPtr++ = val.p;
	CmdLen += strlen(val.p);
	return ACCEPTARG;
}

static void
ConcatCmds()
{
	register int	i = CmdCount;
	register char *	cp;
	register char**	cpp;

	CmdLen += i;	/* For ' ' separators */
	CmdString = cp = Malloc(CmdLen);

	for ( cpp = Cmds ; --i >= 0 ; )
	{
		cp = strcpyend(cp, *cpp++);
		*cp++ = ' ';
	}
	*--cp = '\0';	/* At least one command guaranteed */

	Trace((1, "CmdString => %s", CmdString));
}

/*
**	Turn on debugging if allowed.
*/

static AFuncv
getDebug(PassVal val, Pointer arg, char * str)
{
	if ( val.l == 0 && str[0] != '0' )
		*(int *)arg = val.l = 1;

	if ( val.l && access(SYSFILE, R_OK) == SYSERROR )
	{
		*(int *)arg = 0;
		ErrVal = EX_NOPERM;
		Error("debug requires read access to %s", SYSFILE);
	}

	return ACCEPTARG;
}

/*
**	If number follows grade value, use it to set `GradeDelta'.
*/

static AFuncv
getGrade(PassVal val, Pointer ptr, char * str)
{
	if ( str[1] != '\0' )	/* Value part of flag parameter */
		GradeDelta = atol(&str[1]);

	return ACCEPTARG;
}

/*
**	Add trailing '/' to SPOOLDIR.
*/

static AFuncv
getSPOOLDIR(PassVal val, Pointer ptr, char * str)
{
	Set_SPOOLDIR(val.p);
	return ACCEPTARG;
}

/*
**	If name contains uucp address, strip NODENAME.
*/

static AFuncv
getUname(PassVal val, Pointer ptr, char * str)
{
	if
	(
		strncmp(UserName, NODENAME, NodeNameLen) == STREQUAL
		&&
		UserName[NodeNameLen] == '!'
	)
		UserName += NodeNameLen + 1;

	return ACCEPTARG;
}

/*
**	Set boolean false.
*/

static AFuncv
setfalse(PassVal val, Pointer ptr, char * str)
{
	*(bool *)ptr = false;
	return ACCEPTARG;
}
