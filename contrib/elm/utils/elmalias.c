
static char rcsid[] = "@(#)Id: elmalias.c,v 5.8 1993/09/27 01:05:37 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.1 $
 *
 * 			Copyright (c) 1988-1992 USENET Community Trust
 * 			Copyright (c) 1986,1987 Dave Taylor
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 *******************************************************************************
 * $Log: elmalias.c,v $
 * Revision 1.1  1994/01/14  01:35:13  cp
 * 2.4.23
 *
 * Revision 5.8  1993/09/27  01:05:37  syd
 * Eliminate warning about redeclaring getpwuid().
 * From: riacs!rutgers!tscs.tscs.com!fin!chip (Chip Salzenberg)
 *
 * Revision 5.7  1993/09/19  23:12:00  syd
 * Add ability for elmalias to determine HOME if missing from the environment.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.6  1993/08/23  02:44:41  syd
 * fix where checkalias doesn't fully expand multi-database aliases
 * From: Steve Wolf <woof@atl.hp.com>
 *
 * Revision 5.5  1993/08/03  19:23:25  syd
 * Added -d option to elmalias.
 * From: Jan.Djarv@sa.erisoft.se (Jan Djarv)
 *
 * Revision 5.4  1993/07/27  20:27:33  syd
 * fix strchr, Elm uses define to make index strchr when needed
 * From: Syd
 *
 * Revision 5.3  1993/07/20  02:59:53  syd
 * Support aliases both on 64 bit and 32 bit machines at the same time.
 * From: Dan Mosedale <mosedale@genome.stanford.edu>
 *
 * Revision 5.2  1993/05/16  20:55:32  syd
 * fix elmalias bug
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.1  1993/04/12  02:09:29  syd
 * Initial Checkin
 *
 *
 ******************************************************************************/

/*
 * elmalias - Elm alias database access.
 *
 * This utility will display information from the Elm user and system
 * alias databases.  It will expand alias names specified on the command
 * line and display the alias value.  If an item on the command line is
 * not a valid alias, then it's value is returned.  If no names are
 * specified on the command line then all alises are displayed.  Output
 * is formatted by either a user-defined format specifier or one of a
 * number of predefined specifications.  The default output format
 * displays just the alias value.
 *
 * The format specifiers are:
 *
 *	%a	alias (the alias name)
 *	%l	last_name
 *	%n	name
 *	%c	comment
 *	%v	address (the alias value)
 *	%t	type (Person, Group, or Unknown)
 *
 * In the case that a command line argument does not correspond to
 * an alias, then %a and %v will evaluate to the argument value and
 * all other items will be empty.
 *
 * Printf-like field widths may be used, e.g. "%-20.20a".  Conditionals
 * are bracketed by question marks.  For example "?n(%n)?" means format
 * "(%n)" if the "name" is defined, otherwise produce no output.  Some
 * backslash sequences (e.g. "\t" == tab) are recognized.  Any other
 * characters are displayed as is.
 *
 * Example:
 *	$ elmalias -f "alias %a is address \"%v\" ?n(%n)?" chip
 *	alias chip is address "chip@chinacat.unicom.com" (Chip Rosenthal)
 *
 * Synopsis:
 *	elmalias [-adensuvV] [-f format] [alias ...]
 *
 * Options:
 *	-a	Display alias name.  Equivalent to -f "%-20.20a %v".
 *	-d	Turn debugging on. Only useful if DEBUG is defined.
 *	-e	Fully expand alias values.
 *	-f fmt	User-specified output format.
 *	-n	Display name.  Equivalent to -f "%v?n (%n)?".
 *	-r	Complain about arguments that are not valid aliases.
 *	-s	Consult system-wide alias file.
 *	-u	Consult user-specific alias file.
 *	-v	Verbose output.  Equivalent to -f "%-20.20a %v?n (%n)?".
 *	-V	Babble about *everything* we known about.
 *
 * If neither -s or -u are specified, then the default is to search
 * both alias files.
 */

#include "elmutil.h"
#include "ndbz.h"
#include "s_elmalias.h"
#include <pwd.h>

char *getenv();
#ifndef	_POSIX_SOURCE
struct passwd *getpwuid();
#endif

/*
 * Maximum number of alias files we can consult.
 */
#define MAXDB		2	/* user and system alias files		*/

/*
 * These are used by the "dbz" routines.
 */
#ifdef DEBUG
int debug = 0;
FILE *debugfile = stderr;
#endif

char *Progname;

/*
 * "aliasdb" library routines.
 */
extern struct alias_rec *fetch_alias();
extern char *next_addr_in_list();

/*
 * Local procedures.
 */
DBZ *open_user_aliases(), *open_system_aliases();
struct alias_rec *make_dummy_rec();
void exp_print_alias(), print_alias();
char *sel_alias_mem();


void usage_error()
{
    fprintf(stderr, catgets(elm_msg_cat, ElmaliasSet, ElmaliasUsage,
	"usage: %s [-adenrsuvV] [-f format] [alias ...]\n"), Progname);
    exit(1);
    /*NOTREACHED*/
}


/*ARGSUSED*/
void malloc_fail_handler(proc, size)
char *proc;
unsigned size;
{
    fprintf(stderr, catgets(elm_msg_cat, ElmaliasSet, ElmaliasOutOfMemory,
	"%s: out of memory [could not allocate %d bytes]\n"), Progname, size);
    exit(1);
    /*NOTREACHED*/
}


main(argc, argv)
int argc;
char *argv[];
{
    char *out_fmt;		/* output printing format		*/
    int do_user_alias;		/* TRUE to examine user alias file	*/
    int do_system_alias;	/* TRUE to examine system alias file	*/
    int do_expand;		/* TRUE to recursively expand aliases	*/
    int do_complain;		/* TRUE to insist args are valid aliases*/
    int numdb;			/* number of alias files to consult	*/
    DBZ *dblist[MAXDB+1];	/* NULL terminated list of files	*/
    struct alias_rec *ar;	/* scratch ptr to expansion of curr name*/
    int d, i;
    extern int optind;
    extern char *optarg;

    /*
     * Initialize.
     */
#ifdef I_LOCALE
    setlocale(LC_ALL, "");
#endif
    elm_msg_cat = catopen("elm2.4", 0); /* parlez vous francais?	*/
    safe_malloc_fail_handler =	/* install procedure to trap errors in	*/
	malloc_fail_handler;	/*   the safe_malloc() routines		*/
    Progname = argv[0];		/* program name for diag messages	*/
    do_user_alias = FALSE;	/* indicate the user hasn't selected	*/
    do_system_alias = FALSE;	/*   any alias files (yet)		*/
    do_expand = FALSE;		/* do not recursively expand groups	*/
    do_complain = FALSE;	/* allow non-aliases on cmd line	*/
    out_fmt = "%v";		/* default is to just show alias value	*/
    numdb = 0;			/* no alias files opened yet		*/

    /*
     * Crack command line options.
     */
    while ((i = getopt(argc, argv, "adef:nrsuvV")) != EOF) {
	switch (i) {
	case 'a':			/* show alias name and value	*/
	    out_fmt = "%-20.20a %v";
	    break;
	case 'd':
#ifdef DEBUG
	    debug = 10;
#endif
	    break;
	case 'e':			/* recursively expand aliases	*/
	    do_expand = TRUE;
	    break;
	case 'f':			/* user-specified format	*/
	    out_fmt = optarg;
	    break;
	case 'n':			/* show alias value and fullname*/
	    out_fmt = "%v?n (%n)?";
	    break;
	case 'r':			/* insist args are valid aliases*/
	    do_complain = TRUE;
	    break;
	case 's':			/* use system-wide alias file	*/
	    do_system_alias = TRUE;
	    break;
	case 'u':			/* use per-user alias file	*/
	    do_user_alias = TRUE;
	    break;
	case 'v':			/* verbose output format	*/
	    out_fmt = "%-20.20a %v?n (%n)?";
	    break;
	case 'V':			/* show the user's life story	*/
	    out_fmt = "\
Alias:\t\t%a\n\
  Address:\t%v\n\
  Type:\t\t%t\n\
?n  Name:\t\t%n\n?\
?l  Last Name:\t%l\n?\
?c  Comment:\t%c\n?\
";
	    break;
	default:
	    usage_error();
	    /*NOTREACHED*/
	}
    }

    /*
     * If user didn't request specific alias files then use them all.
     */
    if (!do_system_alias && !do_user_alias)
	do_system_alias = do_user_alias = TRUE;

    /*
     * Open up the alias files we need to access, in the order of priority.
     */
    if (do_user_alias && (dblist[numdb] = open_user_aliases()) != NULL)
	++numdb;
    if (do_system_alias && (dblist[numdb] = open_system_aliases()) != NULL)
	++numdb;
    dblist[numdb] = NULL;

    /*
     * If no names specified on command line then dump all alias files..
     */
    if (optind == argc) {
	if (do_expand) {
	    fprintf(stderr, catgets(elm_msg_cat, ElmaliasSet,
		ElmaliasCannotSpecifyExpand,
		"%s: cannot specify \"-e\" when dumping all aliases\n"),
		Progname);
	    exit(1);
	}
	for (d = 0 ; dblist[d] != NULL ; ++d) {
	    /* assumes file pointer left at first key immediately after open */
	    while ((ar = fetch_alias(dblist[d], (char *)NULL)) != NULL) {
		print_alias(out_fmt, ar);
		(void) free((malloc_t)ar);
	    }
	}
	exit(0);
    }

    /*
     * Expand each name on the command line.
     */
    for (i = optind ; i < argc ; ++i) {

	/*
	 * Try each of the alias files for a match.
	 */
	ar = NULL;
	for (d = 0 ; dblist[d] != NULL ; ++d) {
	    if ((ar = fetch_alias(dblist[d], argv[i])) != NULL)
		break;
	}

	/*
	 * Print the result.
	 */
	if (ar == NULL) {
	    if (do_complain) {
		fprintf(stderr, catgets(elm_msg_cat, ElmaliasSet,
		    ElmaliasUnknownAlias, "%s: \"%s\" is not a known alias\n"),
		    Progname, argv[i]);
		exit(1);
	    }
	    ar = make_dummy_rec(argv[i]);
	    print_alias(out_fmt, ar);
	} else if (do_expand && (ar->type & GROUP)) {
	    exp_print_alias(dblist, out_fmt, ar);
	} else {
	    print_alias(out_fmt, ar);
	}

	(void) free((malloc_t)ar);

    }

    exit(0);
    /*NOTREACHED*/
}


DBZ *open_system_aliases()
{
    return dbz_open(system_data_file, O_RDONLY, 0);
}


DBZ *open_user_aliases()
{
    char *home, *fname;
    DBZ *db;
    struct passwd *pw;

    if ((home = getenv("HOME")) == NULL || *home == '\0') {
	if ((pw = getpwuid(getuid())) == NULL) {
	    fprintf(stderr, catgets(elm_msg_cat, ElmaliasSet,
		ElmaliasCannotDetermineHome,
		"%s: cannot determine your HOME directory\n"), Progname);
	    exit(1);
	}
	home = pw->pw_dir;
	endpwent();
    }
    fname = (char *)safe_malloc(strlen(home) + 1 + strlen(ALIAS_DATA) + 1);
    (void) strcat(strcat(strcpy(fname, home), "/"), ALIAS_DATA);
    db = dbz_open(fname, O_RDONLY, 0);
    (void) free((malloc_t)fname);
    return db;
}


/*
 * Cobble up an alias record structure to hold some address info.
 */
struct alias_rec *make_dummy_rec(val)
char *val;
{
    struct alias_rec *ar;
    ar = (struct alias_rec *) safe_malloc(sizeof(struct alias_rec));
    ar->status = 0;
    ar->alias = val;
    ar->last_name = "";
    ar->name = "";
    ar->comment = "";
    ar->address = val;
    ar->type = 0;
    ar->length = 0;
    return ar;
}


/*
 * Recursively expand out a list of addresses and print the expansions.
 */
void exp_print_alias(dblist, fmt, ar)
DBZ *dblist[];
char *fmt;
struct alias_rec *ar;
{
    char *abuf;		/* list of addresses we can scribble upon	*/
    char *acurr;	/* pointer to current address within "abuf"	*/
    char *anext;	/* pointer to next address within "abuf"	*/
    struct alias_rec *ar0;
    int d;		/* dblist index					*/

    /*
     * Create a copy of this address we can scribble upon.
     */
    anext = abuf = safe_strdup(ar->address);
#ifdef lint
    *abuf = '\0'; /* shutup set but not used complaint */
#endif

    /*
     * Go through all of the addresses and expand them out.
     */
    while ((acurr = next_addr_in_list(&anext)) != NULL) {
	for (d = 0 ; dblist[d] != NULL ; ++d) {
	    if ((ar0 = fetch_alias(dblist[d], acurr)) != NULL)
		break;
	}
	if (ar0 == NULL)
	    ar0 = make_dummy_rec(acurr);
	if (ar0->type & GROUP)
	    exp_print_alias(dblist, fmt, ar0);
	else
	    print_alias(fmt, ar0);
	(void) free((malloc_t)ar0);
    }

    (void) free((malloc_t)abuf);
}


/*
 * Print out alias information according to a format specification.
 */
void print_alias(fmt, ar)
char *fmt;
struct alias_rec *ar;
{
    char pfmt[64];		/* buffer to hold "%m.ns" formats	*/
    int in_conditional;		/* TRUE if in middle of cond expression	*/
    int print_enab;		/* TRUE if OK to print output		*/
    char *s;
    int n, c;

    print_enab = TRUE;
    in_conditional = FALSE;

    while (*fmt != '\0') {

	switch (*fmt) {

	/*
	 * Formatted output.
	 */
	case '%':

	    /*
	     * Extract the "%m.n" portion of the format.
	     */
	    pfmt[0] = *fmt++;
	    n = 1;
	    while (index("-.0123456789", *fmt) != NULL) {
		if (n < sizeof(pfmt)-2)
		    pfmt[n++] = *fmt;
		++fmt;
	    }
	    pfmt[n++] = 's';
	    pfmt[n] = '\0';

	    /*
	     * Determine what we are printing.
	     */
	    if ((s = sel_alias_mem(ar, *fmt)) == NULL) {
		s = catgets(elm_msg_cat, ElmaliasSet, ElmaliasIllegalFmtChar,
		    "<illegal format char>");
	    }

	    /*
	     * Print out the formatted field.
	     */
	    if (print_enab)
		printf(pfmt, s);
	    break;

	/*
	 * Conditional printing.
	 */
	case '?':
	    if (in_conditional) {
		in_conditional = FALSE;
		print_enab = TRUE;
	    } else {
		in_conditional = TRUE;
		s = sel_alias_mem(ar, *++fmt);
		print_enab = (s != NULL && *s != '\0');
	    }
	    break;
	
	/*
	 * Backslash escapes.
	 */
	case '\\':
	    switch (*++fmt) {
		case 'b':  c = '\b'; break;
		case 'f':  c = '\f'; break;
		case 'n':  c = '\n'; break;
		case 'r':  c = '\r'; break;
		case 't':  c = '\t'; break;
		default:   c = *fmt; break;
	    }
	    if (print_enab && c != '\0')
		putchar(c);
	    break;

	/*
	 * Non-special character to print.
	 */
	default:
	    if (print_enab)
		putchar(*fmt);
	    break;

	}

	if (*fmt != '\0')
	    ++fmt;

    }

    putchar('\n');
}


/*
 * Select a member of the alias record structure.
 */
char *sel_alias_mem(ar, sel)
struct alias_rec *ar;
int sel;
{
    switch (sel) {
    case 'a':
	return ar->alias;
    case 'l':
	return ar->last_name;
    case 'n':
	return ar->name;
    case 'c':
	return ar->comment;
    case 'v':
	return ar->address;
    case 't':
	switch (ar->type & (PERSON|GROUP)) {
	case PERSON:
	    return catgets(elm_msg_cat, ElmaliasSet,
		ElmaliasTypePerson, "Person");
	case GROUP:
	    return catgets(elm_msg_cat, ElmaliasSet,
		ElmaliasTypeGroup, "Group");
	default:
	    return catgets(elm_msg_cat, ElmaliasSet,
		ElmaliasTypeUnknown, "Unknown");
	}
    default:
	return (char *) NULL;
    }
    /*NOTREACHED*/
}

