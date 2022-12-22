/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Process args for `uucico'.
**
**	$Log: cico_args.c,v $
 * Revision 1.3  1994/01/31  01:26:45  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:58:30  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:42:44  vixie
 * Initial revision
 *
 * Revision 1.2  1993/01/07  18:37:54  sanders
 * moved logging of RemoteName until after we open the log file.
 *
 * Revision 1.1.1.1  1992/09/28  20:08:53  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

static char rcsid[]	= "$Id: cico_args.c,v 1.3 1994/01/31 01:26:45 donn Exp $";

#define	ARGS
#define	FILES
#define	FILE_CONTROL
#define	SETJMP
#define	SIGNALS
#define	STDIO
#define	SYSEXITS
#define	SYSLOG

#include	"global.h"
#include	"cico.h"

/*
**	Parameters set from arguments.
*/

bool	CheckForWork;			/* If no files to send, give up */
int	Debugflag;			/* Debug level */
int	DebugRemote;			/* Remote end debug level */
char	DefMaxGrade	= '\177';	/* Default job priority */
bool	IgnoreTimeToCall;		/* Call again */
Role	InitialRole	= SLAVE;	/* MASTER/SLAVE */
bool	KeepAudit;			/* Keep time-stamped audit */
bool	LocalOnly;			/* Only call LOCAL, DIR, or TCP sites */
char *	Name		= rcsid;	/* Program invoked name (rcsid => AVersion) */
char *	RemoteNode;			/* Particular host to be called */
bool	ReverseRole;			/* Caller begins by receiving  */
int	Traceflag;			/* Trace level */
int	TurnTime;			/* Turn time between role swaps */

/*
**	Arguments.
**
**	NB: old setproctitle() clobbers the arg list, so all strings must be saved.
*/

static AFuncv	explainArgs(PassVal, Pointer, char *);	/* Explain arguments */
static AFuncv	explainVn(PassVal, Pointer, char *);	/* Explain version */
static AFuncv	getMyNode(PassVal, Pointer, char *);	/* Setup new NODENAME */
static AFuncv	getRemote(PassVal, Pointer, char *);	/* Setup remote system name */
static AFuncv	getRole(PassVal, Pointer, char *);	/* Set initial behaviour */
static AFuncv	getSRemote(PassVal, Pointer, char *);	/* Setup remote system name */
static AFuncv	getSPOOLDIR(PassVal, Pointer, char *);	/* Check SPOOLDIR */
static AFuncv	getTurn(PassVal, Pointer, char *);	/* Mins * 60 => secs */
static AFuncv	ignArg(PassVal, Pointer, char *);	/* Ignore unrecognised args */

static Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_bool(L, &LocalOnly, 0),
	Arg_bool(R, &ReverseRole, 0),
	Arg_bool(c, &CheckForWork, 0),
	Arg_bool(k, &KeepAudit, 0),
	Arg_bool(?, 0, explainArgs),
	Arg_bool(\043, 0, explainVn),
	Arg_string(d, 0, getSPOOLDIR, "spooldir", OPTARG),
	Arg_char(g, &DefMaxGrade, 0, "grade", OPTARG),
	Arg_string(m, 0, getMyNode, "mynode", OPTARG),
	Arg_char(p, &DefMaxGrade, 0, "grade", OPTARG),
	Arg_string(P, 0, 0, "paramsfile", OPTARG),
	Arg_int(r, 0, getRole, "role", OPTARG),
	Arg_string(S, 0, getSRemote, "remote", OPTARG),
	Arg_string(S, 0, getSPOOLDIR, "spooldir", OPTARG),
	Arg_string(s, 0, getRemote, "remote", OPTARG),
	Arg_int(T, &Traceflag, getInt1, "trace", OPTARG|OPTVAL),
	Arg_int(t, &TurnTime, getTurn, "turntime", OPTARG|OPTVAL),
	Arg_int(X, &DebugRemote, getInt1, "remote debug", OPTARG|OPTVAL),
	Arg_int(x, &Debugflag, getInt1, "debug", OPTARG|OPTVAL),
	Arg_any(0, ignArg, EmptyStr, OPTARG|MANY),
	Arg_end
};

#define	ANY_POS	(ARR_SIZE(Usage)-2)

/**	Booleans are repeated because ignored parameters can't be elided	**/

static AFuncv	getPARAMSfile(PassVal, Pointer, char *);	/* Malloc new PARAMSFILE */

static Args	PUsage[] =
{
	Arg_0(0, getName),
	Arg_bool(L, 0, 0),
	Arg_bool(R, 0, 0),
	Arg_bool(c, 0, 0),
	Arg_bool(k, 0, 0),
	Arg_bool(?, 0, 0),
	Arg_bool(\043, 0, 0),
	Arg_string(P, 0, getPARAMSfile, NULLSTR, OPTARG),
	Arg_ignnomatch,
	Arg_end
};

/*
**	Miscellaneous definitions.
*/

char *	HomeDir;			/* Invoker's path */
char *	Invoker;			/* Invoker's name */
char *	LoginName;			/* == Invoker */
char	MaxGrade;			/* Job priority */
Role	CurrentRole	= SLAVE;	/* Current behaviour */
char *	UserName;			/* == LoginName initially */

/*
**	Routines.
*/

static void	ChkDebug(void);

/*
**	Argument processing.
*/

void
cico_args(
	int		argc,
	char *		argv[]
)
{
	DoArgs(argc, argv, PUsage);	/* Possible change to PARAMSFILE */

	MaxWorkFiles = 64;

	InitParams();

	(void)NodeName();

	/*
	**	Set up user id.
	*/

	GetNetUid();	/* Sets R_uid */

	if ( !GetUser(&R_uid, &LoginName, &HomeDir) )
	{
		LoginName = Unknown;
		ErrVal = EX_NOUSER;
		Error("Could not find name for uid %d", R_uid);
	}

	UserName = Invoker = LoginName;

	/*
	**	Process args.
	*/

	TurnTime = DEFAULT_CICO_TURN;

	DoArgs(argc, argv, Usage);

	if ( DebugRemote )
		Debugflag = DebugRemote;

	Name = "uucico";		/* Override invoked name */

	if ( Traceflag > 0 && Debugflag == 0 )
		Debugflag = Traceflag;

	if ( ReverseRole )
	{
		InitialRole = MASTER;
		Debug((2, "Reverse Role"));
	}

	CurrentRole = InitialRole;

	if ( RemoteNode == NULLSTR )
	{
		if ( InitialRole == MASTER )
		{
			Debugflag = Traceflag = 0;
			Uusched();	/* Backward-compatible */
			fprintf(stderr, "No remote system name");
			finish(EX_USAGE);
		}

		LogNode = Name;
	}
	else
	{
		LogNode = RemoteNode;
	}

	if ( (Debugflag || Traceflag) && CurrentRole == MASTER )
		ChkDebug();

	MaxGrade = DefMaxGrade;

	OpenLog(Name, LOG_PID, LOG_UUCP);
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
**	Needed because Arg_any accepts "-?".
*/

static AFuncv
explainArgs(PassVal val, Pointer ptr, char * str)
{
	ExplainArgs = true;
	Usage[ANY_POS].opt |= SPECIAL;	/* Don't show Arg_any */
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
**	Malloc new node name.
*/

static AFuncv
getMyNode(PassVal val, Pointer ptr, char * str)
{
	NODENAME = newstr(val.p);
	return ACCEPTARG;
}

/*
**	Malloc new PARAMSFILE.
*/

static AFuncv
getPARAMSfile(PassVal val, Pointer ptr, char * str)
{
	PARAMSFILE = newstr(val.p);
	return getPARAMSFILE(val, ptr, str);
}

/*
**	Setup remote node name.
*/

static AFuncv
getRemote(PassVal val, Pointer ptr, char * str)
{
	if ( val.p == NULLSTR || val.p[0] == '\0' )
	{
		Debugflag = Traceflag = 0;
		Uusched();	/* Backward-compatible */
		return ARGERROR;
	}

	RemoteNode = newnstr(val.p, NODENAMEMAXSIZE);
	InitialRole = MASTER;

	return ACCEPTARG;
}

/*
**	Set initial behaviour.
*/

static AFuncv
getRole(PassVal val, Pointer ptr, char * str)
{
	if ( val.l == 0 )
		InitialRole = SLAVE;
	else
		InitialRole = MASTER;

	return ACCEPTARG;
}

/*
**	Setup remote node name, ignoring past calls.
*/

static AFuncv
getSRemote(PassVal val, Pointer ptr, char * str)
{
	if ( val.p != NULLSTR && val.p[0] == '/' )
		return REJECTARG;	/* Probably "-S SPOOLDIR" */

	IgnoreTimeToCall = true;

	return getRemote(val, ptr, str);
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
**	Multiply arg by 60 to convert mins to secs.
*/

static AFuncv
getTurn(PassVal val, Pointer ptr, char * str)
{
	*(long *)ptr *= 60;
	return ACCEPTARG;
}

/*
**	Show ignored arg.
*/

static AFuncv
ignArg(PassVal val, Pointer ptr, char * str)
{
	MesgN("ignored", "Unknown argument %s", str);
	return ACCEPTARG;
}
