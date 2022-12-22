/*
 * items.c - handles itemized list commands (formerly part of translate.c)
 *		Release 2.0	January 1990
 *
 * Copyright 1988, 1989, 1990  Beverly A.Erlebacher
 * erlebach@cs.toronto.edu    ...uunet!utai!erlebach
 *
 */

#include <stdio.h> 
#include "texi2roff.h"

#define ITEMIZE	    0
#define ENUMERATE   1
#define TABLE	    2
#define APPLY	    3

#define MAXILEVEL	10
int  icount[MAXILEVEL];
int  what[MAXILEVEL];
char item[MAXILEVEL][MAXLINELEN];

extern int  ilevel;
extern char * gettoken();
extern char * eatwhitespace();
extern void errormsg();
extern struct tablerecd * lookup();

/*
 * itemize - handle the itemizing start commands @enumerate, @itemize
 *	and @table
 */

char * itemize(s, token)
char * s;
char * token;
{
    char *tag;

    tag = item[ilevel];
    if (STREQ(token,"@itemize")) {
	what[ilevel] = ITEMIZE;
	s = gettoken(eatwhitespace(s),tag);
	if (*tag == '\n') { /* this is an error in the input */
	    --s;
	    *tag = '-';
	    errormsg("missing itemizing argument ","");
	} else {
	    if (*tag =='@') {
		if ((lookup(tag)==NULL) && (lookup(strcat(tag,"{"))==NULL))
		     errormsg("unrecognized itemizing argument ",tag);
		else
		    if (*(tag + strlen(tag) - 1) == '{')
		    	(void) strcat(tag,"}");
	    }
	}
	(void) strcat(tag, " ");
    } else if (STREQ(token,"@enumerate")) {
	what[ilevel] = ENUMERATE;
	icount[ilevel] = 1;
    } else if (STREQ(token,"@table")) {
	what[ilevel] = TABLE;
	s = gettoken(eatwhitespace(s),tag);
	if (*tag == '\n') {
	    *tag = '\0';  /* do nothing special */
	    --s;
	} else {
	    if (*tag =='@') {
		if ((lookup(tag)==NULL) && (lookup(strcat(tag,"{"))==NULL))
		    errormsg("unrecognized itemizing argument ",tag);
		else {
		    what[ilevel] = APPLY;
		    if (*(tag + strlen(tag) - 1) != '{')
		    	(void) strcat(tag,"{");
		}
	    }
	}
    }
    while (*s != '\n' && *s != '\0') 
	++s;  /* flush rest of line */
    return s;
}

/*
 * doitem - handle @item and @itemx
 */

char *
doitem(s, tag)
char * s;
char *tag;
{
    switch (what[ilevel]) {
    case ITEMIZE:
	(void) strcpy(tag, item[ilevel]);
	break;
    case ENUMERATE:
	(void) sprintf(tag, "%d.", icount[ilevel]++);
	break;
    case TABLE:
	s = eatwhitespace(s);
	if (*s == '\n') {
	    *tag++ = '-';
	    errormsg("missing table item tag","");
	} else 
	    while(*s != '\n')
		*tag++ = *s++;
	*tag = '\0';
	break;
    case APPLY:
	*tag = '\0';
	(void) strcat(tag,item[ilevel]);
	tag += strlen(tag);
	s = eatwhitespace(s);
	while(*s != '\n')
	    *tag++ = *s++;
	*tag++ = '}';
	*tag = '\0';
	break;
    }
    return s;
}
