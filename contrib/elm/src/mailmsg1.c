
static char rcsid[] = "@(#)Id: mailmsg1.c,v 5.7 1993/07/20 02:46:14 syd Exp ";

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
 * $Log: mailmsg1.c,v $
 * Revision 1.2  1994/01/14  00:55:14  cp
 * 2.4.23
 *
 * Revision 5.7  1993/07/20  02:46:14  syd
 * Handle reply-to in batch mode.
 * From: decwrl!uunet.UU.NET!fin!chip (Chip Salzenberg)
 *
 * Revision 5.6  1993/05/08  20:25:33  syd
 * Add sleepmsg to control transient message delays
 * From: Syd
 *
 * Revision 5.5  1993/04/12  02:34:36  syd
 * I have now added a parameter which controls whether want_to clears the
 * line and centers the question or behaves like it did before. I also
 * added a 0 at the end of the parameter list to all the other calls to
 * want_to where a centered question on a clean line is not desirable.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.4  1993/02/03  17:12:53  syd
 * move more declarations to defs.h, including sleep
 * From: Syd
 *
 * Revision 5.3  1992/12/24  21:42:01  syd
 * Fix messages and nls messages to match.  Plus use want_to
 * where appropriate.
 * From: Syd, via prompting from Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.2  1992/12/20  05:15:58  syd
 * Add a c)hange alias, -u and -t options to listalias to list only user
 * and only system aliases respectively.
 * From: "Robert L. Howard" <robert.howard@matd.gatech.edu>
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** Interface to allow mail to be sent to users.  Part of ELM  **/


#include "headers.h"
#include "s_elm.h"

/** strings defined for the hdrconfg routines **/

char subject[SLEN], in_reply_to[SLEN], expires[SLEN],
     action[SLEN], priority[SLEN], reply_to[SLEN], to[VERY_LONG_STRING], 
     cc[VERY_LONG_STRING], expanded_to[VERY_LONG_STRING], 
     expanded_reply_to[LONG_STRING],
     expanded_cc[VERY_LONG_STRING], user_defined_header[SLEN],
     bcc[VERY_LONG_STRING], expanded_bcc[VERY_LONG_STRING],
     precedence[SLEN], expires_days[SLEN];

char *format_long(), *strip_commas(), *tail_of_string();

static int to_line, to_col;

void
output_abbreviated_to(address)
char *address;
{
	/** Output just the fields in parens, separated by commas if need
	    be, and up to COLUMNS-50 characters...This is only used if the
	    user is at level BEGINNER.
	**/

	char newaddress[LONG_STRING];
	register int iindex, newindex = 0, in_paren = 0, add_len;

	iindex = 0;

	add_len = strlen(address);
	while (newindex < 55 && iindex < add_len) {
	  if (address[iindex] == '(') in_paren++;
	  else if (address[iindex] == ')') { 
	    in_paren--;
	    if (iindex < add_len-4) {
	      newaddress[newindex++] = ',';
	      newaddress[newindex++] = ' ';
	    }
	  }
	  
	  /* copy if in_paren but not at the opening outer parens */
	  if (in_paren && !(address[iindex] == '(' && in_paren == 1))
	      newaddress[newindex++] = address[iindex];
	     
	  iindex++;
	}

	newaddress[newindex] = '\0';

	if (mail_only)
	  if (strlen(newaddress) > 80) 
	    PutLine1(to_line, to_col, "To: (%s)", 
	       tail_of_string(newaddress, 60));
	  else
	    PutLine1(to_line, to_col, "To: %s", newaddress);
	else if (strlen(newaddress) > 50) 
	   PutLine1(to_line, to_col, "To: (%s)", 
	       tail_of_string(newaddress, 40));
	 else {
	   if (strlen(newaddress) > 30)
	     PutLine1(to_line, to_col, "To: %s", newaddress);
	   else
	     PutLine1(to_line, to_col, "          To: %s", newaddress);
	   CleartoEOLN();
	 }

	return;
}

void
display_to(address)
char *address;
{
	/** Simple routine to display the "To:" line according to the
	    current configuration (etc) 			      
	 **/
	register int open_paren;

	to_line = mail_only ? 3 : LINES - 3;
	to_col = mail_only ? 0 : COLUMNS - 50;
	if (names_only)
	  if ((open_paren = chloc(address, '(')) > 0) {
	    if (open_paren < chloc(address, ')')) {
	      output_abbreviated_to(address);
	      return;
	    } 
	  }
	if(mail_only)
	  if(strlen(address) > 80)
	    PutLine1(to_line, to_col, "To: (%s)", 
	        tail_of_string(address, 75));
	  else
	    PutLine1(to_line, to_col, "To: %s", address);
	else if (strlen(address) > 45) 
	  PutLine1(to_line, to_col, "To: (%s)", 
	      tail_of_string(address, 40));
	else {
	  if (strlen(address) > 30) 
	    PutLine1(to_line, to_col, "To: %s", address);
	  else
	    PutLine1(to_line, to_col, "          To: %s", address);
	  CleartoEOLN();
	}
}

int
get_to(to_field, address)
char *to_field, *address;
{
	/** prompt for the "To:" field, expanding into address if possible.
	    This routine returns ZERO if errored, or non-zero if okay **/

	if (strlen(to_field) == 0) {
	  if (user_level < 2) {
	    PutLine0(LINES-2, 0, catgets(elm_msg_cat, ElmSet, ElmSendTheMessageTo,
		"Send the message to: "));
	    (void) optionally_enter(to_field, LINES-2, 21, FALSE, FALSE); 
	  }
	  else {
	    PutLine0(LINES-2, 0, catgets(elm_msg_cat, ElmSet, ElmTo, "To: "));
	    (void) optionally_enter(to_field, LINES-2, 4, FALSE, FALSE); 
	  }
	  if (strlen(to_field) == 0) {
	    ClearLine(LINES-2);	
	    return(0);
	  }
	  (void) build_address(strip_commas(to_field), address); 
	}
	else if (mail_only) 
	  (void) build_address(strip_commas(to_field), address); 
	else 
	  strcpy(address, to_field);
	
	if (strlen(address) == 0) {	/* bad address!  Removed!! */
	  ClearLine(LINES-2);
	  return(0);
	}

	return(1);		/* everything is okay... */
}

int
send_msg(given_to, given_cc, given_subject, edit_message, form_letter, replying)
char *given_to, *given_cc, *given_subject;
int   edit_message, form_letter, replying;
{
	/** Prompt for fields and then call mail() to send the specified
	    message.  If 'edit_message' is true then don't allow the
            message to be edited. 'form_letter' can be "YES" "NO" or "MAYBE".
	    if YES, then add the header.  If MAYBE, then add the M)ake form
	    option to the last question (see mailsg2.c) etc. etc. 
	    if (replying) then add an In-Reply-To: header...
	    Return TRUE if the main part of the screen has been changed
	    (useful for knowing whether a redraw is needed.
	**/

	int  copy_msg = FALSE, is_a_response = FALSE;
	char *p;

	/* First: zero all current global message strings */

	cc[0] = bcc[0] = reply_to[0] = expires[0] = '\0';
	action[0] = priority[0] = user_defined_header[0] = in_reply_to[0] ='\0';
	expanded_to[0] = expanded_cc[0] = expanded_bcc[0] = '\0';
	expanded_reply_to[0] = precedence[0] = expires_days[0] = '\0';

	/* Then: fill in default values for some fields */

	strcpy(subject, given_subject);
	strcpy(to, given_to);
	strcpy(cc, given_cc);

	if ((p = getenv("REPLYTO")) != NULL)
	  strcpy(reply_to, p);

	/******* And now the real stuff! *******/

	/* copy msg into edit buffer? */
	copy_msg = copy_the_msg(&is_a_response);

	/* get the To: address and expand */
	if (! get_to(to, expanded_to))
	  return(0);

	/* expand the Cc: address */
	if (cc[0])
	  build_address(strip_commas(cc), expanded_cc);

	/* expand the Reply-To: address */
	if (reply_to[0])
	  build_address(strip_commas(reply_to), expanded_reply_to);

	/** if we're batchmailing, let's send it and GET OUTTA HERE! **/

	if (batch_only) {
	  return(mail(FALSE, FALSE, form_letter));
	}

	display_to(expanded_to);	/* display the To: field on screen... */

	if (get_subject(subject) == 0)	    /* get the Subject: field */
	  return(0);

	if (prompt_for_cc) {
	  if (get_copies(cc, expanded_to, expanded_cc, copy_msg) == 0)
	    return(0);
	}

	MoveCursor(LINES,0);	/* so you know you've hit <return> ! */

	/** generate the In-Reply-To: header... **/

	if (is_a_response && replying)
	  generate_reply_to(current-1);

	/* and mail that puppy outta here! */
	
	dprint(3, (debugfile, "\nsend_msg() ready to mail...\n"));
	dprint(3, (debugfile, "to=\"%s\" expanded_to=\"%s\"\n",to,expanded_to));
	dprint(4, (debugfile, "subject=\"%s\"\n",subject));
	dprint(5, (debugfile, "cc=\"%s\" expanded_cc=\"%s\"\n",cc,expanded_cc));
	dprint(5, (debugfile, "bcc=\"%s\" expanded_bcc=\"%s\"\n",bcc,expanded_bcc));
	return(mail(copy_msg, edit_message, form_letter));
}

int
get_subject(subject_field)
char *subject_field;
{
	char	ch, msgbuf[SLEN];

	/** get the subject and return non-zero if all okay... **/
	int len = 9, prompt_line;

	prompt_line = mail_only ? 4 : LINES-2;

	if (user_level == 0) {
	  PutLine0(prompt_line,0, catgets(elm_msg_cat, ElmSet, ElmSubjectOfMessage,
		"Subject of message: "));
	  len = 20;
	}
	else
	  PutLine0(prompt_line,0, catgets(elm_msg_cat, ElmSet, ElmSubject, "Subject: "));

	CleartoEOLN();

	if(optionally_enter(subject_field, prompt_line, len, TRUE, FALSE)==-1){
	  /** User hit the BREAK key! **/
	  MoveCursor(prompt_line,0); 	
	  CleartoEOLN();
	  error(catgets(elm_msg_cat, ElmSet, ElmMailNotSent, "Mail not sent."));
	  return(0);
	}

	if (strlen(subject_field) == 0) {	/* zero length subject?? */
	  MCsprintf(msgbuf, catgets(elm_msg_cat, ElmSet, ElmNoSubjectContinue,
	    "No subject - Continue with message? (%c/%c) "),
	    *def_ans_yes, *def_ans_no);

	  ch = want_to(msgbuf, *def_ans_no, prompt_line, 0);
	  if (ch != *def_ans_yes) {	/* user says no! */
	    if (sleepmsg > 0)
		sleep((sleepmsg + 1) / 2);
	    ClearLine(prompt_line);
	    error(catgets(elm_msg_cat, ElmSet, ElmMailNotSend, "Mail not sent."));
	    return(0);
	  }
	  else {
	    PutLine0(prompt_line,0,"Subject: <none>");
	    CleartoEOLN();
	  }
	}

	return(1);		/** everything is cruising along okay **/
}

int
get_copies(cc_field, address, addressII, copy_message)
char *cc_field, *address, *addressII;
int   copy_message;
{
	/** Get the list of people that should be cc'd, returning ZERO if
	    any problems arise.  Address and AddressII are for expanding
	    the aliases out after entry! 
	    If 'bounceback' is nonzero, add a cc to ourselves via the remote
	    site, but only if hops to machine are > bounceback threshold.
	    If copy-message, that means that we're going to have to invoke
	    a screen editor, so we'll need to delay after displaying the
	    possibly rewritten Cc: line...
	**/
	int prompt_line;

	prompt_line = mail_only ? 5 : LINES - 1;
	PutLine0(prompt_line,0, catgets(elm_msg_cat, ElmSet, ElmCopiesTo, "Copies to: "));

	fflush(stdout);

	if (optionally_enter(cc_field, prompt_line, 11, FALSE, FALSE) == -1) {
	  ClearLine(prompt_line-1);
	  ClearLine(prompt_line);
	  
	  error(catgets(elm_msg_cat, ElmSet, ElmMailNotSend, "Mail not sent."));
	  return(0);
	}
	
	/** The following test is that if the build_address routine had
	    reason to rewrite the entry given, then, if we're mailing only
	    print the new Cc line below the old one.  If we're not, then
	    assume we're in screen mode and replace the incorrect entry on
	    the line above where we are (e.g. where we originally prompted
	    for the Cc: field).
	**/

	if (build_address(strip_commas(cc_field), addressII)) {
	  PutLine1(prompt_line, 11, "%s", addressII);
	  if ((strcmp(editor, "builtin") != 0 && strcmp(editor, "none") != 0)
	      || copy_message)
	    if (sleepmsg > 0)
		sleep(sleepmsg);
	}

	if (strlen(address) + strlen(addressII) > VERY_LONG_STRING) {
	  dprint(2, (debugfile, 
		"String length of \"To:\" + \"Cc\" too long! (get_copies)\n"));
	  error(catgets(elm_msg_cat, ElmSet, ElmTooManyPeople, "Too many people. Copies ignored."));
	  if (sleepmsg > 0)
		sleep(sleepmsg);
	  cc_field[0] = '\0';
	}

	return(1);		/* everything looks okay! */
}
	
int
copy_the_msg(is_a_response)
int *is_a_response;
{
	/** Returns True iff the user wants to copy the message being
	    replied to into the edit buffer before invoking the editor! 
	    Sets "is_a_response" to true if message is a response...
	**/

	char msg[SLEN];
	int answer = FALSE;

	if (forwarding)
	  answer = TRUE;
	else if (strlen(to) > 0 && !mail_only) {  /* predefined 'to' line! */
	  if (auto_copy) 
	    answer = TRUE;
	  else {
	    MCsprintf(msg, catgets(elm_msg_cat, ElmSet, ElmCopyMessageYN,
		"Copy message? (%c/%c) "), *def_ans_yes, *def_ans_no);
	    answer = (want_to(msg, *def_ans_no, LINES-3, 0) == *def_ans_yes);
	  }
	  *is_a_response = TRUE;
	}

	return(answer);
}

int
a_sendmsg(edit_message, form_letter)
int   edit_message, form_letter;
{
	/** Prompt for fields and then call mail() to send the specified
	    message.  If 'edit_message' is true then don't allow the
            message to be edited. 'form_letter' can be "YES" "NO" or "MAYBE".
	    if YES, then add the header.  If MAYBE, then add the M)ake form
	    option to the last question (see mailsg2.c) etc. etc. 

	    Return TRUE if the main part of the screen has been changed
	    (useful for knowing whether a redraw is needed.
	**/

	register int tagged = 0, i;
	int ret;

	/* First: zero all current global message strings */

	cc[0] = bcc[0] = reply_to[0] = expires[0] = '\0';
	action[0] = priority[0] = user_defined_header[0] = in_reply_to[0] ='\0';
	expanded_to[0] = expanded_cc[0] = expanded_bcc[0] = '\0';
	cc[0] = subject[0] = to[0] = '\0';
	expanded_reply_to[0] = precedence[0] = expires_days[0] = '\0';

	for (i=0; i < message_count; i++) {
	  if (ison(aliases[i]->status, TAGGED)) {
	    strcat(to, aliases[i]->alias);
	    strcat(to, " ");
	    tagged++;
	  }
	}

	if (tagged == 0) {
	  strcpy(to, aliases[current-1]->alias);
	}

	dprint(4, (debugfile, "%d aliases tagged for mailing (a_sndmsg)\n",
	        tagged));

	/******* And now the real stuff! *******/

	if (build_address(to, expanded_to) == 0)   /* build the To: address and expand */
	  return(0);
	if ( cc[0] != '\0' )			/* expand out CC addresses */
	  build_address(strip_commas(cc), expanded_cc);

	display_to(expanded_to);	/* display the To: field on screen... */

	if (get_subject(subject) == 0)	    /* get the Subject: field */
	  return(0);

	if (prompt_for_cc) {
	  if (get_copies(cc, expanded_to, expanded_cc, FALSE) == 0)
	    return(0);
	}

	MoveCursor(LINES,0);	/* so you know you've hit <return> ! */

	/* and mail that puppy outta here! */
	
	dprint(3, (debugfile, "\na_sendmsg() ready to mail...\n"));
	dprint(3, (debugfile, "to=\"%s\" expanded_to=\"%s\"\n",to,expanded_to));
	dprint(4, (debugfile, "subject=\"%s\"\n",subject));
	dprint(5, (debugfile, "cc=\"%s\" expanded_cc=\"%s\"\n",cc,expanded_cc));
	dprint(5, (debugfile, "bcc=\"%s\" expanded_bcc=\"%s\"\n",bcc,expanded_bcc));

	main_state();
	ret = mail(FALSE, edit_message, form_letter);
	main_state();

/*
 *	Since we got this far, it must be okay to clear the tags.
 */
	i = 0;
	while (tagged) {
	    if (ison(aliases[i]->status, TAGGED)) {
	        clearit(aliases[i]->status, TAGGED);
	        show_msg_tag(i);
	        tagged--;
	    }
	    i++;
	}

	return(ret);
}
