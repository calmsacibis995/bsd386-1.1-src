
static char rcsid[] = "@(#)Id: editmsg.c,v 5.19 1993/09/27 01:51:38 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.2 $   $State: Exp $
 *
 * 			Copyright (c) 1988-1992 USENET Community Trust
 *			Copyright (c) 1986,1987 Dave Taylor
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 *******************************************************************************
 * $Log: editmsg.c,v $
 * Revision 1.2  1994/01/14  00:54:44  cp
 * 2.4.23
 *
 * Revision 5.19  1993/09/27  01:51:38  syd
 * Add elm_chown to consolidate for Xenix not allowing -1
 * From: Syd
 *
 * Revision 5.18  1993/08/23  03:26:24  syd
 * Try setting group id separate from user id in chown to
 * allow restricted systems to change group id of file
 * From: Syd
 *
 * Revision 5.17  1993/08/23  02:55:38  syd
 * Fix problem where deleting to previous line caused duplication due to the
 * file being opened for append (in append mode, all writes are to the end of
 * file regardless of the file pointer).
 * From: pdc@lunch.asd.sgi.com (Paul Close)
 *
 * Revision 5.16  1993/08/03  20:12:46  syd
 * Fix signal type for 386bsd
 * From: Scott Mace <smace@freefall.cdrom.com>
 *
 * Revision 5.15  1993/08/03  19:28:39  syd
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
 * Revision 5.14  1993/07/20  02:41:24  syd
 * Three changes to expand_env() in src/read_rc.c:  make it non-destructive,
 * have it return an error code instead of bailing out, and add a buffer
 * size argument to avoid overwritting the destination.  The first is to
 * avoid all of the gymnastics Elm needed to go through (and occasionally
 * forgot to go through) to protect the value handed to expand_env().
 * The second is because expand_env() was originally written to support
 * "elmrc" and bailing out was a reasonable thing to do there -- but not
 * in the other places where it has since been used.  The third is just
 * a matter of practicing safe source code.
 *
 * This patch changes all invocations to expand_env() to eliminate making
 * temporary copies (now that the routine is non-destructive) and to pass
 * in a destination length.  Since expand_env() no longer bails out on
 * error, a do_expand_env() routine was added to src/read_rc.c handle
 * this.  Moreover, the error message now gives some indication of what
 * the problem is rather than just saying "can't expand".
 *
 * Gratitous change to src/editmsg.c renaming filename variables to
 * clarify the purpose.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.13  1993/06/10  03:07:39  syd
 * This fixes a bug in the MIME code.  Include_Part() uses expand_env()
 * to expand the include file name, but since expand_env() is destructive
 * [it uses strtok()] the file name gets corrupted, and the "Content-Name"
 * header can contain a bogus value.  The easy fix would be a one-line
 * hack to Include_Part to use a temporary buffer.  This patch does not
 * implement the easy fix.  *Every* place expand_env() is used, its side
 * effects cause problems.  I think the right fix is to make expand_env()
 * non-destructive (i.e. have it duplicate the input to a temporary buffer
 * and work from there).  The attached patch modifies expand_env() in
 * that manner, and eliminates all of the `copy to a temporary buffer'
 * calls that precede it throughout elm.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.12  1993/05/08  20:25:33  syd
 * Add sleepmsg to control transient message delays
 * From: Syd
 *
 * Revision 5.11  1993/04/12  03:12:52  syd
 * Added function enforce_newline to enforce newline (what else :-) at
 * end of message.
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.10  1993/04/12  01:23:11  syd
 * Fix builtin editor so you can run "readmsg" with "~<".
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.9  1993/02/03  17:12:53  syd
 * move more declarations to defs.h, including sleep
 * From: Syd
 *
 * Revision 5.8  1993/01/20  03:37:16  syd
 * Nits and typos in the NLS messages and corresponding default messages.
 * From: dwolfe@pffft.sps.mot.com (Dave Wolfe)
 *
 * Revision 5.7  1992/12/12  01:44:03  syd
 * Fix building editor wrap problem
 * From: Syd via prompting from vogt@isa.de (Gerald Vogt)
 *
 * Revision 5.6  1992/12/11  01:56:11  syd
 * If sigset() and sigrelse() are available, release signal before
 * using longjmp() to leave signal handler.
 * From: chip@tct.com (Chip Salzenberg)
 *
 * Revision 5.5  1992/11/26  00:46:13  syd
 * changes to first change screen back (Raw off) and then issue final
 * error message.
 * From: Syd
 *
 * Revision 5.4  1992/11/22  00:03:28  syd
 * Handle the case where a system does
 * >         #define jmp_buf sigjmp_buf
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.3  1992/11/17  04:05:24  syd
 * Fix for editing interrupt broken by posix_signal().
 * From cs.utexas.edu!chinacat!chip Mon Nov 16 09:03:04 1992
 *
 * Revision 5.2  1992/10/17  22:22:33  syd
 * Fix warnings from my ANSI C compiler because the declaration of
 * edit_interrupt did not match the prototype for the second argument of
 * a call to signal.
 * From: Larry Philps <larryp@sco.COM>
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** This contains routines to do with starting up and using an editor (or two)
    from within Elm.  This stuff used to be in mailmsg2.c...
**/

#include "headers.h"
#include "s_elm.h"
#include <errno.h>
#ifndef BSD
/* BSD has already included setjmp.h in headers.h */
#include <setjmp.h>
#endif /* BSD */

#if defined(POSIX_SIGNALS) && !defined(__386BSD__)
# define JMP_BUF		sigjmp_buf
# define SETJMP(env)		sigsetjmp((env), 1)
# define LONGJMP(env,val)	siglongjmp((env), (val))
#else
# define JMP_BUF		jmp_buf
# define SETJMP(env)		setjmp(env)
# define LONGJMP(env,val)	longjmp((env), (val))
#endif

char *error_description(), *format_long(), *strip_commas();
long  fsize();

/* The built-in editor is not re-entrant! */
static int	builtin_editor_active = FALSE;
static char *simple_continue = NULL;
static char *post_ed_continue = NULL;

extern int errno;
extern char to[VERY_LONG_STRING], cc[VERY_LONG_STRING], 
	    expanded_to[VERY_LONG_STRING], expanded_cc[VERY_LONG_STRING], 
	    bcc[VERY_LONG_STRING], expanded_bcc[VERY_LONG_STRING],
	    subject[SLEN];

int      interrupts_while_editing;	/* keep track 'o dis stuff         */
JMP_BUF  edit_location;			/* for getting back from interrupt */

void
tilde_help()
{
	/* a simple routine to print out what is available at this level */

	char	buf[SLEN];

	Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmEditmsgAvailOpts,
	  "\n\r(Available options at this point are:\n\r\n\r"), 0);
	sprintf(buf, catgets(elm_msg_cat, ElmSet, ElmEditmsgHelpMenu,
	  "\t%c?\tPrint this help menu.\n\r"), escape_char);
	Write_to_screen(buf, 0);
	if (escape_char == TILDE_ESCAPE) /* doesn't make sense otherwise... */
	  Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmEditmsgAddLine,
	      "\t~~\tAdd line prefixed by a single '~' character.\n\r"), 0);

	sprintf(buf, catgets(elm_msg_cat, ElmSet, ElmEditmsgBCC,
	  "\t%cb\tChange the addresses in the Blind-carbon-copy list.\n\r"),
	  escape_char);
	Write_to_screen(buf, 0);

	sprintf(buf, catgets(elm_msg_cat, ElmSet, ElmEditmsgCC,
		"\t%cc\tChange the addresses in the Carbon-copy list.\n\r"),
		escape_char);
	Write_to_screen(buf, 0);
	sprintf(buf, catgets(elm_msg_cat, ElmSet, ElmEditmsgEmacs,
	      "\t%ce\tInvoke the Emacs editor on the message, if possible.\n\r"),
		escape_char);
	Write_to_screen(buf, 0);
	sprintf(buf, catgets(elm_msg_cat, ElmSet, ElmEditmsgAddMessage,
		"\t%cf\tAdd the specified message or current.\n\r"),
		escape_char);
	Write_to_screen(buf, 0);
	sprintf(buf, catgets(elm_msg_cat, ElmSet, ElmEditmsgToCCBCC,
	      "\t%ch\tChange all available headers (to, cc, bcc, subject).\n\r"),
		escape_char);
	Write_to_screen(buf, 0);
	sprintf(buf, catgets(elm_msg_cat, ElmSet, ElmEditmsgSameCurrentPrefix,
		"\t%cm\tSame as '%cf', but with the current 'prefix'.\n\r"),
		escape_char, escape_char);
	Write_to_screen(buf, 0);
	sprintf(buf, catgets(elm_msg_cat, ElmSet, ElmEditmsgUserEditor,
		"\t%co\tInvoke a user specified editor on the message.\n\r"),
		escape_char);
	Write_to_screen(buf, 0);
	sprintf(buf, catgets(elm_msg_cat, ElmSet, ElmEditmsgPrintMsg,
	      "\t%cp\tPrint out message as typed in so far.\n\r"), escape_char);
	Write_to_screen(buf, 0);
	sprintf(buf, catgets(elm_msg_cat, ElmSet, ElmEditmsgReadFile,
		"\t%cr\tRead in the specified file.\n\r"), escape_char);
	Write_to_screen(buf, 0);
	sprintf(buf, catgets(elm_msg_cat, ElmSet, ElmEditmsgSubject,
		"\t%cs\tChange the subject of the message.\n\r"), escape_char);
	Write_to_screen(buf, 0);
	sprintf(buf, catgets(elm_msg_cat, ElmSet, ElmEditmsgTo,
		"\t%ct\tChange the addresses in the To list.\n\r"),
		escape_char);
	Write_to_screen(buf, 0);
	sprintf(buf, catgets(elm_msg_cat, ElmSet, ElmEditmsgVi,
		"\t%cv\tInvoke the Vi visual editor on the message.\n\r"),
		escape_char);
	Write_to_screen(buf, 0);
	sprintf(buf, catgets(elm_msg_cat, ElmSet, ElmEditmsgUnixCmd,
	  "\t%c!\tExecute a UNIX command (or give a shell if no command).\n\r"),
	  escape_char);
	Write_to_screen(buf, 0);
	sprintf(buf, catgets(elm_msg_cat, ElmSet, ElmEditmsgAddUnixCmd,
      "\t%c<\tExecute a UNIX command adding the output to the message.\n\r"),
	  escape_char);
	Write_to_screen(buf, 0);
	sprintf(buf, catgets(elm_msg_cat, ElmSet, ElmEditmsgEndMsg,
      "\t.  \tby itself on a line (or a control-D) ends the message.\n\r"));
	Write_to_screen(buf, 0);
	Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmEditmsgContinue,
	  "Continue.)\n\r"), 0);
}

void
read_in_file(fd, filename, show_user_filename)
FILE *fd;
char *filename;
int   show_user_filename;
{
	/** Open the specified file and stream it in to the already opened 
	    file descriptor given to us.  When we're done output the number
	    of lines and characters we added, if any... **/

	FILE *myfd;
	char exp_fname[SLEN], buffer[SLEN];
	register int n;
	register int lines = 0, nchars = 0;

	while (whitespace(*filename))
		++filename;

	/** expand any shell variables or leading '~' **/
	(void) expand_env(exp_fname, filename, sizeof(exp_fname));

	if (exp_fname[0] == '\0') {
	  Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmNoFilenameSpecified,
	      "\n\r(No filename specified for file read! Continue.)\n\r"), 0);
	  return;
	}

	if ((myfd = fopen(exp_fname,"r")) == NULL) {
	  Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmCouldntReadFile,
	    "\n\r(Couldn't read file '%s'! Continue.)\n\r"), 1, exp_fname);
	  return;
	}

	while (n = mail_gets(buffer, SLEN, myfd)) {
	  if(buffer[n-1] == '\n') lines++;
	  nchars += n;
  	  fwrite(buffer, 1, n, fd);
	}
	fflush(fd);

	fclose(myfd);

	if (lines == 1)
	  Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmAddedLine,
	    "\n\r(Added 1 line ["), 0);
	else
	  Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmAddedLinePlural,
	    "\n\r(Added %d lines ["), 1, lines);

	if (nchars == 1)
	  Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmAddedChar,
	    "1 char] "), 0);
	else
	  Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmAddedCharPlural,
	    "%d chars] "), 1, nchars);

	if (show_user_filename)
	  Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmAddedFromFile,
		"from file %s. Continue.)\n\r"), 1, exp_fname);
	else
	  Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmAddedToMessage,
		"to message. Continue.)\n\r"), 0);

	return;
}

void
print_message_so_far(edit_fd, filename)
FILE *edit_fd;
char *filename;
{
	/** This prints out the message typed in so far.  We accomplish
	    this in a cheap manner - close the file, reopen it for reading,
	    stream it to the screen, then close the file, and reopen it
	    for appending.  Simple, but effective!

	    A nice enhancement would be for this to -> page <- the message
	    if it's sufficiently long.  Too much work for now, though.
	**/
	
	char buffer[SLEN];

	fflush(edit_fd);
	fseek(edit_fd, 0L, 0);

	Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmEditmsgPrintTo,
	    "\n\rTo: %s\n\r"), 1, format_long(to, 4));
	Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmEditmsgPrintCC,
	    "Cc: %s\n\r"), 1, format_long(cc, 4));
	Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmEditmsgPrintBCC,
	    "Bcc: %s\n\r"), 1, format_long(bcc, 5));
	Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmEditmsgPrintSubject,
	    "Subject: %s\n\r\n\r"), 1, subject);

	while (fgets(buffer, SLEN, edit_fd) != NULL) {
	  Write_to_screen(buffer, 0);
	  CarriageReturn();
	}

	Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmEditmsgPrintContinue,
	    "\n\r(Continue entering message.)\n\r"), 0);
}

void
read_in_messages(fd, buffer)
FILE *fd;
char *buffer;
{
	/** Read the specified messages into the open file.  If the
	    first character of "buffer" is 'm' then prefix it, other-
	    wise just stream it in straight...Since we're using the
	    pipe to 'readmsg' we can also allow the user to specify
	    patterns and such too...
	**/

	FILE *myfd, *popen();
	char  local_buffer[SLEN], *arg;
	register int add_prefix=0, mindex;
	register int n;
	int lines = 0, nchars = 0;

	add_prefix = tolower(buffer[0]) == 'm';

	/* strip whitespace to get argument */
	for(arg = &buffer[1]; whitespace(*arg); arg++)
		;

	/* a couple of quick checks */
	if(message_count < 1) {
	  Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmNoMessageReadContinue,
	    "(No messages to read in! Continue.)\n\r"), 0);
	  return;
	}
	if (isdigit(*arg)) {
	  if((mindex = atoi(arg)) < 1 || mindex > message_count) {
	    sprintf(local_buffer, catgets(elm_msg_cat, ElmSet, ElmValidNumbersBetween,
	      "(Valid message numbers are between 1 and %d. Continue.)\n\r"),
	      message_count);
	    Write_to_screen(local_buffer, 0);
	    return;
	  }
	}

	/* dump state information for "readmsg" to use */
	if (create_folder_state_file() != 0)
	  return;

	/* go run readmsg and get output */
	sprintf(local_buffer, "%s -- %s", readmsg, arg);
	if ((myfd = popen(local_buffer, "r")) == NULL) {
	   Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmCantFindReadmsg,
	       "(Can't find 'readmsg' command! Continue.)\n\r"),
	       0);
	   (void) remove_folder_state_file();
	   return;	
	}

	dprint(5, (debugfile, "** readmsg call: \"%s\" **\n", local_buffer));

	while (n = mail_gets(local_buffer, SLEN, myfd)) {
	  nchars += n;
	  if (local_buffer[n-1] == '\n') lines++;
	  if (add_prefix)
	    fprintf(fd, "%s", prefixchars);
	  fwrite(local_buffer, 1, n, fd);
	}

	pclose(myfd);
        (void) remove_folder_state_file();
	
	if (lines == 0) {
	  Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmEditmsgCouldntAdd,
	 	 "(Couldn't add the requested message. Continue.)\n\r"), 0);
	  return;
	}

	if (lines == 1)
	  Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmAddedLine,
	    "\n\r(Added 1 line ["), 0);
	else
	  Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmAddedLinePlural,
	    "\n\r(Added %d lines ["), 1, lines);

	if (nchars == 1)
	  Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmAddedChar,
	    "1 char] "), 0);
	else
	  Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmAddedCharPlural,
	    "%d chars] "), 1, nchars);

	Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmAddedToMessage,
		"to message. Continue.)\n\r"), 0);

	return;
}

void
get_with_expansion(prompt, buffer, expanded_buffer, sourcebuf)
char *prompt, *buffer, *expanded_buffer, *sourcebuf;
{
	/** This is used to prompt for a new value of the specified field.
	    If expanded_buffer == NULL then we won't bother trying to expand
	    this puppy out!  (sourcebuf could be an initial addition)
	**/

	Write_to_screen(prompt, 0);	fflush(stdout);	/* output! */

	if (sourcebuf != NULL) {
	  while (!whitespace(*sourcebuf) && *sourcebuf != '\0') 
	    sourcebuf++;
	  if (*sourcebuf != '\0') {
	    while (whitespace(*sourcebuf)) 
	      sourcebuf++;
	    if (strlen(sourcebuf) > 0) {
	      strcat(buffer, " ");
	      strcat(buffer, sourcebuf);
	    }
	  }
	}

	optionally_enter(buffer, -1, -1, TRUE, FALSE);	/* already data! */

	if(expanded_buffer != NULL) {
	  build_address(strip_commas(buffer), expanded_buffer);
	  if(*expanded_buffer != '\0') {
	    if (*prompt == '\n')
	      Write_to_screen("%s%s", 2, prompt, expanded_buffer);
	    else
	      Write_to_screen("\n\r%s%s", 2, prompt, expanded_buffer);
	  }
	}
	NewLine();

	return;
}

SIGHAND_TYPE
edit_interrupt(sig)
int sig;
{
	/** This routine is called when the user hits an interrupt key
	    while in the builtin editor...it increments the number of 
	    times an interrupt is hit and returns it.
	**/

	signal(SIGINT, edit_interrupt);
	signal(SIGQUIT, edit_interrupt);

	if (interrupts_while_editing++ == 0)
	  Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmEditmsgOneMoreCancel,
		"(Interrupt. One more to cancel this letter.)\n\r"),
	  	0);
	else
	  Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmEditmsgCancelled,
		"(Interrupt. Letter canceled.)\n\r"), 0);

#if defined(SIGSET) && defined(HASSIGHOLD)
	/*
	 * During execution of a signal handler set with sigset(),
	 * the originating signal is held.  It must be released or
	 * it cannot recur.
	 */
	sigrelse(sig);
#endif /* SIGSET and HASSIGHOLD */

	LONGJMP(edit_location, 1);		/* get back */
}

static
enforce_newline(filename)
char *filename;
{
	/** Make sure there is a newline at the end of filename. Mostly to
	    be nice to Sun's sendmail, who hangs if there isn't. Return 0
	    if something went wrong, 1 otherwise. **/

	int err, ch, retcode = 0;
	FILE* fp;

	if ((fp = fopen(filename, "r+")) == NULL) {
	  err = errno;
	  dprint(1, (debugfile, "fopen failed on %s with mode r+\n", filename));
	  dprint(1, (debugfile, "** %s **\n", error_description(err)));
	} else if (fseek(fp, -1, 2) == -1) {
	  err = errno;
	  dprint(1, (debugfile, "fseek(-1,2) failed on %s\n", filename));
	  dprint(1, (debugfile, "** %s **\n", error_description(err)));
	} else if ((ch = fgetc(fp)) == EOF) {
	  err = errno;
	  dprint(1, (debugfile, "unexpected EOF on %s\n", filename));
	  if (ferror(fp))
	    dprint(1, (debugfile, "** %s **\n", error_description(err)));
	} else if (ch == '\n') {
	  retcode = 1;
	} else if (fseek(fp, 0, 2) == -1) {
	  err = errno;
	  dprint(1, (debugfile, "fseek(0,2) failed on %s\n", filename));
	  dprint(1, (debugfile, "** %s **\n", error_description(err)));
	  fclose(fp);
	} else if (fputc('\n', fp) == EOF) {
	  err = errno;
	  dprint(1, (debugfile, "fputc('\\n') failed on %s\n", filename));
	  dprint(1, (debugfile, "** %s **\n", error_description(err)));
	} else {
	  retcode = 1;
	}

	fclose(fp);
	return retcode;
}

int
edit_the_message(filename, already_has_text)
char *filename;
int  already_has_text;
{
	/** Invoke the users editor on the filename.  Return when done. 
	    If 'already has text' then we can't use the no-editor option
	    and must use 'alternative editor' (e.g. $EDITOR or default_editor)
	    instead... **/

	char buffer[SLEN];
	register int stat, return_value = 0, old_raw;
	int  err;

	buffer[0] = '\0';

	if (strcmp(editor, "builtin") == 0 || strcmp(editor, "none") == 0) {
	  if (already_has_text && strcmp(alternative_editor, "builtin") &&
	      strcmp(alternative_editor, "none")) {
	    if (in_string(alternative_editor, "%s"))
	      sprintf(buffer, alternative_editor, filename);
	    else
	      sprintf(buffer, "%s %s", alternative_editor, filename);
	  } else
	    return(no_editor_edit_the_message(filename));
	}

	PutLine0(LINES, 0, catgets(elm_msg_cat, ElmSet, ElmInvokeEditor,
	  "Invoking editor..."));
	fflush(stdout);

	if (strlen(buffer) == 0) {
	  if (in_string(editor, "%s"))
	    sprintf(buffer, editor, filename);
	  else
	    sprintf(buffer, "%s %s", editor, filename);
	}

	(void) elm_chown(filename, userid, groupid);

	if (( old_raw = RawState()) == ON)
	  Raw(OFF);

	if (cursor_control)
	  transmit_functions(OFF);		/* function keys are local */

	if ((stat = system_call(buffer, SY_ENAB_SIGHUP|SY_DUMPSTATE)) == -1) {
	  err = errno;
	  dprint(1,(debugfile, 
		  "System call failed with stat %d (edit_the_message)\n", 
		  stat));
	  dprint(1, (debugfile, "** %s **\n", error_description(err)));
	  ClearLine(LINES-1);
	  error1(catgets(elm_msg_cat, ElmSet, ElmCantInvokeEditor,
	    "Can't invoke editor '%s' for composition."), editor);
	  if (sleepmsg > 0)
		sleep(sleepmsg);
	  return_value = 1;
	}

	enforce_newline(filename);

	if (old_raw == ON)
	   Raw(ON);

	SetXYLocation(0, 40);	/* a location not near the next request, so an absolute is used */
	MoveCursor(LINES, 0);	/* dont know where we are, force last row, col 0 */

	if (cursor_control)
	  transmit_functions(ON);		/* function keys are local */
	
	return(return_value);
}

int
no_editor_edit_the_message(filename)
char *filename;
{
	/** If the current editor is set to either "builtin" or "none", then
	    invoke this program instead.  As it turns out, this and the 
	    routine above have a pretty incestuous relationship (!)...
	**/

	FILE *edit_fd;
	char buffer[SLEN], editor_name[SLEN], buf[SLEN], wrap[SLEN];
	int      old_raw, is_wrapped = 0;
	SIGHAND_TYPE	edit_interrupt();
	SIGHAND_TYPE	(*oldint)(), (*oldquit)();
	int  err;

	/* The built-in editor is not re-entrant! */
	if (builtin_editor_active) {
	  Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmIntReentrantBuiltinEditor,
		"\r\nInternal error - reentrant call to builtin editor attempted.\r\n\r\n"), 0);
	  dprint(1, (debugfile,
	    "Reentrant call to builtin editor for %s attempted\n", filename));
	  return(1);
	}

	if (simple_continue == NULL) {
		simple_continue = catgets(elm_msg_cat, ElmSet, ElmSimpleContinue,
		  "(Continue.)\n\r");
		post_ed_continue = catgets(elm_msg_cat, ElmSet, ElmPostEdContinue,
		  "(Continue entering message.  Type ^D or '.' on a line by itself to end.)\n\r");
	}

	if ((edit_fd = fopen(filename, "r+")) == NULL) {
	  err = errno;
	  sprintf(buffer, catgets(elm_msg_cat, ElmSet, ElmCouldntOpenAppend,
	    "Couldn't open %s for update [%s]."),
	    filename, error_description(err));
	  Write_to_screen(buffer, 0);
	  dprint(1, (debugfile,
		"Error encountered trying to open file %s;\n", filename));
	  dprint(1, (debugfile, "** %s **\n", error_description(err)));
	  return(1);
	}

	/* Skip past any existing text */
	fseek(edit_fd, 0, SEEK_END);

	/** is there already text in this file? **/

	if (fsize(edit_fd) > 0L)
	  strcpy(buf, catgets(elm_msg_cat, ElmSet, ElmContinueEntering,
	    "\n\rContinue entering message."));
	else
	  strcpy(buf, catgets(elm_msg_cat, ElmSet, ElmEnterMessage,
	    "\n\rEnter message."));
	strcat(buf, catgets(elm_msg_cat, ElmSet, ElmTypeElmCommands,
	  "  Type Elm commands on lines by themselves.\n\r"));
	sprintf(buf + strlen(buf), catgets(elm_msg_cat, ElmSet, ElmCommandsInclude,
    "Commands include:  ^D or '.' to end, %cp to list, %c? for help.\n\r\n\r"),
		escape_char, escape_char);
	CleartoEOS();
	Write_to_screen(buf, 0);

	oldint  = signal(SIGINT,  edit_interrupt);
	oldquit = signal(SIGQUIT, edit_interrupt);

	builtin_editor_active = TRUE;
	interrupts_while_editing = 0;

	if (SETJMP(edit_location) != 0) {
	  if (interrupts_while_editing > 1) {

	    (void) signal(SIGINT,  oldint);
	    (void) signal(SIGQUIT, oldquit);

	    if (edit_fd != NULL)	/* insurance... */
	      fclose(edit_fd);
	    builtin_editor_active = FALSE;
	    return(1);
	  }
	  goto more_input;	/* read input again, please! */
	}
	
more_input: buffer[0] = '\0';

/*** Code changed to provide for line wrapping ***/
wrap_input: wrap[0] = '\0';

	while (wrapped_enter(buffer, wrap, -1,-1, edit_fd, &is_wrapped) == 0) {

	  if (is_wrapped) { /* No need to check for escape or break */
	    fprintf(edit_fd, "%s\n", buffer);
	    NewLine();
	    sprintf(buffer,"%s",wrap);
	    goto wrap_input;
	  }

/*** End of changes for line wrapping ***/

	  interrupts_while_editing = 0;	/* reset to zero... */

	  if (strcmp(buffer, ".") == 0)
	    break;	/* '.' is as good as a ^D to us dumb programs :-) */
	  if (buffer[0] == escape_char) {
	    switch (tolower(buffer[1])) {
	      case '?' : tilde_help();
			 goto more_input;

	      case TILDE_ESCAPE: move_left(buffer, 1); 
			  	 goto tilde_input;	/*!!*/

	      case 't' : get_with_expansion("\n\rTo: ",
	      		   to, expanded_to, buffer);
			 goto more_input;
	      case 'b' : get_with_expansion("\n\rBcc: ",
			   bcc, expanded_bcc, buffer);
			 goto more_input;
	      case 'c' : get_with_expansion("\n\rCc: ",
			   cc, expanded_cc, buffer);
			 goto more_input;
	      case 's' : get_with_expansion("\n\rSubject: ",
			   subject,NULL,buffer);
   			 goto more_input;

	      case 'h' : get_with_expansion("\n\rTo: ", to, expanded_to, NULL);	
			 get_with_expansion("Cc: ", cc, expanded_cc, NULL);
			 get_with_expansion("Bcc: ", bcc,expanded_bcc, NULL);
			 get_with_expansion("Subject: ", subject, NULL, NULL);
			 goto more_input;

	      case 'r' : read_in_file(edit_fd, (char *) buffer + 2, 1);
		  	 goto more_input;

	      case 'e' : if (strlen(e_editor) > 0) 
			   if (access(e_editor, ACCESS_EXISTS) == 0) {
			     strcpy(buffer, editor);
			     strcpy(editor, e_editor);
	  		     fclose(edit_fd);
			     (void) edit_the_message(filename,0);
			     strcpy(editor, buffer);
			     edit_fd = fopen(filename, "a+");
			     Write_to_screen(post_ed_continue, 0);
			     goto more_input;
			   }
			   else
			     Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmCantFindEmacs,
			"\n\r(Can't find Emacs on this system! Continue.)\n\r"),
			0);
			 else
			   Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmDontKnowEmacs,
			"\n\r(Don't know where Emacs would be. Continue.)\n\r"),
			0);	
			 goto more_input;

	       case 'v' : NewLine();
			  strcpy(buffer, editor);
			  strcpy(editor, v_editor);
			  fclose(edit_fd);
			  (void) edit_the_message(filename,0);
			  strcpy(editor, buffer);
			  edit_fd = fopen(filename, "a+");
			  Write_to_screen(post_ed_continue, 0);
			  goto more_input;

	       case 'o' : Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmEnterNameEditor,
			     "\n\rPlease enter the name of the editor: "), 0);
			  editor_name[0] = '\0';
			  optionally_enter(editor_name,-1,-1, FALSE, FALSE);
			  NewLine();
			  if (strlen(editor_name) > 0) {
			    strcpy(buffer, editor);
			    strcpy(editor, editor_name);
			    fclose(edit_fd);
			    (void) edit_the_message(filename,0);
			    strcpy(editor, buffer);
			    edit_fd = fopen(filename, "a+");
			    Write_to_screen(post_ed_continue, 0);
			    goto more_input;
			  }
	  		  Write_to_screen(simple_continue, 0);
			  goto more_input; 

		case '<' : NewLine();
			   if (strlen(buffer) < 3)
			     Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmUseSpecificCommand,
		 "(You need to use a specific command here. Continue.)\n\r"));
			   else {
			     sprintf(buf, " > %s%s.%d 2>&1", temp_dir, temp_edit, getpid());
			     strcat(buffer, buf);
			     if (( old_raw = RawState()) == ON)
			       Raw(OFF);
			     (void) system_call((char *) buffer+2, SY_ENAB_SIGINT|SY_DUMPSTATE);
			     if (old_raw == ON)
				Raw(ON);
			     sprintf(buffer, "~r %s%s.%d", temp_dir, temp_edit, getpid());
	      		     read_in_file(edit_fd, (char *) buffer + 3, 0);
			     (void) unlink((char *) buffer+3);
			     SetXYLocation(0, 40);	/* a location not near the next request, so an absolute is used */
			     MoveCursor(LINES, 0);	/* and go to a known location, last row col 0 */
			   }
			   goto more_input; 

		case '!' : NewLine();
			   if (( old_raw = RawState()) == ON)
			     Raw(OFF);
			   (void) system_call((strlen(buffer) < 3 ? (char *)NULL : buffer+2),
				SY_USER_SHELL|SY_ENAB_SIGINT|SY_DUMPSTATE);
			   if (old_raw == ON)
			      Raw(ON);
			   SetXYLocation(0, 40);	/* a location not near the next request, so an absolute is used */
			   MoveCursor(LINES, 0);	/* and go to a known location, last row col 0 */
	    		   Write_to_screen(simple_continue, 0);
			   goto more_input;

		 case 'm' : /* same as 'f' but with leading prefix added */

		 case 'f' : /* this can be directly translated into a
			       'readmsg' call with the same params! */
			    NewLine();
			    read_in_messages(edit_fd, (char *) buffer + 1);
			    goto more_input;

		 case 'p' : /* print out message so far.  Soooo simple! */
			    print_message_so_far(edit_fd, filename);
			    goto more_input;

		 default  : MCsprintf(buf, catgets(elm_msg_cat, ElmSet, ElmDontKnowChar,
			 "\n\r(Don't know what %c%c is. Try %c? for help.)\n\r"),
				    escape_char, buffer[1], escape_char);
			    Write_to_screen(buf, 0);
	       }
	     }
	     else {
tilde_input:
	       fprintf(edit_fd, "%s\n", buffer);
	       NewLine();
	     }
	  buffer[0] = '\0';
	}


	Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmEndOfMessage,
	  "\n\r<end-of-message>\n\r\n\r\n\r\n\r"), 0);

	(void) signal(SIGINT,  oldint);
	(void) signal(SIGQUIT, oldquit);

	if (edit_fd != NULL)	/* insurance... */
	  fclose(edit_fd);

	builtin_editor_active = FALSE;
	return(0);
}

