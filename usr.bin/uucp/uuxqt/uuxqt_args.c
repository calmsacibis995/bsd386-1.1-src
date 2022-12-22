/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	`uuxqt_args' - process args for `uuxqt'.
**
**	$Log: uuxqt_args.c,v $
 * Revision 1.2  1994/01/31  01:27:25  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:58:59  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:43:09  vixie
 * Initial revision
 *
 * Revision 1.1.1.1  1992/09/28  20:09:04  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

static char rcsid[]	= "$Id: uuxqt_args.c,v 1.2 1994/01/31 01:27:25 donn Exp $";

#define	ARGS
#define	FILE_CONTROL
#define	SYSEXITS
#define	SYSLOG

#include	"global.h"
#include	"uuxqt.h"

/*
**	Parameters set from arguments.
*/

int	Debugflag;			/* Debug level */
char *	Name		= rcsid;	/* Program invoked name (rcsid => AVersion) */
int	Traceflag;			/* Trace level */
char *	XqtNode;			/* Nodename for commands */

/*
**	Arguments.
*/

static AFuncv	getDebug(PassVal, Pointer, char *);	/* Set debug level */
static AFuncv	getNode(PassVal, Pointer, char *);	/* Take copy of node */
static AFuncv	getSPOOLDIR(PassVal, Pointer, char *);	/* Check SPOOLDIR */

static Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_string(P, 0, 0, "paramsfile", OPTARG),
	Arg_string(S, 0, getSPOOLDIR, "spooldir", OPTARG),
	Arg_string(s, 0, getNode, "name", OPTARG),
	Arg_int(T, &Traceflag, getDebug, "trace", OPTARG|OPTVAL),
	Arg_int(x, &Debugflag, getDebug, "debug", OPTARG|OPTVAL),
	Arg_end
};

/**	Booleans are repeated because ignored parameters can't be elided	**/

static Args	PUsage[] =
{
	Arg_0(0, getName),
	Arg_bool(?, 0, 0),
	Arg_bool(\043, 0, 0),
	Arg_string(P, 0, getPARAMSFILE, NULLSTR, OPTARG),
	Arg_ignnomatch,
	Arg_end
};

/*
**	Miscellaneous definitions.
*/

char *	HomeDir;		/* Invoker's HOME */
char *	Invoker;		/* Invoker's name */
char *	UserName;		/* Current user */

/*
**	Argument processing.
*/

void
uuxqt_args(
	int	argc,		/* If 0, split `args' */
	char *	argv[],
	char *	args
)
{
	MaxWorkFiles = 64;	/* The most common number needed by uuxqt */

	/*
	**	Pre-process args.
	*/

	if ( argc == 0 )
	{
		char *	cmds[MAXVARARGS];

		if ( (argc = SplitSpace(cmds, args, MAXVARARGS)) > MAXVARARGS )
		{
			ErrVal = EX_USAGE;
			Error("too many commands");
		}

		argv = cmds;
	}

	DoArgs(argc, argv, PUsage);	/* Possible change to PARAMSFILE */

	if ( Traceflag > 0 && Debugflag == 0 )
		Debugflag = Traceflag;

	InitParams();

	(void)NodeName();

	/*
	**	Set up user id.
	*/

	FreeStr(&Invoker);
	FreeStr(&HomeDir);

	GetNetUid();	/* Sets R_uid */

	if ( !GetUser(&R_uid, &Invoker, &HomeDir) )
	{
		Warn("Could not find name for uid %d, using %s", R_uid, UUCPUSER);
		Invoker = newstr(UUCPUSER);
		HomeDir = newstr(PUBDIR);
	}

	UserName = Invoker;

	DoArgs(argc, argv, Usage);		/* Process all args */

	LogNode = Name = "uuxqt";		/* Override invoked name */

	OpenLog(Name, LOG_PID, LOG_UUCP);
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
**	Take copy of node name (as old setproctitle() clobbers arglist).
*/

static AFuncv
getNode(PassVal val, Pointer arg, char * str)
{
	XqtNode = newstr(val.p);
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
