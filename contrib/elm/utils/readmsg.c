
static char rcsid[] = "@(#)Id: readmsg.c,v 5.10 1993/08/03 19:28:39 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.2 $   $State: Exp $
 *
 *			Copyright (c) 1988-1992 USENET Community Trust
 *			Copyright (c) 1986,1987 Dave Taylor
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 *******************************************************************************
 * $Log: readmsg.c,v $
 * Revision 1.2  1994/01/14  00:56:22  cp
 * 2.4.23
 *
 * Revision 5.10  1993/08/03  19:28:39  syd
 * Elm tries to replace the system toupper() and tolower() on current
 * BSD systems, which is unnecessary.  Even worse, the replacements
 * collide during linking with routines in isctype.o.  This patch adds
 * a Configure test to determine whether replacements are really needed
 * (BROKE_CTYPE definition).  The <ctype.h> header file is now included
 * globally through hdrs/defs.h and the BROKE_CTYPE patchup is handled
 * there.  Inclusion of <ctype.h> was removed from *all* the individual
 * files, and the toupper() and tolower() routines in lib/opt_utils.c
 * were dropped.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.9  1993/04/21  01:17:51  syd
 * readmsg treated a line with From_ preceeded by whitespace as a valid
 * message delimiter.
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.8  1993/04/12  01:54:07  syd
 * Modified to use new safe_malloc() routines.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.7  1993/02/03  16:46:28  syd
 * xrealloc name conflicts with some os having a routine called xrealloc,
 * renamed it elm_xrealloc.
 * From: Syd
 *
 * Revision 5.6  1993/02/03  15:26:13  syd
 * protect atol in ifndef __STDC__ as some make it a macro, and its in stdlib.h
 *
 * Revision 5.5  1993/01/19  05:07:05  syd
 * Trim erroreous extra log entry
 * From: Syd
 *
 * Revision 5.4  1993/01/19  04:47:12  syd
 * Significant changes to provide consistent Date and From_ header
 * cracking.  Overhauled date utilities and moved into library.  Moved
 * real_from() into library.  Modified frm, newmail, and readmsg utilities
 * to use library version of real_from().  Moved get_word() from Elm
 * source into library.  Added new library routines atonum() and strfcpy().
 * Fixed trailing backslash bug in len_next().
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.3  1992/12/11  01:45:04  syd
 * remove sys/types.h include, it is now included by defs.h
 * and this routine includes defs.h or indirectly includes defs.h
 * From: Syd
 *
 * Revision 5.2  1992/11/07  19:37:21  syd
 * Enhanced printing support.  Added "-I" to readmsg to
 * suppress spurious diagnostic messages.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.1  1992/10/04  00:46:45  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** This program extracts messages from a mail folder.  It is particularly
    useful when the user is composting a mail response in an external
    editor.  He or she can call this program to pull a copy of the original
    message (or any other message in the folder) into the editor buffer.

    One of the first things we do is look for a folder state file.
    If we are running as a subprocess to Elm this file should tell us
    what folder is currently being read, the seek offsets to all the
    messages in the folder, and what message(s) in the folder is/are
    selected.  We will load in all that info and use it for defaults,
    as applicable.

    If a state file does not exist, then the default is to show whatever
    messages the user specifies on the command line.  Unless specified
    otherwise, this would be from the user's incoming mail folder.

    Even if a state file exists, the user can override the defaults
    and select a different set of messages to extract, or select an
    entirely different folder.

    Messages can be selected on the command line as follows:

	readmsg [-options] *

	    Selects all messages in the folder.

	readmsg [-options] pattern ...

	    Selects messsage(s) which match "pattern ...".  The selected
	    message will contain the "pattern ..." somewhere within
	    the header or body, and it must be an exact (case sensitive)
	    match.  Normally selects only the first match.  The "-a"
	    selects all matches.

	readmsg [-options] sel ...

	    where:  sel == a number -- selects that message number
		    sel == $ -- selects last message in folder
		    sel == 0 -- selects last message in folder

    The undocumented "-I" option is a kludge to deal with an Elm race
    condition.  The problem is that Elm does piping/printing/etc. by
    running "readmsg|command" and placing the mail message selection
    into a folder state file.  However, if the "command" portion of
    the pipeline craps out, Elm might regain control before "readmsg"
    completes.  The first thing Elm does is unlink the folder state
    file.  Thus "readmsg" can't figure out what to do -- there is no
    state file or command line args to select a message.  In this
    case, "readmsg" normally gives a usage diagnostic message.  The
    "-I" option says to ignore this condition and silently terminate.

**/

#include "elmutil.h"
#include "s_readmsg.h"

/** three defines for what level of headers to display **/
#define ALL		1
#define WEED		2
#define NONE		3

#define metachar(c)	(c == '=' || c == '+' || c == '%')

/* increment for growing dynamic lists */
#define ALLOC_INCR	256

/* program name for diagnostics */
char *prog;

/*
 * The "folder_idx_list" is a list of seek offsets into the folder,
 * indexed by message number.  The "folder_size" value indicates the
 * number of messages in the folder, as well as the length of the index.
 * This index will be used if we need to access messages by message
 * number.  If possible, the index will be loaded from the external state
 * file, otherwise we will have to make a pass through the entire folder
 * to build up the index.  The index is not needed if we are printing
 * everything ("*" specified on the command line) or if we are searching
 * for pattern match.
 */
int folder_size;
long *folder_idx_list;

/*
 * local procedures
 */
void load_folder_index();
int print_patmatch_mssg();
int print_mssg();
void weed_headers();
long mssg_num_to_index();
char *skip_word();

extern char *optarg;			/* for parsing the ...		*/
extern int   optind;			/*  .. starting arguments	*/

#ifdef DEBUG
int debug = 0;
FILE *debugfile = stderr;
#endif


void usage_error()
{
    fprintf(stderr, catgets(elm_msg_cat, ReadmsgSet, ReadmsgUsage,
	"Usage: %s [-anhp] [-f Folder] {MessageNum ... | pattern | *}\n"),
	prog);
    exit(1);
}


void malloc_fail_handler(proc, size)
char *proc;
unsigned size;
{
    fprintf(stderr, catgets(elm_msg_cat, ReadmsgSet, ReadmsgOutOfMemory,
	"%s: Out of memory [malloc failed].\n"), prog);
    exit(1);
}


main(argc, argv)
int argc;
char *argv[];
{
    struct folder_state fstate;	/* information from external state file	*/
    char folder_name[SLEN];	/* pathname to the mail folder		*/
    int fstate_valid;		/* does "fstate" have valid info?	*/
    int hdr_disp_level;		/* amount of headers to show		*/
    int do_page_breaks;		/* true to FORMFEED between messages	*/
    int do_all_matches;		/* true to show all mssgs which match pat*/
    int ign_no_request;		/* terminate if no actions requested	*/
    int exit_status;		/* normally zero, set to one on error	*/
    FILE *fp;			/* file stream for opened folder	*/
    long idx;			/* seek offset within folder		*/
    char buf[SLEN], *cp;
    int i;

    /**** start of the actual program ****/

					/* Gee...isn't that special?
					 * Somebody wake me up when we
					 * get to the imitation program.
					 *  -chip  */

#ifdef I_LOCALE
    setlocale(LC_ALL, "");
#endif

    elm_msg_cat = catopen("elm2.4", 0);
    prog = argv[0];
    folder_name[0] = '\0';	/* no folder specified yet		*/
    folder_size = -1;		/* message index not loaded yet		*/
    hdr_disp_level = WEED;	/* only display interesting headers	*/
    do_page_breaks = FALSE;	/* suppress formfeed between mssgs	*/
    do_all_matches = FALSE;	/* only show 1st mssg which matches pat	*/
    ign_no_request = FALSE;	/* no action requested is an error	*/
    exit_status = 0;		/* will set nonzero on error		*/

    /* install trap for safe_malloc() failure */
    safe_malloc_fail_handler = malloc_fail_handler;

    /* see if an external folder state file exists */
    if (load_folder_state_file(&fstate) != 0) {
	fprintf(stderr, catgets(elm_msg_cat, ReadmsgSet,
	    ReadmsgStateFileCorrupt,
	    "%s: Elm folder state file appears to be corrupt!\n"), prog);
	exit(1);
    }
    fstate_valid = (fstate.folder_name != NULL);

    /* crack the command line */
    while ((i = getopt(argc, argv, "anhf:pI")) != EOF) {
	switch (i) {
	case 'a' :
	    do_all_matches = TRUE;
	    break;
	case 'n' :
	    hdr_disp_level = NONE;
	    break;
	case 'h' :
	    hdr_disp_level = ALL;
	    break;
	case 'f' :
	    strcpy(folder_name, optarg);
	    if (metachar(folder_name[0]) && !expand(folder_name)) {
		fprintf(stderr, catgets(elm_msg_cat, ReadmsgSet,
		    ReadmsgCannotExpandFolderName,
		    "%s: Cannot expand folder name \"%s\".\n"),
		    prog, folder_name);
		exit(1);
	    }
	    folder_size = -1;	/* zot out index info from extern state file */
	    break;
	case 'p' :
	    do_page_breaks = TRUE;
	    break;
	case 'I':
	    ign_no_request = TRUE;
	    break;
	default:
	    usage_error();
	}
    }

    /* figure out where the folder is */
    if (folder_name[0] == '\0') {
	if (fstate_valid)
	    strcpy(folder_name, fstate.folder_name);
	else if ((cp = getenv("MAIL")) != NULL)
	    strcpy(folder_name, cp);
	else if ((cp = getenv("LOGNAME")) != NULL)
	    sprintf(folder_name, "%s/%s", mailhome, cp);
	else if ((cp = getenv("USER")) != NULL)
	    sprintf(folder_name, "%s/%s", mailhome, cp);
	else {
	    fprintf(stderr, catgets(elm_msg_cat, ReadmsgSet,
		ReadmsgCannotGetIncomingName,
		"%s: Cannot figure out name of your incoming mail folder.\n"),
		prog);
	    exit(1);
	}
    }

    /* open up the message folder */
    if ((fp = fopen(folder_name, "r")) == NULL) {
	fprintf(stderr, catgets(elm_msg_cat, ReadmsgSet,
	    ReadmsgFolderEmpty, "%s: Folder \"%s\" is empty.\n"),
	    prog, folder_name);
	exit(0);
    }

    /* if this is the folder in the state file then grab the index */
    if (fstate_valid && strcmp(folder_name, fstate.folder_name) == 0) {
	folder_size = fstate.num_mssgs;
	folder_idx_list = fstate.idx_list;
    } else {
	fstate_valid = FALSE;
    }

    /* if no selections on cmd line then show selected mssgs from state file */
    if (argc == optind) {
	if (!fstate_valid) {
		/* no applicable state file or it didn't select anything */
		if (ign_no_request)
		    exit(0);
		usage_error();
	}
	if (folder_size < 1) {
	    fprintf(stderr, catgets(elm_msg_cat, ReadmsgSet, ReadmsgFolderEmpty,
		"%s: Folder \"%s\" is empty.\n"), prog, folder_name);
	    exit(0);
	}
	for (i = 0 ; i < fstate.num_sel ; ++i) {
	    if ((idx = mssg_num_to_index(fstate.sel_list[i])) == -1L)
		exit_status = 1;
	    else if (print_mssg(fp, idx, hdr_disp_level, do_page_breaks) != 0)
		exit_status = 1;
	}
	exit(exit_status);
    }

    /* see if we are trying to match a pattern */
    if (index("0123456789$*", argv[optind][0]) == NULL) {
	strcpy(buf, argv[optind]);
	while (++optind < argc)
		strcat(strcat(buf, " "), argv[optind]);
	if (print_patmatch_mssg(fp, buf, do_all_matches,
			hdr_disp_level, do_page_breaks) != 0)
	    exit_status = 1;
	exit(exit_status);
    }

    /* if we do not have an index from the state file then go build one */
    if (folder_size < 0)
	load_folder_index(fp);

    /* make sure there is something there to look at */
    if (folder_size < 1) {
	fprintf(stderr, catgets(elm_msg_cat, ReadmsgSet, ReadmsgFolderEmpty,
	    "%s: Folder \"%s\" is empty.\n"), prog, folder_name);
	exit(0);
    }

    /* see if all messages should be shown */
    if (argc-optind == 1 && strcmp(argv[optind], "*") == 0) {
	for (i = 1 ; i <= folder_size ; ++i) {
	    if ((idx = mssg_num_to_index(i)) == -1L)
		exit_status = 1;
	    else if (print_mssg(fp, idx, hdr_disp_level, do_page_breaks) != 0)
		exit_status = 1;
	}
	exit(exit_status);
    }

    /* print out all the messages specified on the command line */
    for ( ; optind < argc ; ++optind) {

	/* get the message number */
	if (strcmp(argv[optind], "$") == 0 || strcmp(argv[optind], "0") == 0) {
	    i = folder_size;
	} else if ((i = atoi(argv[optind])) == 0) {
	    fprintf(stderr, catgets(elm_msg_cat, ReadmsgSet,
		ReadmsgIDontUnderstand,
		"%s: I don't understand what \"%s\" means.\n"),
		prog, argv[optind]);
	    exit(1);
	}

	/*print it */
	if ((idx = mssg_num_to_index(i)) == -1L)
	    exit_status = 1;
	else if (print_mssg(fp, idx, hdr_disp_level, do_page_breaks) != 0)
	    exit_status = 1;

    }

    exit(exit_status);
    /*NOTREACHED*/
}


/*
 * Scan through the entire folder and build an index of seek offsets.
 */
void load_folder_index(fp)
FILE *fp;
{
    long offset;
    int alloc_size;
    char buf[SLEN];
#ifdef MMDF
    int  newheader = 0;
#endif /* MMDF */

    /* zero out the folder seek offsets index */
    folder_size = 0;
    alloc_size = 0;
    folder_idx_list = NULL;

    /* go through the folder a line at a time looking for messages */
    rewind(fp);
    while (offset = ftell(fp), mail_gets(buf, sizeof(buf), fp) != 0) {
#ifdef MMDF
	if ((strcmp(buf, MSG_SEPARATOR) == 0) && (++newheader % 2) != 0)
#else
	if (first_word(buf, "From ") &&
	    real_from(buf, (struct header_rec *)NULL))
#endif
	{
	    if (folder_size >= alloc_size) {
		alloc_size += ALLOC_INCR;
		folder_idx_list = (long *) safe_realloc((char *)folder_idx_list,
		    alloc_size*sizeof(long));
	    }
	    folder_idx_list[folder_size++] = offset;
	}
    }

}


/*
 * Scan through a folder and print message(s) which match the pattern.
 */
int print_patmatch_mssg(fp, pat, do_all_matches, hdr_disp_level, do_page_breaks)
FILE *fp;
char *pat;
int do_all_matches;
int hdr_disp_level;
int do_page_breaks;
{
    long offset, mssg_idx;
    char buf[SLEN];
    int look_for_pat;
#ifdef MMDF
    int  newheader = 0;
#endif /* MMDF */

    /*
     * This flag ensures that we don't reprint a message if a single
     * message matches the pattern several times.  We turn it on when
     * we get to the top of a message.
     */
    look_for_pat = FALSE;

    rewind(fp);
    while (offset = ftell(fp), mail_gets(buf, sizeof(buf), fp) != 0) {

#ifdef MMDF
	if ((strcmp(buf, MSG_SEPARATOR) == 0) && (++newheader % 2) != 0)
#else
	if (first_word(buf, "From ") &&
	    real_from(buf, (struct header_rec *)NULL))
#endif
	{
	    mssg_idx = offset;
	    look_for_pat = TRUE;
	}

	if (look_for_pat && strstr(buf, pat) != NULL) {
	    offset = ftell(fp);
	    if (print_mssg(fp, mssg_idx, hdr_disp_level, do_page_breaks) != 0)
		return -1;
	    fseek(fp, offset, 0);
	    if (!do_all_matches)
		break;
	    look_for_pat = FALSE;
	}

    }

    return 0;
}


/*
 * Print the message at the indicated location.
 */
int print_mssg(fp, offset, hdr_disp_level, do_page_breaks)
FILE *fp;
long offset;
int hdr_disp_level;
int do_page_breaks;
{
    char buf[SLEN];
    int first_line, is_seperator, in_header, buf_len, newlines;
    static int num_mssgs_listed = 0;

    in_header = TRUE;
    first_line = TRUE;

    if (num_mssgs_listed++ == 0)
	; /* no seperator before first message */
    else if (do_page_breaks)
	putchar(FORMFEED);
    else
	puts("\n------------------------------------------------------------------------------\n");

    /* move to the beginning of the selected message */
    if (fseek(fp, offset, 0) != 0) {
	fprintf(stderr, catgets(elm_msg_cat, ReadmsgSet, ReadmsgCannotSeek,
	    "%s: Cannot seek to selected message. [offset=%ld]\n"),
	    prog, offset);
	return -1;
    }

    /* we will chop off newlines at the end of the message */
    newlines = 0;

    /* print the message a line at a time */
    while ((buf_len = mail_gets(buf, SLEN, fp)) != 0) {

#ifdef MMDF
	is_seperator = (strcmp(buf, MSG_SEPARATOR) == 0);
#else
	is_seperator = first_word(buf, "From ") &&
		       real_from(buf, (struct header_rec *)NULL);
#endif

	/* the first line of the message better be the seperator */
	/* next time we encounter the seperator marks the end of the message */
	if (first_line) {
	    if (!is_seperator) {
		fprintf(stderr, catgets(elm_msg_cat, ReadmsgSet,
		    ReadmsgCannotFindStart,
		    "%s: Cannot find start of selected message. [offset=%ld]\n"),
		    prog, offset);
		return -1;
	    }
	    first_line = FALSE;
	} else {
	    if (is_seperator)
		break;
	}

	/* just accumulate newlines */
	if (buf[0] == '\n' && buf[1] == '\0') {
	    if (in_header) {
		if (hdr_disp_level == WEED)
		    weed_headers((char *)NULL);
		in_header = FALSE;
	    }
	    ++newlines;
	    continue;
	}

	if (in_header) {
	    switch (hdr_disp_level) {
	    case NONE:
		break;
	    case WEED:
		weed_headers(buf);
		break;
	    case ALL:
	    default:
		fwrite(buf, 1, buf_len, stdout);
		break;
	    }
	} else {
	    while (--newlines >= 0)
		putchar('\n');
	    newlines = 0;
	    fwrite(buf, 1, buf_len, stdout);
	}

    }

    /* make sure we didn't do something like seek beyond the */
    /* end of the folder and thus never get anything to print */
    if (first_line) {
	fprintf(stderr, catgets(elm_msg_cat, ReadmsgSet, ReadmsgCannotFindStart,
	    "%s: Cannot find start of selected message. [offset=%ld]\n"),
	    prog, offset);
	return -1;
    }

    return 0;
}


#define Hdrmatch(buf, hdr)	(strincmp((buf), (hdr), sizeof(hdr)-1) == 0)

/*
 * Process interesting mail headers.
 *
 * When in WEED mode, this routine should be called with each header line
 * in sequence, and with a NULL at the end of the header.  If the header
 * is interesting then we will print it out.  Continued header lines (i.e.
 * ones starting with whitespace) are also handled.  When the end of the
 * header is reached, if either the From: or Date: was missing then we
 * will generate replacements from the From_ header.
 *
 * Strict intepretation of RFC822 says { '\\' '\n' } at the end of a header
 * line should (at least in some cases) continue to the next line, but we
 * don't handle that.
 */
void weed_headers(buf)
char *buf;
{
    static int got_from = FALSE;	/* already printed From:	*/
    static int got_date = FALSE;	/* already printed Date:	*/
    static int is_interesting = FALSE;	/* print out curr hdr line	*/
    static char save_from[SLEN];	/* From_ hdr from current mssg	*/
    char *s;

    /* finish off headers on NULL value */
    if (buf == (char *) NULL) {
	if (!got_from) {
	    fputs("From: ", stdout);
	    for (s = skip_word(save_from) ; *s != '\0' && !isspace(*s) ; ++s)
		putchar(*s);
	    putchar('\n');
	}
	if (!got_date)
	    printf("Date: %s", skip_word(skip_word(save_from)));
	is_interesting = got_from = got_date = FALSE;
	save_from[0] = '\0';
	return;
    }

    if (Hdrmatch(buf, "From ")) {
	strcpy(save_from, buf);
	is_interesting = FALSE;
    } else if (Hdrmatch(buf, "To:") || Hdrmatch(buf, "Subject:"))
	is_interesting = TRUE;
    else if (Hdrmatch(buf, "Date:"))
	got_date = is_interesting = TRUE;
    else if (Hdrmatch(buf, "From:"))
	got_from = is_interesting = TRUE;
    else if (!isspace(buf[0]))
	is_interesting = FALSE;
    /* else */
	/* this is a continuation header - preserve "is_interesting" value */

    if (is_interesting)
	fputs(buf, stdout);
}



/*
 * Convert a mailbox message number to a seek location.
 */
long mssg_num_to_index(mssgno)
int mssgno;
{
    if (mssgno < 1 || mssgno > folder_size) {
	fprintf(stderr, catgets(elm_msg_cat, ReadmsgSet,
	    ReadmsgCannotFindMessage, "%s: Cannot find message number %d.\n"),
	    prog, mssgno);
	return -1L;
    }
    return folder_idx_list[mssgno-1];
}


/*
 * Advance to the start of the next word.
 */
char *skip_word(str)
char *str;
{
    while (*str != '\0' && !isspace(*str))
	++str;
    while (isspace(*str))
	++str;
    return str;
}

