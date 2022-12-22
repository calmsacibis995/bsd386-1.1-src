/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Show UUCP node names.
**
**	$Log: uuname.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:59  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1.1.1  1992/07/29  23:00:44  trent
 * UUCP from UUNET (as integrated by DOnn.)
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

static char	rcsid[]	= "$Id: uuname.c,v 1.1.1.1 1992/09/28 20:08:59 trent Exp $";

#define	ARGS
#define	FILES
#define	STDIO
#define	SYSEXITS

#include	"global.h"

/*
**	Parameters set from arguments.
*/

static bool	Aliases;		/* Included aliases */
static bool	CuNames;		/* Show system names for `cu' */
static bool	Exact;			/* Exact match */
static bool	LocalOnly;		/* Show own node name only */
char *		Name	= rcsid;
static char *	Match;			/* Show system names that match */
static bool	Sort;			/* Sort system names */
int		Traceflag;

/*
**	Arguments.
*/

Args	Usage[]	=
{
	Arg_0(0, getName),
	Arg_bool(a, &Aliases, 0),
	Arg_bool(c, &CuNames, 0),
	Arg_bool(l, &LocalOnly, 0),
	Arg_bool(m, &Exact, 0),
	Arg_bool(s, &Sort, 0),
	Arg_string(P, 0, 0, "paramsfile", OPTARG),
	Arg_int(T, &Traceflag, getInt1, "trace", OPTARG|OPTVAL),
	Arg_int(x, &Traceflag, getInt1, "debug", OPTARG|OPTVAL),
	Arg_noflag(&Match, 0, english("match"), OPTARG),
	Arg_end
};

/**	Booleans are repeated because ignored parameters can't be elided	**/

static Args	PUsage[] =
{
	Arg_0(0, 0),
	Arg_bool(a, 0, 0),
	Arg_bool(c, 0, 0),
	Arg_bool(l, 0, 0),
	Arg_bool(s, 0, 0),
	Arg_bool(?, 0, 0),
	Arg_bool(\043, 0, 0),
	Arg_string(P, 0, getPARAMSFILE, NULLSTR, OPTARG),
	Arg_ignnomatch,
	Arg_end
};

/*
**	Miscellaneous.
*/

int		Debugflag;		/* Debug level */
static int	RetVal		= EX_NOHOST;
static char	Space[]		= " \t";

char *		Invoker;
int		Uid;
char *		UserName;

#define	Printf	(void)printf

static void	print(char *, char *, Syst);
static void	scansys(void(*)(char *, char *, Syst));

/*
**	Argument processing.
*/

int
main(
	int		argc,
	char *		argv[]
)
{
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

	if ( LocalOnly )
	{
		puts(NODENAME);
		RetVal = EX_OK;
	}
	else
	if ( Exact )
	{
		char *	match = newstr(Match);	/* EmptyStr if Match == NULLSTR */

		if
		(
			match[0] != '\0'
			&&
			FindSysEntry(&match, NOSYS, Aliases?NEEDALIAS:NOALIAS)
		)
		{
			puts(match);
			RetVal = EX_OK;
		}
	}
	else
	if ( Sort )
	{
		LoadSys(NOSYS);
		WalkSys(print);
	}
	else
		scansys(print);

	exit(RetVal);
}

/*
**	Clean up.
*/

void
finish(
	int	error
)
{
	exit(error);
}

/*
**	Print a nodename.
**
**	Called from WalkSys()/scansys() once for each node.
*/

static void
print(
	char *	name,	/* Node */
	char *	val,	/* Details */
	Syst	type	/* Node/Alias */
)
{
	if ( Match != NULLSTR && strstr(name, Match) == NULLSTR )
		return;

	if ( !Aliases && type == alias_t )
		return;

	puts(name);

	RetVal = EX_OK;
}

/*
**	Scan ALIASFILE (if `Aliases'), then SYSFILE, for match.
*/

static void
scansys(
	void		(*funcp)(char *, char *, Syst)
)
{
	register char *	cp;
	register char *	p;
	register char *	q;
	char *		val;
	char *		bufp;
	char *		data;
	char *		last;

	if ( Aliases && (cp = ReadFile(ALIASFILE)) != NULLSTR )
	{
		bufp = data = cp;

		while ( (cp = GetSysNode(&val, &bufp)) != NULLSTR )
		{
			if ( (p = val) != NULLSTR )
			for ( ;; )
			{
				if ( (q = strpbrk(p, Space)) != NULLSTR )
					*q++ = '\0';

				(*funcp)(p, NULLSTR, alias_t);

				if ( (p = q) == NULLSTR )
					break;

				p += strspn(p, Space);
			}
		}

		free(data);
	}

	while ( (cp = ReadFile(SYSFILE)) == NULLSTR )
		SysError(CouldNot, ReadStr, SYSFILE);

	bufp = data = cp;
	last = EmptyStr;

	while ( (cp = GetSysNode(&val, &bufp)) != NULLSTR )
	{
		if ( strcmp(cp, last) == STREQUAL )
			continue;	/* Multiple entries are contiguous (?) */

		last = cp;

		(*funcp)(cp, NULLSTR, sys_t);
	}

	free(data);
}
