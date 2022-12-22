/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	`uucp_args' - process args for `uucp'.
**
**	$Log: uucp_args.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:57  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

static char rcsid[]	= "$Id: uucp_args.c,v 1.1.1.1 1992/09/28 20:08:57 trent Exp $";

#define	ARGS
#define	FILE_CONTROL
#define	SIGNALS
#define	SYSEXITS
#define	SYSLOG

#include	"global.h"
#include	"uucp.h"

/*
**	Defaults:
*/

#define	MAXFILES	512		/* Too miserly? */

/*
**	Parameters set from arguments.
*/

bool	CopyFiles;			/* Copy original files to spool */
bool	CreateDirs	= true;		/* Create intermediate directories */
int	Debugflag;			/* Debug level */
char	Grade		= 'n';		/* Job priority */
Ulong	GradeDelta	= MAX_ULONG;	/* Files larger than this degrade job priority */
bool	JobID;				/* Output ASCII job ID on stdout */
Ulong	MinInput;			/* Abort if less than this */
char *	Name		= rcsid;	/* Program invoked name (rcsid => AVersion) */
bool	NoMail		= true;		/* Send no mail notification */
char *	RmtNode;			/* Remote node for file copy */
char *	RmtUser;			/* User to notify on reception */
bool	StartJob	= true;		/* Start uucico after queuing job */
int	Traceflag;			/* Trace level */

/*
**	Arguments.
*/

static AFuncv	explainArgs(PassVal, Pointer, char *);	/* Explain arguments */
static AFuncv	explainVn(PassVal, Pointer, char *);	/* Explain version */
static AFuncv	getDebug(PassVal, Pointer, char *);	/* Set debug level */
static AFuncv	get1File(PassVal, Pointer, char *);	/* Collect file names */
static AFuncv	getFile(PassVal, Pointer, char *);	/* Collect file names */
static AFuncv	getGrade(PassVal, Pointer, char *);	/* Collect optional delta */
static AFuncv	getSPOOLDIR(PassVal, Pointer, char *);	/* Check SPOOLDIR */
static AFuncv	setfalse(PassVal, Pointer, char *);	/* Set boolean false */

static Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_bool(c, &CopyFiles, setfalse),
	Arg_bool(C, &CopyFiles, 0),
	Arg_bool(d, &CreateDirs, 0),
	Arg_bool(f, &CreateDirs, setfalse),
	Arg_bool(j, &JobID, 0),
	Arg_bool(m, &NoMail, setfalse),
	Arg_bool(r, &StartJob, setfalse),
	Arg_bool(?, 0, explainArgs),
	Arg_bool(\043, 0, explainVn),
	Arg_string(e, &RmtNode, 0, "remote node", OPTARG),
	Arg_char(g, &Grade, getGrade, "grade[delta]", OPTARG),
	Arg_long(M, &MinInput, 0, "mininput", OPTARG),
	Arg_string(n, &RmtUser, 0, "remote user", OPTARG),
	Arg_string(P, 0, 0, "paramsfile", OPTARG),
	Arg_string(s, 0, getSPOOLDIR, "spooldir", OPTARG),
	Arg_string(S, 0, getSPOOLDIR, "spooldir", OPTARG),
	Arg_int(T, &Traceflag, getDebug, "trace", OPTARG|OPTVAL),
	Arg_int(x, &Debugflag, getDebug, "debug", OPTARG|OPTVAL),
	Arg_any(0, get1File, "source file", MANY),
	Arg_any(0, getFile, "destination file", 0),
	Arg_end
};

/**	Booleans are repeated because ignored parameters can't be elided	**/

static Args	PUsage[] =
{
	Arg_0(0, getName),
	Arg_bool(c, 0, 0),
	Arg_bool(C, 0, 0),
	Arg_bool(d, 0, 0),
	Arg_bool(f, 0, 0),
	Arg_bool(j, 0, 0),
	Arg_bool(m, 0, 0),
	Arg_bool(r, 0, 0),
	Arg_bool(?, 0, 0),
	Arg_bool(\043, 0, 0),
	Arg_string(P, 0, getPARAMSFILE, NULLSTR, OPTARG),
	Arg_ignnomatch,
	Arg_end
};

/*
**	File collection.
*/

int		FileCount;
char **		Files;		/* Array of pointers */
static char **	FilePtr;	/* Next to be collected */

/*
**	Miscellaneous definitions.
*/

char *	HomeDir;		/* Invoker's path */
char *	Invoker;		/* Invoker's name */
int	LastOptn;		/* Index for `Optns' */
char	Optns[5];		/* Command options */
char *	UserName;		/* == Invoker */

/*
**	Argument processing.
*/

void
uucp_args(
	int		argc,	/* If 0, split `args' */
	char *		argv[],
	char *		args
)
{
	register int	i;
	register Args *	up;

	/*
	**	Pre-process args.
	*/

	if ( (FilePtr = Files) == (char **)0 )
		FilePtr = Files = (char **)Malloc(MAXFILES*sizeof(char *));	
	FileCount = 0;

	if ( argc == 0 )
	{
		if ( (argc = SplitSpace(Files, args, MAXFILES)) > MAXFILES )
		{
			ErrVal = EX_USAGE;
			Error("too many files");
		}

		argv = Files;
	}

	for ( up = PUsage ; up->type ; up++ )
		up->opt &= ~(REJECT|FOUND);

	DoArgs(argc, argv, PUsage);	/* Possible change to PARAMSFILE */

	/*
	**	Reset variables.
	*/

	if ( UUCPReRun )
		MaxWorkFiles = 64;
	else
		MaxWorkFiles = 8;	/* The most common number needed by uucp */

	CopyFiles	= false;
	CreateDirs	= true;
	Grade		= 'n';
	GradeDelta	= MAX_ULONG;
	JobID		= false;
	MinInput	= 0;
	NoMail		= true;
	RmtNode		= NULLSTR;
	RmtUser		= NULLSTR;
	StartJob	= true;

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

	Optns[i=0] = CreateDirs ? 'd' : 'f';
	Optns[++i] = CopyFiles ? 'C' : 'c';
	if ( !NoMail )
		Optns[++i] = 'm';
	Optns[++i] = '\0';
	LastOptn = i;

	LogNode = Name = "uucp";	/* Override invoked name */

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
**	Collect file names.
*/

static AFuncv
get1File(PassVal val, Pointer ptr, char * str)
{
	if ( FileCount )
		return REJECTARG;	/* Select `getFile' */

	FileCount = 1;
	*FilePtr++ = val.p;
	return ACCEPTARG;
}

static AFuncv
getFile(PassVal val, Pointer ptr, char * str)
{
	if ( ++FileCount > MAXFILES )
	{
		NoArgsUsage = true;
		return (AFuncv)"too many files";
	}

	*FilePtr++ = val.p;
	return ACCEPTARG;
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
**	Set boolean false.
*/

static AFuncv
setfalse(PassVal val, Pointer ptr, char * str)
{
	*(bool *)ptr = false;
	return ACCEPTARG;
}
