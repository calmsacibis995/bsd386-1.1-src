/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Argument parsing.
**
**	Returns either a positive number of arguments processed,
**	or a negative number of errors encountered.
**
**	Errors call (*usagerr)(usagestring).
**
**	RCSID $Id: Args.c,v 1.1.1.1 1992/09/28 20:08:31 trent Exp $
**
**	$Log: Args.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:31  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	ARGS

#include	"global.h"


bool		ArgsIgnFlag;		/* All succeeding flags are strings */

#define	QUERYARGS	"-?"
#define	QUERYVERSION	"-#"

static char *	QueryArgs	= QUERYARGS;
static char *	QueryVersion	= QUERYVERSION;

typedef enum
{
	accept1, accept2, acceptbool, reject, error
}
		Result;

static AFuncv	FuncVal;
static Result	process_arg();
static char *	argusage();

char *		ArgTypes[MAXTYPE+1] =
{
	EmptyStr,
	EmptyStr,			/* BOOL */
	english("string"),		/* STRING */
	english("char"),		/* CHAR */
	english("integer"),		/* INT */
	english("float"),		/* FLOAT */
	english("integer"),		/* LONG */
	english("octal_number"),	/* OCT */
	english("hex_number"),		/* HEX */
	english("number"),		/* NUMBER */
	english("float"),		/* DOUBLE */
	english("integer"),		/* SHORT */
};



int
EvalArgs(argc, argv, usage, usagerr)
	int		argc;
	char *		argv[];
	Args *		usage;
	Funcp		usagerr;
{
	register Args *	up;
	register char *	argv0;
	register int	count	= 0;
	register int	flag	= -1;
	register int	nfcount	= 0;
	char *		val;
	int		errs	= 0;
	bool		ign_no_match = false;
	bool		ign_dups = false;
	char		buf[512];

	if ( AVersion == NULLSTR )
		if ( (AVersion = Name) == NULLSTR )
			AVersion = EmptyStr;
		else
		if ( strncmp(AVersion, "@(#)", 4) == STREQUAL )
			AVersion += 4;

	Trace((1, "EvalArgs(%d, \"%s\", 0x%x, 0x%x)", argc, argv[0], usage, usagerr));

	for ( up = usage ; up->type ; up++ )
		if ( up->opt & SPECIAL )
		{
			if ( up->type == IGNNOMATCH )
			{
				ign_no_match = true;
				Trace((2, "ign_no_match"));
			}
			if ( up->type == IGNDUPS )
			{
				ign_dups = true;
				Trace((2, "ign_dups"));
			}
		}

	for ( argv0 = argv[0] ; argc ; )
	{
		Trace((3, "\"%s\"", argv0));

		if ( flag < 0 )
			flag = argv0[0] == '-';

		val = argv0;

		for ( up = usage ; up->type ; up++ )
			if ( up->posn == count && !(up->opt & (SPECIAL|REJECT|NFPOS)) )
				goto found;

		DODEBUG(if(ArgsIgnFlag)Trace((2, "ArgsIgnFlag")));

		if ( flag && !ArgsIgnFlag )
		{
			Uchar	c = ((Uchar *)argv0)[1];

			Trace((4, "\tflag '%c'", c?c:'-'));

			for ( up = usage ; up->type ; up++ )
			{
				Trace((9, "\tflagchar '%o'", up->flagchar));

				if
				(
					(
						up->flagchar == c
						||
						(up->opt & ANYFLAG)
						||
						(up->flagchar == NULLFLAG && argv0[0] == '-')
					)
					&&
					!(up->opt & (SPECIAL|REJECT))
					&&
					(
						!(up->opt & FOUND)
						||
						(up->opt & MANY)
						||
						ign_dups
					)
				)
				{
					if ( c && up->flagchar != NULLFLAG )
						val = (up->type==BOOL)?&argv0[1]:&argv0[2];
					goto found;
				}
			}
		}
		else
		{
			Trace((4, "\tnon-flag \"%s\" nfc %d", argv0, nfcount));

			for ( up = usage ; up->type ; up++ )
				if
				(
					up->posn == nfcount
					&&
					(up->opt & (NFPOS|REJECT)) == NFPOS
				)
					goto found;

			for ( up = usage ; up->type ; up++ )
				if
				(
					(up->flagchar == NONFLAG || up->flagchar == NULLFLAG)
					&&
					!(up->opt & (SPECIAL|REJECT))
					&&
					(
						!(up->opt & FOUND)
						||
						(up->opt & MANY)
					)
				)
					goto found;
		}

		if
		(
			ign_no_match
			&&
			strcmp(argv0, QueryArgs) != STREQUAL
			&&
			strcmp(argv0, QueryVersion) != STREQUAL
		)
		{
			Trace((2, "\tignored nomatch"));
			goto out;
		}

		up = (Args *)0;
err:
		Trace((2, "\terror"));
		if ( (val = argusage(buf, up, count, argv0)) != NULLSTR )
			(void)(*usagerr)(val);
		errs++;
		goto out;

found:
		if ( (up->opt & (FOUND|MANY)) == FOUND && !ign_dups )
		{
			up = (Args *)0;
			goto err;
		}

		Trace((3, "\trule %d", up-usage));

		switch ( process_arg(up, val, argc, argv) )
		{
		case accept2:	argv++;
				argc--;
		case accept1:	break;

		case acceptbool:argv0++;
				if ( up->flagchar == MINFLAG || argv0[1] == '\0' )
					break;
				up->opt |= FOUND;
				continue;

		case reject:	up->opt |= REJECT;
				continue;

		default:	goto err;
		}

		up->opt |= FOUND;
out:
		for ( up = usage ; up->type ; up++ )
			up->opt &= ~REJECT;

		if ( --argc <= 0 )
			break;

		argv0 = *++argv;
		count++;
		if ( !flag )
			nfcount++;
		flag = -1;
	}

	if ( errs )
		return -errs;

	for ( up = usage ; up->type ; up++ )
		if ( (up->opt & (SPECIAL|OPTARG|FOUND)) == 0 )
			return -1;

	return count;
}

static Result
process_arg(up, val, argc, argv)
	register Args *		up;
	register char *		val;
	int			argc;
	char *			argv[];
{
	register Pointer	vp = up->argp;
	register Result		retval = accept1;
	register int		zero = 0;
	PassVal			r;
	int			type = up->type;
	int			maybezero = 0;

	Trace((2, "process_arg(up=%x, val=%s, argc=%d, argv[0]=%s)", up, val, argc, argv[0]));

	if ( type != BOOL )
		if ( up->opt & OPTVAL )
		{
/*			if ( val[0] == '\0' && argc > 1 && argv[1][0] != '-' )
**			{
**				val = argv[1];
**				retval = accept2;
**			}
*/			maybezero = (val[0] == '0' || val[0] == '\0');
		}
		else
		{
			while ( val[0] == '\0' && up->flagchar > SPECIALFLAGS )
			{
				if ( argc <= 1 || retval == accept2 || argv[1][0] == '-' )
					return error;
				val = argv[1];
				retval = accept2;
			}
			maybezero = val[0] == '0';
		}

	switch ( type )
	{
	case BOOL:	r.b = true;
			if ( vp != (Pointer)0 )
				*(bool *)vp = r.b;
			retval = acceptbool;
			break;

	case STRING:	r.p = val;
			if ( vp != (Pointer)0 )
				*(char **)vp = r.p;
			break;

	case CHAR:	r.c = val[0];
			if ( vp != (Pointer)0 )
				*(char *)vp = r.c;
			break;

_int_:	case LONG:	if ( (r.l = atol(val)) == 0 )
				zero = 1;
_numb_:			if ( vp != (Pointer)0 )
				*(long *)vp = r.l;
_zero_:			if ( zero )
			{
				if ( !maybezero )
					return error;
				if ( (up->opt & OPTVAL) && val[0] != '\0' && val[0] != '0' )
					retval = acceptbool;
			}
			break;

	case FLOAT:	if ( (r.f = atof(val)) == 0 )
				zero = 1;
			if ( vp != (Pointer)0 )
				*(float *)vp = r.f;
			goto _zero_;

	case DOUBLE:	if ( (r.d = atof(val)) == 0 )
				zero = 1;
			if ( vp != (Pointer)0 )
				*(double *)vp = r.d;
			goto _zero_;

_hex_:	case HEX:	if ( (r.l = xtol(val)) == 0 )
				zero = 1;
			goto _numb_;

	case INT:	if ( (r.l = atoi(val)) == 0 )
				zero = 1;
			if ( vp != (Pointer)0 )
				*(int *)vp = r.l;
			goto _zero_;

	case SHORT:	if ( (r.s = atol(val)) == 0 )
				zero = 1;
			if ( vp != (Pointer)0 )
				*(short *)vp = r.s;
			goto _zero_;

	case NUMBER:	if ( !maybezero || val[0] == '\0' )
				goto _int_;
			if ( ((++val)[0] | 040) == 'x' )
			{
				++val;
				goto _hex_;
			}
	case OCT:	if ( (r.l = otol(val)) == 0 )
				zero = 1;
			goto _numb_;

	default:	return error;
	}

	if ( up->func != (AFuncp)0 )
	{
		FuncVal = (*up->func)(r, vp, val);

		switch ( (int)FuncVal )
		{
		case (int)ACCEPTARG:	break;
		case (int)REJECTARG:	return reject;
		default:		return error;
		}
	}

	return retval;
}

static char *
argusage(buf, up, argn, argp)
	char *		buf;
	register Args *	up;
	int		argn;
	char *		argp;
{
	register char *	cp;
	register char *	np;
	static char	rd[] = english("thstndrdthththththth");

	Trace((1, "argusage(%d, %s)", argn, argp));

	if ( up == (Args *)0 )
	{
		if ( strcmp(argp, QueryArgs) == STREQUAL )
		{
			ExplainArgs = true;
			return NULLSTR;
		}

		if ( strcmp(argp, QueryVersion) == STREQUAL )
		{
			ExplVersion = true;
			return NULLSTR;
		}

		if ( argn > 3 && argn < 21 )
			cp = rd;
		else
			cp = &rd[(argn%10)*2];

		(void)sprintf(buf, english("\"%s\" unexpected in %d%.2s argument"), argp, argn, cp);
		return buf;
	}

	if ( FuncVal > ARGERROR )
	{
		if ( up->posn != OPTPOS || up->flagchar == NONFLAG || up->flagchar == MINFLAG )
			(void)sprintf(buf, english("argument %d %s \"%s\""), argn, FuncVal, argp);
		else
			(void)sprintf(buf, english("flag '%c' %s"), up->flagchar, FuncVal);
		return buf;
	}

	if ( up->type == 0 || up->type > MAXTYPE )
		cp = "[BAD TYPE]";
	else
	if ( (cp = up->descr) == NULLSTR )
		cp = ArgTypes[up->type];

	if ( up->opt & OPTVAL )
		np = english("an optional");
	else
		np = strchr(english("aeiou"), cp[0]) != NULLSTR ? english("an") : english("a");

	if ( up->posn != OPTPOS )
		(void)sprintf(buf, english("argument %d must be %s <%s>"), argn, np, cp);
	else
	if ( up->flagchar == NONFLAG )
		(void)sprintf(buf, english("expected %s <%s> at argument %d"), np, cp, argn);
	else
	if ( up->flagchar == MINFLAG )
		(void)sprintf
		(
			buf,
			english("expected \"-%s%s%s\" at argument %d"),
			up->opt&OPTVAL?"[":EmptyStr,
			cp,
			up->opt&OPTVAL?"]":EmptyStr,
			argn
		);
	else
		(void)sprintf(buf, english("flag '%c' needs %s <%s>"), up->flagchar, np, cp);

	return buf;
}
