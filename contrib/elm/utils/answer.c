
static char rcsid[] = "@(#)Id: answer.c,v 5.7 1993/08/10 18:54:13 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.2 $   $State: Exp $
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
 * $Log: answer.c,v $
 * Revision 1.2  1994/01/14  00:56:06  cp
 * 2.4.23
 *
 * Revision 5.7  1993/08/10  18:54:13  syd
 * A change to answer:s mail command to be like those of elm and filter.
 * From: Jan.Djarv@sa.erisoft.se (Jan Djarv)
 *
 * Revision 5.6  1993/08/03  19:28:39  syd
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
 * Revision 5.5  1993/02/04  15:32:52  syd
 * Add cast to silence compiler warning.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.4  1993/01/20  03:37:16  syd
 * Nits and typos in the NLS messages and corresponding default messages.
 * From: dwolfe@pffft.sps.mot.com (Dave Wolfe)
 *
 * Revision 5.3  1992/10/11  01:46:35  syd
 * change dbm name to dbz to avoid conflicts with partial call
 * ins from shared librarys, and from mixing code with yp code.
 * From: Syd via prompt from Jess Anderson
 *
 * Revision 5.2  1992/10/11  01:25:58  syd
 * Add undefs of tolower so BSD macro isnt used from ctype.h
 * From: Syd
 *
 * Revision 5.1  1992/10/04  00:46:45  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** This program is a phone message transcription system, and
    is designed for secretaries and the like, to allow them to
    painlessly generate electronic mail instead of paper forms.

    Note: this program ONLY uses the local alias file, and does not
	  even read in the system alias file at all.

**/

#include "elmutil.h"
#include "ndbz.h"
#include "s_answer.h"
#include "sysdefs.h"

#define  ELM		"elm"		/* where the elm program lives */

int user_data;		/* fileno of user data file   */
DBZ *hash;		/* dbz file for same */

#ifdef DEBUG
FILE *debugfile = stderr;
int  debug = 0;
#endif

char *get_alias_address(), *get_token(), *strip_parens(), *shift_lower();

static char *quit_word, *exit_word, *done_word, *bye_word;

main(argc, argv)
int argc;
char *argv[];
{
	FILE *fd;
	char *address, buffer[LONG_STRING], tempfile[SLEN], *cp;
	char  name[SLEN], user_name[SLEN], in_line[SLEN];
	int   msgnum = 0, eof, allow_name = 0, phone_slip = 0;
	int   ans_pid = getpid();
	
#ifdef I_LOCALE
	setlocale(LC_ALL, "");
#endif

	elm_msg_cat = catopen("elm2.4", 0);

	quit_word = catgets(elm_msg_cat, AnswerSet, AnswerQuitWord, "quit");
	exit_word = catgets(elm_msg_cat, AnswerSet, AnswerExitWord, "exit");
	done_word = catgets(elm_msg_cat, AnswerSet, AnswerDoneWord, "done");
	bye_word = catgets(elm_msg_cat, AnswerSet, AnswerByeWord, "bye");
/*
 *	simplistic crack arguments, looking for -u/-p
 *	-u = allow user names not in alias table
 *	-p = prompt for phone slip messages
 */
	for (msgnum = 1; msgnum < argc; msgnum++) {
	  if (istrcmp(argv[msgnum], "-u") == 0)
	    allow_name = 1;
	  if (istrcmp(argv[msgnum], "-p") == 0)
	    phone_slip = 1;
	  if (istrcmp(argv[msgnum], "-pu") == 0) {
	    allow_name = 1;
	    phone_slip = 1;
	  }
	  if (istrcmp(argv[msgnum], "-up") == 0) {
	    allow_name = 1;
	    phone_slip = 1;
	  }
	}

	open_alias_file();

	while (1) {
	  if (msgnum > 9999) msgnum = 0;
	
	  printf("\n-------------------------------------------------------------------------------\n");

prompt:   printf(catgets(elm_msg_cat, AnswerSet, AnswerMessageTo, "\nMessage to: "));
	  if (fgets(user_name, SLEN, stdin) == NULL) {
		putchar('\n');
		exit(0);
	  }
	  if(user_name[0] == '\0')
	    goto prompt;
	  
	  cp = &user_name[strlen(user_name)-1];
	  if(*cp == '\n') *cp = '\0';
	  if(user_name[0] == '\0')
		goto prompt;

	  if ((istrcmp(user_name, quit_word) == 0) ||
	      (istrcmp(user_name, exit_word) == 0) ||
	      (istrcmp(user_name, done_word) == 0) ||
	      (istrcmp(user_name, bye_word)  == 0))
	     exit(0);

	  if (translate(user_name, name) == 0)
	    goto prompt;

	  address = get_alias_address(name, 1, 0);

	  if (address == NULL || strlen(address) == 0) {
	    if (allow_name)
	      address =  name;
	    else {
	      printf(catgets(elm_msg_cat, AnswerSet, AnswerSorryNotFound,
		     "Sorry, could not find '%s' [%s] in list!\n"),
		     user_name, name);
	      goto prompt;
	    }
	  }

	  printf("address '%s'\n", address);

	  sprintf(tempfile, "%sans.%d.%d", default_temp, ans_pid, msgnum++);

	  if ((fd = fopen(tempfile,"w")) == NULL)
	    exit(printf(catgets(elm_msg_cat, AnswerSet, AnswerCouldNotOpenWrite,
			"** Fatal Error: could not open %s to write\n"),
		 tempfile));


	/** Enter standard phone message fields **/
	  if (phone_slip) {
	    strcpy(buffer, catgets(elm_msg_cat, AnswerSet, AnswerCaller, "Caller: "));
	    printf("\n%s",buffer);
	    fflush(stdout);
	    fgets(in_line, SLEN, stdin);
	    if (strlen(in_line) > 1)
	      fprintf(fd,"%s%s",buffer,in_line);

	    strcpy(buffer, catgets(elm_msg_cat, AnswerSet, AnswerOf, "of:     "));
	    printf("%s",buffer);
	    fflush(stdout);
	    fgets(in_line, SLEN, stdin);
	    if (strlen(in_line) > 1)
	      fprintf(fd,"%s%s",buffer,in_line);

	    strcpy(buffer, catgets(elm_msg_cat, AnswerSet, AnswerPhone, "Phone:  "));
	    printf("%s",buffer);
	    fflush(stdout);
	    fgets(in_line, SLEN, stdin);
	    if (strlen(in_line) > 1)
	      fprintf(fd,"%s%s\n",buffer,in_line);

	    strcpy(buffer, catgets(elm_msg_cat, AnswerSet, AnswerTelephoned, "TELEPHONED         - "));
	    printf("\n%s",buffer);
	    fflush(stdout);
	    fgets(in_line, SLEN, stdin);
	    if (strlen(in_line) > 1)
	      fprintf(fd,"%s%s",buffer,in_line);

	    strcpy(buffer, catgets(elm_msg_cat, AnswerSet, AnswerCalledToSeeYou, "CALLED TO SEE YOU  - "));
	    printf("%s",buffer);
	    fflush(stdout);
	    fgets(in_line, SLEN, stdin);
	    if (strlen(in_line) > 1)
	      fprintf(fd,"%s%s",buffer,in_line);

	    strcpy(buffer, catgets(elm_msg_cat, AnswerSet, AnswerWantsToSeeYou, "WANTS TO SEE YOU   - "));
	    printf("%s",buffer);
	    fflush(stdout);
	    fgets(in_line, SLEN, stdin);
	    if (strlen(in_line) > 1)
	      fprintf(fd,"%s%s",buffer,in_line);

	    strcpy(buffer, catgets(elm_msg_cat, AnswerSet, AnswerReturnedYourCall, "RETURNED YOUR CALL - "));
	    printf("%s",buffer);
	    fflush(stdout);
	    fgets(in_line, SLEN, stdin);
	    if (strlen(in_line) > 1)
	      fprintf(fd,"%s%s",buffer,in_line);

	    strcpy(buffer, catgets(elm_msg_cat, AnswerSet, AnswerPleaseCall, "PLEASE CALL        - "));
	    printf("%s",buffer);
	    fflush(stdout);
	    fgets(in_line, SLEN, stdin);
	    if (strlen(in_line) > 1)
	      fprintf(fd,"%s%s",buffer,in_line);

	    strcpy(buffer, catgets(elm_msg_cat, AnswerSet, AnswerWillCallAgain, "WILL CALL AGAIN    - "));
	    printf("%s",buffer);
	    fflush(stdout);
	    fgets(in_line, SLEN, stdin);
	    if (strlen(in_line) > 1)
	      fprintf(fd,"%s%s",buffer,in_line);

	    strcpy(buffer, catgets(elm_msg_cat, AnswerSet, AnswerUrgent, "*****URGENT******  - "));
	    printf("%s",buffer);
	    fflush(stdout);
	    fgets(in_line, SLEN, stdin);
	    if (strlen(in_line) > 1)
	      fprintf(fd,"%s%s",buffer,in_line);
	  }

	  printf(catgets(elm_msg_cat, AnswerSet, AnswerEnterMessage,
		"\n\nEnter message for %s ending with a blank line.\n\n"), 
		 user_name);

	  fprintf(fd,"\n\n");

	  do {
	   printf("> ");
	   if (! (eof = (fgets(buffer, SLEN, stdin) == NULL))) 
	     fprintf(fd, "%s", buffer);
	  } while (! eof && strlen(buffer) > 1);
	
	  fclose(fd);
 
	  sprintf(buffer, catgets(elm_msg_cat, AnswerSet, AnswerElmCommand,
	     "( ( %s -s \"While You Were Out\" %s < %s ; %s %s) & ) > /dev/null"),
	     ELM, strip_parens(address), tempfile, remove_cmd, tempfile);

	  system(buffer);
	}
}

int
translate(fullname, name)
char *fullname, *name;
{
	/** translate fullname into name..
	       'first last'  translated to first_initial - underline - last
	       'initial last' translated to initial - underline - last
	    Return 0 if error.
	**/
	register int i, lastname = 0, len;

	for (i=0, len = strlen(fullname); i < len; i++) {

	  fullname[i] = tolower(fullname[i]);

	  if (fullname[i] == ' ') 
	    if (lastname) {
	      printf(catgets(elm_msg_cat, AnswerSet, AnswerCannotHaveMoreNames,
	      "** Can't have more than 'FirstName LastName' as address!\n"));
	      return(0);
	    }
	    else
	      lastname = i+1;
	
	}

	if (lastname) 
	  sprintf(name, "%c_%s", fullname[0], (char *) fullname + lastname);
	else
	  strcpy(name, fullname);

	return(1);
}

	    
open_alias_file()
{
	/** open the user alias file **/

	char fname[SLEN];

	sprintf(fname,  "%s/.elm/aliases", getenv("HOME")); 

	if ((hash = dbz_open(fname, O_RDONLY, 0)) == NULL) 
	  exit(printf("** Fatal Error: Could not open %s!\n", fname));

	if ((user_data = open(fname, O_RDONLY)) == -1) 
	  return;
}

char *get_alias_address(name, mailing, depth)
char *name;
int   mailing, depth;
{
	/** return the line from either datafile that corresponds 
	    to the specified name.  If 'mailing' specified, then
	    fully expand group names.  Returns NULL if not found.
	    Depth is the nesting depth, and varies according to the
	    nesting level of the routine.  **/

	static char buffer[VERY_LONG_STRING];
	static char sprbuffer[VERY_LONG_STRING];
	datum  key, value;
	int    loc;
	struct alias_rec entry;

	name = shift_lower(name);
	key.dptr = name;
	key.dsize = strlen(name);
	value = dbz_fetch(hash, key);
	if (value.dptr == NULL)
	    return( (char *) NULL); /* not found */

	bcopy(value.dptr, (char *) &loc, sizeof(loc));
	loc -= sizeof(entry);
	lseek(user_data, loc, 0L);
	read(user_data, (char *) &entry, sizeof(entry));
	read(user_data, buffer, entry.length > VERY_LONG_STRING ? VERY_LONG_STRING : entry.length);
	if ((entry.type & GROUP) != 0 && mailing) {
	    if (expand_group(sprbuffer, buffer + (int) entry.address,
			     depth) < 0)
		return NULL;
	} else {
	    sprintf(sprbuffer, "%s (%s)", buffer + (int) entry.address,
		    buffer + (int) entry.name);
	}
	return sprbuffer;
}

int expand_group(target, members, depth)
char *target;
char *members;
int   depth;
{
	/** given a group of names separated by commas, this routine
	    will return a string that is the full addresses of each
	    member separated by spaces.  Depth is the current recursion
	    depth of the expansion (for the 'get_token' routine) **/

	char   buf[VERY_LONG_STRING], *word, *address, *bufptr;

	strcpy(buf, members); 	/* parameter safety! */
	target[0] = '\0';	/* nothing in yet!   */
	bufptr = (char *) buf;	/* grab the address  */
	depth++;		/* one more deeply into stack */

	while ((word = (char *) get_token(bufptr, "!, ", depth)) != NULL) {
	  if ((address = (char *) get_alias_address(word, 1, depth)) == NULL) {
	    fprintf(stderr, catgets(elm_msg_cat, AnswerSet, AnswerNotFoundForGroup,
		"Alias %s not found for group expansion!\n"), word);
	    return -1;
	  }
	  else if (strcmp(target,address) != 0) {
	    sprintf(target + strlen(target), " %s", address);
	  }

	  bufptr = NULL;
	}
	return 0;
}

print_long(buffer, init_len)
char *buffer;
int   init_len;
{
	/** print buffer out, 80 characters (or less) per line, for
	    as many lines as needed.  If 'init_len' is specified, 
	    it is the length that the first line can be.
	**/

	register int i, loc=0, space, length, len; 

	/* In general, go to 80 characters beyond current character
	   being processed, and then work backwards until space found! */

	length = init_len;

	do {
	  if (strlen(buffer) > loc + length) {
	    space = loc + length;
	    while (buffer[space] != ' ' && space > loc + 50) space--;
	    for (i=loc;i <= space;i++)
	      putchar(buffer[i]);
	    putchar('\n');
	    loc = space;
	  }
	  else {
	    for (i=loc, len = strlen(buffer);i < len;i++)
	      putchar(buffer[i]);
	    putchar('\n');
	    loc = len;
	  }
	  length = 80;
	} while (loc < strlen(buffer));
}

/****
     The following is a newly chopped version of the 'strtok' routine
  that can work in a recursive way (up to 20 levels of recursion) by
  changing the character buffer to an array of character buffers....
****/

#define MAX_RECURSION		20		/* up to 20 deep recursion */

#undef  NULL
#define NULL			(char *) 0	/* for this routine only   */

extern char *strpbrk();

char *get_token(string, sepset, depth)
char *string, *sepset;
int  depth;
{

	/** string is the string pointer to break up, sepstr are the
	    list of characters that can break the line up and depth
	    is the current nesting/recursion depth of the call **/

	register char	*p, *q, *r;
	static char	*savept[MAX_RECURSION];

	/** is there space on the recursion stack? **/

	if (depth >= MAX_RECURSION) {
	 fprintf(stderr, catgets(elm_msg_cat, AnswerSet, AnswerRecursionTooDeep,
		"Error: Get_token calls nested greater than %d deep!\n"),
			MAX_RECURSION);
	 exit(1);
	}

	/* set up the pointer for the first or subsequent call */
	p = (string == NULL)? savept[depth]: string;

	if(p == 0)		/* return if no tokens remaining */
		return(NULL);

	q = p + strspn(p, sepset);	/* skip leading separators */

	if (*q == '\0')		/* return if no tokens remaining */
		return(NULL);

	if ((r = strpbrk(q, sepset)) == NULL)	/* move past token */
		savept[depth] = 0;	/* indicate this is last token */
	else {
		*r = '\0';
		savept[depth] = ++r;
	}
	return(q);
}
