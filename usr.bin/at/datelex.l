%{
/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

/*	BSDI datelex.l,v 1.1 1992/08/24 21:18:11 sanders Exp */

#ifndef lint
static char sccsid[] = "@(#)datelex.l	1.1 (BSDI) 08/24/92";
#endif

/*
 * lex for date parser
 */

#include <sys/param.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "when.h"
#include "errlib.h"

#include "y.tab.h"

int	yy_ac;
char	**yy_av;
char	*parsed_buf = NULL;
int	parsed_eof = 0;

/*
 * Fill parsed_buf with the actual text we read.
 * Used for debugging and informative error messages.
 */

void
parsed(char *s)
{
	static int bufsize = 0;
	static int len = 0;
	int i = strlen(s);

#ifdef	DEBUG_PARSER
	printf("parsing ``%s''\n", s);
#endif
	if (parsed_buf == NULL) {
		parsed_buf = malloc(bufsize = 512);
		parsed_buf[0] = '\0';
	}

	if (i+len > bufsize)
		realloc(parsed_buf, bufsize+=(i+1)*10);	/* room for 10 more */

	strcat(parsed_buf, s); len += i;
}

int
at_yyinput(char *buf, int maxlen)
{
	if (yy_ac) {
		int len = strlen(*yy_av) + 1;
		strcpy(buf, " ");
		strcpy(buf+1, *yy_av);
		yy_ac--, yy_av++;
		return len;
	}
	return 0;			/* no more */
}

#undef YY_INPUT
#define YY_INPUT(buf,result,maxlen) ((result) = at_yyinput(buf, maxlen))

%}

space	[ \t]+
number	[0123456789]+
u1	minutes|hours|days|weeks|months|years|fortnights
u2	minute|hour|day|week|month|year|fortnight
u3	min|hr|wk|yr
units	{u1}|{u2}|{u3}
d1	sunday|monday|tuesday|wednesday|thursday|friday|saturday
d2	sun|mon|tue|wed|thu|fri|sat
d3	su|mo|tu|we|th|fr|sa
days	{d1}|{d2}|{d3}
m1	january|febuary|march|april|may|june
m2	july|august|september|october|november|december
m3	jan|feb|mar|apr|may|jun|jul|aug|sep|oct|nov|dec
months	{m1}|{m2}|{m3}

%%

{space}		{ parsed(yytext); /* nothing */; }
{number}	{ parsed(yytext); yylval.number = strdup(yytext);
		  return NUMBER; }
{months}	{ parsed(yytext); yylval.month = strdup(yytext);
		  return MONTH; }
{days}		{ parsed(yytext); yylval.day = strdup(yytext);
		  return DAY; }
{units}		{ parsed(yytext); yylval.unit = strdup(yytext);
		  return UNIT; }
a		{ parsed(yytext); return AM; }
am		{ parsed(yytext); return AM; }
p		{ parsed(yytext); return PM; }
pm		{ parsed(yytext); return PM; }
zulu		{ parsed(yytext); return ZULU; }
n		{ parsed(yytext); return NOON; }
noon		{ parsed(yytext); return NOON; }
m		{ parsed(yytext); return MIDNIGHT; }
midnight	{ parsed(yytext); return MIDNIGHT; }
now		{ parsed(yytext); return NOW; }
next		{ parsed(yytext); return NEXT; }
":"		{ parsed(yytext); return COLON; }
"+"		{ parsed(yytext); return PLUS; }
","		{ parsed(yytext); /* nothing */; }
<<EOF>>		{ parsed_eof = 1; yyterminate(); }
.		{ Pmsg(("lex stop at ``%s''\n", yytext)); yyterminate(); }

%%
