/*
 * translate.c - main guts of texi2roff
 * 		Release 1.0a	August 1988
 *		Release 2.0	January 1990
 *
 * Copyright 1988, 1989, 1990  Beverly A.Erlebacher
 * erlebach@cs.toronto.edu    ...uunet!utai!erlebach
 *
 */

#include <stdio.h>
#include "texi2roff.h"

extern int transparent;		/* -t flag: dont discard things	   */
int	displaylevel = 0;	/* nesting level of 'display' text */
int	inmacroarg = NO;	/* protect roff macro args flag */
int     ilevel = 0;		/* nesting level of itemized lists */
int	linecount;
char	*filename;
char 	*inp;			/* pointer into input buffer */

extern struct tablerecd * lookup();

/* forward references */
extern char * gettoken();
extern char * eatwhitespace();
extern char * itemize();
extern char * doitem();
extern char * interpret();

/*
 * errormsg - print error messages to stderr
 */

void
errormsg( message, other)
    char	   *message;
    char	   *other;
{
    (void) fprintf(stderr, "%s line %d : %s%s\n",
	filename, linecount, message, other);
}

/*
 * translate - translate one Texinfo file
 */

int 
translate(in, inname)
    FILE	   *in;
    char	   *inname;
{
    char	   input[MAXLINELEN];
    char	   output[MAXLINELEN * 2];
    char	   token[MAXLINELEN];
    char	   *c, *cprev;
    static char	   lastchar;
    int		   savlinct;
    char	   *savfilnam;

    /*
     * save global variables linecount and filename in case this is a
     * recursive call to translate an @include file.  these variables
     * are used by errormsg() which is called from many places.
     */
    savfilnam = filename;
    savlinct = linecount;
       
    filename = inname;
    linecount = 0;
    lastchar = '\n';

    /*
     * if fgets() truncates a line in the middle of a token, a blank will
     * appear in the word containing the MAXLINELENth char in the absurdly
     * long line. this is handled by gettoken()
     */

    while (fgets(input, MAXLINELEN, in) != NULL) {
	++linecount;
	inp = input;
	*output = 0;
	if (*inp == '.')		/* protect leading '.' in input */
	    (void) strcpy(output, "\\&");
	else if (*inp == '\n') {
	    if (displaylevel > 0)
		(void) strcat(output,"\\&\n");	  /* protect newline */
	    else if (ilevel > 0)	/* indented paragraph */
		(void) strcat(output, cmds->dfltipara);
	    else			/* default paragraph */
		(void) strcat(output,cmds->dfltpara);
	}
	while (*inp != '\0') {
	    inp = gettoken(inp, token);
	    inp = interpret(token, output);
	    if (inp == NULL)
		return ERROR;
	}

	/*
	 * output, stripping surplus newlines when possible. 
	 * protect lines starting with ' or \ by adding leading \&.
	 */
	cprev = &lastchar;   /* character at end of previous output buffer */
	for( c = output; *c != '\0'; cprev = c, ++c) {
	    if (*c != '\n' || *cprev != '\n') {
		if ( *cprev == '\n' && (*c == '\\' || *c == '\''))
		    (void) fputs("\\&", stdout);
		(void) putc(*c, stdout);
	    }
	}
	lastchar = *cprev;
    }
    /* restore the globals */
    filename = savfilnam;
    linecount = savlinct;
    return 0;
}

/*
 * PUSH - macro to push pointer to table entry onto command stack
 *	  and current font onto font stack
 */

#define MAXDEPTH    20

#define PUSH(tptr)							\
    if (++stackptr == MAXDEPTH) {					\
	errormsg("stack overflow - commands nested too deeply", "");	\
	return NULL;							\
    }									\
    stack[stackptr] = tptr;						\
    (void) strcpy(fontstack[++fontptr], curfont);			\
    if (*tptr->font != '\0') 						\
	(void) strcpy(curfont, tptr->font);				


/*
 * interpret - interprets and concatenates interpreted token onto outstring
 */

char *
interpret(token, outstring)
char	*token;
char	*outstring;
{
    static struct tablerecd *stack[MAXDEPTH];
    static int	    stackptr = 0; /* zeroth element is not used */
    static int      discarding = NO;
    static int	    discardlevel = MAXDEPTH;
    static int	    fontptr;
/* large enough for "\\fR\\&\\f(CW" */
#define FONTSTRSIZE 12
    static char	    fontstack[MAXDEPTH][FONTSTRSIZE];
    static char	    curfont[FONTSTRSIZE];
    static char	    defaultfont[] = "\\fR";
    static int	    init = NO;
    struct tablerecd *tptr;
    char	    *s, *cp, tempstr[MAXLINELEN],itemtag[MAXLINELEN];
    FILE	    *fp;	/* for @include files */
    extern int	    process();	/* for @include files */

    if (init == NO) {
	(void) strcpy(fontstack[0], defaultfont);
	(void) strcpy(curfont, defaultfont);
	fontptr = 0;
	init = YES;
    }
    s = inp;
    if (stackptr > 0 && STREQ(token, stack[stackptr]->texend)) {
    /* have fetched closing token of current Texinfo command */
	if (STREQ(token, "@end")) {/* WARNING! only works from translate() */
	    s = gettoken(eatwhitespace(s),tempstr);
	    if	(! STREQ(&(stack[stackptr]->texstart[1]), tempstr) 
				&& !discarding) {
		errormsg("unexpected @end found for Texinfo cmd @", tempstr);
		return s;
	    }
	}
	if (!discarding)
	    (void) strcat(outstring, stack[stackptr]->trfend);
	
	/* restore previous active font */
	if (STREQ(curfont, fontstack[fontptr]) == NO) {
		(void) strcpy(curfont, fontstack[fontptr]);
		(void) strcat(outstring, curfont);
	}
	if (fontptr > 0)
		--fontptr;

	if (stack[stackptr]->type == DISPLAY)
	    --displaylevel;
	else if (stack[stackptr]->type == ITEMIZING) {
	    --ilevel;
	    if (!discarding && ilevel > 0)
		(void) strcat(outstring, cmds->indentend);
	}

	if (--stackptr < 0) {
	    errormsg("stack underflow", "");
	    return NULL;
	}
    	if (discarding && stackptr < discardlevel) {
	    discarding = NO;
	    discardlevel = MAXDEPTH;
    	}
	if (*token == '\n' || STREQ(token, "@end")) {
	    inmacroarg = NO;
	    return "";  		/* flush rest of line if any */
	}
    } else if (*token != '@') { 	/* ordinary piece of text */
	if (!discarding)
	    (void) strcat(outstring, token);
	if (*token == '\n') {
	    inmacroarg = NO;
	    return "";
	}
    } else {				/* start of Texinfo command */
	if ((tptr = lookup(token)) == NULL){
	    if (!discarding)
		errormsg("unrecognized Texinfo command ", token);
	} else {
	    switch (tptr->type) {
	    case ESCAPED:
		if (!discarding)
		    (void) strcat(outstring, tptr->trfstart);
		break;
	    case DISPLAY:
		++displaylevel;
		PUSH(tptr);
		if (!discarding)
		    (void) strcat(outstring, tptr->trfstart);
		break;
	    case HEADING:
		inmacroarg = YES;   /* protect ',", space in hdr macro args */
		/* fall through */
	    case INDEX:
		s = eatwhitespace(s);
		/* fall through */
	    case CHAR:	/* may be some need to distinguish these 3 in future */
	    case INPARA:
	    case PARAGRAPH:
		PUSH(tptr);
		if (!discarding)
		    (void) strcat(outstring, tptr->trfstart);
		break;
	    case DISCARD:
		PUSH(tptr);
		if (!discarding && !transparent) {
		    discarding = YES;
		    discardlevel = stackptr;
		}
		break;
	    case ITEMIZING:
		if (!discarding) {
		    (void) strcat(outstring, tptr->trfstart);
		    if (ilevel > 0)
			(void) strcat(outstring, cmds->indentstart);
		}
		PUSH(tptr);
		++ilevel;
		s = itemize(s, token);
		break;
	    case ITEM:
		PUSH(tptr);
		if (!discarding) {
		    (void) strcat(outstring, tptr->trfstart);
		    inmacroarg = YES;
		    /* set up, parse and interpret item tag */
		    s = doitem(eatwhitespace(s),itemtag);
		    cp = itemtag;
		    while (*cp != '\n' && *cp != '\0') {
			cp = gettoken(cp, tempstr);
			(void) interpret(tempstr, outstring);
		    }
		}
		break;
	    case END:
		s = gettoken(eatwhitespace(s),tempstr);
		if (!discarding) 
		 errormsg("unexpected @end found for Texinfo cmd @", tempstr);
		break;
	    case FOOTNOTE:
		PUSH(tptr);
		if (!discarding) {
		    s = gettoken(eatwhitespace(s),tempstr);
		    cp = outstring + strlen(outstring);
		    (void) interpret(tempstr, outstring);
		    (void) strcpy(tempstr, cp);
		    (void) strcat(outstring, tptr->trfstart);
			/* replicate footnote mark */
		    (void) strcat(outstring, tempstr);
		}
		break;
	    case INCLUDE:
		s = eatwhitespace(s);
		for (cp = tempstr; strchr(" \t\n",*s) == NULL; *cp++ = *s++)
			;
		*cp = '\0';
		if (!discarding && ( fp = fopen(tempstr, "r")) == NULL)
		    errormsg("can't open included file ", tempstr);
		else {
		    (void) process(fp, tempstr);
		    (void) fclose(fp);
		}
		break;
	    default:
		/* can't happen */
		errormsg("ack ptui, what was that thing? ", token);
	    }
	}
    }
    return s;
}	

/*
 * eatwhitespace - move input pointer to first char that isnt a blank or tab
 *	(note that newlines are *not* whitespace)
 */

char	       *
eatwhitespace(s)
    register char	   *s;
{
    while(*s == ' ' || *s == '\t')
	++s ;
    return s;
}


/* 
 * strpbrk_like - returns pointer to the leftmost occurrence in str of any
 *	character in set, else pointer to terminating null. 
*/

char *
strpbrk_like(str, set)
    register char *str;
    char *set;
{
    static int inited_set = 0;
    static char set_vec[256] = { 0 };

    if (!inited_set) {	/* we *know* it'll be the same every time... */
	while (*set)
 	    set_vec[(unsigned char)*set++] = 1;
	set_vec[0] = 1;
	inited_set = 1;
	}
    while (set_vec[*str] == 0)
	++str;
    return str;
}


/*
 * gettoken - fetch next token from input buffer. leave the input pointer
 *	pointing to char after token.	 may need to be modified when 
 *	new Texinfo commands are added which use different token boundaries.
 *
 *	will handle case where fgets() has split a long line in the middle
 *	of a token, but the token will appear to have been divided by a blank
 */
 
char	       *
gettoken(s, token)
    char	   *s;
    char	   *token;
{
    static char	   endchars[] = " \n\t@{}:.*";
    static char    singlequote[] = "\\(fm";
    static char    doublequote[] = "\\(pd";
    char	   *q, *t;

    q = s;
    s = strpbrk_like(q, endchars);
    if (s != q) {
	switch (*s) {
	case ' ':
	case '\n':
	case '\t':
	case '@':
	case '}':
	case ':':
	case '.':
	case '*':
	case '\0':
	    --s;
	    break;
	case '{':
	    break;
	}
    } else {	/* *s == *q */
	switch (*s) {
	case ' ':
	case '\n':
	case '\t':
	case '{':
	case ':':
	case '.':
	case '*':
	case '\0':
	    break;
	case '}':
	    if (*(s+1) == '{') /* footnotes with daggers and dbl daggers!! */
		++s;
	    break;
	case '@':
	    s = strpbrk_like(q + 1, endchars );
	    /* handles 2 char @ tokens: @{ @} @@ @: @. @* */
	    if ( strchr("{}@:.*", *s) == NULL
			|| (s > q+1 && (*s =='}' || *s == '@')))
		--s;
	    break;
	}
    }
    for (t = token; q <= s; ++q, ++t) {
	switch (*q) {
	    case '\\' :		 /* replace literal \ with \e */
		*t = *q;
		*++t = 'e';
		break;
	    case ' '  :		/* protect spaces in macro args */
		if (inmacroarg == YES) {
		    *t = '\\';
		    *++t = *q;
		} else
		    *t = *q;
		break;
	    case '\'' :		/* convert ' to footmark in macro args */
		if (inmacroarg == YES) {
		    *t = 0;
		    (void) strcat(t,singlequote);
		    t += strlen(singlequote) - 1;
		} else
		    *t = *q;
		break;
	    case '\"' :		/* try to avoid " trouble in macro args */
		if (inmacroarg == YES) {
		    *t = 0;
		    (void) strcat(t,doublequote);
		    t += strlen(doublequote) - 1;
		} else
		    *t = *q;
		break;
	    case '\0':
		*t = ' ';
		break;
	    default   :
		*t = *q;
		break;
	}
    }
    *t = 0;
    return ++s;
}
