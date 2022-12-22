/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Routines to process SYSFILE fields.
**
**	RCSID $Id: SysFile.c,v 1.1.1.1 1992/09/28 20:08:43 trent Exp $
**
**	$Log: SysFile.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:43  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	CTYPE
#define	FILES
#define	TIME

#include "global.h"

/*
**	Defaults.
*/

#define	RETRYTIME	600	/* Sec. between calls */

/*
**	Time field selection.
*/

typedef struct
{
	char *	name;
	int	len;
	bool	(*funcp)();
	int	day;
}
		Tact;

static bool	f_any(Tact *, struct tm *, bool *);
static bool	f_eve(Tact *, struct tm *, bool *);
static bool	f_day(Tact *, struct tm *, bool *);
static bool	f_night(Tact *, struct tm *, bool *);
static bool	f_nonpk(Tact *, struct tm *, bool *);
static bool	f_poll(Tact *, struct tm *, bool *);
static bool	f_wk(Tact *, struct tm *, bool *);

Tact	Times[] =	/* Sort ! */
{
	{"Any",		3,	f_any},
	{"Evening",	7,	f_eve},
	{"Fr",		2,	f_day,	5},
	{"Mo",		2,	f_day,	1},
	{"Night",	5,	f_night},
	{"NonPeak",	7,	f_nonpk},
	{"Polled",	6,	f_poll},
	{"Sa",		2,	f_day,	6},
	{"Su",		2,	f_day,	0},
	{"Th",		2,	f_day,	4},
	{"Tu",		2,	f_day,	2},
	{"We",		2,	f_day,	3},
	{"Wk",		2,	f_wk}
};

#define	NTIMES	ARR_SIZE(Times)

/*
**	Variables set from TIME field.
*/

int		SysDebug;
bool		SysKeepDebug;
int		SysPktSize;
char *		SysProtocols;
int		SysRetryTime;
int		SysWindows;

extern char	MaxGrade;
extern char	DefMaxGrade;

/*
**	Local functions.
*/

static bool	check_date(char *, struct tm *);
static bool	check_time(char *, struct tm *, char *);
static int	time_cmp(char *, char *);


/*
**	Check system time field for currency.
*/

CallType
CheckFieldTime(
	char *		field
)
{
	struct tm *	tp;

	Trace((1, "CheckFieldTime(%s)", field));

	SetTimes();
	tp = localtime((long *)&Time);	/* Used by check_time() */

	if ( check_date(field, tp) )
		return CF_SYSTEM;	/* Time ok */

	Debug((3, "Wrong time (%s) to call", field));

	return CF_TIME;
}

/*
**	Check system time fields for currency.
**
**	(Use `FindSysEntry(node, NEEDSYS, ...)' first.)
**
**	NB: modifies `SysDetails'!
*/

CallType
CheckSysTime(
	char *		node,
	Mst		role
)
{
	register char *	cp;
	register char *	dp;
	struct tm *	tp;
	char *		fields[SYS_LINE];

	Trace((	1,
		"CheckSysTime(%s, %s) [%.32s]",
		node,
		(role==Master) ? "Master" : "Slave",
		SysDetails
	));

	/** Parse SysDetails **/

	SetTimes();
	tp = localtime((long *)&Time);	/* Used by check_time() */

	/** NB: `SysDetails' start *after* SYS_NAME **/

	for ( dp = SysDetails ; dp != NULLSTR ; dp = cp )
	{
		if ( (cp = strchr(dp, '\n')) != NULLSTR )
			*cp++ = '\0';

		if ( SplitSpace(fields, dp, SYS_LINE) < SYS_LINE )
		{
			Debug((3, "%s: entry with no time/line field", node));
			continue;
		}

		if ( role == Master && strcmp("SLAVE", fields[SYS_LINE-1]) == STREQUAL )
			continue;	/* Reject SLAVE */

		dp = fields[SYS_TIME-1];

		if ( check_date(dp, tp) )
			return CF_SYSTEM;	/* Time ok */

		Debug((3, "Wrong time (%s) to call", dp));
	}

	Debug((2, "Wrong time to call"));

	return CF_TIME;
}

/*
**	Check time field of SYSFILE entry for match of current time.
**
**	Format:
**	  time[</>grade][<;>mins][<$>protocols][<@>pktsize][<*>window][<!>[<k>]debug][{<,>|<|>}]...
*/

static bool
check_date(
	char *		field,
	struct tm *	tp
)
{
	register char *	cp;
	register char *	np;
	char		grade;
	bool		res;

	Trace((3, "check_date(%s)", field));

	/** Step through date[{<,>|<|>}date]... **/

	grade = '\0';
	res = false;

	for ( cp = field ; ; cp++ )
	{
		char	g;

		np = strpbrk(cp, ",|");	/* `|' for compat. */

		if ( check_time(cp, tp, &g) )
		{
			res = true;
			if ( g > grade )
				grade = g;	/* Find greatest */
		}

		if ( (cp = np) == NULLSTR )
			break;
	}

	if ( (MaxGrade = grade) == '\0' )
		MaxGrade = DefMaxGrade;

	Debug((
		res?1:5,
		"MaxGrade=%#o'%c', PktZ=%d, Windows=%d, Proto=%s, Retry=%d, Debug=%d",
		(int)MaxGrade, (MaxGrade>' '&&MaxGrade<0177)?MaxGrade:'?',
		SysPktSize,
		SysWindows,
		(SysProtocols==NULLSTR)?NullStr:SysProtocols,
		SysRetryTime,
		SysDebug
	));

	return res;
}

/*
**	Parse: type[</>grade][<;>mins][<$>protocols][<@>pktsize][<*>window][<!>[<k>]debug]
*/

static bool
check_time(
	char *		field,
	struct tm *	tp,
	char *		grade
)
{
	register char *	cp;
	register int	now;
	int		r1;
	int		r2;
	bool		ok;

	Trace((4, "check_time(%s)", field));

	/** Find type **/

	ok = false;

	for ( cp = field ; isalpha(*cp) ; )
	{
		Tact	record;
		Tact *	dp;

		record.name = cp;

		if
		(
			(dp = (Tact *)bsearch
				      (
					(char *)&record,
					(char *)Times,
					NTIMES,
					sizeof(Tact),
					time_cmp
				      )
			) == (Tact *)0
		)
		{
			cp++;
			continue;
		}

		Debug((5, "check time for %s", dp->name));

		if ( !(*dp->funcp)(dp, tp, &ok) )
			return false;

		cp += dp->len;
	}

	if ( !ok && cp != field )
		return false;

	field = cp;

	/** Pick off debug [<!>[<k>]debug] **/

	if ( (cp = strchr(field, '!')) == NULLSTR )
	{
		SysKeepDebug = false;
		SysDebug = 0;		/* => use default */
	}
	else
	{
		if ( cp[1] == 'k' )
		{
			++cp;
			SysKeepDebug = true;
		}
		else
			SysKeepDebug = false;

		if ( (SysDebug = atoi(cp+1)) <= 0 )
			SysDebug = -1;	/* Show that it was set */
	}

	/** Pick off windows [<*>number] **/

	if ( (cp = strchr(field, '*')) == NULLSTR )
		SysWindows = 0;		/* => use default */
	else
	if ( (SysWindows = atoi(cp+1)) < 0 )
		SysWindows = 0;

	/** Pick off packet size [<@>number] **/

	if ( (cp = strchr(field, '@')) == NULLSTR )
		SysPktSize = 0;		/* => use default */
	else
	if ( (SysPktSize = atoi(cp+1)) < 0 )
		SysPktSize = 0;

	/** Pick off protocols [<$>string] **/

	FreeStr(&SysProtocols);

	if ( (cp = strchr(field, '$')) != NULLSTR )
	{
		register char *	xp;

		for ( xp = ++cp ; isalpha(*xp) ; xp++ );

		if ( (now = xp-cp) > 0 )
			SysProtocols = newnstr(cp, now);
	}

	/** Pick off retry time [<;>mins] **/

	if ( (cp = strchr(field, ';')) == NULLSTR )
		SysRetryTime = RETRYTIME;	/* Default */
	else
	{
		if ( (SysRetryTime = atoi(cp+1)) <= 0 )
			SysRetryTime = 5;
		SysRetryTime *= 60;	/* Mins => secs. */
	}

	/** Pick off grade **/

	if ( (cp = strchr(field, '/')) != NULLSTR )
		*grade = cp[1];
	else
		*grade = DefMaxGrade;

	/** Pick off range "hhmm-hhmm" **/

	if ( sscanf(field, "%d-%d", &r1, &r2) != 2 )
		return true;

	Debug((5, "check time for %s", field));

	now = tp->tm_hour * 100 + tp->tm_min;	/* Convert to "hhmm" form */

	if ( r2 < r1 )	/* Crosses midnight */
	{
		if ( r1 <= now || now < r2 )
			return true;
	}
	else
		if ( r1 <= now && now < r2 )
			return true;

	return false;
}

/*
**	Routines for particular time requests.
*/

static bool
f_any(Tact *dp, struct tm *tp, bool *ok)
{
	*ok = true;
	return true;
}

static bool
f_eve(Tact *dp, struct tm *tp, bool *ok)
{
	if
	(
		tp->tm_wday == 6 || tp->tm_wday == 0	/* Sat || Sun */
		||
		tp->tm_hour >= 17 || tp->tm_hour < 8	/* ! 8am-5pm */
	)
		*ok = true;

	return true;
}

static bool
f_day(Tact *dp, struct tm *tp, bool *ok)
{
	if ( tp->tm_wday == dp->day )			/* Today */
		*ok = true;

	return true;
}

static bool
f_night(Tact *dp, struct tm *tp, bool *ok)
{
	if
	(
		tp->tm_wday == 6			/* Sat */
		||
		(tp->tm_wday == 0 && tp->tm_hour < 17)	/* Sun < 5pm */
		||
		tp->tm_hour >= 23 || tp->tm_hour < 8	/* ! 8am-11pm */
	)
		*ok = true;

	return true;
}

static bool
f_nonpk(Tact *dp, struct tm *tp, bool *ok)
{
	if
	(
		tp->tm_wday == 6 || tp->tm_wday == 0	/* Sat || Sun */
		||
		tp->tm_hour >= 18 || tp->tm_hour < 7	/* ! 7am-6pm */
	)
		*ok = true;

	return true;
}

static bool
f_poll(Tact *dp, struct tm *tp, bool *ok)
{
	return false;
}

static bool
f_wk(Tact *dp, struct tm *tp, bool *ok)
{
	if ( tp->tm_wday >= 1 && tp->tm_wday <= 5 )	/* Mon-Fri */
		*ok = true;

	return true;
}

/*
**	Compare two time field names.
*/

static int
time_cmp(
	char *	p1,
	char *	p2
)
{
	return strncmp(((Tact *)p1)->name, ((Tact *)p2)->name, ((Tact *)p2)->len);
}

/*
**	If SYSFILE has entry for SLAVE, set MaxGrade etc.
**
**	NB: modifies `SysDetails'!
*/

bool
FindSlaveGrade(
	int		baud,
	char *		line
)
{
	register char *	cp;
	register char *	dp;
	register int	f;

	Trace((1, "FindSlaveGrade(%d, %s) [%.32s]", baud, line, SysDetails));

	for ( dp = SysDetails ; dp != NULLSTR ; dp = cp )
	{
		char *	fields[SYS_PHONE];

		if ( (cp = strchr(dp, '\n')) != NULLSTR )
			*cp++ = '\0';

		if ( (f = SplitSpace(fields, dp, SYS_PHONE)) < SYS_LINE )
			continue;

		if ( strnccmp(fields[SYS_LINE-1], "SLAVE", 5) != STREQUAL )
			continue;

		if ( baud && f >= SYS_CLASS && atoi(fields[SYS_CLASS-1]) != baud )
			continue;

		if
		(
			line != NULLSTR
			&&
			f >= SYS_PHONE
			&&
			strnccmp(fields[SYS_PHONE-1], line, strlen(line)) != STREQUAL
		)
			continue;

		if ( CheckFieldTime(fields[SYS_TIME-1]) == CF_TIME )
			continue;

		return true;
	}

	return false;
}
