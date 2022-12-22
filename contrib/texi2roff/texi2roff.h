/*
 * texi2roff.h - texi2roff general header
 *		Release 1.0a	August 1988
 *		Release 2.0	January 1990
 *
 * Copyright 1988, 1989, 1990  Beverly A.Erlebacher
 * erlebach@cs.toronto.edu    ...uunet!utai!erlebach
 *
 */

/* troff macro packages supported */
#define NONE	0	/* dummy value for error detection */
#define MS	1
#define ME	2
#define MM	3

/* useful confusion-reducing things */
#define STREQ(s,t) (*(s)==*(t) && strcmp(s, t)==0)
#define NO	0
#define YES	1
#define ERROR	(-1)

/* some [nt]roffs have big problems with long lines */
#define MAXLINELEN      1024

/* miscellaneous troff command strings in macro header files. */
struct misccmds {
    char * init;	/* emit before the first input. this is the place to
			 * put troff commands controlling default point size,
			 * margin size, line length, etc.
			 */
    char * dfltpara;	/* emit when 2 consecutive newlines are detected */
			/* in the input and the indentation level is <= 1. */
    char * dfltipara;	/* same but for indentation level > 1.  */
    char * indentstart; /* emit to increase indent level for itemized list */
    char * indentend;	/* emit to decrease indent level for itemized list */
};

extern struct misccmds * cmds;

struct tablerecd {
    char *  texstart;	/* starting token for a Texinfo command */
    char *  texend;	/* ending token for a Texinfo command */
    char *  trfstart;	/* troff commands to emit when texstart is found */
    char *  trfend;	/* troff commands to emit when texend is found */
    char *  font;	/* font in effect between trfstart & trfend */
    int	    type;	/* kind of Texinfo command, as #defined below */
};

/* Texinfo command types */

#define ESCAPED	    0	/* special character (special to Texinfo) */
#define INPARA	    1	/* in-paragraph command */
#define HEADING	    2	/* chapter structuring command */
#define DISCARD	    3	/* not supported - discard following text */
#define PARAGRAPH   4	/* applies to following paragraph */
#define ITEMIZING   5	/* starts itemized list */
#define ITEM	    6	/* item in list */
#define END	    7	/* end construct */
#define CHAR	    8	/* really special char: dagger, bullet - scary, eh? */
#define FOOTNOTE    9	/* footnote */
#define DISPLAY    10	/* text block of the kind called a 'display' */
#define INDEX	   11	/* index entry */
#define INCLUDE	   12	/* include file command */
 
/* portability */
#ifdef BSD
#include <strings.h>
#define strchr  index
#else
#include <string.h>
#endif

