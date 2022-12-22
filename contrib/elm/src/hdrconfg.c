
static char rcsid[] = "@(#)Id: hdrconfg.c,v 5.3 1993/08/03 19:28:39 syd Exp ";

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
 * $Log: hdrconfg.c,v $
 * Revision 1.2  1994/01/14  00:55:01  cp
 * 2.4.23
 *
 * Revision 5.3  1993/08/03  19:28:39  syd
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
 * Revision 5.2  1992/11/22  01:15:15  syd
 * Add on initial display or display where the entire screen is being
 * drawn, we should not output the trailing blanks that clear the old
 * value.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/**   This file contains the routines necessary to be able to modify
      the mail headers of messages on the way off the machine.  The
      headers currently supported for modification are:

	Subject:
	To:
	Cc:
	Bcc:
	Reply-To:
	Expires:
	Priority:
	Precedence:
	In-Reply-To:
	Action:

	<user defined>
**/

#include "headers.h"
#include "s_elm.h"

/*
 * Placement of prompts and messages at the bottom of the screen.
 */
#define INSTRUCT_LINE           (LINES-4)
#define INPUT_LINE		(LINES-2)
#define ERROR_LINE		(LINES-1)
#define TOPMOST_PROMPTAREA_LINE INSTRUCT_LINE

/*
 * Option flags for the fields in a (struct hdr_menu_item).
 */
#define HF_DISP_1ROW	0001	/* field is displayed on one line	*/
#define HF_DISP_2ROW	0002	/* field display spans two lines	*/
#define HF_DISP_3ROW	0003	/* field display spans three lines	*/
#define HF_DISP_LEFT	0004	/* field occupies left half of a line	*/
#define HF_DISP_RIGHT	0005	/* field occupies right half of a line	*/
#define HF_DISP_MASK	0007	/* -- mask to pull out display option	*/
#define HF_PROMPT_EXP	0010	/* prompt for expires data entry	*/
#define HF_PROMPT_USR	0020	/* prompt for user defined hdr entry	*/
#define HF_PROMPT_MASK	0070	/* -- mask to pull out prompt option	*/
#define HF_APPENDENTRY	0100	/* append user entry to existing value	*/

/*
 * Structure to describe a header which can be edited in this menu.
 */
struct hdr_menu_item {
    int menucmd;	/* The single keystroke (lower-case letter) the	*/
			/*   user strikes to edit this menu item.	*/
    char *hdrname;	/* Header name to display in the menu.  Parens	*/
			/*   should be used to bracket the "menucmd"	*/
			/*   char in the name, e.g. "S)ubject".  This	*/
			/*   will be NULL for the user-defined header.	*/
    int lineno;		/* Screen line at which the field is displayed.	*/
    int flags;		/* Various flags which effect the display and	*/
			/*   user entry of this item.			*/
    char *inpval;	/* Pointer to the buffer to hold the value	*/
			/*   entered by the user.			*/
    char *expval;	/* Pointer to the expanded header value to	*/
			/*   display.  If no special expansions are	*/
			/*   required then this will point to "inpval".	*/
    int (*hdrproc)();	/* Pointer to a procedure which verifies the	*/
			/*   user data entry, and if required converts	*/
			/*   the "inpval" value to "expval" value.  If	*/
			/*   no verification or expansion is needed	*/
			/*   then this will be NULL.			*/
};

/*
 * These are all defined in the mailmsg file.
 */
extern char subject[SLEN], in_reply_to[SLEN], expires[SLEN],
            action[SLEN], priority[SLEN], reply_to[SLEN], to[VERY_LONG_STRING],
	    cc[VERY_LONG_STRING], expanded_to[VERY_LONG_STRING],
	    expanded_reply_to[LONG_STRING],
	    expanded_cc[VERY_LONG_STRING], user_defined_header[SLEN],
	    bcc[VERY_LONG_STRING], expanded_bcc[VERY_LONG_STRING],
	    precedence[SLEN], expires_days[SLEN];

/*
 * Local procedures.
 */
static void hdrmenu_clear_promptarea();
static int hdrmenu_get();
static void hdrmenu_put();
static int hdrproc_addr();
static int hdrproc_expires();
static int hdrproc_precedence();
static int hdrproc_userhdr();
static int domainize_submenu();
static void domainize();
static void domainize_addr();

/*
 * Definition of all the header editing menu fields.
 */
struct hdr_menu_item hmenu_item_list[] = {
    { 't', "T)o",		 2, HF_DISP_3ROW|HF_APPENDENTRY,
					to, expanded_to, hdrproc_addr },
    { 'c', "C)c",		 5, HF_DISP_3ROW|HF_APPENDENTRY,
					cc, expanded_cc, hdrproc_addr },
    { 'b', "B)cc",		 8, HF_DISP_2ROW|HF_APPENDENTRY,
					bcc, expanded_bcc, hdrproc_addr },
    { 's', "S)ubject",		10, HF_DISP_2ROW, subject, subject, NULL },
    { 'r', "R)eply-to",		12, HF_DISP_1ROW, reply_to, expanded_reply_to,
					hdrproc_addr },
    { 'a', "A)ction",		13, HF_DISP_LEFT, action, action, NULL },
    { 'e', "E)xpires",		13, HF_DISP_RIGHT|HF_PROMPT_EXP, expires_days,
					expires, hdrproc_expires },
    { 'p', "P)riority",		14, HF_DISP_LEFT, priority, priority, NULL },
    { 'n', "Precede(n)ce",	14, HF_DISP_RIGHT, precedence,
					precedence, hdrproc_precedence },
    { 'i', "I)n-reply-to",	15, HF_DISP_2ROW,
					in_reply_to, in_reply_to, NULL },
    { 'u', NULL,		17, HF_DISP_1ROW|HF_PROMPT_USR,
					user_defined_header,
					user_defined_header, hdrproc_userhdr },
    { -1, NULL, -1, -1, NULL, NULL, NULL },
};

/*
 * Selection of individual fields.  The indices *must* correspond
 * to the above "hmenu_item_list[]" list.
 */
#define hmenu_to		(hmenu_item_list[0])
#define hmenu_cc		(hmenu_item_list[1])
#define hmenu_bcc		(hmenu_item_list[2])
#define hmenu_subject		(hmenu_item_list[3])
#define hmenu_replyto		(hmenu_item_list[4])
#define hmenu_action		(hmenu_item_list[5])
#define hmenu_expires		(hmenu_item_list[6])
#define hmenu_priority		(hmenu_item_list[7])
#define hmenu_precedence	(hmenu_item_list[8])
#define hmenu_inreplyto		(hmenu_item_list[9])
#define hmenu_userdef		(hmenu_item_list[10])


edit_headers()
{
    int c, i, do_redraw;
    struct hdr_menu_item *h;
    
    /* expand out all of the header values */
    /* menu displays expanded values, user edits unexpended versions */
    for (h = hmenu_item_list ; h->menucmd > 0 ; ++h) {
	if (h->hdrproc != NULL)
	    (*h->hdrproc)(h);
    }

    clearerr(stdin);
    do_redraw = TRUE;
    while (TRUE) {	/* forever */

	/* redraw the entire display if required */
	if (do_redraw) {
	    ClearScreen();
	    Centerline(0, catgets(elm_msg_cat, ElmSet,
		ElmHdrmenuScreenTitle, "Message Header Edit Screen"));
	    for (h = hmenu_item_list ; h->menucmd > 0 ; ++h)
		hdrmenu_put(h, TRUE);
	    do_redraw = FALSE;
	}

	/* display the instructions */
#ifdef ALLOW_SUBSHELL
	Centerline(INSTRUCT_LINE, catgets(elm_msg_cat, ElmSet,
	    ElmHdrmenuInstruct,
	    "Choose header, u)ser defined header, d)omainize, !)shell, or <return>."));
#else
	Centerline(INSTRUCT_LINE, catgets(elm_msg_cat, ElmSet,
	    ElmHdrmenuInstructNoShell,
	    "Choose header, u)ser defined header, d)omainize, or <return>."));
#endif

	/* prompt for command */
	PutLine0(INPUT_LINE, 0, catgets(elm_msg_cat, ElmSet,
	    ElmHdrmenuPrompt, "Choice: "));
	c = getchar();
	if (isupper(c))
	    c = tolower(c);
	hdrmenu_clear_promptarea();

	/* execute the command */
	switch (c) {

	case EOF:
	case RETURN:
	case LINE_FEED:
	case 'q':
	    return 0;

	case 'd':
	    if (domainize_submenu() != 0)
		return 0;
	    break;

#ifdef ALLOW_SUBSHELL
	case '!':
	    if (subshell())
		do_redraw = TRUE;
	    break;
#endif

	case ctrl('L'):
	    do_redraw = TRUE;
	    break;
    
	default:
	    for (h = hmenu_item_list ; h->menucmd > 0 ; ++h) {
		if (h->menucmd == c) {
		    if (hdrmenu_get(h) != 0)
			return 0;
		    hdrmenu_put(h, FALSE);
		    break;
		}
	    }
	    if (h->menucmd <= 0) {
		Centerline(ERROR_LINE, catgets(elm_msg_cat, ElmSet,
		    ElmHdrmenuBadChoice, "No such header!"));
		Writechar('\007');
	    }
	    break;

	}

    }

    /*NOTREACHED*/
}


/*
 * Erase instructions, user input, left-over errors, etc.
 * This should be run after every user input command in this module.
 */
static void hdrmenu_clear_promptarea()
{
    clear_error();
    MoveCursor(TOPMOST_PROMPTAREA_LINE, 0);
    CleartoEOS();
}


/*
 * Prompt the user for a header value, and do any required post-processing.
 */
static int hdrmenu_get(h)
struct hdr_menu_item *h;
{
    char *s;
    int plen, ret, do_append;

    /* display the instructions */
    switch (h->flags & HF_PROMPT_MASK) {
    case HF_PROMPT_EXP:
	Centerline(INSTRUCT_LINE, catgets(elm_msg_cat, ElmSet,
	    ElmHdrmenuGetExpiresInstruct,
	    "In how many days should this message expire? "));
	break;
    case HF_PROMPT_USR:
	Centerline(INSTRUCT_LINE, catgets(elm_msg_cat, ElmSet,
	    ElmHdrmenuGetUserdefInstruct,
	    "Enter in the format \"HeaderName: HeaderValue\"."));
	break;
    default:
	Centerline(INSTRUCT_LINE, catgets(elm_msg_cat, ElmSet,
	    ElmHdrmenuGetInstruct, "Enter value for the header."));
	break;
    }

    /* display a prompt */
    plen = 0;
    if (h->hdrname != NULL) {
	MoveCursor(INPUT_LINE, 0);
	for (s = h->hdrname ; *s != '\0' ; ++s) {
	    if (*s != '(' && *s != ')') {
		Writechar(*s);
		++plen;
	    }
	}
	Writechar(':');
	Writechar(' ');
	plen += 2;
    }

    /* get input from the user */
    do_append = ((h->flags & HF_APPENDENTRY) != 0);
    ret = optionally_enter(h->inpval, INPUT_LINE, plen, do_append, FALSE);
    hdrmenu_clear_promptarea();

    /* bail out on error */
    if (ret < 0)
	return -1;

    /* see if there is some processing required on this value */
    if (h->hdrproc != NULL)
	(void) (*h->hdrproc)(h);

    return 0;
}


/*
 * Dispay a header and its value in the appropriate field.
 */
static void hdrmenu_put(h, already_clear)
struct hdr_menu_item *h;
int already_clear;
{
    char    *p;
    int     start_row, max_row, start_col, max_col, row, col;

    /* figure out the dimensions of the field */
    switch (h->flags & HF_DISP_MASK) {
    case HF_DISP_LEFT:
	start_row = h->lineno;		max_row = h->lineno;
	start_col = 0;			max_col = COLUMNS/2 - 2;
	break;
    case HF_DISP_RIGHT:
	start_row = h->lineno;		max_row = h->lineno;
	start_col = COLUMNS/2 + 1;	max_col = COLUMNS-1;
	break;
    case HF_DISP_3ROW:
	start_row = h->lineno;		max_row = h->lineno+2;
	start_col = 0;			max_col = COLUMNS-1;
	break;
    case HF_DISP_2ROW:
	start_row = h->lineno;		max_row = h->lineno+1;
	start_col = 0;			max_col = COLUMNS-1;
	break;
    default:
	start_row = h->lineno;		max_row = h->lineno;
	start_col = 0;			max_col = COLUMNS-1;
	break;
    }

    /* display the header name */
    MoveCursor(start_row, start_col);
    if (h->hdrname != NULL) {
	for (p = h->hdrname ; *p != '\0' ; ++p)
	    Writechar(*p);
	Writechar(':');
	Writechar(' ');
    }

    /* display the header value */
    GetXYLocation(&row, &col);
    for (p = h->expval ; *p != '\0' && row <= max_row ; ++p) {
	if (row == max_row && col == max_col-4 && strlen(p) > 4)
	    p = " ...";		/* neat hack alert */
	Writechar(*p);
	if (!isprint(*p) || ++col > max_col)
	    GetXYLocation(&row, &col);
    }

    /* save some drawing if we know the screen is already empty */
    if (!already_clear) {

	/* clear out remaining space in this line of the field */
	if (max_col == COLUMNS-1) {
	    /* people on slow terminals might appreciate doing it this way */
	    CleartoEOLN();
	} else {
	    while (col++ <= max_col)
		Writechar(' ');
	}

	/* for multi-line fields, clear out any unused lines */
	/* this assumes that multi-line fields span the entire screen width */
	while (++row <= max_row) {
	    /* grrrrrr -- this is a multi-statement macro */
	    ClearLine(row);
	}

    }

}


/*
 * Process the to, cc, and bcc headers.  The value entered by the
 * user is expanded.  A successful status is always returned.
 */
static int hdrproc_addr(h)
struct hdr_menu_item *h;
{
    (void) build_address(strip_commas(h->inpval), h->expval);
    return 0;
}


/*
 * Process the expires header.  The value entered by the user is interpreted
 * as a number of days, and is expanded out to a date specification.  If
 * an error occurs a message is printed, the expanded value is cleared
 * out, and a -1 is returned.
 */
static int hdrproc_expires(h)
struct hdr_menu_item *h;
{
    int days;

    /* initialize expanded date spec to empty */
    h->expval[0] = '\0';

    /* blank is ok */
    if (h->inpval[0] == '\0')
	return 0;

    /* verify the number of days is valid and in range */
    days = atoi(h->inpval);
    if (days < 1) {
	Centerline(ERROR_LINE, catgets(elm_msg_cat, ElmSet,
	    ElmHdrmenuExpiresNotNumber,
	    "Expiration must be specified as a number of days."));
	return -1;
    }
    if (days > 8*7) {
	Centerline(ERROR_LINE, catgets(elm_msg_cat, ElmSet,
	    ElmHdrmenuExpiresOutOfRange,
	    "Expiration date must be within eight weeks of today."));
	return -1;
    }

    /* convert number of days to a date */
    days_ahead(days, h->expval);
    return 0;
}


/*
 * Process the precedence header.  The value entered by the user is
 * checked against the list of allowed precedences, if one exists.  If
 * the precedence has a priority assigned to it, then an empty priority
 * field will be filled in with that value.  If an error occurs a message
 * is printed, the precedence value is cleared out, and a -1 is returned.
 */
static int hdrproc_precedence(h)
struct hdr_menu_item *h;
{
    char buf[SLEN];	/* assumes sizeof(allowed_precedences) <= SLEN */
    char *bp, *prec, *prio;

    /* empty is ok */
    if (h->inpval[0] == '\0')
	return 0;

    /* if there are no restrictions on precedence then anything is ok */
    if (allowed_precedences[0] == '\0')
	return 0;

    /* the "allowed_precedences[]" format is: */
    /*   precedence[:priority-value] precedence[:priority-value] ... */
    bp = strcpy(buf, allowed_precedences);
    while ((prec = strtok(bp, " \t\n")) != NULL) {
	bp = NULL;
	if ((prio = index(prec, ':')) != NULL)
	    *prio++ = '\0';
	if (istrcmp(prec, h->inpval) == 0)
	    break;
    }

    /* see if we reached the end of the list without a match */
    if (prec == NULL) {
	Centerline(ERROR_LINE, catgets(elm_msg_cat, ElmSet,
	    ElmHdrmenuPrecedenceBadValue,
	    "Unknown precedence value specified."));
	h->inpval[0] = '\0';
	return -1;
    }

    /* see if this precedence has an associated priority */
    if (prio != NULL && hmenu_priority.inpval[0] == '\0') {
	(void) strcpy(hmenu_priority.inpval, prio);
	hdrmenu_put(&hmenu_priority, FALSE);
    }

    return 0;
}


/*
 * Process the user-defined header.  The value entered by the user is
 * verified for proper format.  If an error occurs a message is printed,
 * the expanded value is cleared out, and a -1 is returned.
 */
static int hdrproc_userhdr(h)
struct hdr_menu_item *h;
{
    char *s;

    /* empty is ok */
    if (h->inpval[0] == '\0')
	return 0;

    /* make sure the header name doesn't begin with some strange character */
    if (!isalnum(h->inpval[0])) {
	Centerline(ERROR_LINE, catgets(elm_msg_cat, ElmSet,
	    ElmHdrmenuUserdefNotAlnum,
	    "The user-defined header must begin with a letter or number."));
	h->inpval[0] = '\0';
	return -1;
    }

    /* locate the end of the header name */
    for (s = h->inpval ; *s != ':' && isprint(*s) && !isspace(*s) ; ++s)
	;

    /* there needs to be a colon at the end of the header name */
    if (*s != ':') {
	Centerline(ERROR_LINE, catgets(elm_msg_cat, ElmSet,
	    ElmHdrmenuUserdefMissingColon,
	    "The user-defined header must have a colon after the field name."));
	h->inpval[0] = '\0';
	return -1;
    }

    return 0;
}


/*
 * Prompt the user to domainize a header.
 */
static int domainize_submenu()
{
    int c;
    struct hdr_menu_item *h;

    Centerline(INSTRUCT_LINE, catgets(elm_msg_cat, ElmSet,
	ElmHdrmenuDomInstruct,
	"Select header to domainize:  T)o, C)c, B)cc, or <return>."));
    PutLine0(INPUT_LINE, 0, catgets(elm_msg_cat, ElmSet,
	ElmHdrmenuDomPrompt, "Domainize choice: "));

    for (;;) {

	c = getchar();

	switch (tolower(c)) {
	case 't':
	    h = &hmenu_to;
	    break;
	case 'c':
	    h = &hmenu_cc;
	    break;
	case 'b':
	    h = &hmenu_bcc;
	    break;
	case '\r':
	case '\n':
	case EOF:
	    h = NULL;
	    break;
	default:
	    Writechar('\007');
	    continue;
	}

	if (h != NULL) {
	    domainize(h->expval);
	    hdrmenu_put(h, FALSE);
	}

	hdrmenu_clear_promptarea();
	return (c == EOF ? -1 : 0);

    }

    /*NOTREACHED*/
}



static void domainize(addresses)
char *addresses;
{
	/*** Convert the given addresses from bang paths to domain format.
	     This policy amounts to Rabid Rerouting.  However, since it's
	     under the sender's control, I don't mind. ***/

	char buffer[VERY_LONG_STRING];
	char *a, *d;
	int how_many;

	strcpy(buffer, addresses);
	a = buffer;
	d = addresses;
	how_many = 0;

	for (;;) {
	  while (*a == ' ' || *a == ',')
	    ++a;
	  if (*a == '\0')
	    break;

	  if (*a == '(' || *a == '"') {
	    int endch;

	    if (d != addresses)
	      *d++ = ' ';
	    endch = (*a == '(') ? ')' : '"';
	    while (*a && *a != endch) {
	      if (*a == '\\' && *(a + 1))
		*d++ = *a++;
	      *d++ = *a++;
	    }
	    if (*a)
	      *d++ = *a++;
	  }
	  else {
	    char *addr;

	    if (how_many) {
	      *d++ = ',';
	      *d++ = ' ';
	    }
	    ++how_many;

	    if (*a == '<') {
	      *d++ = *a++;
	      addr = a;
	      while (*a && *a != '>') {
		if (*a == '\\' && *(a + 1))
		  ++a;
		++a;
	      }
	      if (*a)
		*a++ = '\0';
	      domainize_addr(addr, d);
	      d += strlen(d);
	      *d++ = '>';
	    }
	    else {
	      addr = a;
	      while (*a && *a != ' ' && *a != ',')
		++a;
	      if (*a)
		*a++ = '\0';
	      domainize_addr(addr, d);
	      d += strlen(d);
	    }
	  }
	}

	*d = '\0';
}

static void domainize_addr(src, dest)
char *src, *dest;
{
	/*** Convert one address to domain form. ***/

	char *locpart, *host;

	if (index(src, '@') != NULL
	 || (locpart = rindex(src, '!')) == NULL) {
	  strcpy(dest, src);
	  return;
	}

	*locpart++ = '\0';

	if ((host = rindex(src, '!')) != NULL)
	  ++host;
	else
	  host = src;
	sprintf(dest, "%s@%s", locpart, host);
	if (!index(host, '.'))
	  strcat(dest, ".uucp");

	*--locpart = '!';
}

