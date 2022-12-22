/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Show parameter settings.
**
**	$Log: uuparams.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:59  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1.1.1  1992/07/29  23:01:15  trent
 * UUCP from UUNET (as integrated by DOnn.)
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

static char	rcsid[]	= "$Id: uuparams.c,v 1.1.1.1 1992/09/28 20:08:59 trent Exp $";

#define	ARGS
#define	EXECUTE
#define	FILE_CONTROL
#define	PARAMS
#define	STDIO
#define	SYSEXITS

#include	"global.h"
#include	<grp.h>

/*
**	Parameters set from arguments.
*/

bool	All;					/* Show all parameters */
bool	Defaults;				/* Show default settings */
bool	ShowFreeSpace;				/* Show FSFree() setting */
bool	ShowVersion;				/* Show Version[] */
char *	Name		= rcsid;
char *	NewSpool;				/* Alternate spool directory */
bool	NoWarnings;
char *	Param;					/* Optional parameter to be matched */
int	Traceflag;
bool	Verbose;

/*
**	Arguments.
*/

Args	Usage[]	=
{
	Arg_0(0, getName),
	Arg_bool(a, &All, 0),
	Arg_bool(d, &Defaults, 0),
	Arg_bool(f, &ShowFreeSpace, 0),
	Arg_bool(n, &ShowVersion, 0),
	Arg_bool(v, &Verbose, 0),
	Arg_bool(w, &NoWarnings, 0),
	Arg_string(P, 0, 0, "paramsfile", OPTARG),
	Arg_string(S, &NewSpool, 0, "spooldir", OPTARG),
	Arg_int(T, &Traceflag, getInt1, "trace", OPTARG|OPTVAL),
	Arg_int(x, &Traceflag, getInt1, "debug", OPTARG|OPTVAL),
	Arg_noflag(&Param, 0, english("match"), OPTARG),
	Arg_end
};

/**	Booleans are repeated because ignored parameters can't be elided	**/

static Args	PUsage[] =
{
	Arg_0(0, getName),
	Arg_bool(a, 0, 0),
	Arg_bool(d, 0, 0),
	Arg_bool(f, 0, 0),
	Arg_bool(n, 0, 0),
	Arg_bool(v, 0, 0),
	Arg_bool(w, 0, 0),
	Arg_bool(?, 0, 0),
	Arg_bool(\043, 0, 0),
	Arg_string(P, 0, getPARAMSFILE, NULLSTR, OPTARG),
	Arg_int(x, &Traceflag, getInt1, NULLSTR, OPTARG|OPTVAL),
	Arg_ignnomatch,
	Arg_end
};

/*
**	Miscellaneous.
*/

int	Debugflag;		/* Debug level */
char *	Invoker;
int	Uid;
char *	UserName;

#define	Printf	(void)printf

void	checknames(), print();

extern struct group *	getgrnam();



/*
**	Argument processing.
*/

int
main(argc, argv)
	int		argc;
	char *		argv[];
{
	DoArgs(argc, argv, PUsage);	/* Possible change to PARAMSFILE */

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

	if ( EvalArgs(argc, argv, Usage, argerror) < 0 )
	{
		InitParams();
		usagerror(ArgsUsage(Usage));
	}

	if ( !Defaults )
	{
		if ( !NoWarnings )
			CheckParams = true;
		InitParams();

		if ( NewSpool != NULLSTR )
			Set_SPOOLDIR(NewSpool);
	}
	else
	{
		All = true;
		Pid = getpid();
		(void)SetTimes();
	}

	setbuf(stdout, _sobuf);

	if ( ShowVersion )
	{
		Printf("%s\n", Version);
		exit(EX_OK);
	}

	if ( ShowFreeSpace )
	{
		int	val = EX_OK;

		if ( !FSFree(SPOOLDIR, (Ulong)0) )
		{
			val = EX_UNAVAILABLE;

			Printf
			(
				english("NO SPACE on %s (MINSPOOLFSFREE=%ld Kbytes)\n"),
				SPOOLDIR,
				MINSPOOLFSFREE
			);
		}
		else
			Printf
			(
				english("available free space on %s is %ld Kbytes\n"),
				SPOOLDIR,
				FreeFSpace/1024
			);

		exit(val);
	}

	print(Params, SizeofParams);

	if ( ParamsErr )
		exit(EX_DATAERR);

	exit(EX_OK);
}



/*
**	Check user/group names.
*/

void
checknames(tp, n)
	register ParTb *tp;
	register int	n;
{
	char *		cp;
	static char	ws[] = english("parameter \"%s\": %s \"%s\" does not exist!");

	Trace((1, "checknames()"));

	for ( n /= sizeof(ParTb) ; --n >= 0 ; tp++ )
		if
		(
			tp->type == PSTRING
			&&
			(cp = *tp->val) != NULLSTR
		)
		{
			if ( strstr(tp->id, "USER") != NULLSTR )
			{
				Trace((2, "check user \"%s=%s\"", tp->id, cp));

				if ( !GetUid((int *)0, &cp, (char **)0) )
				{
					Warn(ws, tp->id, english("user"), cp);
					ParamsErr = true;
				}
			}
			else
			if ( strstr(tp->id, "GROUP") != NULLSTR )
			{
				Trace((2, "check group \"%s=%s\"", tp->id, cp));

				if ( getgrnam(cp) == (struct group *)0 )
				{
					Warn(ws, tp->id, english("group"), cp);
					ParamsErr = true;
				}
			}
		}
}



/*
**	Clean up.
*/

void
finish(error)
	int	error;
{
	exit(error);
}



/*
**	Print out changed parameters.
*/

void
print(tp, n)
	register ParTb *tp;
	register int	n;
{
	register char **cpp;
	register char *	cp;
	register int	i;
	static char	space[]	= " \t\n";
	static char	params[]= "PARAMSFILE";

	if ( !NoWarnings )
		checknames(tp, n);

	if ( Verbose && (Param == NULLSTR || strstr(params, Param) != NULLSTR) )
	{
		if ( NewParamsFile && (All || Param != NULLSTR) )
			cp = " *";
		else
			cp = EmptyStr;

		Printf("%s=%s%s\n", params, PARAMSFILE, cp);
	}

	for ( n /= sizeof(ParTb) ; --n >= 0 ; tp++ )
	{
		if ( Param != NULLSTR )
		{
			if ( strstr(tp->id, Param) == NULLSTR )
				continue;
		}
		else
			if ( !All && !(tp->flag & SET) )
				continue;

		Printf("%s=", tp->id);

		switch ( tp->type )
		{
		case PLONG:	Printf("%ld", *(long *)tp->val);	break;
		case PINT:	Printf("%d", *(int *)tp->val);		break;
		case POCT:	Printf("0%o", *(int *)tp->val);		break;
		case PHEX:	Printf("0x%x", *(int *)tp->val);	break;

		case PSTRING:
		case PDIR:
		case PSPOOLD:
		case PPARAMD:
		case PSPOOL:
		case PPARAM:
		case PPROG:
			if ( (cp = *tp->val) == NULLSTR )
				break;
			if ( strpbrk(cp, space) != NULLSTR )
				Printf("\"%s\"", cp);
			else
				Printf("%s", cp);
			break;

		case PVECTOR:
			if ( (cpp = *(char ***)tp->val) == (char **)0 )
				break;
			for ( i = 0 ; *cpp != NULLSTR ; i++, cpp++ )
			{
				if ( i )
					putchar(',');
				Printf("\"%s\"", *cpp);
			}
		}

		if ( (tp->flag & SET) && Verbose && (All || Param != NULLSTR) )
			Printf(" *");

		putchar('\n');
	}
}
