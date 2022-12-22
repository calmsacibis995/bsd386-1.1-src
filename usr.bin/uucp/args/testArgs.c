/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Test argument parsing routines.
**
**	RCSID $Id: testArgs.c,v 1.1.1.1 1992/09/28 20:08:32 trent Exp $
**
**	$Log: testArgs.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:32  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	ARGS
#define	STDIO
#define	SYSEXITS

#include	<global.h>

int	Aoptbool;
int	argerror();
int	Boptbool;
AFuncv	Bstrfunc();
char *	Bstring;
AFuncv	Ddfunc();
char *	Ddstring;
double	Ddouble;
float	Ffloat;
int	Minus;
char *	Name;
AFuncv	opt2strfunc();
AFuncv	optstrfunc();
char *	Opt2string;
char *	Optstring;
int	Traceflag;

Args	Usage[] =
{
	Arg_0(0, getName),				/* Extracts last component of pathname from program name */
	Arg_bool(a, &Aoptbool, 0),			/* Optional boolean argument */
	Arg_bool(b, &Boptbool, 0),			/* Optional boolean argument */
	Arg_float(f, &Ffloat, 0, "decay", OPTARG),	/* Optional flag arg expecting floating point value */
	Arg_double(d, &Ddouble, 0, "decay2", OPTARG),	/* Optional flag arg expecting double-precision floating point value */
	Arg_string(B, 0, Bstrfunc, "name", OPTARG|MANY),/* Optional flag args expecting string value */
	Arg_int(T, &Traceflag, getInt1, "level", OPTARG|OPTVAL),/* Optional flag arg expecting optional integer value */
	Arg_minus(&Minus, 0),				/* Optional flag -- just '-' */
	Arg_N(1, &Ddstring, Ddfunc, "infile", STRING, 0),/* Mandatory positional string (must be 1st non-flag argument) */
	Arg_noflag(0, opt2strfunc, "name1", OPTARG),	/* Optional string argument */
	Arg_noflag(0, opt2strfunc, "name2", OPTARG),	/* Optional string argument */
	Arg_noflag(0, optstrfunc, "param", OPTARG|MANY),/* Optional string arguments */
	Arg_end
};

#define	Fprintf	(void)fprintf



int
main(argc, argv)
	int	argc;
	char *	argv[];
{
	DoArgs(argc, argv, Usage);

	Fprintf
	(
		stderr,
		"%s: a=%c, b=%c, f=%.1f, d=%.1f, B=\"%s\", D=\"%s\", Opt2=\"%s\", Opt=\"%s\"%s\n",
		Name,
		Aoptbool?'T':'F',
		Boptbool?'T':'F',
		Ffloat,
		Ddouble,
		Bstring,
		Ddstring,
		Opt2string,
		Optstring,
		Minus?" -.":"."
	);

	exit(EX_OK);
}

void
finish(reason)
	int	reason;
{
	exit(reason);
}

AFuncv
Bstrfunc(val, arg)
	PassVal		val;
	Pointer		arg;
{
	register char *	os;
	static char *	string;

	if ( (os = string) != NULLSTR )
		string = concat(os, " ", val.p, NULLSTR);
	else
		string = val.p;

	Trace((1, "Bstrfunc(\"%s\") [%s] => %s\n", val.p, Bstring, string));

	Bstring = string;

	if ( os != NULLSTR )
		free(os);

	return ACCEPTARG;
}

AFuncv
Ddfunc(val, arg)
	PassVal		val;
	Pointer		arg;
{
	Trace((1, "Ddfunc(\"%s\") [%s]\n", val.p, Ddstring));
	*(char **)arg = val.p;
	return ACCEPTARG;
}

AFuncv
opt2strfunc(val, arg)
	PassVal		val;
	Pointer		arg;
{
	register char *	os;
	static char *	string;
	static int	count;

	if ( ++count > 2 )
		return REJECTARG;	/* Accept first two arguments only */

	if ( (os = string) != NULLSTR )
		string = concat(os, " ", val.p, NULLSTR);
	else
		string = val.p;

	Trace((1, "opt2strfunc(\"%s\") => %s\n", val.p, string));

	Opt2string = string;

	if ( os != NULLSTR )
		free(os);

	return ACCEPTARG;
}

AFuncv
optstrfunc(val, arg)
	PassVal		val;
	Pointer		arg;
{
	register char *	os;
	static char *	string;

	if ( (os = string) != NULLSTR )
		string = concat(os, " ", val.p, NULLSTR);
	else
		string = val.p;

	Trace((1, "optstrfunc(\"%s\") => %s\n", val.p, string));

	Optstring = string;

	if ( os != NULLSTR )
		free(os);

	return ACCEPTARG;
}
