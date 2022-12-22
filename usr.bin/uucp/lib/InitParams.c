/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	RCSID $Id: InitParams.c,v 1.4 1994/01/31 04:44:34 donn Exp $
**
**	$Log: InitParams.c,v $
 * Revision 1.4  1994/01/31  04:44:34  donn
 * The new params dir is /etc/uucp, which isn't writable by uucp.  Put SEQF
 * in the spool directory instead.
 *
 * Revision 1.3  1994/01/31  01:25:55  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:57:32  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:41:56  vixie
 * Initial revision
 *
 * Revision 1.2  1993/02/28  15:28:44  pace
 * Add recent uunet changes
 *
 * Revision 1.1.1.1  1992/09/28  20:08:36  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.2  1992/09/10  14:37:51  ziegast
 * Dropped bsearch since it required a sorted CONFIG file.  Use lfind.
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	ARGS
#define	ERRNO
#define	PARAMS
#define	SYSEXITS

#include	"global.h"

/*
**	Global parameters.
*/

static char	Spoolaltname[]	= "SPOOLALTDIRS";
static char	Spooldirname[]	= "SPOOLDIR";

ParTb		Params[] =
{
	{"ACCESS_REMOTE_FILES",	(char **)&ACCESS_REMOTE_FILES,	PINT},
	{"ACTIVEFILE",		&ACTIVEFILE,			PSTRING},
	{"ALIASFILE",		&ALIASFILE,			PPARAM},
	{"ALLACUINOUT",		(char **)&ALLACUINOUT,		PINT},
	{"ANON_USERS",		(char **)&ANON_USERS,		PVECTOR},
	{"CMDSFILE",		&CMDSFILE,			PPARAM},
	{"CMDTIMEOUT",		(char **)&CMDTIMEOUT,		PINT},
	{"CNNCT_ACCT_FILE",	&CNNCT_ACCT_FILE,		PPARAM},
	{"CORRUPTDIR",		&CORRUPTDIR,			PSTRING},
	{"DEADLOCKTIME",	(char **)&DEADLOCKTIME,		PINT},
	{"DEBUG",		(char **)&Debugflag,		PINT},
	{"DEFAULT_CICO_TURN",	(char **)&DEFAULT_CICO_TURN,	PINT},
	{"DEVFILE",		&DEVFILE,			PPARAM},
	{"DIALFILE",		&DIALFILE,			PPARAM},
	{"DIALINOUT",		&DIALINOUT,			PPARAM},
	{"DIRCHECKTIME",	(char **)&DIRCHECKTIME,		PINT},
	{"DIR_MODE",		(char **)&DIR_MODE,		POCT},
	{"EXPLAINDIR",		&EXPLAINDIR,			PPARAMD},
	{"FILE_MODE",		(char **)&FILE_MODE,		POCT},
	{"LOCKDIR",		&LOCKDIR,			PSPOOLD},
	{"LOCKNAMEISDEV",	(char **)&LOCKNAMEISDEV,	PINT},
	{"LOCKPIDISSTR",	(char **)&LOCKPIDISSTR,		PINT},
	{"LOCKPRE",		&LOCKPRE,			PSTRING},
	{"LOCK_MODE",		(char **)&LOCK_MODE,		POCT},
	{"LOGDIR",		&LOGDIR,			PSPOOLD},
	{"LOGIN_MUST_MATCH",	(char **)&LOGIN_MUST_MATCH,	PVECTOR},
	{"LOG_BAD_SYS_NAME",	(char **)&LOG_BAD_SYS_NAME,	PINT},
	{"LOG_MODE",		(char **)&LOG_MODE,		POCT},
	{"MAILPROG",		&MAILPROG,			PPROG},
	{"MAILPROGARGS",	(char **)&MAILPROGARGS,		PVECTOR},
	{"MAXUUSCHEDS",		(char **)&MAXUUSCHEDS,		PINT},
	{"MAXXQTS",		(char **)&MAXXQTS,		PINT},
	{"MINSPOOLFSFREE",	(char **)&MINSPOOLFSFREE,	PINT},
	{"NEEDUUXQT",		&NEEDUUXQT,			PSPOOL},
	{"NODENAME",		&NODENAME,			PSTRING},
	{"NODESEQFILE",		&NODESEQFILE,			PSTRING},
	{"NODESEQLOCK",		&NODESEQLOCK,			PSTRING},
	{"NODESEQTEMP",		&NODESEQTEMP,			PSTRING},
	{"NOLOGIN",		&NOLOGIN,			PSTRING},
	{"NOSTRANGERS",		(char **)&NOSTRANGERS,		PVECTOR},
	{"ORIG_UUCP",		(char **)&ORIG_UUCP,		PINT},
	{"PARAMSDIR",		&PARAMSDIR,			PDIR},
	{"PROGDIR",		&PROGDIR,			PDIR},
	{"PROG_PARAMSDIR",	&PROG_PARAMSDIR,		PPARAMD},
	{"PUBDIR",		&PUBDIR,			PSPOOLD},
	{"SEQFILE",		&SEQFILE,			PSPOOL},
	{"SHELL",		&SHELL,				PSTRING},
	{Spoolaltname,		(char **)&SPOOLALTDIRS,		PVECTOR},
	{Spooldirname,		&SPOOLDIR,			PDIR},
	{"STATUSDIR",		&STATUSDIR,			PSPOOLD},
	{"STRANGERSCMD",	&STRANGERSCMD,			PPARAM},
	{"SYSFILE",		&SYSFILE,			PPARAM},
	{"TELNETD",		&TELNETD,			PSTRING},
	{"TMPDIR",		&TMPDIR,			PDIR},
	{"TRACEFLAG",		(char **)&Traceflag,		PINT},
	{"USERFILE",		&USERFILE,			PPARAM},
	{"UUCICO",		&UUCICO,			PPROG},
	{"UUCICOARGS",		(char **)&UUCICOARGS,		PVECTOR},
	{"UUCICO_ONLY",		(char **)&UUCICO_ONLY,		PINT},
	{"UUCP",		&UUCP,				PPROG},
	{"UUCPARGS",		(char **)&UUCPARGS,		PVECTOR},
	{"UUCPGROUP",		&UUCPGROUP,			PSTRING},
	{"UUCPUSER",		&UUCPUSER,			PSTRING},
	{"UUSCHED",		&UUSCHED,			PPROG},
	{"UUSCHEDARGS",		(char **)&UUSCHEDARGS,		PVECTOR},
	{"UUX",			&UUX,				PPROG},
	{"UUXARGS",		(char **)&UUXARGS,		PVECTOR},
	{"UUXQT",		&UUXQT,				PPROG},
	{"UUXQTARGS",		(char **)&UUXQTARGS,		PVECTOR},
	{"UUXQTHOOK",		&UUXQTHOOK,			PPARAM},
	{"VERIFY_TCP_ADDRESS",	(char **)&VERIFY_TCP_ADDRESS,	PINT},
	{"WARN_NAME_TOO_LONG",	(char **)&WARN_NAME_TOO_LONG,	PINT},
	{"WORK_FILE_MASK",	(char **)&WORK_FILE_MASK,	POCT},
	{"XQTDIR",		&XQTDIR,			PSPOOLD},
};

int		SizeofParams	= sizeof Params;

bool		CheckParams;	/* Set true to check parameters */
char *		CSpooldir;	/* Current SPOOLDIR */
bool		NewParamsFile;	/* PARAMSFILE changed from default */
bool		ParamsErr;	/* True if any parameter inconsistencies detected */
int		Pid;		/* Process ID */
int		SpoolAltDirs;	/* Number of directories in SPOOLALTDIRS */
int		SpoolDirLen;	/* Length of SPOOLDIR */

static bool	ParamsInit;
static char	DirErr[]	= "%s: %s directory must start with '/'";

static long	convert(char *);
static ParTb *	lookup(char *, ParTb *, int);
static char *	get_def(char **, char **);
static char **	splitvec(char *);
static char *	stripquote(char *, int);


/*
**	Compare two names (case insensitive).
*/

static int
cmp_id(
	char *	p1,
	char *	p2
)
{
	return strccmp(((ParTb *)p1)->id, ((ParTb *)p2)->id);
}



/*
**	Convert ascii number to long.
*/

static long
convert(
	register char *	s
)
{
	while ( s[0] == ' ' || s[0] == '\t' )
		s++;
	if ( s[0] != '0' )
		return atol(s);
	if ( ((++s)[0] | 040) == 'x' )
		return xtol(++s);
	return otol(s);
}



/*
**	Fix up any strings requiring pre-pended directories,
**	or appended '/'.
*/

static void
fix_PRED(
	ParTb *		table,
	register int	size
)
{
	register char *	cp;
	register ParTb *tp;
	register int	n;
	char *		dp;

	DODEBUG(static char	fmt[] = "%s=%s => %s");

	Trace((3, "fix_PRED()"));

	for ( tp = table, n = size ; --n >= 0 ; tp++ )
	{
		switch ( tp->type )
		{
		default:
			continue;

		case PDIR:
		case PPARAMD:
		case PSPOOLD:
			break;
		}

		if ( (cp = *tp->val) == NULLSTR )
			continue;

		if ( cp[strlen(cp)-1] == '/' )
			continue;

		*tp->val = concat(cp, Slash, NULLSTR);
		Trace((2, fmt, tp->id, cp, *tp->val));

		if ( tp->flag & NEW )
			free(cp);
		tp->flag |= NEW;
	}

	for ( tp = table, n = size ; --n >= 0 ; tp++ )
	{
		switch ( tp->type )
		{
		default:
			continue;

		case PSPOOL: case PSPOOLD:
			dp = SPOOLDIR;
			break;

		case PPARAM: case PPARAMD:
			dp = PARAMSDIR;
			break;

		case PPROG:
			dp = PROGDIR;
			break;

		case PDIR:
			dp = NULLSTR;
			break;
		}

		if ( (cp = *tp->val) == NULLSTR || cp[0] == '/' )
			continue;

		if ( dp == NULLSTR )
		{
			ErrVal = EX_USAGE;
			Error(DirErr, cp, tp->id);
			continue;
		}

		*tp->val = concat(dp, cp, NULLSTR);
		Trace((2, fmt, tp->id, cp, *tp->val));

		if ( tp->flag & NEW )
			free(cp);
		tp->flag |= NEW;
	}
}



/*
**	Check new PARAMSFILE.
*/

AFuncv
getPARAMSFILE(PassVal val, Pointer ptr, char * str)
{
	if ( val.p == NULLSTR || val.p[0] != '/' )
		return ARGERROR;

	PARAMSFILE = newstr(val.p);	/* In case setproctitle() used. */
	NewParamsFile = true;
	return ACCEPTARG;
}



/*
**	Fetch operating parameters from a file.
*/

void
GetParams(
	char *		afile,
	ParTb *		table,
	register int	size
)
{
	register char *	cp;
	register ParTb *tp;
	char *		val;
	char *		data;
	char *		file;
	char *		bufp;
	bool		free_file;

	Trace((1, "GetParams(%s, 0x%lx, %d)", afile, (Ulong)table, size));

	if ( (size /= sizeof(ParTb)) == 0 || afile == NULLSTR || afile[0] == '\0' )
		return;

	if ( afile[0] == '/' )
	{
again:		file = afile;
		free_file = false;
	}
	else
	{
		file = concat(PROG_PARAMSDIR, afile, NULLSTR);
		free_file = true;
	}

	if ( (cp = ReadFile(file)) == NULLSTR )
	{
		if ( errno != ENOENT )
		{
			if ( CheckParams )
				(void)SysWarn(CouldNot, ReadStr, file);
			ParamsErr = true;
		}
		if ( free_file )
			free(file);
		fix_PRED(table, size);
		return;
	}

	bufp = data = cp;

	while ( (cp = get_def(&val, &bufp)) != NULLSTR )
	{
		Trace((3, "process param %s", cp));

		if ( (tp = lookup(cp, table, size)) == (ParTb *)0 )
		{
			if ( CheckParams )
				Warn(english("Unmatched parameter \"%s\" in \"%s\""), cp, file);
			ParamsErr = true;
			continue;
		}

		Trace((2, "%s=[%s]", cp, val==NULLSTR?NullStr:val));

		switch ( tp->type )
		{
		case PVECTOR:	*(char ***)tp->val = splitvec(val);
				tp->flag |= NEW;
				break;
		case PLONG:	*(long *)tp->val = convert(val);	break;
		case PINT: case POCT: case PHEX:
				*(int *)tp->val = (int)convert(val);	break;
		default:	*tp->val = stripquote(val, tp->type);
				tp->flag |= NEW;
				break;
		}

		tp->flag |= SET;
	}

	free(data);
	if ( free_file )
		free(file);

	fix_PRED(table, size);
}



/*
**	Return next definition from file.
**
**	name[<space>]<=>val
*/

static char *
get_def(
	char **		vp,
	char **		bufp
)
{
	register char *	cp;
	register char *	dp;
	register int	c;

	for ( ;; )
	{
		if ( (dp = GetLine(bufp)) == NULLSTR )
			return NULLSTR;

		if ( (cp = strchr(dp, '=')) != NULLSTR && cp[1] != '\0' )
		{
			*vp = cp+1;
			while ( (c = cp[-1]) == ' ' || c == '\t' )
				--cp;
			*cp = '\0';
			return dp;
		}

		Trace((1, "bad params line: %s", dp));
	}
}



/*
**	Initialise operating parameters for a program.
*/

void
InitParams()
{
	if ( ParamsInit )
		return;

	Pid = getpid();

	(void)SetTimes();

	GetParams(PARAMSFILE, Params, sizeof Params);
	Set_ALTDIRS();
	GetParams(Name, Params, sizeof Params);

	(void)umask(WORK_FILE_MASK);

	InitVC();

	ParamsInit = true;
}



/*
**	Lookup parameter name in table.
*/

static ParTb *
lookup(
	char *	name,
	ParTb *	table,
	int	nel
)
{
	ParTb	key;

	Trace((4, "lookup(%s)", name));

	key.id = name;

	return (ParTb *)lfind
			(
				(char *)&key,
				(char *)table,
				&nel,
				sizeof(ParTb),
				cmp_id
			);
}



/*
**	Fix up special case of SPOOLALTDIRS:
**	it must start with SPOOLDIR,
**	and each directory must begin and end in '/'.
*/

void
Set_ALTDIRS()
{
	register char *	cp;
	register char **cpp;
	register char **npp;
	register int	len;
	register ParTb *tp;
	char **		new;

	Trace((3, "Set_ALTDIRS()"));

	CSpooldir = SPOOLDIR;
	SpoolDirLen = strlen(SPOOLDIR);
	SpoolAltDirs = 1;
	len = 2;	/* SPOOLDIR + NULLSTR */

	if ( (cpp = SPOOLALTDIRS) != (char **)0 )
		while ( (cp = *cpp++) != NULLSTR )
			len++;

	new = npp = (char **)Malloc(len*sizeof(char *));

	*npp++ = newstr(SPOOLDIR);

	if ( (cpp = SPOOLALTDIRS) != (char **)0 )
		while ( (cp = *cpp++) != NULLSTR )
		{
			if ( cp[0] != '/' )
			{
				ErrVal = EX_USAGE;
				Error(DirErr, cp, Spoolaltname);
			}

			len = strlen(cp);

			if ( cp[len-1] != '/' )
			{
				if
				(
					len == SpoolDirLen-1
					&&
					strncmp(cp, SPOOLDIR, len) == STREQUAL
				)
					continue;

				cp = concat(cp, Slash, NULLSTR);
			}
			else
			if ( strcmp(cp, SPOOLDIR) != STREQUAL )
				cp = newstr(cp);
			else
				continue;

			*npp++ = cp;

			SpoolAltDirs++;
		}

	*npp++ = NULLSTR;

	tp = lookup(Spoolaltname, Params, ARR_SIZE(Params));

	if ( tp->flag & NEW )	/* Free old vector */
	{
		for ( cpp = *(char ***)tp->val ; (cp = *cpp++) != NULLSTR ; )
			free(cp);

		free(*(char **)tp->val);
	}

	SPOOLALTDIRS = new;

	tp->flag |= NEW;

	Trace((3, "SpoolAltDirs => %d", SpoolAltDirs));
}



/*
**	Set and add trailing '/' to new SPOOLDIR.
*/

void
Set_SPOOLDIR(
	char *	new
)
{
	ParTb *	tp;
	char *	cp;
	int	len = strlen(new);

	Trace((2, "Set_SPOOLDIR(%s)", new));

	if ( new[len-1] != '/' )
	{
		if ( len == SpoolDirLen-1 && strncmp(new, SPOOLDIR, len) == STREQUAL )
			return;

		cp = concat(new, Slash, NULLSTR);
	}
	else
	{
		if ( strcmp(new, SPOOLDIR) == STREQUAL )
			return;

		cp = newstr(new);
	}

	tp = lookup(Spooldirname, Params, ARR_SIZE(Params));
	if ( tp->flag & NEW )
		free(SPOOLDIR);
	SPOOLDIR = cp;
	tp->flag |= NEW;

	fix_PRED(Params, ARR_SIZE(Params));
	Set_ALTDIRS();				/* Resets SpoolDirLen */
}



/*
**	Skip over quoted strings, return non-null if more.
*/

static char *
skipquote(
	register char *	cp
)
{
	if ( cp[0] == '"' )
	{
		do
			if ( (cp = strchr(++cp, '"')) == NULLSTR )
				break;
		while
			( cp[-1] == '\\' );

		if ( *++cp == '\0' )
			cp = NULLSTR;
	}
	else
		cp = strchr(cp, ',');

	Trace((5, "skipquote => %s", cp==NULLSTR?NullStr:cp));

	return cp;
}



/*
**	Split parameter of form:-
**		param	::= el[,el ...]
**		el	::= qstring|string
**		qstring	::= <">string<">
*/

static char **
splitvec(
	char *		val
)
{
	register int	n;
	register char *	cp;
	register char *	np;
	register char **vp;
	char **		vec;

	if ( (cp = val) == NULLSTR )
	{
		Trace((3, "splitvec(%s)", NullStr));
		return (char **)0;
	}

	Trace((3, "splitvec(%s)", val));

	for ( n = 2 ; (cp = skipquote(cp)) != NULLSTR ; cp++ )
		n++;

	Trace((4, "splitvec => %d strings", n-1));

	vec = vp = (char **)Malloc(n * sizeof(char *));

	for ( np = val ; (cp = np) != NULLSTR ; )
	{
		if ( (np = skipquote(cp)) != NULLSTR )
			*np++ = '\0';

		if ( (*vp++ = stripquote(cp, PSTRING)) == NULLSTR )
			--vp;
	}

	*vp = NULLSTR;

	return vec;
}



/*
**	Strip optional leading/trailing spaces and quotes from value.
*/

static char *
stripquote(
	register char *	cp,
	int		type
)
{
	register int	n;
	register int	c;
	register char *	np;
		
	Trace((4, "stripquote(%s, %s)", cp==NULLSTR?NullStr:cp, type==PSTRING?"STRING":"DIR"));

	if ( cp == NULLSTR )
		goto null;

	while ( (c = cp[0]) == ' ' || c == '\t' )
		cp++;

	if ( cp[0] == '"' )
	{
		n = strlen(cp+1);

		while ( n > 0 )
			if ( (c = cp[n]) == ' ' || c == '\t' )
				cp[n--] = '\0';
			else
				break;

		if ( n > 0 && cp[n] == '"' )
		{
			cp[n--] = '\0';
			cp++;
		}
		else
			n++;
	}
	else
	{
		if ( cp[0] == '\\' && cp[1] == '"' )
			cp++;

		n = strlen(cp);

		while ( n > 0 )
			if ( (c = cp[n-1]) == ' ' || c == '\t' )
				cp[--n] = '\0';
			else
				break;
	}

	if ( n == 0 )
	{	
null:		Trace((3, "stripquote => NULLSTR"));
		return NULLSTR;
	}

	switch ( type )
	{
	case PDIR: case PPARAMD: case PSPOOLD:
		if ( cp[--n] != '/' )
		{
			cp = concat(cp, Slash, NULLSTR);
			break;
		}
	default:
		cp = newstr(cp);
	}

	for ( np = cp ; (np = strchr(np, '"')) != NULLSTR ; )
		if ( np[-1] == '\\' )
			(void)strcpy(np-1, np);
		else
			++np;
	
	Trace((3, "stripquote => <%s>", cp));

	return cp;
}
