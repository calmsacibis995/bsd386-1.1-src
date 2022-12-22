
static char rcsid[] = "@(#)Id: reply.c,v 5.15 1993/08/03 19:28:39 syd Exp ";

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
 * $Log: reply.c,v $
 * Revision 1.2  1994/01/14  00:55:35  cp
 * 2.4.23
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
 * Revision 5.14  1993/07/20  02:05:17  syd
 * A long-standing bug of handling replies to VMS systems.
 * Original "From: " -line is of format:
 * 	From: "NAME \"Real Name\"" <USERNAME@vms-system>
 * (PMDF mailer)
 * 	Anyway,  parse_arpa_who()  strips quotes too cleanly
 * resulting data:
 * 	NAME \"Real Name\
 * which, when put into parenthesis, becomes:
 * 	(NAME \"Real Name\)
 * which in its turn lacks closing `)'
 * Patch of  lib/parsarpwho.c  fixes that.
 * strtokq() started one position too late to search for next double-quote (") char.
 * Another one-off (chops off trailing comment character, quote or not..)  in   src/reply.c
 * From:	Matti Aarnio <mea@utu.fi>
 *
 * Revision 5.13  1993/06/10  03:02:46  syd
 * break_down_tolist() tried to blindly split address lists at "," which
 * caused bogus results with addreses that had a comma inside a comment
 * or quoted text, such as "user@domain (Last, First)".  This patch steps
 * through the address in quanta of RFC-822 tokens when searching for a
 * delimiting comma.  It also adds "rfc822_toklen()" to the library to
 * get that length.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.12  1993/04/12  03:02:05  syd
 * If a To: or Cc: line is split in a comment, that is between ( and ),
 * get_and_expand_everyone won't parse that correctly.
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.11  1993/04/12  02:34:36  syd
 * I have now added a parameter which controls whether want_to clears the
 * line and centers the question or behaves like it did before. I also
 * added a 0 at the end of the parameter list to all the other calls to
 * want_to where a centered question on a clean line is not desirable.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.10  1993/02/06  16:46:06  syd
 * remove outdate include of utsname, no longer needed in reply.c
 * From: Syd
 *
 * Revision 5.9  1993/02/03  19:06:31  syd
 * Remove extra strchr/strcat/strcpy et al declarations
 * From: Syd
 *
 * Revision 5.8  1993/02/03  16:25:45  syd
 * Adresses with double quoted strings that contains comma was parsed
 * wrongly by break_down_tolist() and figure_out_addressee().
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.7  1992/12/24  21:42:01  syd
 * Fix messages and nls messages to match.  Plus use want_to
 * where appropriate.
 * From: Syd, via prompting from Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.6  1992/12/11  01:45:04  syd
 * remove sys/types.h include, it is now included by defs.h
 * and this routine includes defs.h or indirectly includes defs.h
 * From: Syd
 *
 * Revision 5.5  1992/11/26  00:46:50  syd
 * Fix how errno is used so err is inited and used instead
 * as errno gets overwritten by print system call
 * From: Syd
 *
 * Revision 5.4  1992/11/07  20:05:52  syd
 * change to use header_cmp to allow for linear white space around the colon
 * From: Syd
 *
 * Revision 5.3  1992/10/24  13:35:39  syd
 * changes found by using codecenter on Elm 2.4.3
 * From: Graham Hudspith <gwh@inmos.co.uk>
 *
 * Revision 5.2  1992/10/11  01:25:58  syd
 * Add undefs of tolower so BSD macro isnt used from ctype.h
 * From: Syd
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/*** routine allows replying to the sender of the current message 

***/

#include "headers.h"
#include "s_elm.h"
#include <errno.h>

/** Note that this routine generates automatic header information
    for the subject and (obviously) to lines, but that these can
    be altered while in the editor composing the reply message! 
**/

char *strip_parens(), *get_token();

extern int errno;

char *error_description();


/* Determine the subject to use for a reply.  */
void
get_reply_subj(out_subj,in_subj,dflt_subj)
char *out_subj;		/* store the resulting subject here		*/
char *in_subj;		/* subject of the original message		*/
char *dflt_subj;	/* default to use if "in_subj" is empty		*/
{
	if ( *in_subj == '\0' ) {
	  strcpy(out_subj,dflt_subj);
	  return;
	}
	if (
	  ( in_subj[0] == 'r' || in_subj[0] == 'R' ) &&
	  ( in_subj[1] == 'e' || in_subj[1] == 'E' ) &&
	  ( in_subj[2] == ':' )
	) {
	  for ( in_subj += 3 ; whitespace(*in_subj) ; ++in_subj ) ;
	}
	strcat( strcpy( out_subj, "Re: " ), in_subj);
}

int
optimize_and_add(new_address, full_address)
char *new_address, *full_address;
{
	/** This routine will add the new address to the list of addresses
	    in the full address buffer IFF it doesn't already occur.  It
	    will also try to fix dumb hops if possible, specifically hops
	    of the form ...a!b...!a... and hops of the form a@b@b etc 
	**/

	register int len, host_count = 0, i;
	char     hosts[MAX_HOPS][SLEN];	/* array of machine names */
	char     *host, *addrptr;

	if (in_list(full_address, new_address))
	  return(1);	/* duplicate address */

	/** optimize **/
	/*  break down into a list of machine names, checking as we go along */
	
	addrptr = (char *) new_address;

	while ((host = get_token(addrptr, "!", 1)) != NULL) {
	  for (i = 0; i < host_count && ! equal(hosts[i], host); i++)
	      ;

	  if (i == host_count) {
	    strcpy(hosts[host_count++], host);
	    if (host_count == MAX_HOPS) {
	       dprint(2, (debugfile,
              "Error: hit max_hops limit trying to build return address (%s)\n",
		      "optimize_and_add"));
	       error(catgets(elm_msg_cat, ElmSet, ElmBuildRAHitMaxHops,
		"Can't build return address. Hit MAX_HOPS limit!"));
	       return(1);
	    }
	  }
	  else 
	    host_count = i + 1;
	  addrptr = NULL;
	}

	/** fix the ARPA addresses, if needed **/
	
	if (qchloc(hosts[host_count-1], '@') > -1)
	  fix_arpa_address(hosts[host_count-1]);
	  
	/** rebuild the address.. **/

	new_address[0] = '\0';

	for (i = 0; i < host_count; i++)
	  sprintf(new_address, "%s%s%s", new_address, 
	          new_address[0] == '\0'? "" : "!",
	          hosts[i]);

	if (full_address[0] == '\0')
	  strcpy(full_address, new_address);
	else {
	  len = strlen(full_address);
	  full_address[len  ] = ',';
	  full_address[len+1] = ' ';
	  full_address[len+2] = '\0';
	  strcat(full_address, new_address);
	}

	return(0);
}

void
get_and_expand_everyone(return_address, full_address)
char *return_address, *full_address;
{
	/** Read the current message, extracting addresses from the 'To:'
	    and 'Cc:' lines.   As each address is taken, ensure that it
	    isn't to the author of the message NOR to us.  If neither,
	    prepend with current return address and append to the 
	    'full_address' string.
	**/

    char ret_address[SLEN], buf[VERY_LONG_STRING], new_address[SLEN],
	 address[SLEN], comment[SLEN], next_line[SLEN];
    int  lines, iindex, line_pending = 0, line_len, err;

    /** First off, get to the first line of the message desired **/

    if (fseek(mailfile, headers[current-1]->offset, 0) == -1) {
	err = errno;
	dprint(1,(debugfile,"Error: seek %ld resulted in errno %s (%s)\n", 
		 headers[current-1]->offset, error_description(err), 
		 "get_and_expand_everyone"));
	error2(catgets(elm_msg_cat, ElmSet, ElmSeekFailedFile,
		"ELM [seek] couldn't read %d bytes into file (%s)."),
		headers[current-1]->offset, error_description(err));
	return;
    }
 
    /** okay!  Now we're there!  **/

    /** let's fix the ret_address to reflect the return address of this
	message with '%s' instead of the persons login name... **/

    translate_return(return_address, ret_address);

    /** now let's parse the actual message! **/

    for (lines = headers[current-1]->lines;;) {

      /* read in another line if required - break out if end of mbox reached */
      if ( !line_pending && (line_len = mail_gets(buf, SLEN, mailfile)) == 0 )
	  return;
      line_pending = 0;

      /* break out at end of header or start of next message */
      if ( line_len < 2 )
	return;
      if (buf[line_len - 1] == '\n')
	lines--;
      if (lines <= 0)
	return;

      /* we only want lines with addresses */
      if (!header_cmp(buf, "To", NULL) && !header_cmp(buf, "cc", NULL))
	continue;

      /* extract the addresses from this line and possible continuation lines */
      next_line[0] = '\0';

      /* read in another line - continuation lines start with whitespace */
      while ( (line_len = mail_gets(next_line, SLEN, mailfile)) != 0 &&
	      whitespace(next_line[0]) ) {
	no_ret(buf);
	strcat(buf, next_line); /* Append continuation line */

	if (next_line[line_len - 1] == '\n')
	  lines--;
	next_line[0] = '\0';
      }

      no_ret(buf);
      iindex = chloc(buf, ':')+1;		/* point beyond header name */
      dprint(2,(debugfile,"> %s\n",buf));

      /* go through all addresses in this line */
      while (break_down_tolist(buf, &iindex, address, comment)) {
	if (okay_address(address, return_address)) {

	  /**
	      Some mailers can emit unqualified addresses in the
	      headers, e.g. a Cc to a local user might appear as
	      just "user" and not "user@dom.ain".  We do a real
	      low-rent check here.  If it looks like a domain
	      address then we will pass it through.  Otherwise we
	      send it back through the originating host for routing.
	  **/
	  if (qchloc(address, '@') >= 0)
	    strcpy(new_address, address);
	  else
	    sprintf(new_address, ret_address, address);
	  optimize_and_add(new_address, full_address);

	}
      }
      if (next_line[0] != '\0') {
	strcpy(buf, next_line);
	line_pending++;
      }
    }
}

int
reply()
{
	/** Reply to the current message.  Returns non-zero iff
	    the screen has to be rewritten. **/

	char return_address[SLEN], subject[SLEN];
	int  return_value, form_letter;

	form_letter = (headers[current-1]->status & FORM_LETTER);

	if (get_return(return_address, current-1)) {
	  strcpy(subject, headers[current-1]->subject);
	} else {
	  get_reply_subj( subject, headers[current-1]->subject,
		  ( form_letter ?
		  catgets(elm_msg_cat, ElmSet, ElmFilledInForm, "Filled in form") :
		  catgets(elm_msg_cat, ElmSet, ElmReYourMail, "Re: your mail") ) );
	}
	if (form_letter)
	  return_value = mail_filled_in_form(return_address, subject);
	else
	  return_value = send_msg(return_address, "", subject, TRUE, NO, TRUE);
	return(return_value);
}

int
reply_to_everyone()
{
	/** Reply to everyone who received the current message.  
	    This includes other people in the 'To:' line and people
	    in the 'Cc:' line too.  Returns non-zero iff the screen 
            has to be rewritten. **/

	char return_address[SLEN], subject[SLEN];
	char full_address[VERY_LONG_STRING];
	int  return_value;

	get_return(return_address, current-1);

	full_address[0] = '\0';			/* no copies yet    */
	get_and_expand_everyone(return_address, full_address);
	dprint(2,(debugfile,
		"reply_to_everyone() - return_addr=\"%s\" full_addr=\"%s\"\n",
		return_address,full_address));

	get_reply_subj( subject, headers[current-1]->subject,
		  catgets(elm_msg_cat, ElmSet, ElmReYourMail, "Re: your mail"));

        return_value = send_msg(return_address, full_address, subject, 
		TRUE, NO, TRUE);
	return(return_value);

}

int
forward()
{
	/** Forward the current message.  What this actually does is
	    to temporarily set forwarding to true, then call 'send' to
	    get the address and route the mail.   Modified to also set
	    'noheader' to FALSE also, so that the original headers
	    of the message sent are included in the message body also.
	    Return TRUE if the main part of the screen has been changed
	    (useful for knowing whether a redraw is needed.
	**/

	char subject[SLEN], address[VERY_LONG_STRING];
	int  results, edit_msg = FALSE;

	forwarding = TRUE;

	address[0] = '\0';

	if (headers[current-1]->status & FORM_LETTER)
	  PutLine0(LINES-3,COLUMNS-40, catgets(elm_msg_cat, ElmSet, ElmNoEditingAllowed,
		"<No editing allowed.>"));
	else {
	  MCsprintf(subject, catgets(elm_msg_cat, ElmSet, ElmEditOutgoingMessage,
		  "Edit outgoing message? (%c/%c) "), *def_ans_yes, *def_ans_no);
	  edit_msg = (want_to(subject,
			      *def_ans_yes, LINES-3, 0) != *def_ans_no);
	}

	if (strlen(headers[current-1]->subject) > 0) {

	  strcpy(subject, headers[current-1]->subject); 

	  /* this next strange compare is to see if the last few chars are
	     already '(fwd)' before we tack another on */

	  if (strlen(subject) < 6 || (strcmp((char *) subject+strlen(subject)-5,
					     "(fwd)") != 0))
	    strcat(subject, " (fwd)");

	  results = send_msg(address, "", subject, edit_msg,
	    headers[current-1]->status & FORM_LETTER? 
	    PREFORMATTED : allow_forms, FALSE);
	}
	else
	  results = send_msg(address, "",
		catgets(elm_msg_cat, ElmSet, ElmForwardedMail, "Forwarded mail..."), edit_msg,
		headers[current-1]->status & FORM_LETTER ? PREFORMATTED : allow_forms, FALSE);
	
	forwarding = FALSE;

	return(results);
}

int
get_return_name(address, name, trans_to_lowercase)
char *address, *name;
int   trans_to_lowercase;
{
	/** Given the address (either a single address or a combined list 
	    of addresses) extract the login name of the first person on
	    the list and return it as 'name'.  Modified to stop at
	    any non-alphanumeric character. **/

	/** An important note to remember is that it isn't vital that this
	    always returns just the login name, but rather that it always
	    returns the SAME name.  If the persons' login happens to be,
	    for example, joe.richards, then it's arguable if the name 
	    should be joe, or the full login.  It's really immaterial, as
	    indicated before, so long as we ALWAYS return the same name! **/

	/** Another note: modified to return the argument as all lowercase
	    always, unless trans_to_lowercase is FALSE... **/

	/**
	 *  Yet another note: Modified to return a reasonable name
	 *  even when double quoted addresses and DecNet addresses
	 *  are embedded in a domain style address.
	 **/

	char single_address[SLEN], *sa;
	register int	i, loc, iindex = 0,
			end, first = 0;
	register char	*c;

	dprint(6, (debugfile,"get_return_name called with (%s, <>, shift=%s)\n",
		   address, onoff(trans_to_lowercase)));

	/* First step - copy address up to a comma, space, or EOLN */

	for (sa = single_address; *address; ) {
	    i = len_next_part(address);
	    if (i > 1) {
		while (--i >= 0)
		    *sa++ = *address++;
	    } else if (*address == ',' || whitespace(*address))
		break;
	    else
		*sa++ = *address++;
	}
	*sa = '\0';

	/* Now is it an Internet address?? */

	if ((loc = qchloc(single_address, '@')) != -1) {	  /* Yes */

	    /*
	     *	Is it a double quoted address?
	     */

	    if (single_address[0] == '"') {
		first = 1;
		/*
		 *  Notice `end' will really be the index of
		 *  the last char in a double quoted address.
		 */
		loc = ((end = chloc (&single_address[1], '"')) == -1)
		    ? loc
		    : end;
	    }
	    else {
		first = 0;
	    }

	    /*
	     *	Hope it is not one of those weird X.400
	     *	addresses formatted like
	     *	/G=Jukka/S=Ukkonen/O=CSC/@fumail.fi
	     */

	    if (single_address[first] == '/') {
		/* OK then, let's assume it is one of them. */

		iindex = 0;
		
		if ((c = strstr (&single_address[first], "/G"))
		    || (c = strstr (&single_address[first], "/g"))) {

		    for (c += 2; *c && (*c++ != '='); );
		    for ( ;*c && (*c != '/'); c++) {
			name[iindex++] = trans_to_lowercase
					? tolower (*c) : *c;
		    }
		    if (iindex > 0) {
			name[iindex++] = '.';
		    }
		}
		if ((c = strstr (&single_address[first], "/S"))
		    || (c = strstr (&single_address[first], "/s"))) {

		    for (c += 2; *c && (*c++ != '='); );
		    for ( ;*c && (*c != '/'); c++) {
			name[iindex++] = trans_to_lowercase
					? tolower (*c) : *c;
		    }
		}
		name[iindex] = '\0';

		for (c = name; *c; c++) {
		    *c = ((*c == '.') || (*c == '-') || isalnum (*c))
			? *c : '_';
		}

		if (iindex == 0) {
		    strcpy (name, "X.400.John.Doe");
		}
		return 0;
	    }

	    /*
	     *	Is it an embedded DecNet address?
	     */

	    while (c = strstr (&single_address[first], "::")) {
		first = c - single_address + 2;
	    }
		    

	    /*
	     *	At this point the algorithm is to keep shifting our
	     *	copy window left until we hit a '!'.  The login name
	     *	is then located between the '!' and the first meta-
	     *	character to it's right (ie '%', ':', '/' or '@').
	     */

	    for (i=loc; single_address[i] != '!' && i > first-1; i--)
		if (single_address[i] == '%' || 
		    single_address[i] == ':' ||
		    single_address[i] == '/' ||
		    single_address[i] == '@') loc = i-1;
	
	    if (i < first || single_address[i] == '!') i++;

	    for (iindex = 0; iindex < loc - i + 1; iindex++)
		if (trans_to_lowercase)
		    name[iindex] = tolower(single_address[iindex+i]);
		else
		    name[iindex] = single_address[iindex+i];
	    name[iindex] = '\0';

	}
	else {	/* easier - standard USENET address */

	    /*
	     *	This really is easier - we just cruise left from
	     *	the end of the string until we hit either a '!'
	     *	or the beginning of the line.  No sweat.
	     */

	    loc = strlen(single_address)-1; 	/* last char */

	    for (i = loc; i > -1 && single_address[i] != '!'
		 && single_address[i] != '.'; i--) {
		if (trans_to_lowercase)
		    name[iindex++] = tolower(single_address[i]);
		else
		    name[iindex++] = single_address[i];
	    }
	    name[iindex] = '\0';
	    reverse(name);
	}
	return 0;
}

int
break_down_tolist(buf, iindex, address, comment)
char *buf, *address, *comment;
int  *iindex;
{
	/** This routine steps through "buf" and extracts a single address
	    entry.  This entry can be of any of the following forms;

		address (name)
		name <address>
		address
	
	    Once it's extracted a single entry, it will then return it as
	    two tokens, with 'name' (e.g. comment) surrounded by parens.
	    Returns ZERO if done with the string...
	**/

	char buffer[LONG_STRING];
	register int i, loc = 0, hold_index, len;

	if (*iindex > strlen(buf)) return(FALSE);

	while (whitespace(buf[*iindex])) (*iindex)++;

	if (*iindex > strlen(buf)) return(FALSE);

	/** Now we're pointing at the first character of the token! **/

	hold_index = *iindex;

	if (buf[*iindex] == '"') {	/* A quoted string */
	  buffer[loc++] = buf[(*iindex)++];
	  while (buf[*iindex] != '"' && buf[*iindex] != '\0') {
	    if (buf[*iindex] == '\\' && buf[(*iindex)+1] != '\0')
	      buffer[loc++] = buf[(*iindex)++];	/* Copy backslash */
	    buffer[loc++] = buf[(*iindex)++];
	  }
	
	  if (buf[*iindex] == '"')
	    buffer[loc++] = buf[(*iindex)++]; /* Copy final " */
	}

	/*
	 * Previously, we just went looking for a "," to seperate the
	 * addresses.  This meant that addresses like:
	 *
	 *	joe@acme.com (LastName, Firstname)
	 *
	 * got split right down the middle.  The following was changed
	 * to step through the address in quanta of RFC-822 tokens.
	 * That fixes the bug, but this routine is still incurably ugly.
	 */
	i = *iindex;
	while (buf[i] != ',' && buf[i] != '\0') {
		len = rfc822_toklen(buf+i);
		strncpy(buffer+loc, buf+i, len);
		loc += len;
		i += len;
	}
	*iindex = i + (buf[i] != '\0' ? 1 : 0);
	buffer[loc] = '\0';

	while (whitespace(buffer[loc])) 	/* remove trailing whitespace */
	  buffer[--loc] = '\0';

	if (strlen(buffer) == 0) return(FALSE);

	dprint(5, (debugfile, "\n* got \"%s\"\n", buffer));

	if (buffer[loc-1] == ')') {	/*   address (name)  format */
	  for (loc = 0, len = strlen(buffer);buffer[loc] != '(' && loc < len; loc++)
		/* get to the opening comment character... */ ;

	  loc--;	/* back up to just before the paren */
	  while (whitespace(buffer[loc])) loc--;	/* back up */

	  /** get the address field... **/

	  for (i=0; i <= loc; i++)
	    address[i] = buffer[i];
	  address[i] = '\0';

	  /** now get the comment field, en toto! **/

	  loc = 0;

	  for (i = chloc(buffer, '('), len = strlen(buffer); i < len; i++)
	    comment[loc++] = buffer[i];
	  comment[loc] = '\0';
	}
	else if (buffer[loc-1] == '>') {	/*   name <address>  format */
	  dprint(7, (debugfile, "\tcomment <address>\n"));
	  for (loc = 0, len = strlen(buffer);buffer[loc] != '<' && loc < len; loc++)
		/* get to the opening comment character... */ ;
	  while (whitespace(buffer[loc])) loc--;	/* back up */
	  if (loc >= 0 && !whitespace(buffer[loc])) loc++; /* And fwd again! */

	  /** get the comment field... **/

	  comment[0] = '(';
	  for (i=1; i <= loc; i++)
	    comment[i] = buffer[i-1];
	  comment[i++] = ')';
	  comment[i] = '\0';

	  /** now get the address field, en toto! **/

	  loc = 0;

	  for (i = chloc(buffer,'<') + 1, len = strlen(buffer); i < len - 1; i++)
	    address[loc++] = buffer[i];
	
	  address[loc] = '\0';
	}
	else {
	  /** the next section is added so that all To: lines have commas
	      in them accordingly **/

	  for (i=0; buffer[i] != '\0'; i++)
	    if (whitespace(buffer[i])) break;
	  if (i < strlen(buffer)) {	/* shouldn't be whitespace */
	    buffer[i] = '\0';
	    *iindex = hold_index + strlen(buffer) + 1;
	  }
	  strcpy(address, buffer);
	  comment[0] = '\0';
	}

	dprint(5, (debugfile, "-- returning '%s' '%s'\n", address, comment));

	return(TRUE);
}
