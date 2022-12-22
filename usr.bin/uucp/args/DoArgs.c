/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Default argument parsing.
**
**	Errors print on stderr and "finish(EX_USAGE)".
**
**	RCSID $Id: DoArgs.c,v 1.1.1.1 1992/09/28 20:08:31 trent Exp $
**
**	$Log: DoArgs.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:31  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	ARGS
#define	STDIO
#define	SYSEXITS

#include	"global.h"

bool		NoArgsUsage;


/*
**	Call EvalArgs with default program name and error processing
*/

void
DoArgs(argc, argv, args)
	int	argc;
	char *	argv[];
	Args *	args;
{
	if ( EvalArgs(argc, argv, args, argerror) < 0 )
		usagerror(NoArgsUsage?NULLSTR:ArgsUsage(args));
}

/*ARGSUSED*/
AFuncv
getName(val, arg, str)
	PassVal		val;
	Pointer		arg;
	char *		str;
{
	register char **cpp;

	if ( (cpp = (char **)arg) == (char **)0 )
		cpp = &Name;

	if ( (*cpp = strrchr(val.p, '/')) != NULLSTR )
		(*cpp)++;
	else
	{
		if ( val.p[0] == '-' )
			++val.p;
		*cpp = val.p;
	}

	return ACCEPTARG;
}

AFuncv
getInt1(val, arg, str)
	PassVal		val;
	Pointer		arg;
	char *		str;
{
	register int *	ip;

	if ( (ip = (int *)arg) == (int *)0 )
		return (AFuncv)english("missing variable address");

	if ( (*ip = val.l) == 0 && str[0] != '0' )
		*ip = 1;

	return ACCEPTARG;
}

void
usagerror(s)
	char *	s;
{
	if ( s != NULLSTR )
		(void)fprintf(stderr, "%s\n", s);

	finish(EX_USAGE);
}

int
argerror(s)
	char *	s;
{
	(void)fprintf(stderr, "%s: argument error: %s\n", Name, s);
	return 0;
}
