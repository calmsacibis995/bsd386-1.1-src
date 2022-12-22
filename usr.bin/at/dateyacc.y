%{
/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

/*	BSDI dateyacc.l,v 1.1 1992/08/24 21:18:11 sanders Exp */

#ifndef lint
static char sccsid[] = "@(#)dateyacc.l	1.1 (BSDI) 08/24/92";
#endif

/*
 * date parser for at: time [date] [+increment]
 */

#include <sys/param.h>
#include <errno.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <tzfile.h>
#include "errlib.h"

#define FMT_AM   0
#define FMT_PM   1
#define FMT_ZULU 2
#define FMT_24HR 3
#define FMT_NOON 4
#define FMT_MIDNIGHT 5

struct	tm time_v;
struct	tm gm_time;
struct	tm lc_time;
time_t	now;
int	zulu_offset;

int	adjday = 0;
int	adjyear = 0;

%}

%union {
	char	*number;
	char	*month;
	char	*day;
	char	*unit;
	int	ticks;
}

%token	<number>NUMBER
%token	<month>	MONTH
%token	<day>	DAY
%token	<unit>	UNIT

%token	<int>	AM PM ZULU
%token	<int>	NOON MIDNIGHT NOW
%token	<int>	NEXT
%token	<int>	COLON PLUS

%type	<ticks>	time

%start format

%%

format	: xyzzy plugh
	;
xyzzy	: clock date		{ adjustyear(); }
	;
plugh	: incr			{ adjustday(); }
	;

clock	: /* nothing */
	| time			{ do_time($1,FMT_24HR); }
	| time AM		{ do_time($1,FMT_AM); }
	| time PM		{ do_time($1,FMT_PM); }
	| time ZULU		{ do_gmt($1); }
	| time NOON		{ do_time($1,FMT_NOON); }
	| NOON			{ do_time(12*100,FMT_24HR); }
	| time MIDNIGHT		{ do_time($1,FMT_MIDNIGHT); }
	| MIDNIGHT		{ do_time(12*100,FMT_MIDNIGHT); }
	| NOW			{ /* nothing */; }
	;
time	: NUMBER		{ $$ = do_hrmin($1); }
	| NUMBER COLON NUMBER	{ $$ = do_hr_min(atoi($1),atoi($3)); }
	;

date	: month mday year	{ do_date(); }
	| DAY			{ adjday = 0; do_wday($1); }
	;

month	: /* nothing */
	| MONTH			{ adjday = 0; do_month($1); }
	;
/*
 * NOTE: the shift/reduce conflict resolves to mday which decides it was
 * really a year if NUMBER > 31
 */
mday	: /* nothing */
	| NUMBER		{ adjday = 0; do_mday(atoi($1)); }
	;
year	: /* nothing */
	| NUMBER		{ adjday = 0; do_year(atoi($1)); }
	;
incr	: /* nothing */
	| PLUS NUMBER UNIT	{ do_incr(atoi($2),$3); }
	| UNIT			{ do_incr(1,$1); }
	| NEXT UNIT		{ do_incr(1,$2); }
	;

%%

static
yyerror(s)
	char *s;
{
	extern char *parsed_buf, **yy_av;
	errno = 0;
	Perror("%s parsing date: %s ^ %s",
		s ? s : "error", parsed_buf+1, *(yy_av-1));
	/* NOTREACHED */
}

static int dpm[2][12] = {
        { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
        { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
};

/*
 * XXX
 * Could be faster by calculating add_nunit value
 */
#define DEF_ADDUNIT(min,max,unit,nunit)				\
static void							\
add_ ## unit (int n)						\
{								\
	time_v.tm_ ## unit += n;				\
	while (time_v.tm_ ## unit > (max)) {			\
		time_v.tm_ ## unit -= (max) - (min) + 1;	\
		add_ ## nunit (1);				\
	}							\
}

static void add_year(int n) { time_v.tm_year += n; }
DEF_ADDUNIT(0,11,mon,year)
DEF_ADDUNIT(1,dpm[isleap(time_v.tm_year)][time_v.tm_mon],mday,mon)
DEF_ADDUNIT(0,23,hour,mday)
DEF_ADDUNIT(0,59,min,hour)
static void normalize() { add_min(0); add_hour(0); add_mday(0); add_mon(0); }

static void
do_time(int i, int fmt)
{
	time_v.tm_hour = i/100;
	time_v.tm_min = i%100;

	if (fmt == FMT_NOON && i != 12*100)
		Perror("noon is 12:00n not %02d:%02d", i/100, i%100);
	if (fmt == FMT_MIDNIGHT && i != 12*100)
		Perror("midnight is 12:00m not %02d:%02d", i/100, i%100);
	if (fmt == FMT_MIDNIGHT)
		i = 0;

	if (fmt == FMT_AM || fmt == FMT_PM) {
		if (time_v.tm_hour > 12)
			Perror("hours field is invalid for a 12-hour clock");
		if (time_v.tm_hour < 1)
			Perror("hours field is invalid for a 12-hour clock");
	}
	if (fmt == FMT_PM)
		time_v.tm_hour += 12;
	if (time_v.tm_hour == 24)
		time_v.tm_hour = 0;

	if (i < (lc_time.tm_hour * 100 + lc_time.tm_min)) {
		Pmsg(("may need a new day\n"));
		adjday = 1;
	}

	Pmsg(("time: %02d:%02d\n", time_v.tm_hour, time_v.tm_min));
}

static void
do_gmt(int i)
{
	zulu_offset = time_v.tm_gmtoff;
	time_v = lc_time = gm_time;
	do_time(i, FMT_ZULU);
}

static
do_hrmin(char *s)
{
	int i = atoi(s);
	char *t;
	int hr, min, len = strlen(s);

	Pmsg(("do_hrmin got ``%s''\n", s));
	if (len > 4)
		Perror("invalid time specification ``%s''", s);
	if (len > 2) {
		/* may have been something like 0002 */

		t = strdup(s);		/* because we're going to munge it */
		min = atoi(t+(len-2));	/* minute part: 02 */
		t[len-2] = '\0';	/* fix for hour: 00\02 */
		hr = atoi(t);
		free(t);
	} else {
		hr = i;
		min = 0;
	}

	if (hr > 23)
		Perror("hours field is invalid for a 24-hour clock");
	if (min > 59)
		Perror("minutes field is invalid");
	return hr * 100 + min;
}

static
do_hr_min(int hr, int min)
{
	if (hr > 23)
		Perror("hours field is invalid for a 24-hour clock");
	if (min > 59)
		Perror("minutes field is invalid");
	return hr * 100 + min;
}

/*
 * Copy first three bytes into buf from s and make lower case
 */
static void
lowern(int n, char *buf, char *s)
{
	register char *p;
	for (p=s; *p && ((p-s)<n); p++)
		buf[p-s] = isupper(*p) ? tolower(*p) : *p;
	buf[n] = '\0';
}

static void
do_month(char *s)
{
	char	*p;
	char	buf[4];
	char	*months = "janfebmaraprmayjunjulaugsepoctnovdec";

	adjyear = 1;		/* adjust year if needed */
	lowern(3, buf, s);
	if ((p = strstr(months, buf)) == NULL || ((p-months)%3) != 0)
		Perror("month %s not valid", s);
	Pmsg(("month: %s %d\n", s, ((p - months) / 3)));
	time_v.tm_mon = ((p - months) / 3);
}

static void
do_wday(char *s)
{
	char	*p;
	int	n = 2;
	char	buf[n+1];
	char	*days = "sumotuwethfrsa";
	int	day;

	lowern(n, buf, s);
	if ((p = strstr(days, buf)) == NULL || ((p-days)%n) != 0)
		Perror("wday %s is not valid", s);
	Pmsg(("p-days/n = %d, tday=%d\n",
		((p - days) / n), lc_time.tm_wday ));
	day = ((p - days) / n) - lc_time.tm_wday;
	if (day < 1)
		day += 7;
	Pmsg(("adjusted days: %d\n", day));
	add_mday(day);
	Pmsg(("wday: %s %d\n", s, time_v.tm_wday));
}

static void
do_year(int i)
{
	adjyear = 0;		/* don't adjust if specified on cmd line */
	if (i < 0)
		Perror("year too small: %d",i);
	if (i >= 1900)
		i -= 1900;
	Pmsg(("year: %d\n",i+1900));
	time_v.tm_year = i;
}

static void
do_mday(int i)
{
	if (i > 31)
		do_year(i);	/* resolve shift/reduce conflict */
	else {
		Pmsg(("mday: %d\n",i));
		time_v.tm_mday = i;
	}
}

static void
do_date()
{
	int x = dpm[isleap(time_v.tm_year)][time_v.tm_mon];
	if (time_v.tm_mday > x)
		Perror("only %d days in month", x);
}

struct unit_s {
	char	*name;
	int	minutes;
	int	hours;
	int	days;
	int	months;
	int	years;
	int	adjday;
};

static void
do_incr(int i, char *s)
{
	char *p;
	char buf[4];
	struct unit_s *q;
	struct unit_s unit_tbl[] = {
		{ "minute",	1, 0, 0, 0, 0, 1 },
		{ "hour",	0, 1, 0, 0, 0, 1 },
		{ "hr",		0, 1, 0, 0, 0, 1 },
		{ "day",	0, 0, 1, 0, 0, 1 },
		{ "week",	0, 0, 7, 0, 0, 0 },
		{ "wk",		0, 0, 7, 0, 0, 0 },
		{ "fortnight",	0, 0,14, 0, 0, 0 },
		{ "month",	0, 0, 0, 1, 0, 0 },
		{ "year",	0, 0, 0, 0, 1, 0 },
		{ "yr",		0, 0, 0, 0, 1, 0 },
		{  NULL,	0, 0, 0, 0, 0, 0 },
	};

	lowern(2, buf, s);
	for (q=unit_tbl; q->name; q++) {
		if (strncmp(q->name,buf,2) == 0) {
			add_min(q->minutes * i);
			add_hour(q->hours * i);
			add_mday(q->days * i);
			add_mon(q->months * i);
			add_year(q->years * i);
			/*
			 * ugh! try and make it feel natural
			 *
			 * What we really want to do is say if
			 * the time would be before now then
			 * adjust a day, else not.
			 *
			 * this at least works for stuff like
			 *     at 10am + 1 min   adjusts if needed
			 *     at 10am + 1 day	 doesn't adjust
			 *     at 10am + 1 week	 doesn't adjust
			 */
			if (q->adjday == 0) adjday = 0;
			Pmsg(("incr: %d %s\n", i, s));
			return;
		}
	}
	Perror("increment %d %s is not valid", i, s);
}

void
adjustday()
{
	time_t delta = mktime(&time_v) - mktime(&lc_time);

	Pmsg(("adjust: adjday=%d, delta=%d\n",
		adjday, delta));

	if (adjday && delta < 0) {
		Pmsg(("adding a new day\n"));
		add_mday(1);
	}
}

void
adjustyear()
{
	time_t delta = mktime(&time_v) - mktime(&lc_time);

	Pmsg(("adjust: adjyear=%d, delta=%d\n",
		adjyear, delta));

	if (adjyear && delta < 0) {
		Pmsg(("adding a year\n"));
		add_year(1);
		adjday = 0;
	}
}
