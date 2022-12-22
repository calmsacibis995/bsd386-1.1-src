
static char rcsid[] ="@(#)Id: actions.c,v 5.8 1993/08/03 19:28:39 syd Exp ";

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
 * $Log: actions.c,v $
 * Revision 1.2  1994/01/14  00:51:31  cp
 * 2.4.23
 *
 * Revision 5.8  1993/08/03  19:28:39  syd
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
 * Revision 5.7  1993/08/03  19:07:58  syd
 * Removed bogus string lockfile.
 * From: Jan.Djarv@sa.erisoft.se (Jan Djarv)
 *
 * Revision 5.6  1993/04/21  01:25:45  syd
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
 * Revision 5.5  1993/01/27  19:40:01  syd
 * I implemented a change to filter's default verbose message format
 * including %x %X style date and time along with username
 * From: mark@drd.com (Mark Lawrence)
 *
 * Revision 5.4  1993/01/20  03:37:16  syd
 * Nits and typos in the NLS messages and corresponding default messages.
 * From: dwolfe@pffft.sps.mot.com (Dave Wolfe)
 *
 * Revision 5.3  1992/12/24  19:22:05  syd
 * Quote from the filter of phrase to prevent RFC-822 parsing problems
 * From: Syd via request from Ian Stewartson <istewart@dlvax2.datlog.co.uk>
 *
 * Revision 5.2  1992/12/11  02:16:08  syd
 * remove unreachable return(0) at end of function
 * From: Syd
 *
 * Revision 5.1  1992/10/03  22:18:09  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/


/** RESULT oriented routines *chuckle*.  These routines implement the
    actions that result from either a specified rule being true or from
    the default action being taken.
**/

#include <stdio.h>
#include <pwd.h>
#include <fcntl.h>

#include "defs.h"
#include "filter.h"
#include "s_filter.h"

FILE *emergency_local_delivery();
extern char *date_n_user();

mail_message(address)
char *address;
{
      /* Called with an address to send mail to.   For various reasons
       * that are too disgusting to go into herein, we're going to actually
       * open the users mailbox and by hand add this message.  Yech.
       * NOTE, of course, that if we're going to MAIL the message to someone
       * else, that we'll try to do nice things with it on the fly...
       */

	FILE *pipefd, *tempfd, *mailfd;
	int  in_header = TRUE, line_count = 0, mailunit, pid, statusp;
	char tempfile[SLEN], mailbox[SLEN],
	     buffer[VERY_LONG_STRING], *cp;

	if (verbose && ! log_actions_only && outfd != NULL)
	  fprintf(outfd, catgets(elm_msg_cat,FilterSet,FilterMailingMessage,
				 "filter (%s): Mailing message to %s\n"), 
		  date_n_user(), address);

	if (! show_only) {
	  sprintf(tempfile, "%s.%d", filter_temp, getpid());

	  if ((tempfd = fopen(tempfile, "r")) == NULL) {
	    if (outfd != NULL)
	      fprintf(outfd, catgets(elm_msg_cat,FilterSet,
				     FilterCantOpenTempFile2,
              "filter (%s): Can't open temp file %s!!\n"), 
		    date_n_user(), tempfile);
	    if (outfd != NULL) fclose(outfd);
	    exit(1);
	  }
	 	
	  if (strcmp(address, username) != 0) {	/* mailing to someone else */
	    
	    if (already_been_forwarded) {	/* potential looping! */
	      if (contains(from, username)) {
		if (outfd != NULL)
	          fprintf(outfd,
			  catgets(elm_msg_cat,FilterSet,FilterLoopDetected, 
	"filter (%s): Filter loop detected!  Message left in file %s.%d\n"), 
			date_n_user(), filter_temp, getpid());
	        if (outfd != NULL) fclose(outfd);
	        exit(0);
	      }
	    }

	    if (strcmp(sendmail, mailer) == 0)
	      sprintf(buffer, "%s %s %s", sendmail, smflags, address);
	    else
	      sprintf(buffer, "%s %s", mailer, address);

	    /*
	     * we do a fork/exec here to prevent evil people
	     * from abusing the setgid privileges of filter.
	     */
	    
	    if ( (pid=fork()) > 0 ) /* we're the parent */
	      {
		    wait(&statusp);
		    if (statusp!=0)
		      {
			    fprintf(outfd,
				    catgets(elm_msg_cat,
					    FilterSet,FilterBadWaitStat,
		    "filter (%s): Command \"%s\" exited with value %d\n"),
				    date_n_user(),buffer,statusp);
		      }
	      }
	    else if (pid==0) /* we're the child */
	      {
		    /* use safe permissions */
		    
		    setuid(user_uid);
		    setgid(user_gid);
		    
		    if ((pipefd = popen(buffer, "w")) == NULL) {
			  if (outfd != NULL)
			    fprintf(outfd,catgets(elm_msg_cat,
						  FilterSet,FilterPopenFailed,
		  "filter (%s): popen %s failed!\n"), date_n_user(), buffer);
			  sprintf(buffer, "(%s %s %s < %s ; %s %s) &",
				  sendmail, smflags, address, tempfile,
				  remove_cmd, tempfile);
			  exit(system(buffer));
		    }
		    
		    fprintf(pipefd, "Subject: \"%s\"\n", subject);
		    fprintf(pipefd, catgets(elm_msg_cat,FilterSet,
					    FilterFromTheFilterOf,
				    "From: \"The Filter of %s@%s\" <%s>\n"), 
			    username, hostname, username);
		    fprintf(pipefd, "To: %s\n", address);
		    fprintf(pipefd, catgets(elm_msg_cat,
					    FilterSet,FilterXFilteredBy,
			    "X-Filtered-By: filter, version %s\n\n"),
			    VERSION);
		    
		    fprintf(pipefd, catgets(elm_msg_cat,
					    FilterSet,FilterBeginMesg,
			    "-- Begin filtered message --\n\n"));
		    
		    while (fgets(buffer, SLEN, tempfd) != NULL)
		      if (already_been_forwarded && in_header)
			in_header = (strlen(buffer) == 1? 0 : in_header);
		      else
			fprintf(pipefd," %s", buffer);
		    
		    fprintf(pipefd, catgets(elm_msg_cat,
					    FilterSet,FilterEndMesg,
			    "\n-- End of filtered message --\n"));
		    fclose(pipefd);
		    fclose(tempfd);

		    exit(0);

	      }
	    else /* fork failed */
	      {
		    fprintf(outfd,
			    catgets(elm_msg_cat,
				    FilterSet,FilterForkFailed,
		    "filter (%s): fork of command \"%s\" failed\n"),
			    date_n_user(),buffer);
	      }
	    
	    return;		/* YEAH!  Wot a slick program, eh? */
	  
	  }
	  
	  /** OTHERWISE it is to the current user... **/

	  sprintf(mailbox, "%s%s", mailhome, username);

	  if (!lock()) {
	    if (outfd != NULL) {
	      fprintf(outfd, catgets(elm_msg_cat,FilterSet,
				     FilterCouldntCreateLockFile,
		     "filter (%s): Couldn't create lock file\n"),
		    date_n_user());
	      fprintf(outfd, catgets(elm_msg_cat,FilterSet,
				     FilterCantOpenMailBox,
		     "filter (%s): Can't open mailbox %s!\n"),
		    date_n_user(), mailbox);
	    }
	    if ((mailfd = emergency_local_delivery()) == NULL)
	      exit(1);
	  }
	  else if ((mailunit = open(mailbox,
				    O_APPEND | O_WRONLY | O_CREAT, 0600)) >= 0)
	    mailfd = fdopen(mailunit, "a");
	  else if ((mailfd = emergency_local_delivery()) == NULL)
	    exit(1);

	  while (fgets(buffer, sizeof(buffer), tempfd) != NULL) {
	    line_count++;
	    if (the_same(buffer, "From ") && line_count > 1)
	      fprintf(mailfd, ">%s", buffer);
	    else
	      fputs(buffer, mailfd);
	  }

	  fputs("\n", mailfd);

	  fclose(mailfd);
	  unlock();		/* blamo or not?  Let it decide! */
	  fclose(tempfd);
	} /* end if show only */
}

save_message(foldername)
char *foldername;
{
	/** Save the message in a folder.  Use full file buffering to
	    make this work without contention problems **/

	FILE
	     *fd, *tempfd;
	int
	     filter_pid, gid, pid,
	     statusp, ret;

	/* allow for ~ file names expansion in rules */
	(void) expand_filename(foldername);

	if (verbose && outfd != NULL)
	  fprintf(outfd, catgets(elm_msg_cat,FilterSet,FilterSavedMessage,
				 "filter (%s): Message saved in folder %s\n"), 
		  date_n_user(), foldername);

	if (show_only)
	     return(0);

	gid=getgid();
	filter_pid = getpid();
	
	/*
	 * need to save filter's PID because the child process
	 * will have different PID
	 */
	
	if (gid != user_gid) /* then fork */
	{
	      
	      if ((pid=fork()) > 0) /* we're the parent */
	      {
		    wait(&statusp);
		    return(statusp);
			  
	      }
	      else if (pid == 0) /* we're the child */
	      {
		    /*
		     *  we want the file to have
		     *  the user's group id
		     */
		    
		    ret = setgid(user_gid);
		    ret = save_to_folder(foldername,filter_pid);
		    
		    exit(ret);
		    
	      }
	      else /* fork failed */
	      {
		    fprintf(outfd,
			    catgets(elm_msg_cat,
				    FilterSet,FilterForkSaveFailed,
		    "filter (%s): fork-and-save message failed\n\
\tsaving with current group ID\n"),
			    date_n_user());
		    /*
		     * save with the current group id.
		     */
		    ret = save_to_folder(foldername,filter_pid);
		    return(ret);
		    
	      }
	}
	else
	{
	      /*
	       *  ok to save with current group id.
	       */
	      ret = save_to_folder(foldername,filter_pid);
	      return(ret);
	      
	}
}

save_to_folder(foldername,filter_pid)
char
     *foldername;
int
     filter_pid;
{
      /*
       * this function does the actual work of saving the message
       * in the named folder.
       */
      
      FILE  *fd, *tempfd;
      char  filename[SLEN], buffer[SLEN];
      int   fdunit;
      
      sprintf(filename, "%s.%d", filter_temp, filter_pid);
      
      if ((fdunit = open(foldername,
			 O_APPEND | O_WRONLY | O_CREAT, 0600)) < 0) {
	    if (outfd != NULL)
		 fprintf(outfd, 
			 catgets(elm_msg_cat,
				 FilterSet,FilterCantSaveMessageToFolder,
		 "filter (%s): can't save message to requested folder %s!\n"),
			 date_n_user(), foldername);
	    return(1);
      }
      fd = fdopen(fdunit,"a");
      
      if ((tempfd = fopen(filename, "r")) == NULL) {
	    if (outfd != NULL)
		 fprintf(outfd,catgets(elm_msg_cat,FilterSet,
				       FilterCantOpenTempFile3, 
		       "filter (%s): can't open temp file %s for reading!\n"),
			 date_n_user(),filename);
	    return(1);
      }
      
      while (fgets(buffer, sizeof(buffer), tempfd) != NULL)
	   fputs(buffer, fd);
      
      /*
       * Add two newlines, to ensure that other mailers (which, unlike
       * elm, may only look for \n\nFrom_ as the start-of-message
       * indicator).
       */
      
      /** fprintf(fd, "%s", "\n\n"); **/
      fprintf(fd, "\n\n");
      
      fclose(fd);
      fclose(tempfd);
      
      return(0);
}

execute(command)
char *command;
{
	/** execute the indicated command, feeding as standard input the
	    message we have.
	**/

        int pid, statusp;
	char buffer[SLEN];

	if (verbose && outfd != NULL)
	  fprintf(outfd, catgets(elm_msg_cat,FilterSet,FilterExecutingCmd,
	       "filter (%s): Executing %s\n"), date_n_user(), command);

	if (! show_only) {
	      sprintf(buffer, "%s %s.%d | %s",
		      cat, filter_temp, getpid(), command);

	      if ( (pid=fork()) > 0) /* we're the parent */
		{
		      wait(&statusp);
		      if (statusp!=0)
			{
			      fprintf(outfd,
				      catgets(elm_msg_cat,
					      FilterSet,FilterBadWaitStat,
		      "filter (%s): Command \"%s\" exited with value %d\n"),
					      date_n_user(),command,statusp);
			}
		      
		}
	      else if (pid==0)/* we're the child */
		{
		      /*
		       * reset uid/gid to user's uid and gid
		       */
		      setgid(user_gid);
		      setuid(user_uid);
		      exit(system(buffer));
		}
	      else /* fork() failed */
		{
		      fprintf(outfd,
			      catgets(elm_msg_cat,
				      FilterSet,FilterForkFailed,
		      "filter (%s): fork of command \"%s\" failed\n"),
			      date_n_user(),command);
		}
	      
	}
}

FILE *
emergency_local_delivery()
{
	/** This is called when we can't deliver the mail to the usual
	    mailbox in the usual way ...
	**/

	FILE *tempfd;
	char  mailbox[SLEN];
	int   mailunit;

	sprintf(mailbox, "%s/%s", home, EMERGENCY_MAILBOX);

	if ((mailunit = open(mailbox, O_APPEND | O_WRONLY | O_CREAT, 0600)) < 0) {
	  if (outfd != NULL)
	    fprintf(outfd, catgets(elm_msg_cat,FilterSet,FilterCantOpenEither,
				 "filter (%s): Can't open %s either!!\n"),
		    date_n_user(), mailbox);

	  sprintf(mailbox,"%s/%s", home, EMERG_MBOX); 

	  if ((mailunit = open(mailbox, O_APPEND | O_WRONLY | O_CREAT, 0600)) < 0) {

	    if (outfd != NULL) {
		  fprintf(outfd, catgets(elm_msg_cat,FilterSet,
					 FilterCantOpenEither,
				 "filter (%s): Can't open %s either!!\n"),
		      date_n_user(), mailbox);
	      fprintf(outfd,catgets(elm_msg_cat,FilterSet,FilterCantOpenAny, 
		      "filter (%s): I can't open ANY mailboxes!  Augh!!\n"),
		       date_n_user());
	     }
	    
	    /* DIE DIE DIE DIE!! */
	     leave(catgets(elm_msg_cat,FilterSet,FilterCantOpenAnyLeave,
			   "Cannot open any mailbox"));
	   }
	   else
	     if (outfd != NULL)
	       fprintf(outfd,catgets(elm_msg_cat,FilterSet,
				     FilterUsingEmergMbox,
			"filter (%s): Using %s as emergency mailbox\n"),
		       date_n_user(), mailbox);
	  }
	  else
	    if (outfd != NULL)
	      fprintf(outfd,catgets(elm_msg_cat,FilterSet,
				     FilterUsingEmergMbox,
			"filter (%s): Using %s as emergency mailbox\n"), 
		      date_n_user(), mailbox);

	tempfd = fdopen(mailunit, "a");
	return((FILE *) tempfd);
}
