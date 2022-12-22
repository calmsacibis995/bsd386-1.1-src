
static char rcsid[] = "@(#)Id: showmsg.c,v 5.15 1993/08/23 02:46:07 syd Exp ";

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
 * $Log: showmsg.c,v $
 * Revision 1.2  1994/01/14  00:55:44  cp
 * 2.4.23
 *
 * Revision 5.15  1993/08/23  02:46:07  syd
 * Don't declare _exit() if <unistd.h> already did it.
 * From: decwrl!uunet.UU.NET!fin!chip (Chip Salzenberg)
 *
 * Revision 5.14  1993/08/03  19:28:39  syd
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
 * Revision 5.13  1993/05/08  20:25:33  syd
 * Add sleepmsg to control transient message delays
 * From: Syd
 *
 * Revision 5.12  1993/02/03  19:06:31  syd
 * Remove extra strchr/strcat/strcpy et al declarations
 * From: Syd
 *
 * Revision 5.11  1993/01/20  04:01:07  syd
 * Adds a new integer parameter builtinlines.
 * if (builtinlines < 0) and (the length of the message < LINES on
 *       screen + builtinlines) use internal.
 * if (builtinlines > 0) and (length of message < builtinlines)
 * 	use internal pager.
 * if (builtinlines = 0) or none of the above conditions hold, use the
 * external pager if defined.
 * From: "John P. Rouillard" <rouilj@ra.cs.umb.edu>
 *
 * Revision 5.10  1993/01/19  05:11:45  syd
 * Elm switches screens prematurely when calling metamail. It switches
 * before writing the "Press any key..." message, thus losing metamail output.
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.9  1992/12/07  04:29:12  syd
 * add missing err declare
 * From: Syd
 *
 * Revision 5.8  1992/11/26  01:46:26  syd
 * add Decode option to copy_message, convert copy_message to
 * use bit or for options.
 * From: Syd and bjoerns@stud.cs.uit.no (Bjoern Stabell)
 *
 * Revision 5.7  1992/11/26  00:46:50  syd
 * Fix how errno is used so err is inited and used instead
 * as errno gets overwritten by print system call
 * From: Syd
 *
 * Revision 5.6  1992/11/15  01:29:37  syd
 * Clear the screen before displaying MIME:
 * From: marius@rhi.hi.is (Marius Olafsson)
 *
 * Revision 5.5  1992/11/07  16:23:48  syd
 * fix null dereferences from patch 5
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.4  1992/10/30  21:47:30  syd
 * Use copy_message in mime shows to get encode processing
 * From: bjoerns@stud.cs.uit.no (Bjoern Stabell)
 *
 * Revision 5.3  1992/10/25  01:47:45  syd
 * fixed a bug were elm didn't call metamail on messages with a characterset,
 * which could be displayed by elm itself, but message is encoded with QP
 * or BASE64
 * From: Klaus Steinberger <Klaus.Steinberger@Physik.Uni-Muenchen.DE>
 *
 * Revision 5.2  1992/10/24  13:35:39  syd
 * changes found by using codecenter on Elm 2.4.3
 * From: Graham Hudspith <gwh@inmos.co.uk>
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** This file contains all the routines needed to display the specified
    message.
**/

#include "headers.h"
#include "s_elm.h"
#include <errno.h>

#ifdef BSD
# include <sys/wait.h>
#endif

#ifndef I_UNISTD
void _exit();
#endif

extern int errno;

extern char *elm_date_str(), *error_description();

int    memory_lock = FALSE;	/* is it available?? */
int    pipe_abort  = FALSE;	/* did we receive a SIGNAL(SIGPIPE)? */

FILE *pipe_wr_fp;		/* file pointer to write to external pager */
extern int lines_displayed,	/* defined in "builtin" */	
	   lines_put_on_screen;	/*    ditto too!        */

int
show_msg(number)
int number;
{
	/*** Display number'th message.  Get starting and ending lines
	     of message from headers data structure, then fly through
	     the file, displaying only those lines that are between the
	     two!

	     Return 0 to return to the index screen or a character entered
	     by the user to initiate a command without returning to
	     the index screen (to be processed via process_showmsg_cmd()).
	***/

	char title1[SLEN], title2[SLEN], title3[SLEN], titlebuf[SLEN];
	char who[LONG_STRING], buffer[VERY_LONG_STRING];
#if defined(BSD) && !defined(WEXITSTATUS)
	union wait wait_stat;
#else
	int wait_stat;
#endif

	int crypted = 0;			/* encryption */
	int weed_header, weeding_out = 0;	/* weeding    */ 
	int using_to,				/* misc use   */
	    pipe_fd[2],				/* pipe file descriptors */
	    new_pipe_fd,			/* dup'ed pipe fil des */
	    lines,				/* num lines in msg */
	    fork_ret,				/* fork return value */
	    wait_ret,				/* wait return value */
	    form_letter = FALSE,		/* Form ltr?  */
	    form_letter_section = 0,		/* section    */
	    padding = 0,			/*   counter  */
	    builtin = FALSE,			/* our pager? */
	    val = 0,				/* return val */
	    buf_len,				/* line length */
	    err;				/* place holder for errno */
	struct header_rec *current_header = headers[number-1];
#ifdef	SIGTSTP
	SIGHAND_TYPE	(*oldstop)(), (*oldcont)();
#endif

	lines = current_header->lines; 

	dprint(4, (debugfile,"displaying %d lines from message %d using %s\n", 
		lines, number, pager));

	if (number > message_count || number < 1)
	  return(val);

	if(ison(current_header->status, NEW)) {
	  clearit(current_header->status, NEW);   /* it's been read now! */
	  current_header->status_chgd = TRUE;
	}
	if(ison(current_header->status, UNREAD)) {
	  clearit(current_header->status, UNREAD);   /* it's been read now! */
	  current_header->status_chgd = TRUE;
	}

	memory_lock = FALSE;

	/* some explanation for that last one - We COULD use memory locking
	   to speed up the paging, but the action of "ClearScreen" on a screen
	   with memory lock turned on seems to vary considerably (amazingly so)
	   so it's safer to only allow memory lock to be a viable bit of
	   trickery when dumping text to the screen in scroll mode.
	   Philosophical arguments should be forwarded to Bruce at the 
	   University of Walamazoo, Australia, via ACSNet  *wry chuckle* */

#ifdef MIME
	if ((current_header->status & MIME_MESSAGE) &&
	    ((current_header->status & MIME_NEEDDECOD) ||
	     (current_header->status & MIME_NOTPLAIN)) &&
	    !getenv("NOMETAMAIL") ) {
	    char fname[STRING], Cmd[SLEN], line[VERY_LONG_STRING];
	    int code, err;
	    long lines = current_header->lines;
	    FILE *fpout;

	    if (fseek(mailfile, current_header->offset, 0) != -1) {
		sprintf(fname, "%semm.%d.%d", temp_dir, getpid(), getuid());
		if ((fpout = fopen(fname, "w")) != NULL) {
		    copy_message("", fpout, CM_DECODE);
		    (void) fclose (fpout);
		    sprintf(Cmd, "metamail -p -z -m Elm %s", fname);
		    Raw(OFF);
                  ClearScreen();
		    code = system_call(Cmd, SY_ENAB_SIGINT);
		    Raw(ON | NO_TITE);	/* Raw on but don't switch screen */
		    (void) unlink (fname);
		    PutLine0(LINES,0, catgets(elm_msg_cat, ElmSet, ElmPressAnyKeyIndex,
			     "Press any key to return to index."));
		    (void) ReadCh();
		    printf("\r\n");
		    Raw(OFF | NO_TITE); /* Raw off so raw on takes effect */
		    Raw(ON); /* Finally raw on and switch screen */
		    return(0);
		}
	    }
	}
#endif

	if (fseek(mailfile, current_header->offset, 0) == -1) {
	  err = errno;
	  dprint(1, (debugfile,
		  "Error: seek %d bytes into file, errno %s (show_message)\n",
		  current_header->offset, error_description(err)));
	  error2(catgets(elm_msg_cat, ElmSet, ElmSeekFailedFile,
		  "ELM [seek] couldn't read %d bytes into file (%s)."),
		  current_header->offset, error_description(err));	
	  return(val);
	}
	if(current_header->encrypted)
	  getkey(OFF);

	if(builtin=(first_word(pager,"builtin")||
	   first_word(pager,"internal"))||
	   ( builtin_lines < 0 ? lines < LINES + builtin_lines :
	    lines < builtin_lines))

	  start_builtin(lines);

	else {

	  /* put terminal out of raw mode so external pager has normal env */
	  Raw(OFF);

	  /* create pipe for external pager and fork */

	  if(pipe(pipe_fd) == -1) {
	    err = errno;
	    dprint(1, (debugfile, "Error: pipe failed, errno %s (show_msg)\n",
	      error_description(err)));
	    error1(catgets(elm_msg_cat, ElmSet, ElmPreparePagerPipe,
	      "Could not prepare for external pager(pipe()-%s)."),
	      error_description(err));	
	    Raw(ON);
	    return(val);
	  }

	  if((fork_ret = fork()) == -1) {

	    err = errno;
	    dprint(1, (debugfile, "Error: fork failed, errno %s (show_msg)\n",
	      error_description(err)));
	    error1(catgets(elm_msg_cat, ElmSet, ElmPreparePagerFork,
	      "Could not prepare for external pager(fork()-%s)."),
	      error_description(err));	
	    Raw(ON);
	    return(val);

	  } else if (fork_ret == 0) {

	    /* child fork */

	    /* close write-only pipe fd and fit read-only pipe fd to stdin */

	    close(pipe_fd[1]);
	    close(fileno(stdin));
	    if((new_pipe_fd = dup(pipe_fd[0])) == -1) {
	      err = errno;
	      dprint(1, (debugfile, "Error: dup failed, errno %s (show_msg)\n",
		error_description(err)));
	      error1(catgets(elm_msg_cat, ElmSet, ElmPreparePagerDup,
	        "Could not prepare for external pager(dup()-%s)."),
		error_description(err));	
	      _exit(err);
	    }
	    close(pipe_fd[0]);	/* original pipe fd no longer needed */

	    /* use stdio on new pipe fd */
	    if(fdopen(new_pipe_fd, "r") == NULL) {
	      err = errno;
	      dprint(1,
		(debugfile, "Error: child fdopen failed, errno %s (show_msg)\n",
		error_description(err)));
	      error1(catgets(elm_msg_cat, ElmSet, ElmPreparePagerChildFdopen,
		"Could not prepare for external pager(child fdopen()-%s)."),
		error_description(err));	
	      _exit(err);
	    }

	    /* now execute pager and exit */
	    
	    /* system_call() will return user to user's normal permissions.
	     * This is what makes this pipe secure - user won't have elm's
	     * special setgid permissions (if so configured) and will only
	     * be able to execute a pager that user normally has permission
	     * to execute */

	    _exit(system_call(pager, SY_ENAB_SIGINT));
	  
	  } /* else this is the parent fork */

	  /* close read-only pipe fd and do write-only with stdio */
	  close(pipe_fd[0]);

	  if((pipe_wr_fp = fdopen(pipe_fd[1], "w")) == NULL) {
	    err = errno;
	    dprint(1,
	      (debugfile, "Error: parent fdopen failed, errno %s (show_msg)\n",
	      error_description(err)));
	    error1(catgets(elm_msg_cat, ElmSet, ElmPreparePagerParentFdopen,
	      "Could not prepare for external pager(parent fdopen()-%s)."),
	      error_description(err));	

	    /* Failure - must close pipe and wait for child */
	    close(pipe_fd[1]);
	    while ((wait_ret = wait(&wait_stat)) != fork_ret && wait_ret!= -1)
	      ;

	    Raw(OFF);
	    return(val);	/* pager may have already touched the screen */
	  }

	  /* and that's it! */
	  lines_displayed = 0;
#ifdef	SIGTSTP
	  oldstop = signal(SIGTSTP,SIG_DFL);
	  oldcont = signal(SIGCONT,SIG_DFL);
#endif 
	}

	ClearScreen();

	if (cursor_control) transmit_functions(OFF);

	pipe_abort = FALSE;

	if (form_letter = (current_header->status&FORM_LETTER)) {
	  if (filter)
	    form_letter_section = 1;	/* initialize to section 1 */
	}

	if (title_messages && filter) {

	  using_to =
	    tail_of(current_header->from, who, current_header->to);

	  MCsprintf(title1, "%s %d/%d  ",
		    headers[current-1]->status & DELETED ? nls_deleted :
		    form_letter ? nls_form: nls_message,
		    number, message_count);
	  MCsprintf(title2, "%s %s", using_to? nls_to : nls_from, who);
	  elm_date_str(title3, current_header->time_sent + current_header->tz_offset);
	  strcat(title3, " ");
	  strcat(title3, current_header->time_zone);

	  /* truncate or pad title2 portion on the right
	   * so that line fits exactly */
	  padding =
	    COLUMNS -
	    (strlen(title1) + (buf_len=strlen(title2)) + strlen(title3));

	  sprintf(titlebuf, "%s%-*.*s%s\n", title1, buf_len+padding,
	      buf_len+padding, title2, title3);

	  if (builtin)
	    display_line(titlebuf, strlen(titlebuf));
	  else
	    fprintf(pipe_wr_fp, "%s", titlebuf);

	  /** if there's a subject, let's output it next,
	      centered if it fits on a single line.  **/

	  if ((buf_len = strlen(current_header->subject)) > 0 && 
		matches_weedlist("Subject:")) {
	    padding = (buf_len < COLUMNS ? COLUMNS - buf_len : 0);
	    sprintf(buffer, "%*s%s\n", padding/2, "", current_header->subject);
	  } else
	    strcpy(buffer, "\n");

	  if (builtin)
	    display_line(buffer, strlen(buffer));
	  else
	    fprintf(pipe_wr_fp, "%s", buffer);
	  
	  /** was this message address to us?  if not, then to whom? **/

	  if (! using_to && matches_weedlist("To:") && filter &&
	      strcmp(current_header->to,username) != 0 &&
	      strlen(current_header->to) > 0) {
	    sprintf(buffer, catgets(elm_msg_cat, ElmSet, ElmMessageAddressedTo,
		  "%s(message addressed to %.60s)\n"), 
	          strlen(current_header->subject) > 0 ? "\n" : "",
		  current_header->to);
	    if (builtin)
	      display_line(buffer, strlen(buffer));
	    else
	      fprintf(pipe_wr_fp, "%s", buffer);
	  }
	
	  /** The test above is: if we didn't originally send the mail
	      (e.g. we're not reading "mail.sent") AND the user is currently
	      weeding out the "To:" line (otherwise they'll get it twice!)
	      AND the user is actually weeding out headers AND the message 
	      wasn't addressed to the user AND the 'to' address is non-zero 
	      (consider what happens when the message doesn't HAVE a "To:" 
	      line...the value is NULL but it doesn't match the username 
	      either.  We don't want to display something ugly like 
	      "(message addressed to )" which will just clutter up the 
	      screen!).

	      And you thought programming was EASY!!!!
	  **/

	  /** one more friendly thing - output a line indicating what sort
	      of status the message has (e.g. Urgent etc).  Mostly added
	      for X.400 support, this is nonetheless generally useful to
	      include...
	  **/

	  buffer[0] = '\0';

	  /* we want to flag Urgent, Confidential, Private and Expired tags */

	  if (current_header->status & PRIVATE)
	    strcpy(buffer, catgets(elm_msg_cat, ElmSet, ElmTaggedPrivate,
		"\n(** This message is tagged Private"));
	  else if (current_header->status & CONFIDENTIAL) 
	    strcpy(buffer, catgets(elm_msg_cat, ElmSet, ElmTaggedCompanyConfidential,
		"\n(** This message is tagged Company Confidential"));

	  if (current_header->status & URGENT) {
	    if (buffer[0] == '\0')
	      strcpy(buffer, catgets(elm_msg_cat, ElmSet, ElmTaggedUrgent,
		"\n(** This message is tagged Urgent"));
	    else if (current_header->status & EXPIRED)
	      strcat(buffer, catgets(elm_msg_cat, ElmSet, ElmCommaUrgent, ", Urgent"));
	    else
	      strcat(buffer, catgets(elm_msg_cat, ElmSet, ElmAndUrgent, " and Urgent"));
	  }

	  if (current_header->status & EXPIRED) {
	    if (buffer[0] == '\0')
	      strcpy(buffer, catgets(elm_msg_cat, ElmSet, ElmMessageHasExpired,
		"\n(** This message has Expired"));
	    else
	      strcat(buffer, catgets(elm_msg_cat, ElmSet, ElmAndHasExpired,
		", and has Expired"));
	  }

	  if (buffer[0] != '\0') {
	    strcat(buffer, " **)\n");
	    if (builtin)
	      display_line(buffer, strlen(buffer));
	    else
	      fprintf(pipe_wr_fp, buffer);
	  }

	  if (builtin)			/* this is for a one-line blank    */
	    display_line("\n",1);	/*   separator between the title   */
	  else				/*   stuff and the actual message  */
	    fprintf(pipe_wr_fp, "\n");	/*   we're trying to display       */

	}

	weed_header = filter;	/* allow us to change it after header */

	while (lines > 0 && pipe_abort == FALSE) {

	    if ((buf_len = mail_gets(buffer, VERY_LONG_STRING, mailfile)) == 0) {

	      dprint(1, (debugfile,
		"Premature end of file! Lines left = %d msg = %s (show_msg)\n",
		lines, number));

	      error(catgets(elm_msg_cat, ElmSet, ElmPrematureEndOfFile,
		"Premature end of file!"));
	      if (sleepmsg > 0)
		    sleep(sleepmsg);
	      break;
	    }
	    if (buf_len > 0)  {
	      if(buffer[buf_len - 1] == '\n') {
	        lines--;
	        lines_displayed++;
		}
	      while(buf_len > 0 && (buffer[buf_len - 1] == '\n'
				    ||buffer[buf_len - 1] == '\r'))
		--buf_len;
	    }

  	    if (buf_len == 0) {
	      weed_header = 0;		/* past header! */
	      weeding_out = 0;
	    }

	    if (form_letter && weed_header)
		/* skip it.  NEVER display random headers in forms! */;
	    else if (weed_header && matches_weedlist(buffer)) 
	      weeding_out = 1;	 /* aha!  We don't want to see this! */
	    else if (buffer[0] == '[') {
	      if (strncmp(buffer, START_ENCODE, strlen(START_ENCODE))==0)
	        crypted = ON;
	      else if (strncmp(buffer, END_ENCODE, strlen(END_ENCODE))==0)
	        crypted = OFF;
	      else if (crypted) {
                encode(buffer);
	        val = show_line(buffer, buf_len, builtin);
	      }
	      else
	        val = show_line(buffer, buf_len, builtin);
	    } 
	    else if (crypted) {
	      encode(buffer);
	      val = show_line(buffer, buf_len, builtin); 
	    }
	    else if (weeding_out) {
	      weeding_out = (whitespace(buffer[0]));	/* 'n' line weed */
	      if (! weeding_out) 	/* just turned on! */
	        val = show_line(buffer, buf_len, builtin);
	    } 
	    else if (form_letter && first_word(buffer,"***") && filter) {
	      strcpy(buffer,
"\n------------------------------------------------------------------------------\n");
	      val = show_line(buffer, buf_len, builtin);	/* hide '***' */
	      form_letter_section++;
	    }
	    else if (form_letter_section == 1 || form_letter_section == 3)
	      /** skip this stuff - we can't deal with it... **/;
	    else
	      val = show_line(buffer, buf_len, builtin);
	
	    if (val != 0)	/* discontinue the display */
	      break;
	}

        if (cursor_control) transmit_functions(ON);

	if (!builtin) {
	  fclose(pipe_wr_fp);
	  while ((wait_ret = wait(&wait_stat)) != fork_ret
		  && wait_ret!= -1)
	    ;
	  /* turn raw on **after** child terminates in case child
	   * doesn't put us back to cooked mode after we return ourselves
	   * to raw.
	   */
	  Raw(ON);
#ifdef	SIGTSTP
	  (void)signal(SIGTSTP,oldstop);
	  (void)signal(SIGCONT,oldcont);
#endif
	}

	/* If we are to prompt for a user input command and we don't
	 * already have one */
	if ((prompt_after_pager || builtin) && val == 0) {
	  MoveCursor(LINES,0);
	  StartBold();
	  Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmCommandIToReturn,
		" Command ('i' to return to index): "), 0);
	  EndBold();
	  fflush(stdout);
	  val = ReadCh();
	}
	
	if (memory_lock) EndMemlock();	/* turn it off!! */

	/* 'q' means quit current operation and pop back up to previous level -
	 * in this case it therefore means return to index screen.
	 */
	return(val == 'i' || val == 'q' ? 0 : val);
}

int
show_line(buffer, buf_len, builtin)
char *buffer;
int  buf_len;
int  builtin;
{
	/** Hands the given line to the output pipe.  'builtin' is true if
	    we're using the builtin pager.
	    Return the character entered by the user to indicate
	    a command other than continuing with the display (only possible
	    with the builtin pager), otherwise 0. **/

	strcpy(buffer + buf_len, "\n");
	++buf_len;
#ifdef MMDF
	if (strncmp(buffer, MSG_SEPARATOR, buf_len) == 0)
	  return(0);	/* no reason to show the ending terminator */
#endif /* MMDF */
	if (builtin) {
	  return(display_line(buffer, buf_len));
	}
	errno = 0;
	fprintf(pipe_wr_fp, "%s", buffer);
	if (errno != 0)
	  dprint(1, (debugfile, "\terror %s hit!\n", error_description(errno)));
	return(0);
}

