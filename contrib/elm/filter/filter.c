
static char rcsid[] ="@(#)Id: filter.c,v 5.6 1993/08/03 19:28:39 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.2 $   $State: Exp $
 *
 * 			Copyright (c) 1988-1992 USENET Community Trust
 * 			Copyright (c) 1986,1987 Dave Taylor
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein - elm@DSI.COM
 *			dsinc!elm
 *
 *******************************************************************************
 * $Log: filter.c,v $
 * Revision 1.2  1994/01/14  00:51:33  cp
 * 2.4.23
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
 * Revision 5.5  1993/06/06  17:58:20  syd
 * make white space skipping work for blank or tab
 *
 * Revision 5.4  1993/01/27  19:40:01  syd
 * I implemented a change to filter's default verbose message format
 * including %x %X style date and time along with username
 * From: mark@drd.com (Mark Lawrence)
 *
 * Revision 5.3  1992/11/15  01:40:43  syd
 * Add regexp processing to filter.
 * Add execc operator
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.2  1992/11/07  16:20:56  syd
 * The first is that when doing a summary, macros are expanded when printing the
 * rule. IMHO they should be printed as with the -r option (i.e. %t is
 * printed as "<time>" and so on).
 *
 * The second one is that the summary printed "applied n time" regardless of
 * the value of n, not "applied n times" when n > 1.
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.1  1992/10/03  22:18:09  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/


/** This program is used as a filter within the users ``.forward'' file
    and allows intelligent preprocessing of mail at the point between
    when it shows up on the machine and when it is actually put in the
    mailbox.

    The program allows selection based on who the message is FROM, TO, or
    what the subject is.  Acceptable actions are for the program to DELETE_MSG
    the message, SAVE the message in a specified folder, FORWARD the message
    to a specified user, SAVE the message in a folder, but add a copy to the
    users mailbox anyway, or simply add the message to the incoming mail.

    Filter also keeps a log of what it does as it goes along, and at the
    end of each `quantum' mails a summary of actions, if any, to the user.

    Uses the files: $HOME/.elm/filter for instructions to this program, and
    $HOME/.elm/filterlog for a list of what has been done since last summary.

**/

#include <stdio.h>
#include <pwd.h>
#include "defs.h"
#ifdef I_TIME
#  include <time.h>
#endif
#ifdef I_SYSTIME
#  include <sys/time.h>
#endif
#include <fcntl.h>

#define  MAIN_ROUTINE			/* for the filter.h file, of course! */
#include "filter.h"
#include "s_filter.h"

extern char *date_n_user();

main(argc, argv)
int argc;
char *argv[];
{
	extern char *optarg;
	FILE   *fd;				/* for output to temp file! */
	struct passwd  *passwd_entry;
#ifndef	_POSIX_SOURCE
	struct passwd  *getpwuid();		/* for /etc/passwd          */
#endif
	char filename[SLEN],			/* name of the temp file    */
	     action_argument[SLEN], 		/* action arg, per rule    */
	     buffer[MAX_LINE_LEN];		/* input buffer space       */
	int  in_header = TRUE,			/* for header parsing       */
	     in_to     = FALSE,			/* are we on 'n' line To: ? */
	     summary   = FALSE,			/* a summary is requested?  */
	     c;					/* var for getopt routine   */

#ifdef I_LOCALE
        setlocale(LC_ALL, "");
#endif

        elm_msg_cat = catopen("elm2.4", 0);

		
	/* first off, let's get the info from /etc/passwd */ 
	
	if ((passwd_entry = getpwuid(getuid())) == NULL) 
	  leave(catgets(elm_msg_cat,FilterSet,FilterCantGetPasswdEntry,
		"Cannot get password entry for this uid!"));

	strcpy(home, passwd_entry->pw_dir);
	strcpy(username, passwd_entry->pw_name);
	user_uid = passwd_entry->pw_uid;
	user_gid = passwd_entry->pw_gid;
	
	/* nothing read in yet, right? */
	outfname[0] = to[0] = filterfile[0] = '\0';
	
	
#ifdef HOSTCOMPILED
	strncpy(hostname, HOSTNAME, sizeof(hostname));
#else
	gethostname(hostname, sizeof(hostname));
#endif

	/* now parse the starting arguments... */

	while ((c = getopt(argc, argv, "clno:qrSsvf:")) != EOF)
	{
	      switch (c) {
	       case 'c' :
		    clear_logs = TRUE;
		    break;
	       case 'f' :
		    strcpy(filterfile,optarg);
		    break;
	       case 'l' :
		    log_actions_only = TRUE;
		    break;
	       case 'n' :
		    show_only = TRUE;
		    break;
	       case 'o' :
		    strcpy(outfname, optarg);
		    break;
	       case 'q' :
		    logging = FALSE;
		    break;
	       case 'r' :
		    printing_rules = TRUE;
		    break;
	       case 's' :
		    summary = TRUE;
		    break;
	       case 'S' :
		    long_summary = TRUE;
		    break;
	       case 'v' :
		    verbose = TRUE;
		    break;
	       case '?' :
 	       default  :
		    fprintf(stderr, 
			       catgets(elm_msg_cat,FilterSet,FilterUsage,
  "Usage: | filter [-nrvlq] [-f rules] [-o file]\n\
\tor: filter [-c] -[s|S]\n"));
          	       exit(1);
	  }
	}

	if (c < 0) {
	}

	/* use default filter file name if none specified */
	if (!*filterfile)
	     sprintf(filterfile,"%s/%s",home,FILTERFILE);
	
	
	/* let's open our outfd logfile as needed... */

	if (outfname[0] == '\0') 	/* default is stdout */
	  outfd = stdout;
	else 
	  if ((outfd = fopen(outfname, "a")) == NULL) {
	    if (isatty(fileno(stderr)))
	      fprintf(stderr,
		      catgets(elm_msg_cat,FilterSet,FilterCouldntOpenLogFile,
	      "filter (%s): couldn't open log file %s\n"),
		      username, outfname);
	  }

	if (summary || long_summary) {
          if (get_filter_rules() == -1) {
	    if (outfd != NULL) fclose(outfd);
	    exit(1);
	  }
	  printing_rules = TRUE;
	  show_summary();
	  if (outfd != NULL) fclose(outfd);
	  exit(0);
	}

	if (printing_rules) {
          if (get_filter_rules() == -1)
	    fprintf(outfd,catgets(elm_msg_cat,FilterSet,FilterCouldntGetRules,
		  "filter (%s): Couldn't get rules!\n"), date_n_user());
          else
	    print_rules();
	  if (outfd != NULL) fclose(outfd);
          exit(0);
	}

	/* next, create the tempfile and save the incoming message */

	sprintf(filename, "%s.%d", filter_temp, getpid());

	if ((fd = fopen(filename,"w")) == NULL)
	  {
		sprintf(buffer,
			catgets(elm_msg_cat,FilterSet,
				FilterCantOpenTempFileLeave,
			"Cannot open temporary file %s"),
				filename);
		leave(buffer);
	  }
	

	while (fgets(buffer, MAX_LINE_LEN, stdin) != NULL) {

	  remove_return(buffer);

	  if (in_header) {

	    if (! whitespace(buffer[0])) 
		in_to = FALSE;

	    if (the_same(buffer, "From ")) 
	      save_from(buffer);
	    else if (the_same(buffer, "Subject:")) 
	      save_subject(buffer);
	    else if (the_same(buffer, "Sender:")) 
	      save_sender(buffer);
	    else if (the_same(buffer, "To:") || the_same(buffer, "Cc:") ||
		     the_same(buffer, "Apparently-To:")) {
	      in_to++;
	      save_to(buffer);
	    }
	    else if (the_same(buffer, "X-Filtered-By:")) 
	      already_been_forwarded++;	/* could be a loop here! */
#ifdef USE_EMBEDDED_ADDRESSES
	    else if (the_same(buffer, "From:"))
	      save_embedded_address(buffer, "From:");
	    else if (the_same(buffer, "Reply-To:"))
	      save_embedded_address(buffer, "Reply-To:");
#endif
	    else if (strlen(buffer) < 2) 
	      in_header = 0;
	    else if (whitespace(buffer[0]) && in_to)
	      strcat(to, buffer);
	  }
	
          fprintf(fd, "%s\n", buffer);	/* and save it regardless! */
	  fflush(fd);
	  lines++;
	}

	fclose(fd);

	/** next let's see if the user HAS a filter file, and if so what's in
            it (and so on) **/

	if (get_filter_rules() == -1)
	  mail_message(username);
	else {
	  int action = action_from_ruleset();

	  if (rule_choosen >= 0) {
	    expand_macros(rules[rule_choosen].argument2, action_argument,
			rules[rule_choosen].line, printing_rules);
	    /* Got to do this because log_msg() uses argument2 in rules[] */
	    strcpy(rules[rule_choosen].argument2, action_argument);
	  }

	  switch (action) {

	    case DELETE_MSG : if (verbose && outfd != NULL)
			    fprintf(outfd,
				    catgets(elm_msg_cat,FilterSet,
					    FilterMessageDeleted,
					    "filter (%s): Message deleted\n"),
				    date_n_user());
			  log_msg(DELETE_MSG);				break;

	    case SAVE   : if (save_message(rules[rule_choosen].argument2)) {
			    mail_message(username);
			    log_msg(FAILED_SAVE);
			  }
			  else
		 	    log_msg(SAVE);
	                  break;

	    case SAVECC : if (save_message(rules[rule_choosen].argument2))
			    log_msg(FAILED_SAVE);
			  else
		            log_msg(SAVECC);
			  mail_message(username);
	                  break;

	    case FORWARDC:mail_message(username);
	    		  mail_message(rules[rule_choosen].argument2);
			  log_msg(FORWARDC);
	                  break;
	    case FORWARD: mail_message(rules[rule_choosen].argument2);
			  log_msg(FORWARD);
	                  break;

	    case EXECC	: mail_message(username);
	    		  execute(rules[rule_choosen].argument2);
			  log_msg(EXECC);
	                  break;

	    case EXEC   : execute(rules[rule_choosen].argument2);
			  log_msg(EXEC);
	                  break;

	    case LEAVE  : mail_message(username);
			  log_msg(LEAVE);
	                  break;
	  }
	}

	(void) unlink(filename);	/* remove the temp file, please! */
	if (outfd != NULL)
	  fclose(outfd);
	exit(0);
}

save_from(buffer)
char *buffer;
{
	/** save the SECOND word of this string as FROM **/

	register char *f = from;

	while (!whitespace(*buffer))
	  buffer++;				/* get to word     */

	while (whitespace(*buffer))
	  buffer++;				/* skip delimited  */

	for (; (!whitespace(*buffer)) && *buffer; buffer++, f++) 
	  *f = *buffer;				/* copy it and     */

	*f = '\0';				/* Null terminate! */
}

save_subject(buffer)
char *buffer;
{
	/** save all but the word "Subject:" for the subject **/

	register int skip = 8;  /* skip "Subject:" initially */

	while (whitespace(buffer[skip]))
		skip++;

	strcpy(subject, (char *) buffer + skip);
}

save_sender(buffer)
char *buffer;
{
	/** save all but the word "Sender:" for the sender **/

	register int skip = 7;  /* skip "Sender:" initially */

	while (whitespace(buffer[skip]))
		skip++;

	strcpy(sender, (char *) buffer + skip);
}

save_to(buffer)
char *buffer;
{
	/** save all but the word "To:" or "Cc:" or
	    "Apparently-To:" for the to list **/

	register int skip = 0;

	while (!whitespace(buffer[skip]))
		skip++;

	while (whitespace(buffer[skip]))
		skip++;

	if (*to)
		strcat(to, " "); /* place one blank between items */

	strcat(to, (char *) buffer + skip);
}

#ifdef USE_EMBEDDED_ADDRESSES

save_embedded_address(buffer, fieldname)
char *buffer, *fieldname;
{
	/** this will replace the 'from' address with the one given, 
	    unless the address is from a 'reply-to' field (which overrides 
	    the From: field).  The buffer given to this routine can have one 
            of three forms:
		fieldname: username <address>
		fieldname: address (username)
		fieldname: address
	**/
	
	static int processed_a_reply_to = 0;
	char address[LONG_STRING];
	register int i, j = 0;

	/** first let's extract the address from this line.. **/

	if (buffer[strlen(buffer)-1] == '>') {	/* case #1 */
	  for (i=strlen(buffer)-1; buffer[i] != '<' && i > 0; i--)
		/* nothing - just move backwards .. */ ;
	  i++;	/* skip the leading '<' symbol */
	  while (buffer[i] != '>')
	    address[j++] = buffer[i++];
	  address[j] = '\0';
	}
	else {	/* get past "from:" and copy until white space or paren hit */
	  for (i=strlen(fieldname); whitespace(buffer[i]); i++)
	     /* skip past that... */ ;
	  while (buffer[i] != '(' && ! whitespace(buffer[i]) && buffer[i]!='\0')
	    address[j++] = buffer[i++];
	  address[j] = '\0';
	}

	/** now let's see if we should overwrite the existing from address
	    with this one or not.. **/

	if (processed_a_reply_to)
	  return;	/* forget it! */

	strcpy(from, address);			/* replaced!! */

	if (istrcmp(fieldname, "Reply-To:") == 0)
	  processed_a_reply_to++;
}
#endif

