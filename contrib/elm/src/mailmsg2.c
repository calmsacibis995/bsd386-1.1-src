
static char rcsid[] = "@(#)Id: mailmsg2.c,v 5.35 1993/09/27 01:51:38 syd Exp ";

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
 * $Log: mailmsg2.c,v $
 * Revision 1.2  1994/01/14  00:55:15  cp
 * 2.4.23
 *
 * Revision 5.35  1993/09/27  01:51:38  syd
 * Add elm_chown to consolidate for Xenix not allowing -1
 * From: Syd
 *
 * Revision 5.34  1993/08/23  03:26:24  syd
 * Try setting group id separate from user id in chown to
 * allow restricted systems to change group id of file
 * From: Syd
 *
 * Revision 5.33  1993/08/10  18:53:31  syd
 * I compiled elm 2.4.22 with Purify 2 and fixed some memory leaks and
 * some reads of unitialized memory.
 * From: vogt@isa.de
 *
 * Revision 5.32  1993/08/03  19:28:39  syd
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
 * Revision 5.31  1993/07/20  01:47:47  syd
 * Fix time() declaration
 * From: Syd via prompt from Dan Mosedale
 *
 * Revision 5.30  1993/06/06  17:53:06  syd
 * Remove extranious clear
 * From: Chip
 *
 * Revision 5.29  1993/05/31  19:31:31  syd
 * Total rewrite of verify_transmission().  The existing code was a monsterous
 * mess and rife with bugs (couldn't forget zero-length messages in send-only
 * mode, prompts were being setup wrong, perms on Canceled.mail not done right,
 * etc. etc. etc.)  Previously, the work had been split among two places,
 * the main code in mail() and the menu in verify_transmission(), and Elm kept
 * bouncing back and forth between the two.  Now, all work is performed in
 * verify_transmission() and it simply returns a send-it/forget-it status.
 * Modified "Canceled.mail" handling so that the files it creates are in
 * an mbox format (i.e. now includes headers as well as message body).
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.28  1993/05/14  03:56:19  syd
 * A MIME body-part must end with a newline even when there was no newline
 * at the end of the actual body or the body is null. Otherwise the next
 * mime boundary may not be recognized.  The same goes with the closing
 * boundary too.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.27  1993/05/08  20:25:33  syd
 * Add sleepmsg to control transient message delays
 * From: Syd
 *
 * Revision 5.26  1993/05/08  19:54:17  syd
 * fix nesting error
 *
 * Revision 5.25  1993/05/08  19:06:58  syd
 * add not keeping empty cancelled messages
 * From: Syd via request from Phil Richards
 *
 * Revision 5.24  1993/05/08  18:51:32  syd
 * Make Elm quote full names it adds, its easier than checking
 * for specials and only quoting for specials in the name
 * From: Syd
 *
 * Revision 5.23  1993/04/21  01:25:45  syd
 * I'm using Elm 2.4.21 under Linux.  Linux has no Bourne shell.  Each
 * user installs her favorite shell as /bin/sh.  I use Bash 1.12.
 *
 * Elm invokes the mail transport (MTA) like so:
 *
 *    ( ( MTA destination; rm -f tempfile ) & ) < tempfile &
 *
 * This form of command doesn't work with my Bash, in which any command
 * which is backgrounded ("&") gets its stdin attached to /dev/null.
 *
 * The below patch arranges for Elm to call the MTA thusly:
 *
 *    ( MTA destination <tempfile; rm -f tempfile ) &
 * From: decwrl!uunet.UU.NET!fin!chip (Chip Salzenberg)
 *
 * Revision 5.22  1993/04/12  03:57:45  syd
 * Give up and add an Ultrix specific patch. There is a bug in Ispell under
 * ultrix.  The problem is that when ispell returns, the terminal is no
 * longer in raw mode. (Ispell isn't restoring the terminal parameters)
 * From: Scott Ames <scott@cwis.unomaha.edu>
 *
 * Revision 5.21  1993/04/12  02:34:36  syd
 * I have now added a parameter which controls whether want_to clears the
 * line and centers the question or behaves like it did before. I also
 * added a 0 at the end of the parameter list to all the other calls to
 * want_to where a centered question on a clean line is not desirable.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.20  1993/02/12  21:35:07  syd
 * fix edit form catting on to end, strcat->strcpy
 * From: Syd
 *
 * Revision 5.19  1993/02/03  17:12:53  syd
 * move more declarations to defs.h, including sleep
 * From: Syd
 *
 * Revision 5.18  1993/02/03  16:45:26  syd
 * A Raw(OFF) was missing so when in mail only mode and one
 * does f)orget, the "Message saved" ends up on wrong screen.
 * Also added \r\n to end of messages to make output look nicer.
 *
 * When composing mail in the builtin editor, it wrapped on /.
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.17  1993/02/03  16:36:56  syd
 * Fix where we remove the extra hostbangs and on which fields we do
 * it to include bcc and cc as well as to, and to do it in the file
 * copies as well as the command line.
 *
 * Revision 5.16  1993/01/20  03:02:19  syd
 * Move string declarations to defs.h
 * From: Syd
 *
 * Revision 5.15  1992/12/28  14:24:25  syd
 * Fix problem where it won't let no answer on recall last message
 * From: Syd
 *
 * Revision 5.14  1992/12/24  21:42:01  syd
 * Fix messages and nls messages to match.  Plus use want_to
 * where appropriate.
 * From: Syd, via prompting from Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.13  1992/12/20  05:04:51  syd
 * restore unintnded prefix_chars that caused forwarding without editing
 * to add prefix
 * From: Syd
 *
 * Revision 5.12  1992/12/11  01:58:50  syd
 * Allow for use from restricted shell by putting SHELL=/bin/sh in the
 * environment of spawned mail transport program.
 * From: chip@tct.com (Chip Salzenberg)
 *
 * Revision 5.11  1992/12/07  04:26:26  syd
 * add missing err declare
 * From: Syd
 *
 * Revision 5.10  1992/12/07  03:20:30  syd
 * Change $HOME/Cancelled.mail in message to /fullpath/Cancelled.mail
 * as AFS uses different meaning for $HOME than Elm does and it was
 * confusing to some users.
 * From: Syd
 *
 * Revision 5.9  1992/11/26  01:46:26  syd
 * add Decode option to copy_message, convert copy_message to
 * use bit or for options.
 * From: Syd and bjoerns@stud.cs.uit.no (Bjoern Stabell)
 *
 * Revision 5.8  1992/11/26  00:46:13  syd
 * changes to first change screen back (Raw off) and then issue final
 * error message.
 * From: Syd
 *
 * Revision 5.7  1992/11/22  01:14:20  syd
 * Allow SCO MMDF to use the mmdf library for mailer via execmail.
 * From: Larry Philps <larryp@sco.com>
 *
 * Revision 5.6  1992/11/07  20:29:28  syd
 * These small typos stopped me from making forms, so here is a patch.
 * From: andrew.mcparland@rd.eng.bbc.co.uk (Andrew McParland)
 *
 * Revision 5.5  1992/11/07  16:21:56  syd
 * There is no need to write out the MIME-Version header in subparts
 * From: Klaus Steinberger <Klaus.Steinberger@Physik.Uni-Muenchen.DE>
 *
 * Revision 5.4  1992/10/30  21:04:04  syd
 * fix a bug in the forms reply caused by the Cc: buffer not being
 * cleared.  If one has sent mail just prior to replying to the form and
 * specified a Cc:  address, the form reply is also sent to those Cc:
 * address(es).
 * From: dwolfe@pffft.sps.mot.com (Dave Wolfe)
 *
 * Revision 5.3  1992/10/25  01:47:45  syd
 * fixed a bug were elm didn't call metamail on messages with a characterset,
 * which could be displayed by elm itself, but message is encoded with QP
 * or BASE64
 * From: Klaus Steinberger <Klaus.Steinberger@Physik.Uni-Muenchen.DE>
 *
 * Revision 5.2  1992/10/24  13:44:41  syd
 * There is now an additional elmrc option "displaycharset", which
 * sets the charset supported on your terminal. This is to prevent
 * elm from calling out to metamail too often.
 * Plus a slight documentation update for MIME composition (added examples)
 * From: Klaus Steinberger <Klaus.Steinberger@Physik.Uni-Muenchen.DE>
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** Interface to allow mail to be sent to users.  Part of ELM  **/

#include "headers.h"
#include "s_elm.h"
#include <errno.h>

extern int errno;
extern char version_buff[];

char *error_description(), *strip_parens();
char *format_long(), *strip_commas(), *tail_of_string(); 
long ftell();

#ifdef SITE_HIDING 
 char *get_ctime_date();
#endif
FILE *write_header_info();

/* these are all defined in the mailmsg1.c file! */

extern char subject[SLEN], in_reply_to[SLEN], expires[SLEN],
            action[SLEN], priority[SLEN], reply_to[SLEN], to[VERY_LONG_STRING], 
	    cc[VERY_LONG_STRING], expanded_to[VERY_LONG_STRING], 
	    expanded_reply_to[LONG_STRING],
	    expanded_cc[VERY_LONG_STRING], user_defined_header[SLEN],
	    bcc[VERY_LONG_STRING], expanded_bcc[VERY_LONG_STRING],
	    precedence[SLEN], expires_days[SLEN];


#ifdef	MIME
int msg_is_multipart;
#endif /* MIME */
long C_L_Position[2];   /*To Remember position of the Content-Length*/
long C_StartData[2];    /*To Remember length of Header Area */
long C_EndData[2];	/* To Remember the end of the BodyPart */

int  gotten_key;
char *bounce_off_remote();

/*
 * remove_hostbang - Given an expanded list of addresses, remove all
 * occurrences of "thishost!" at the beginning of addresses.
 * This hack is useful in itself, but it is required now because of the
 * kludge disallowing alias expansion on return addresses.
 */

void
remove_hostbang(addrs)
char *addrs;
{
	int i, j, hlen, flen;

	if ((hlen = strlen(hostname)) < 1)
	  return;

	flen = strlen(hostfullname);
	i = j = 0;

	while (addrs[i]) {
	  if (j == 0 || isspace(addrs[j - 1])) {
	    if (strncmp(&addrs[i], hostname, hlen) == 0 &&
	      addrs[i + hlen] == '!') {
	        i += hlen + 1;
	        continue;
	    }
	    if (strncmp(&addrs[i], hostfullname, flen) == 0 &&
	      addrs[i + flen] == '!') {
	        i += flen + 1;
	        continue;
	    }
	  }
	  addrs[j++] = addrs[i++];
	}
	addrs[j] = 0;
}

int
mail(copy_msg, edit_message, form)
int  copy_msg, edit_message, form;
{
	/** Given the addresses and various other miscellany (specifically, 
	    'copy-msg' indicates whether a copy of the current message should 
	    be included, 'edit_message' indicates whether the message should 
	    be edited) this routine will invoke an editor for the user and 
	    then actually mail off the message. 'form' can be YES, NO, or
	    MAYBE.  YES=add "Content-Type: mailform" header, MAYBE=add the
	    M)ake form option to last question, and NO=don't worry about it!
	    Also, if 'copy_msg' = FORM, then grab the form temp file and use
	    that...
	    Return TRUE if the main part of the screen has been changed
	    (useful for knowing whether a redraw is needed.
	**/

	FILE *reply, *real_reply; /* second is post-input buffer */
	char *whole_msg_file, *tempnam();
	char filename[SLEN], fname[SLEN], copy_file[SLEN],
             very_long_buffer[VERY_LONG_STRING], mailerflags[NLEN];
	int ch, sys_status, line_len;
	register int retransmit = FALSE; 
	int      already_has_text = FALSE;		/* we need an ADDRESS */
	int	 signature_done = FALSE;
	int	 need_redraw = 0;
	int	 err;

	static int cancelled_msg = 0;

	dprint(4, (debugfile, "\nMailing to \"%s\" (with%s editing)\n",
		  expanded_to, edit_message? "" : "out"));
	
	gotten_key = 0;		/* ignore previously gotten encryption key */

	/** first generate the temporary filename **/

	sprintf(filename,"%s%s%d", temp_dir, temp_file, getpid());

	/** if possible, let's try to recall the last message? **/

	if (! batch_only && copy_msg != FORM && user_level != 0)
	  retransmit = recall_last_msg(filename, copy_msg, &cancelled_msg, 
		       &already_has_text);

	/** if we're not retransmitting, create the file.. **/

	if (! retransmit)
	  if ((reply = fopen(filename,"w")) == NULL) {
	    err = errno;
	    dprint(1, (debugfile, 
               "Attempt to write to temp file %s failed with error %s (mail)\n",
		 filename, error_description(err)));
	    if(batch_only) {
	      MCprintf(catgets(elm_msg_cat, ElmSet, ElmCouldNotCreateFile,
		"Could not create file %s (%s)."),
		filename, error_description(err));
	      printf("\n");
	    } else
	      error2(catgets(elm_msg_cat, ElmSet, ElmCouldNotCreateFile,
		"Could not create file %s (%s)."),
		filename, error_description(errno));
	    return(need_redraw);
	  }

	(void) elm_chown(filename, userid, groupid);

	/* copy the message from standard input */
	if (batch_only) {
	  while (line_len = fread(very_long_buffer, 1, sizeof(very_long_buffer), stdin))
	    fwrite(very_long_buffer, 1, line_len, reply);
	}

	/** if there is an included file, copy it into the temp file **/
	if (*included_file) {
	  FILE *input;
	  if ((input = fopen(included_file,"r")) == NULL) {
	    dprint(1, (debugfile, 
              "Can't open included file %s.  Failed with error %s (mail)\n",
	      included_file, error_description(errno)));
	    error2(catgets(elm_msg_cat, ElmSet, ElmCouldNotOpenFile,
	      "Could not open file %s."), included_file);
	    return(need_redraw);
	  }

	  while (fgets(very_long_buffer, VERY_LONG_STRING, input) != NULL) 
	    fputs(very_long_buffer, reply);

	  fclose(input);
	  already_has_text = TRUE;
	}

	if (copy_msg == FORM) {
	  sprintf(fname, "%s%s%d", temp_dir, temp_form_file, getpid());
	  fclose(reply);	/* we can't retransmit a form! */
	  if (access(fname,ACCESS_EXISTS) != 0) {
	    if(batch_only) {
	      printf(catgets(elm_msg_cat, ElmSet, ElmCouldNotFindForm,
		"Couldn't find forms file!"));
	      printf("\n");
	    } else
	      error(catgets(elm_msg_cat, ElmSet, ElmCouldNotFindForm,
		"Couldn't find forms file!"));
	    return(need_redraw);
	  }
	  dprint(4, (debugfile, "-- renaming existing file %s to file %s --\n",
		  fname, filename));
	  rename(fname, filename);

	  /* kill leftover headers since forms reply skips regular init */
	  expires[0] = '\0';
	  expires_days[0] = '\0';
	  action[0] = '\0';
	  priority[0] = '\0';
	  reply_to[0] = '\0';
	  expanded_reply_to[0] = '\0';
	  cc[0] = '\0';
	  expanded_cc[0] = '\0';
	  user_defined_header[0] = '\0';
	  bcc[0] = '\0';
	  expanded_bcc[0] = '\0';
	  precedence[0] = '\0';
	}
	else if (copy_msg && ! retransmit) {  /* if retransmit we have it! */
	  if (attribution[0]) {
	    fprintf(reply, attribution, headers[current-1]->from);
	    fputc('\n', reply);
	  }
	  else if (forwarding) {
	    fputs("Forwarded message:\n", reply);
	  }
	  if (edit_message) {
	    copy_message(prefixchars, reply,
		( noheader ? CM_REMOVE_HEADER : 0 ) | CM_MMDF_HEAD | CM_DECODE);
	    already_has_text = TRUE;	/* we just added it, right? */
	  }
	  else
	    copy_message("", reply,
		( noheader ? CM_REMOVE_HEADER : 0 ) | CM_MMDF_HEAD);
	}

        /* append signature now if we are going to use an external editor */
	/* Don't worry about the remote/local determination too much */

        if (already_has_text || 
           (strcmp(editor,"builtin") != 0 && strcmp(editor,"none") != 0)) {
	     signature_done = TRUE;
             if (!retransmit && copy_msg != FORM) 
	       already_has_text |= append_sig(reply);
	}

	if (! retransmit && copy_msg != FORM)
	  if (reply != NULL)
	    (void) fclose(reply);	/* on replies, it won't be open! */

	/** Edit the message **/

	/* calculate default save_file name */
	if(auto_cc) {
	  if(save_by_name) {
	    if(force_name) {
	      strcpy(copy_file, "=");	/* signals save by 'to' logname */
	    } else {
	      strcpy(copy_file, "=?");	/* conditional save by 'to' logname */
	    }
	  } else {
	    strcpy(copy_file, "<");	/* signals save to sentmail */
	  }
	} else *copy_file = '\0';	/* signals to not save a copy */

	/* ask the user to confirm transmission of the message */
	if (!batch_only) {
	    ch = (edit_message? 'e' : '\0');
	    if (verify_transmission(filename, &form, &need_redraw,
		    already_has_text, copy_file, ch) != 0) {
		cancelled_msg = (bytes(filename) > 0);
		return need_redraw;
	    }
	    if (form == YES && format_form(filename) < 1) {
		cancelled_msg = (bytes(filename) > 0);
		return need_redraw;
	    }
	}

	if ((reply = fopen(filename,"r")) == NULL) {
	    err = errno;
	    dprint(1, (debugfile,
		"Attempt to open file %s for reading failed with error %s (mail)\n",
		filename, error_description(err)));
	    if (!batch_only) {
		error1(catgets(elm_msg_cat, ElmSet, ElmCouldNotOpenReply,
		    "Could not open reply file (%s)."), error_description(err));
	    } else {
		printf(catgets(elm_msg_cat, ElmSet, ElmCouldNotOpenReply,
		    "Could not open reply file (%s)."), error_description(err));
		putchar('\n');
	    }
	    return need_redraw;
	}

	cancelled_msg = FALSE;	/* it ain't cancelled, is it? */

	/** ask about bounceback if the user wants us to.... **/

	if (uucp_hops(to) > bounceback && bounceback > 0 && copy_msg != FORM) 
	  if (verify_bounceback() == TRUE) {
	    if (strlen(cc) > 0) strcat(expanded_cc, ", ");
	    strcat(expanded_cc, bounce_off_remote(to));
	  }

#ifdef	MIME
	/* Look if its a Multimedia/multipart Message */
	msg_is_multipart = check_for_multipart(reply);

	if (msg_is_multipart == -1) {
		return(need_redraw);
	}
#endif /* MIME */

	/** grab a copy if the user so desires... **/

	remove_hostbang(expanded_to);
	remove_hostbang(expanded_cc);
	remove_hostbang(expanded_bcc);

	if (*copy_file) /* i.e. if copy_file contains a name */
	  save_copy(expanded_to, expanded_cc, expanded_bcc,
	       filename, copy_file, form);

	/** write all header information into whole_msg_file **/

	if((whole_msg_file=tempnam(temp_dir, "snd.")) == NULL) {
	  dprint(1, (debugfile, "couldn't make temp file nam! (mail)\n"));
	  if(batch_only) {
	    printf(catgets(elm_msg_cat, ElmSet, ElmCouldNotMakeTemp,
		"Sorry - couldn't make temp file name!"));
	    printf("\n");
	  } else if(mail_only)
	    error(catgets(elm_msg_cat, ElmSet, ElmCouldNotMakeTemp,
		"Sorry - couldn't make temp file name."));
	  else
	    set_error(catgets(elm_msg_cat, ElmSet, ElmCouldNotMakeTemp,
		"Sorry - couldn't make temp file name."));
	  return(need_redraw);
	}

	/** try to write headers to new temp file **/

	dprint(6, (debugfile, "Composition file='%s' and mail buffer='%s'\n", 
		    filename, whole_msg_file));

	dprint(2,(debugfile,"--\nTo: %s\nCc: %s\nBcc: %s\nSubject: %s\n---\n", 
		  expanded_to, expanded_cc, expanded_bcc, subject));

	if ((real_reply = 
	   write_header_info(whole_msg_file, expanded_to,
	     expanded_cc, expanded_bcc, form == YES, FALSE)) == NULL) {

	  /** IT FAILED!!  MEIN GOTT!  Use a dumb mailer instead! **/

	  dprint(3, (debugfile, "** write_header failed: %s\n", 
		 error_description(errno)));

	  if (cc[0] != '\0')  		/* copies! */
	    sprintf(expanded_to,"%s %s", expanded_to, expanded_cc);

	  quote_args(very_long_buffer, strip_parens(strip_commas(expanded_to)));
	  strcpy(expanded_to, very_long_buffer);

	  sprintf(very_long_buffer, "(%s -s \"%s\" %s < %s ; %s %s) &",
		  mailx, subject, expanded_to, filename, remove_cmd, filename);

	  if(batch_only) {
	    printf(catgets(elm_msg_cat, ElmSet, ElmUsedDumpMailer,
		"Message sent using dumb mailer %s."), mailx);
	    printf("\n");
	  } else
	    error1(catgets(elm_msg_cat, ElmSet, ElmUsedDumpMailer,
		"Message sent using dumb mailer %s."), mailx);
	    if (sleepmsg > 0)
		sleep(sleepmsg);

	}
	else {

	  C_StartData[0] = ftell(real_reply);

	  copy_message_across(reply, real_reply, FALSE);

          /* Append signature if not done earlier */

          if (!signature_done && !retransmit && copy_msg != FORM)
               append_sig(real_reply);
	  
#ifdef MIME
	  if (!copy_msg != FORM && msg_is_multipart) {
	    fprintf(real_reply, "\n--%s--\n", MIME_BOUNDARY);
	    if (C_L_Position[1] != 0L) {
	      C_EndData[1] = ftell(real_reply);
	      C_L_Position[1] = fseek(real_reply, C_L_Position[1], 0);
	      fprintf(real_reply, "%d", C_EndData[1] - C_StartData[1]);
	      fseek(real_reply, C_EndData[1], 0);
	    }
	  }
#endif

	  C_EndData[0] = ftell(real_reply);
	    
	  fseek(real_reply, C_L_Position[0], 0);
	  fprintf(real_reply, "%d", C_EndData[0] - C_StartData[0]);
	  fclose(real_reply);

	  if (cc[0] != '\0')  				         /* copies! */
	    sprintf(expanded_to,"%s %s", expanded_to, expanded_cc);

	  if (bcc[0] != '\0') {
	    strcat(expanded_to, " ");
	    strcat(expanded_to, expanded_bcc);
	  }

	  if (strcmp(sendmail, mailer) == 0
#ifdef SITE_HIDING
	      && ! is_a_hidden_user(username)
#endif
	  ) {
	    strcpy(mailerflags, (sendmail_verbose ? smflagsv : smflags));
	    if (metoo) strcat(mailerflags, smflagmt);
	  } else if (strcmp(submitmail, mailer) == 0)
	    strcpy(mailerflags, submitflags_s);
	  else if (strcmp(execmail, mailer) == 0) {
	    strcpy(mailerflags, (sendmail_verbose ? emflagsv : emflags));
	    if (metoo) strcat(mailerflags, emflagmt);
	  } else
	    mailerflags[0] ='\0';

	  if (strcmp(submitmail, mailer) == 0)
	    strcpy(expanded_to, " ");
	  else {
	    quote_args(very_long_buffer, strip_parens(strip_commas(expanded_to)));
	    strcpy(expanded_to, very_long_buffer);
	  }

	  sprintf(very_long_buffer,"(%s %s %s < %s ; %s %s) &", 
		mailer, mailerflags, expanded_to, whole_msg_file,
		remove_cmd, whole_msg_file);
	}

	free(whole_msg_file);

	fclose(reply);

	if(batch_only) {
	  printf(catgets(elm_msg_cat, ElmSet, ElmSendingMail,
		"Sending mail..."));
	  printf("\n");
	} else {
	  PutLine0(LINES,0,catgets(elm_msg_cat, ElmSet, ElmSendingMail,
		"Sending mail..."));
	  CleartoEOLN();
	}

	/* Take note of mailer return value */

	if ( sys_status = system_call(very_long_buffer, SY_ENV_SHELL) ) {
		/* problem case: */
		if (mail_only || batch_only) {
		   printf("\r\n");
		   printf(catgets(elm_msg_cat, ElmSet, ElmMailerReturnedError,
			"mailer returned error status %d"), sys_status);
		   printf("\r\n");
		} else {
		   sprintf(very_long_buffer, catgets(elm_msg_cat, ElmSet, ElmMailerReturnedError,
			"mailer returned error status %d"), sys_status);
		   set_error(very_long_buffer);
		}
	} else {
		/* Success case: */
		if(batch_only) {
		  printf(catgets(elm_msg_cat, ElmSet, ElmMailSent, "Mail sent!"));
	          printf("\n");
		} else if(mail_only)
		  error(catgets(elm_msg_cat, ElmSet, ElmMailSent, "Mail sent!"));
		else
		  set_error(catgets(elm_msg_cat, ElmSet, ElmMailSent, "Mail sent!"));
	}

	/* Unlink temp file now.
	 * This is a precaution in case the message was encrypted.
	 * I.e. even though this file is readable by the owner only,
	 * encryption is supposed to hide things even from someone
	 * with root privelges. The best we can do is not let this
	 * file just hang after we're finished with it.
	 */
	(void)unlink(filename);

	return(need_redraw);
}

mail_form(address, subj)
char *address, *subj;
{
	/** copy the appropriate variables to the shared space... */

	strcpy(subject, subj);
	strcpy(to, address);
	strcpy(expanded_to, address);

	return(mail(FORM, NO, NO));
}

int
recall_last_msg(filename, copy_msg, cancelled_msg, already_has_text)
char *filename;
int  copy_msg, *cancelled_msg, *already_has_text;
{
	char ch;
	char msg[SLEN];

	/** If filename exists and we've recently cancelled a message,
	    the ask if the user wants to use that message instead!  This
	    routine returns TRUE if the user wants to retransmit the last
	    message, FALSE otherwise...
	**/

	register int retransmit = FALSE;

	if (access(filename, EDIT_ACCESS) == 0 && *cancelled_msg) {
	  Raw(ON);
	  CleartoEOLN();
	  if (copy_msg)
	    MCsprintf(msg, catgets(elm_msg_cat, ElmSet, ElmRecallLastInstead,
		     "Recall last kept message instead? (%c/%c) "),
		     *def_ans_yes, *def_ans_no);
	  else
	    MCsprintf(msg, catgets(elm_msg_cat, ElmSet, ElmRecallLastKept,
		     "Recall last kept message? (%c/%c) "),
		     *def_ans_yes, *def_ans_no);
	  do {
	    ch = want_to(msg, '\0', LINES-1, 0);
	    if (ch == *def_ans_yes) {
              retransmit++;
	      *already_has_text = TRUE;
	    } else if (ch != *def_ans_no) {
	      Write_to_screen("%c??", 1, 07);	/* BEEP */
	      if (sleepmsg > 0)
		    sleep((sleepmsg + 1) / 2);
	      ch = 0;
	    }
	  } while (ch == 0);

	  fflush(stdout);

	  *cancelled_msg = 0;
	}

	return(retransmit);
}


/*
 * verify_transmission() - Ask the user to confirm transmission of the
 * message.  Returns 0 to send it, -1 to forget it.
 */
int
verify_transmission(filename, form_p, need_redraw_p,
	already_has_text, copy_file, force_cmd)
char *filename;		/* pathname to mail mssg composition file	*/
int  *form_p;		/* pointer to form message state		*/
int *need_redraw_p;	/* pointer to flag indicating screen stepped on	*/
int already_has_text;	/* indicates there is already text in the mssg	*/
char *copy_file;	/* pointer to buffer holding copy file name	*/
int force_cmd;		/* command to do, '\0' to prompt user for cmd	*/
{
    char *prompt_mssg;		/* message to display prompting for cmd	*/
    char prompt_menu[SLEN];	/* menu of available commands		*/
    int bad_cmd;		/* set TRUE to bitch about user's entry	*/
    int did_prompt;		/* TRUE if cmd prompted for and entered	*/
    int prev_form;		/* "*form_p" value last time thru loop	*/
    int cmd;			/* command to perform			*/
    char lbuf[VERY_LONG_STRING];
    int x_coord, y_coord;

    prev_form = *form_p + 1;	/* force build of prompt strings	*/
    bad_cmd = FALSE;		/* nothing to complain about yet	*/

    for (;;) {

	/* build up prompt and menu strings */
	if (prev_form == *form_p) {
	    ; /* not changed - no need to rebuild the strings */
	} else if (user_level == 0) {
	    prompt_mssg = catgets(elm_msg_cat, ElmSet, ElmVfyPromptPleaseChoose,
		"Please choose one of the following options by parenthesized letter: s");
	    strcpy(prompt_menu, catgets(elm_msg_cat, ElmSet, ElmVfyMenuUser0,
		"e)dit message, edit h)eaders, s)end it, or f)orget it."));
	} else {
	    prompt_mssg = catgets(elm_msg_cat, ElmSet, ElmVfyPromptAndNow,
		"And now: s");
	    switch (*form_p) {
	    case PREFORMATTED:
		strcpy(prompt_menu, "");
		break;
	    case YES:
		strcpy(prompt_menu, catgets(elm_msg_cat, ElmSet,
		    ElmVfyMenuEditForm, "e)dit form, "));
		break;
	    case MAYBE:
		strcpy(prompt_menu, catgets(elm_msg_cat, ElmSet,
		    ElmVfyMenuEditMake, "e)dit msg, m)ake form, "));
		break;
	    default:
		strcpy(prompt_menu, catgets(elm_msg_cat, ElmSet,
		    ElmVfyMenuEditMsg, "e)dit message, "));
		break;
	    }
	    strcat(prompt_menu, catgets(elm_msg_cat, ElmSet, ElmVfyMenuVfyCpy,
		"h)eaders, c)opy, "));
#ifdef ISPELL
	    strcat(prompt_menu, catgets(elm_msg_cat, ElmSet, ElmVfyMenuIspell,
		"i)spell, "));
#endif
#ifdef ALLOW_SUBSHELL
	    strcat(prompt_menu, catgets(elm_msg_cat, ElmSet, ElmVfyMenuShell,
		"!)shell, "));
#endif
	    strcat(prompt_menu, catgets(elm_msg_cat, ElmSet, ElmVfyMenuSndFgt,
		"s)end, or f)orget"));
	}
	prev_form = *form_p;

	/* complain if last entry was bad */
	if (bad_cmd) {
	    Write_to_screen("%c??", 1, 07);
	    if (sleepmsg > 0)
		sleep((sleepmsg + 1) / 2);
	    bad_cmd = FALSE;
	}

	/* if we don't have a cmd, display prompt and get response from user */
	if (force_cmd != '\0') {
	    cmd = tolower(force_cmd);
	    force_cmd = '\0';
	    did_prompt = FALSE;
	} else {
	    MoveCursor(LINES-2, 0);
	    CleartoEOS();
	    PutLine0(LINES-2, 0, prompt_mssg);
	    GetXYLocation(&x_coord, &y_coord);
	    y_coord--; /* backspace over default answer */
	    Centerline(LINES-1, prompt_menu);
	    fflush(stdin);
	    fflush(stdout);
	    Raw(ON); /* double check... testing only... */
	    MoveCursor(x_coord, y_coord);
	    cmd = ReadCh();
	    cmd = tolower(cmd);
	    did_prompt = TRUE;
	}

	/* handle command */
	switch (cmd) {

	case '\n':
	case '\r':
	case 's':
	    if (did_prompt)
		Write_to_screen("Send", 0);
	    return 0;
	    /*NOTREACHED*/

	case 'f': 
	    if (did_prompt)
		Write_to_screen("Forget", 0);
	    if (bytes(filename) <= 0) {
		; /* forget about empty files */
	    } else if (mail_only) {
		sprintf(lbuf, "%s/%s", home, dead_letter);
		(void) append_copy_to_file(to, cc, bcc, lbuf, filename,
		    *form_p);
	    } else if (user_level > 0) {
		set_error(catgets(elm_msg_cat, ElmSet, ElmVfyMessageKept,
		    "Message kept.  Can be restored at next f)orward, m)ail or r)eply."));
	    }
	    return -1;
	    /*NOTREACHED*/

	case 'c':
	    if (did_prompt)
		Write_to_screen("Copy file", 0);
	    if (name_copy_file(copy_file) != 0)
		*need_redraw_p = TRUE;
	    break;

	case 'e':
	    if (did_prompt)
		Write_to_screen("Edit", 0);
	    if (*form_p == PREFORMATTED) {
		bad_cmd = TRUE;
	    } else {
		if (*form_p == YES)
		    *form_p = MAYBE;
		*need_redraw_p = TRUE;
		if (edit_the_message(filename, already_has_text) != 0)
		    return -1;
	    }
	    break;

	case 'h':
	    if (did_prompt)
		Write_to_screen("Headers", 0);
	    (void) edit_headers();
	    *need_redraw_p = TRUE;
	    break;

	case 'm':
	    if (*form_p != MAYBE) {
		bad_cmd = TRUE;
	    } else {
		switch (check_form_file(filename)) {
		case -1:
		    /* couldn't open file??? */
		    return -1;
		case 0:
		    Write_to_screen(catgets(elm_msg_cat, ElmSet,
			ElmVfyNoFieldsInForm, "No fields in form!\007"), 0);
		    if (sleepmsg > 0)
			sleep(sleepmsg);
		    break;
		default:
		    /* looks like a good form */
		    *form_p = YES;
		    break;
		}
	    }
	    break;

#ifdef ISPELL
	case 'i':
	    if (did_prompt)
		Write_to_screen("Ispell", 0);
	    if (*form_p == PREFORMATTED) {
		bad_cmd = TRUE;
	    } else {
		if (*form_p == YES)
		    *form_p = MAYBE;
		sprintf(lbuf, "%s %s %s",
		    ISPELL_PATH, ISPELL_OPTIONS, filename);
		system_call(lbuf, SY_ENAB_SIGHUP);
		*need_redraw_p = TRUE;
#ifdef ultrix
		/* I'm told this is required to work around some sort of bug */
		force_raw();
#endif
	    }
	    break;
#endif

#ifdef ALLOW_SUBSHELL
	case '!':
	    if (subshell() != 0) {
		ClearScreen();
		*need_redraw_p = TRUE;
	    }
	    break;
#endif

	default:
	    bad_cmd = TRUE;
	    break;

	}

    }

}


FILE *
write_header_info(filename, long_to, long_cc, long_bcc, form, copy)
char *filename, *long_to, *long_cc, *long_bcc;
int   form, copy;
{
	/** Try to open filedesc as the specified filename.  If we can,
	    then write all the headers into the file.  The routine returns
	    'filedesc' if it succeeded, NULL otherwise.  Added the ability
	    to have backquoted stuff in the users .elmheaders file!
	    If copy is TRUE, then treat this as the saved copy of outbound
	    mail.
	**/

	char opentype[3];
	time_t time(), thetime;
	char *ctime();
	static FILE *filedesc;		/* our friendly file descriptor  */
        char to_buf[VERY_LONG_STRING];
	int err;

#ifdef SITE_HIDING
	char  buffer[SLEN];
	int   is_hidden_user;		/* someone we should know about?  */
#endif
#ifdef MMDF
	int   is_submit_mailer;		/* using submit means change From: */
#endif /* MMDF */

	char  *get_arpa_date();

	if(copy) 
	    strcpy(opentype, "r+");
	else
	    strcpy(opentype, "w+");

	save_file_stats(filename);

	filedesc = fopen(filename, opentype);
	if (copy && filedesc == NULL)
	  filedesc = fopen(filename, "w+");

	if (filedesc == NULL) {
	  err = errno;
	  dprint(1, (debugfile,
	    "Attempt to open file %s for writing failed! (write_header_info)\n",
	     filename));
	  dprint(1, (debugfile, "** %s **\n\n", error_description(err)));
	  error2(catgets(elm_msg_cat, ElmSet, ElmErrorTryingToWrite,
		"Error %s encountered trying to write to %s."), 
		error_description(err), filename);
	  if (sleepmsg > 0)
		sleep(sleepmsg);
	  return(NULL);		/* couldn't open it!! */
	}

	if (copy) {
		/* Position to the end of the file */
		fseek(filedesc, 0L, 2);
	}

	restore_file_stats(filename);

	if(copy) {	/* Add top line that mailer would add */
#ifdef MMDF
	  fprintf(filedesc, MSG_SEPARATOR);
#endif /* MMDF */
	  thetime = time((long *) 0);
	  fprintf(filedesc,"From %s %s", username, ctime(&thetime));
#ifdef MMDF
	} else if (strcmp(submitmail,mailer) == 0) {
	  sprintf(to_buf, "%s %s %s", long_to, long_cc, long_bcc);
	  do_mmdf_addresses(filedesc, strip_parens(strip_commas(to_buf)));
#endif /* MMDF */
	}

#ifdef SITE_HIDING
	if ( !copy && (is_hidden_user = is_a_hidden_user(username))) {
	  /** this is the interesting part of this trick... **/
	  sprintf(buffer, "From %s!%s %s\n",  HIDDEN_SITE_NAME,
		  username, get_ctime_date());
	  fprintf(filedesc, "%s", buffer);
	  dprint(1,(debugfile, "\nadded: %s", buffer));
	  /** so is this perverted or what? **/
	}
#endif


	/** Subject moved to top of headers for mail because the
	    pure System V.3 mailer, in its infinite wisdom, now
	    assumes that anything the user sends is part of the 
	    message body unless either:
		1. the "-s" flag is used (although it doesn't seem
		   to be supported on all implementations?? )
		2. the first line is "Subject:".  If so, then it'll
		   read until a blank line and assume all are meant
		   to be headers.
	    So the gory solution here is to move the Subject: line
	    up to the top.  I assume it won't break anyone elses program
	    or anything anyway (besides, RFC-822 specifies that the *order*
	    of headers is irrelevant).  Gahhhhh....
	**/

	fprintf(filedesc, "Subject: %s\n", subject);

	  fprintf(filedesc, "To: %s\n", format_long(long_to, strlen("To:")));

	fprintf(filedesc,"Date: %s\n", get_arpa_date());

#ifndef DONT_ADD_FROM
#ifdef MMDF
	is_submit_mailer = (strcmp(submitmail,mailer) == 0);
#endif /* MMDF */
/*
 *	quote full user name in case it contains specials
 */
# ifdef SITE_HIDING
#    ifdef MMDF
	if (is_submit_mailer)
	  fprintf(filedesc,"From: \"%s\" <%s>\n", full_username, username);
	else
#    endif /* MMDF */
	if (is_hidden_user)
	  fprintf(filedesc,"From: \"%s\" <%s!%s!%s>\n", full_username,
		  hostname, HIDDEN_SITE_NAME, username);
	else
	  fprintf(filedesc,"From: \"%s\" <%s!%s>\n", full_username,
		  hostname, username);
# else
#  ifdef  INTERNET
#   ifdef  USE_DOMAIN
#    ifdef MMDF
	if (is_submit_mailer)
	  fprintf(filedesc,"From: \"%s\" <%s>\n", full_username, username);
	else
#    endif /* MMDF */
	  fprintf(filedesc,"From: \"%s\" <%s@%s>\n", full_username, 
		username, hostfullname);
#   else
#    ifdef MMDF
	if (is_submit_mailer)
	  fprintf(filedesc,"From: \"%s\" <%s>\n", full_username, username);
	else
#    endif /* MMDF */
	fprintf(filedesc,"From: \"%s\" <%s@%s>\n", full_username,
		username, hostname);
#   endif
#  else
#    ifdef MMDF
	if (is_submit_mailer)
	  fprintf(filedesc,"From: \"%s\" <%s>\n", full_username, username);
	else
#    endif /* MMDF */
	fprintf(filedesc,"From: \"%s\" <%s!%s>\n", full_username,
		hostname, username);
#  endif
# endif
#endif

	if (cc[0] != '\0')
	    fprintf(filedesc, "Cc: %s\n", format_long(long_cc, strlen("Cc: ")));

	if (copy && (bcc[0] != '\0'))
	    fprintf(filedesc, "Bcc: %s\n", format_long(long_bcc, strlen("Bcc: ")));

	if (strlen(action) > 0)
	    fprintf(filedesc, "Action: %s\n", action);
	
	if (strlen(priority) > 0)
	    fprintf(filedesc, "Priority: %s\n", priority);

	if (strlen(precedence) > 0)
	    fprintf(filedesc, "Precedence: %s\n", precedence);
	
	if (strlen(expires) > 0)
	    fprintf(filedesc, "Expires: %s\n", expires);
	
	if (strlen(expanded_reply_to) > 0)
	    fprintf(filedesc, "Reply-To: %s\n", expanded_reply_to);

	if (strlen(in_reply_to) > 0)
	    fprintf(filedesc, "In-Reply-To: %s\n", in_reply_to);

	if (strlen(user_defined_header) > 0)
	    fprintf(filedesc, "%s\n", user_defined_header);

	add_mailheaders(filedesc);

#ifndef NO_XHEADER
	fprintf(filedesc, "X-Mailer: ELM [version %s]\n", version_buff);
#endif /* !NO_XHEADER */

	if (form)
	  fprintf(filedesc, "Content-Type: mailform\n");
#ifdef MIME
	else if (msg_is_multipart) {
	  fprintf(filedesc, "%s\n", MIME_HEADER);
	  fprintf(filedesc,
		"%s multipart/mixed; boundary=%s\n", MIME_CONTENTTYPE, MIME_BOUNDARY);
	}
#endif /* MIME */
	else {
#ifdef	MIME
	  fprintf(filedesc, "%s\n", MIME_HEADER);
	  fprintf(filedesc, "%s text/plain; charset=%s\n",MIME_CONTENTTYPE, charset);
	  fprintf(filedesc, "Content-Transfer-Encoding: %s\n", text_encoding);
#else
	  fprintf(filedesc, "Content-Type: text\n");
#endif /* MIME */
	}
	fprintf(filedesc, "Content-Length: ");
	C_L_Position[0] = ftell(filedesc);
	fprintf(filedesc, "          \n"); /* Print Blanks as Placeholders */

	putc('\n', filedesc);

	return((FILE *) filedesc);
}

copy_message_across(source, dest, copy)
FILE *source, *dest;
int copy;
{
	/** Copy the message in the file pointed to by source to the
	    file pointed to by dest.
	    If copy is TRUE, treat as a saved copy of outbound mail. **/

	int  crypted = FALSE;			/* are we encrypting?  */
	int  encoded_lines = 0;			/* # lines encoded     */
	int  line_len;
	char buffer[SLEN];			/* file reading buffer */
#ifdef MIME
	int	text_lines = 0;
	int	at_boundary = FALSE;

	C_L_Position[1] = 0L;
	C_StartData[1] = 0L;
	C_EndData[1] = 0L;

#endif /* MIME */

	while (line_len = mail_gets(buffer, SLEN, source)) {
	  if (buffer[0] == '[') {
	    if (strncmp(buffer, START_ENCODE, strlen(START_ENCODE))==0)
	      crypted = TRUE;
	    else if (strncmp(buffer, END_ENCODE, strlen(END_ENCODE))==0)
	      crypted = FALSE;
	    else if ((strncmp(buffer, DONT_SAVE, strlen(DONT_SAVE)) == 0)
	          || (strncmp(buffer, DONT_SAVE2, strlen(DONT_SAVE2)) == 0)) {
	      if(copy) break;  /* saved copy doesn't want anything after this */
	      else continue;   /* next line? */
#ifdef MIME
	    } else if (strncmp(buffer, MIME_INCLUDE, strlen(MIME_INCLUDE))==0) {
	      text_lines = 0;
	      if (!at_boundary) {
		if (C_L_Position[1] != 0L) {
		  C_EndData[1] = ftell(dest);
		  C_L_Position[1] = fseek(dest, C_L_Position[1], 0);
		  fprintf(dest, "%d", C_EndData[1] - C_StartData[1]);
		  fseek(dest, C_EndData[1], 0);
		}
		fprintf(dest, "\n--%s\n", MIME_BOUNDARY);
	      }
	      Include_Part(dest, buffer, FALSE);
	      fprintf(dest, "--%s\n", MIME_BOUNDARY);
	      at_boundary = TRUE;
	      continue;
#endif /* MIME */
	    }

	    if (crypted) {
	      if (! gotten_key++)
	        getkey(ON);
	      else if (! encoded_lines)
	        get_key_no_prompt();		/* reinitialize.. */
	      if (strncmp(buffer, START_ENCODE, strlen(START_ENCODE))) {
	        encode(buffer);
	        encoded_lines++;
	      }
	    }
	  }
	  else if (crypted) {
	    if (batch_only) {
	      printf(catgets(elm_msg_cat, ElmSet, ElmNoEncryptInBatch,
		"Sorry. Cannot send encrypted mail in \"batch mode\".\n"));
	      leave(0);
	    } else if (! gotten_key++)
	      getkey(ON);
	    else if (! encoded_lines)
	      get_key_no_prompt();		/* reinitialize.. */
	    if (strncmp(buffer, START_ENCODE, strlen(START_ENCODE))) {
	      encode(buffer);
	      encoded_lines++;
	    }
#ifdef MIME
	  } else {
	    if (text_lines == 0) {
	      if (msg_is_multipart) {
		if (!at_boundary) {
	 	  fprintf(dest,"--%s\n",MIME_BOUNDARY);
		}
	        fprintf(dest, "%s text/plain; charset=%s\n",
			MIME_CONTENTTYPE, charset);
		fprintf(dest, "Content-Transfer-Encoding: %s\n", text_encoding);
		fprintf(dest, "Content-Length: ");
		C_L_Position[1] = ftell(dest);
		fprintf(dest, "          \n"); /* Print Placeholders */
	        fprintf(dest, "\n");
		C_StartData[1] = ftell(dest);
		at_boundary = FALSE;
	      }
	    }	
	    text_lines++;
#endif /* MIME */
          }

#ifndef DONT_ESCAPE_MESSAGES
	  if (copy && (strncmp(buffer, "From ", 5) == 0)) {
	    /* Add in the > to a From on our copy */
	    fprintf(dest, ">");
	    if (fwrite(buffer, 1, line_len, dest) != line_len) {
		MoveCursor(LINES, 0);
		Raw(OFF);
		Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmWriteFailedCopyAcross,
			"\nWrite failed in copy_message_across\n"), 0);
		emergency_exit();
	    }
	  }
#ifdef NEED_LONE_PERIOD_ESCAPE
	  else if (!copy && strcmp(buffer, ".\n") == 0)
	    /* Because some mail transport agents take a lone period to
	     * mean EOF, we add a blank space on outbound message.
	     */
	    fputs(". \n", dest);
#endif /* NEED_LONE_PERIOD_ESCAPE */
  	  else
#endif /* DONT_ESCAPE_MESSAGES */
  	    if (fwrite(buffer, 1, line_len, dest) != line_len) {
		MoveCursor(LINES, 0);
		Raw(OFF);
		Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmWriteFailedCopyAcross,
			"\nWrite failed in copy_message_across\n"), 0);
		emergency_exit();
	    }
	} 
#ifdef MMDF
	if (copy) fputs(MSG_SEPARATOR, dest);
#else
	if (copy) fputs("\n", dest);	/* ensure a blank line at the end */
#endif /* MMDF */
}

int
verify_bounceback()
{
	char	ch;
	char	msg[SLEN];

	/** Ensure the user wants to have a bounceback copy too.  (This is
	    only called on messages that are greater than the specified 
	    threshold hops and NEVER for non-uucp addresses.... Returns
	    TRUE iff the user wants to bounce a copy back.... 
	 **/

	MoveCursor(LINES,0);
	CleartoEOLN();
	MCsprintf(msg, catgets(elm_msg_cat, ElmSet, ElmBounceOffRemote,
	      "\"Bounce\" a copy off the remote machine? (%c/%c) "),
	      *def_ans_yes, *def_ans_no);
	ch = want_to(msg, *def_ans_no, LINES, 0);
	return (ch == *def_ans_yes);
}


int
append_sig(file)
FILE *file;
{
	/* Append the correct signature file to file.  Return TRUE if
           we append anything.  */

        /* Look at the to and cc list to determine which one to use */

	/* We could check the bcc list too, but we don't want people to
           know about bcc, even indirectly */

	/* Some people claim that  user@anything.same_domain should be 
	   considered local.  Since it's not the same machine, better be 
           safe and use the remote sig (presumably it has more complete
           information).  You can't necessarily finger someone in the
           same domain. */

	  if (!batch_only && (local_signature[0] || remote_signature[0])) {

            char filename2[SLEN];
	    char *sig;

  	    if (index(expanded_to, '!') || index(expanded_cc,'!'))
              sig = remote_signature;		/* ! always means remote */
            else {
	      /* check each @ for @thissite.domain */
	      /* if any one is different than this, then use remote sig */
	      int len;
	      char *ptr;
	      char sitename[SLEN];
	      sprintf(sitename,"@%s",hostfullname);
	      len = strlen(sitename);
              sig = local_signature;
              for (ptr = index(expanded_to,'@'); ptr;  /* check To: list */
	          ptr = index(ptr+1,'@')) {
		if (strincmp(ptr,sitename,len) != 0
		    || (*(ptr+len) != ',' && *(ptr+len) != 0
		    && *(ptr+len) != ' ')) {
	          sig = remote_signature;
                  break;
                }
              }
              if (sig == local_signature)		   /* still local? */ 
                for (ptr = index(expanded_cc,'@'); ptr;   /* check Cc: */
		    ptr = index(ptr+1,'@')) {
		  if (strincmp(ptr,sitename,len) != 0
		      || (*(ptr+len) != ',' && *(ptr+len) != 0 
		      && *(ptr+len) != ' ')) {
	            sig = remote_signature;
                    break;
                  }
                }
            }

            if (sig[0]) {  /* if there is a signature file */
	      if (sig[0] != '/')
	        sprintf(filename2, "%s/%s", home, sig);
	      else
	        strcpy(filename2, sig);
	      /* append the file - with a news 2.11 compatible */
	      /* seperator if "sig_dashes" is enabled */
	      (void) append(file, filename2, (sig_dashes ? "\n-- \n" : NULL));

              return TRUE;
            }
          }

return FALSE;

}

