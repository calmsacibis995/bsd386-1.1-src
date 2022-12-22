
static char rcsid[] = "@(#)Id: leavembox.c,v 5.20 1993/09/27 01:51:38 syd Exp ";

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
 * $Log: leavembox.c,v $
 * Revision 1.2  1994/01/14  00:55:08  cp
 * 2.4.23
 *
 * Revision 5.20  1993/09/27  01:51:38  syd
 * Add elm_chown to consolidate for Xenix not allowing -1
 * From: Syd
 *
 * Revision 5.19  1993/08/23  03:26:24  syd
 * Try setting group id separate from user id in chown to
 * allow restricted systems to change group id of file
 * From: Syd
 *
 * Revision 5.18  1993/08/03  19:59:49  syd
 * Check for chown restricted and if so, do copyover and
 * back to avoid need for chown
 * From: Syd
 *
 * Revision 5.17  1993/05/31  19:47:45  syd
 * change is_symlink to no_restore and use it for special modes as well
 * From: Syd
 *
 * Revision 5.16  1993/05/31  19:44:46  syd
 * If 'enforcement' modes set, use copy not link
 *
 * Revision 5.15  1993/05/08  20:25:33  syd
 * Add sleepmsg to control transient message delays
 * From: Syd
 *
 * Revision 5.14  1993/04/12  02:34:36  syd
 * I have now added a parameter which controls whether want_to clears the
 * line and centers the question or behaves like it did before. I also
 * added a 0 at the end of the parameter list to all the other calls to
 * want_to where a centered question on a clean line is not desirable.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.13  1993/02/03  17:12:53  syd
 * move more declarations to defs.h, including sleep
 * From: Syd
 *
 * Revision 5.12  1993/01/27  20:43:26  syd
 * The following minor patch for leavembox.c is useful for BSD systems
 * which implement correct (per SVID & POSIX) struct utimbuf.  Where the
 * source previously tested just '#ifdef BSD' it now tests '#if
 * defined(BSD) && !defined(UTIMBUF)'.  This suppresses a compile-time
 * warning on ConvexOS due to the prototype of utime.
 * From: rwright@dhostwo.convex.com (Randy Wright)
 *
 * Revision 5.11  1993/01/20  03:08:43  syd
 * alter the message on aborts to report the temp file name to the user
 * From: The Postmaster <sysnet@central1.lancaster.ac.uk>
 *
 * Revision 5.10  1993/01/20  03:02:19  syd
 * Move string declarations to defs.h
 * From: Syd
 *
 * Revision 5.9  1992/12/24  21:58:52  syd
 * Add lstat call to properly detect symlink
 * From: Syd via partial patch from Bryan Curnutt
 *
 * Revision 5.8  1992/12/24  21:42:01  syd
 * Fix messages and nls messages to match.  Plus use want_to
 * where appropriate.
 * From: Syd, via prompting from Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.7  1992/12/11  01:45:04  syd
 * remove sys/types.h include, it is now included by defs.h
 * and this routine includes defs.h or indirectly includes defs.h
 * From: Syd
 *
 * Revision 5.6  1992/12/07  03:49:49  syd
 * use BSD or not apollo on file.h include as its not valid
 * for Apollos under sys5.3 compile type
 * From: gordonb@mcil.comm.mot.com (Gordon Berkley)
 *
 * Revision 5.5  1992/11/26  01:46:26  syd
 * add Decode option to copy_message, convert copy_message to
 * use bit or for options.
 * From: Syd and bjoerns@stud.cs.uit.no (Bjoern Stabell)
 *
 * Revision 5.4  1992/11/26  00:46:13  syd
 * changes to first change screen back (Raw off) and then issue final
 * error message.
 * From: Syd
 *
 * Revision 5.3  1992/10/27  01:52:16  syd
 * Always include <sys/ioctl.h> in curses.c When calling ioctl()
 *
 * Remove declaration of getegid() from leavembox.c & lock.c
 * They aren't even used there.
 * From: tom@osf.org
 *
 * Revision 5.2  1992/10/17  22:18:36  syd
 * Correct reversed usage of $d_utimbuf.
 * From: chip@tct.com (Chip Salzenberg)
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** leave current folder, updating etc. as needed...
  
**/

#include "headers.h"
#include "s_elm.h"
#include <sys/stat.h>
#ifdef USE_FLOCK_LOCKING
#define SYSCALL_LOCKING
#endif
#ifdef USE_FCNTL_LOCKING
#define SYSCALL_LOCKING
#endif
#ifdef SYSCALL_LOCKING
#  if (defined(BSD) || !defined(apollo))
#    include <sys/file.h>
#  endif
#endif
#include <errno.h>
#ifdef I_TIME
#  include <time.h>
#endif
#ifdef I_SYSTIME
#  include <sys/time.h>
#endif
#ifdef I_UTIME
#  include <utime.h>
#endif
#ifdef I_SYSUTIME
#  include <sys/utime.h>
#endif


/**********
   Since a number of machines don't seem to bother to define the utimbuf
   structure for some *very* obscure reason.... 

   Suprise, though, BSD has a different utime() entirely...*sigh*
**********/

#ifndef BSD
# ifndef UTIMBUF

struct utimbuf {
	time_t	actime;		/** access time       **/ 
	time_t	modtime;	/** modification time **/
       };

# endif /* UTIMBUF */
#endif /* BSD */

extern int errno;

char *error_description();

int
leave_mbox(resyncing, quitting, prompt)
int resyncing, quitting, prompt;
{
	/** Close folder, deleting some messages, storing others in mbox,
	    and keeping others, as directed by user input and elmrc options.

	    Return	1	Folder altered
			0	Folder not altered
			-1	New mail arrived during the process and
					closing was aborted.
	    If "resyncing" we are just writing out folder to reopen it. We
		therefore only consider deletes and keeps, not stores to mbox.
		Also we don't remove NEW status so that it can be preserved
		across the resync.

	    If "quitting" and "prompt" is false, then no prompting is done.
		Otherwise prompting is dependent upon the variable
		question_me, as set by an elmrc option.  This behavior makes
		the 'q' command prompt just like 'c' and '$', while
		retaining the 'Q' command for a quick exit that never
		prompts.
	**/

	FILE *temp;
	char temp_keep_file[SLEN], buffer[SLEN];
	struct stat    buf;		/* stat command  */
#ifdef SYMLINK
	struct stat    lbuf;		/* lstat command  */
#endif
#if defined(BSD) && !defined(UTIMBUF)
	time_t utime_buffer[2];		/* utime command */
#else
	struct utimbuf utime_buffer;	/* utime command */
#endif
	register int to_delete = 0, to_store = 0, to_keep = 0, i,
		     marked_deleted, marked_read, marked_unread,
		     last_sortby, ask_questions,  asked_storage_q,
		     num_chgd_status, need_to_copy, no_restore = FALSE;
	char answer;
	int  err;
	long bytes();

	dprint(1, (debugfile, "\n\n-- leaving folder --\n\n"));

	if (message_count == 0)
	  return(0);	/* nothing changed */

	ask_questions = ((quitting && !prompt) ? FALSE : question_me);

	/* YES or NO on softkeys */
	if (hp_softkeys && ask_questions) {
	  define_softkeys(YESNO);
	  softkeys_on();
	}

	/* Clear the exit dispositions of all messages, just in case
	 * they were left set by a previous call to this function
	 * that was interrupted by the receipt of new mail.
	 */
	for(i = 0; i < message_count; i++)
	  headers[i]->exit_disposition = UNSET;
	  
	/* Determine if deleted messages are really to be deleted */

	/* we need to know if there are none, or one, or more to delete */
	for (marked_deleted=0, i=0; i<message_count && marked_deleted<2; i++)
	  if (ison(headers[i]->status, DELETED))
	    marked_deleted++;

        if(marked_deleted) {
	  answer = (always_del ? *def_ans_yes : *def_ans_no);	/* default answer */
	  if(ask_questions) {
	    if (marked_deleted == 1)
	      MCsprintf(buffer, catgets(elm_msg_cat, ElmSet, ElmLeaveDeleteMessage,
		"Delete message? (%c/%c) "), *def_ans_yes, *def_ans_no);
	    else
	      MCsprintf(buffer, catgets(elm_msg_cat, ElmSet, ElmLeaveDeleteMessages,
		"Delete messages? (%c/%c) "), *def_ans_yes, *def_ans_no);
	    answer = want_to(buffer, answer, LINES-3, 0);
	  }

	  if(answer == *def_ans_yes) {
	    for (i = 0; i < message_count; i++) {
	      if (ison(headers[i]->status, DELETED)) {
		headers[i]->exit_disposition = DELETE;
		to_delete++;
	      }
	    }
	  }
	}
	dprint(3, (debugfile, "Messages to delete: %d\n", to_delete));

	/* If this is a non spool file, or if we are merely resyncing,
	 * all messages with an unset disposition (i.e. not slated for
	 * deletion) are to be kept.
	 * Otherwise, we need to determine if read and unread messages
	 * are to be stored or kept.
	 */
	if(folder_type == NON_SPOOL || resyncing) {
	  to_store = 0;
	  for (i = 0; i < message_count; i++) {
	    if(headers[i]->exit_disposition == UNSET) {
	      headers[i]->exit_disposition = KEEP;
	      to_keep++;
	    }
	  }
	} else {

	  /* Let's first see if user wants to store read messages 
	   * that aren't slated for deletion */

	  asked_storage_q = FALSE;

	  /* we need to know if there are none, or one, or more marked read */
	  for (marked_read=0, i=0; i < message_count && marked_read < 2; i++) {
	    if((isoff(headers[i]->status, UNREAD))
	      && (headers[i]->exit_disposition == UNSET))
		marked_read++;
	  }
	  if(marked_read) {
	    answer = (always_store ? *def_ans_yes : *def_ans_no);	/* default answer */
	    if(ask_questions) {
	      if (marked_read == 1)
		MCsprintf(buffer, catgets(elm_msg_cat, ElmSet, ElmLeaveMoveMessage,
			"Move read message to \"received\" folder? (%c/%c) "),
			*def_ans_yes, *def_ans_no);
	      else
		MCsprintf(buffer, catgets(elm_msg_cat, ElmSet, ElmLeaveMoveMessages,
			"Move read messages to \"received\" folder? (%c/%c) "),
			*def_ans_yes, *def_ans_no);
	      answer = want_to(buffer, answer, LINES-3, 0);
	      asked_storage_q = TRUE;
	    }

	    for (i = 0; i < message_count; i++) {
	      if((isoff(headers[i]->status, UNREAD)) 
		&& (headers[i]->exit_disposition == UNSET)) {

		  if(answer == *def_ans_yes) {
		    headers[i]->exit_disposition = STORE;
		    to_store++;
		  } else {
		    headers[i]->exit_disposition = KEEP;
		    to_keep++;
		  }
	      }
	    } 
	  }

	  /* If we asked the user if read messages should be stored,
	   * and if the user wanted them kept instead, then certainly the
	   * user would want the unread messages kept as well.
	   */
	  if(asked_storage_q && answer == *def_ans_no) {

	    for (i = 0; i < message_count; i++) {
	      if((ison(headers[i]->status, UNREAD))
		&& (headers[i]->exit_disposition == UNSET)) {
		  headers[i]->exit_disposition = KEEP;
		  to_keep++;
	      }
	    }

	  } else {

	    /* Determine if unread messages are to be kept */

	    /* we need to know if there are none, or one, or more unread */
	    for (marked_unread=0, i=0; i<message_count && marked_unread<2; i++)
	      if((ison(headers[i]->status, UNREAD))
		&& (headers[i]->exit_disposition == UNSET))
		  marked_unread++;

	    if(marked_unread) {
	      answer = (always_keep ? *def_ans_yes : *def_ans_no);	/* default answer */
	      if(ask_questions) {
		if (marked_unread == 1)
		  MCsprintf(buffer, catgets(elm_msg_cat, ElmSet, ElmLeaveKeepMessage,
		    "Keep unread message in incoming mailbox? (%c/%c) "),
		    *def_ans_yes, *def_ans_no);
		else
		  MCsprintf(buffer, catgets(elm_msg_cat, ElmSet, ElmLeaveKeepMessages,
		    "Keep unread messages in incoming mailbox? (%c/%c) "),
		    *def_ans_yes, *def_ans_no);
		answer = want_to(buffer, answer, LINES-3, 0);
	      }

	      for (i = 0; i < message_count; i++) {
		if((ison(headers[i]->status, UNREAD))
		  && (headers[i]->exit_disposition == UNSET)) {

		    if(answer == *def_ans_no) {
		      headers[i]->exit_disposition = STORE;
		      to_store++;
		    } else {
		      headers[i]->exit_disposition = KEEP;
		      to_keep++;
		    }
	      
		}
	      }
	    }
	  }
	}

	dprint(3, (debugfile, "Messages to store: %d\n", to_store));
	dprint(3, (debugfile, "Messages to keep: %d\n", to_keep));

	if(to_delete + to_store + to_keep != message_count) {
	  MoveCursor(LINES, 0);
	  Raw(OFF);
	  dprint(1, (debugfile,
	  "Error: %d to delete + %d to store + %d to keep != %d message cnt\n",
	    to_delete, to_store, to_keep, message_count));
	  printf(catgets(elm_msg_cat, ElmSet, ElmSomethingWrongInCounts,
		"Something wrong in message counts! Folder unchanged.\n"));
	  emergency_exit();
	}
	  

	/* If we are not resyncing, we are leaving the mailfile and
	 * the new messages are new no longer. Note that this changes
	 * their status.
	 */
	if(!resyncing) {
	  for (i = 0; i < message_count; i++) {
	    if (ison(headers[i]->status, NEW)) {
	      clearit(headers[i]->status, NEW);
	      headers[i]->status_chgd = TRUE;
	    }
	  }
	}

	/* If all messages are to be kept and none have changed status
	 * we don't need to do anything because the current folder won't
	 * be changed by our writing it out - unless we are resyncing, in
	 * which case we force the writing out of the mailfile.
	 */

	for (num_chgd_status = 0, i = 0; i < message_count; i++)
	  if(headers[i]->status_chgd == TRUE)
	    num_chgd_status++;
	
	if(!to_delete && !to_store && !num_chgd_status && !resyncing) {
	  dprint(3, (debugfile, "Folder keep as is!\n"));
	  error(catgets(elm_msg_cat, ElmSet, ElmFolderUnchanged,
		"Folder unchanged."));
	  return(0);
	}

	/** we have to check to see what the sorting order was...so that
	    the order in which we write messages is the same as the order
	    of the messages originally.
	    We only need to do this if there are any messages to be
	    written out (either to keep or to store). **/

	if ((to_keep || to_store ) && sortby != MAILBOX_ORDER) {
	  last_sortby = sortby;
	  sortby = MAILBOX_ORDER;
	  sort_mailbox(message_count, FALSE);
	  sortby = last_sortby;
	}

	/* Formulate message as to number of keeps, stores, and deletes.
	 * This is only complex so that the message is good English.
	 */
	if (to_keep > 0) {
	  if (to_store > 0) {
	    if (to_delete > 0) {
	      if (to_keep == 1)
	        MCsprintf(buffer, catgets(elm_msg_cat, ElmSet, ElmLeaveKeepStoreDelete,
		      "[Keeping 1 message, storing %d, and deleting %d.]"), 
		    to_store, to_delete);
	      else
	        MCsprintf(buffer, catgets(elm_msg_cat, ElmSet, ElmLeaveKeepStoreDeletePlural,
		      "[Keeping %d messages, storing %d, and deleting %d.]"), 
		    to_keep, to_store, to_delete);
	    } else {
	      if (to_keep == 1)
		sprintf(buffer, catgets(elm_msg_cat, ElmSet, ElmLeaveKeepStore,
			"[Keeping 1 message and storing %d.]"), 
		      to_store);
	      else
		MCsprintf(buffer, catgets(elm_msg_cat, ElmSet, ElmLeaveKeepStorePlural,
			"[Keeping %d messages and storing %d.]"), 
		      to_keep, to_store);
	    }
	  } else {
	    if (to_delete > 0) {
	      if (to_keep == 1)
		sprintf(buffer, catgets(elm_msg_cat, ElmSet, ElmLeaveKeepDelete,
			"[Keeping 1 message and deleting %d.]"), 
		      to_delete);
	      else
		MCsprintf(buffer, catgets(elm_msg_cat, ElmSet, ElmLeaveKeepDeletePlural,
			"[Keeping %d messages and deleting %d.]"), 
		      to_keep, to_delete);
	    } else {
	      if (to_keep == 1)
		strcpy(buffer, catgets(elm_msg_cat, ElmSet, ElmLeaveKeep,
			"[Keeping message.]"));
	      else
		strcpy(buffer, catgets(elm_msg_cat, ElmSet, ElmLeaveKeepPlural,
			"[Keeping all messages.]"));
	    }
	  }
	} else if (to_store > 0) {
	  if (to_delete > 0) {
	    if (to_store == 1)
	      sprintf(buffer, catgets(elm_msg_cat, ElmSet, ElmLeaveStoreDelete,
		      "[Storing 1 message and deleting %d.]"), 
		    to_delete);
	    else
	      MCsprintf(buffer, catgets(elm_msg_cat, ElmSet, ElmLeaveStoreDeletePlural,
		      "[Storing %d messages and deleting %d.]"), 
		    to_store, to_delete);
	  } else {
	    if (to_store == 1)
	      strcpy(buffer, catgets(elm_msg_cat, ElmSet, ElmLeaveStore,
		      "[Storing message.]"));
	    else
	      strcpy(buffer, catgets(elm_msg_cat, ElmSet, ElmLeaveStorePlural,
		      "[Storing all messages.]"));
	  }
	} else {
	  if (to_delete > 0)
	    strcpy(buffer, catgets(elm_msg_cat, ElmSet, ElmLeaveDelete,
		"[Deleting all messages.]"));
	  else
	    buffer[0] = '\0';
	}
	/* NOTE: don't use variable "buffer" till message is output later */

	/** next, let's lock the file up and make one last size check **/

	if (folder_type == SPOOL)
	  lock(OUTGOING);
	
	if (mailfile_size != bytes(cur_folder)) {
	  unlock();
	  error(catgets(elm_msg_cat, ElmSet, ElmLeaveNewMailArrived,
		"New mail has just arrived. Resynchronizing..."));
	  return(-1);
	}
	
	/* Everything's GO - so ouput that user message and go to it. */

	block_signals();
	
	dprint(2, (debugfile, "Action: %s\n", buffer));
	error(buffer);

	/* Store messages slated for storage in received mail folder */
	if (to_store > 0) {
	  if ((err = can_open(recvd_mail, "a"))) {
	    error1(catgets(elm_msg_cat, ElmSet, ElmLeaveAppendDenied,
	      "Permission to append to %s denied!  Leaving folder intact.\n"),
	      recvd_mail);
	    dprint(1, (debugfile,
	      "Error: Permission to append to folder %s denied!! (%s)\n",
	      recvd_mail, "leavembox"));
	    dprint(1, (debugfile, "** %s **\n", error_description(err)));
	    unlock();
	    unblock_signals();
	    return(0);
	  }
	  if ((temp = fopen(recvd_mail,"a")) == NULL) {
	    err = errno;
	    unlock();
	    MoveCursor(LINES, 0);
	    Raw(OFF);
	    dprint(1, (debugfile, "Error: could not append to file %s\n", 
	      recvd_mail));
	    dprint(1, (debugfile, "** %s **\n", error_description(err)));
	    printf(catgets(elm_msg_cat, ElmSet, ElmLeaveCouldNotAppend,
		"Could not append to folder %s!\n"), recvd_mail);
	    emergency_exit();
	  }
	  dprint(2, (debugfile, "Storing message%s ", plural(to_store)));
	  for (i = 0; i < message_count; i++) {
	    if(headers[i]->exit_disposition == STORE) {
	      current = i+1;
	      dprint(2, (debugfile, "#%d, ", current));
	      copy_message("", temp, CM_UPDATE_STATUS);
	    }
	  }
	  fclose(temp);
	  dprint(2, (debugfile, "\n\n"));
	  (void) elm_chown(recvd_mail, userid, groupid);
	}

	/* If there are any messages to keep, first copy them to a
	 * temp file, then remove original and copy whole temp file over.
	 */
	if (to_keep > 0) {
	  sprintf(temp_keep_file, "%s%s%d", temp_dir, temp_file, getpid());
	  if ((err = can_open(temp_keep_file, "w"))) {
	    error1(catgets(elm_msg_cat, ElmSet, ElmLeaveTempFileDenied,
"Permission to create temp file %s for writing denied! Leaving folder intact."),
	      temp_keep_file);
	    dprint(1, (debugfile,
	      "Error: Permission to create temp file %s denied!! (%s)\n",
	      temp_keep_file, "leavembox"));
	    dprint(1, (debugfile, "** %s **\n", error_description(err)));
	    unlock();
	    unblock_signals();
	    return(0);
	  }
	  if ((temp = fopen(temp_keep_file,"w")) == NULL) {
	    err = errno;
	    unlock();
	    MoveCursor(LINES, 0);
	    Raw(OFF);
	    dprint(1, (debugfile, "Error: could not create file %s\n", 
	      temp_keep_file));
	    dprint(1, (debugfile, "** %s **\n", error_description(err)));
	    printf(catgets(elm_msg_cat, ElmSet, ElmLeaveCouldNotCreate,
		"Could not create temp file %s!\n"), temp_keep_file);
	    emergency_exit();
	  }
	  dprint(2, (debugfile, "Copying to temp file message%s to be kept ",
	    plural(to_keep)));
	  for (i = 0; i < message_count; i++) {
	    if(headers[i]->exit_disposition == KEEP) {
	      current = i+1;
	      dprint(2, (debugfile, "#%d, ", current));
	      copy_message("", temp, CM_UPDATE_STATUS);
	    }
	  }
	  if ( fclose(temp) == EOF ) {
	    MoveCursor(LINES, 0);
	    Raw(OFF);
	    printf(catgets(elm_msg_cat, ElmSet, ElmLeaveCloseFailedTemp,
		"\nClose failed on temp keep file in leavembox\n"));
	    perror(temp_keep_file);
	    dprint(2, (debugfile, "\n\rfclose err on temp_keep_file - leavembox\n\r"));
	    rm_temps_exit();
	  }
	  dprint(2, (debugfile, "\n\n"));

	} else if (folder_type == NON_SPOOL && !keep_empty_files && !resyncing) {

	  /* i.e. if no messages were to be kept and this is not a spool
	   * folder and we aren't keeping empty non-spool folders,
	   * simply remove the old original folder and that's it!
	   */
	  (void)unlink(cur_folder);
	  unblock_signals();
	  return(1);
	}

	/* Otherwise we have some work left to do! */

	/* Get original permissions and access time of the original
	 * mail folder before we remove it.
	 */
	if(save_file_stats(cur_folder) != 0) {
	  error1(catgets(elm_msg_cat, ElmSet, ElmLeaveProblemsSavingPerms,
		"Problems saving permissions of folder %s!"), cur_folder);
	  if (sleepmsg > 0)
		sleep(sleepmsg);
	}
	  
        if (stat(cur_folder, &buf) != 0) {
	  err = errno;
	  dprint(1, (debugfile, "Error: errno %s attempting to stat file %s\n", 
		     error_description(err), cur_folder));
          error2(catgets(elm_msg_cat, ElmSet, ElmLeaveErrorOnStat,
		"Error %s on stat(%s)."), error_description(err), cur_folder);
	}

#ifdef SYMLINK
        if (lstat(cur_folder, &lbuf) != 0) {
	  err = errno;
	  dprint(1, (debugfile, "Error: errno %s attempting to stat file %s\n", 
		     error_description(err), cur_folder));
          error2(catgets(elm_msg_cat, ElmSet, ElmLeaveErrorOnStat,
		"Error %s on stat(%s)."), error_description(err), cur_folder);
	}
#endif

	/* Close and remove the original folder.
	 * However, if we are going to copy a temp file of kept messages
	 * to it, and this is a locked (spool) mailbox, we need to keep
	 * it locked during this process. Unfortunately,
	 * if we did our USE_FLOCK_LOCKING or USE_FCNTL_LOCKING unlinking
	 * the original will kill the lock, so we have to resort to copying
	 * the temp file to the original file while keeping the original open.
	 * Also, if the file has a link count > 1, then it has links, so to
	 * prevent destroying the links, we do a copy back, even though its
	 * slower.  If the file is a symlink, then we also need to do a copy
	 * back to prevent destroying the linkage.
	 */

	/*
	 * fclose(mailfile);
	 *
	 * While this fclose is OK for BSD flock file locking, it is
	 * incorrect for SYSV fcntl file locking.  For some reason AT&T
	 * decided to release all file locks when *any* fd to a file
	 * is closed, even if it is not the fd that acquired the lock.
	 * Thus this fclose would release the mailbox file locks.
	 * Instead I am going to just fflush the file here, and do the
	 * individual closes in the subcases to ensure that the
	 * mailbox is locked until we are finished with it.
	 */
	fflush(mailfile);

	if(to_keep) {
#ifdef SYSCALL_LOCKING
	  need_to_copy = (folder_type == SPOOL ? TRUE : FALSE);
#else
	  need_to_copy = FALSE;
#endif /* SYSCALL_LOCKING */

	  if (buf.st_nlink > 1)
	    need_to_copy = TRUE;

	  if (buf.st_mode & 0x7000) { /* copy if special modes set */
		 need_to_copy = TRUE;    /* such as enforcement lock */
		 no_restore = TRUE;
	  }

#ifdef SYMLINK
#ifdef S_ISLNK
	  if (S_ISLNK(lbuf.st_mode))
#else
	  if ((lbuf.st_mode & S_IFMT) == S_IFLNK)
#endif
	  {
		 need_to_copy = TRUE;
		 no_restore = TRUE;
	  }
#endif

#ifdef _PC_CHOWN_RESTRICTED
	  if (!need_to_copy) {
/*
 * Chown may or may not be restricted to root in SVR4, if it is,
 *	then need to copy must be true, and no restore of permissions
 *	should be performed.
 */
	      if (pathconf(cur_folder, _PC_CHOWN_RESTRICTED)) {
		 need_to_copy = TRUE;
		 no_restore = TRUE;
	      }
	  }
#endif  /* _PC_CHOWN_RESTRICTED */

#ifdef SAVE_GROUP_MAILBOX_ID
	  if (folder_type == SPOOL)
      	    setgid(mailgroupid);
#endif

	  if(!need_to_copy) {
	    unlink(cur_folder);
	    if (link(temp_keep_file, cur_folder) != 0) {
	      if(errno == EXDEV || errno == EEXIST) {
		/* oops - can't link across file systems - use copy instead */
		need_to_copy = TRUE;
	      } else {
		err = errno;
		MoveCursor(LINES, 0);
		Raw(OFF);
		dprint(1, (debugfile, "link(%s, %s) failed (leavembox)\n", 
		       temp_keep_file, cur_folder));
		dprint(1, (debugfile, "** %s **\n", error_description(err)));
		printf(catgets(elm_msg_cat, ElmSet, ElmLeaveLinkFailed,
			"Link failed! %s.\n"), error_description(err));
#ifdef SAVE_GROUP_MAILBOX_ID
	        if (folder_type == SPOOL)
		  setgid(groupid);
#endif
		unlock();
		fclose(mailfile);
		emergency_exit();
	      }
	    }
	  }

	  if(need_to_copy) {

	    if (copy(temp_keep_file, cur_folder) != 0) {

	      /* copy to cur_folder failed - try to copy to special file */
	      err = errno;
	      dprint(1, (debugfile, "leavembox: copy(%s, %s) failed;",
		      temp_keep_file, cur_folder));
	      dprint(1, (debugfile, "** %s **\n", error_description(err)));
	      error(catgets(elm_msg_cat, ElmSet, ElmLeaveCouldntModifyFolder,
			"Couldn't modify folder!"));
	      if (sleepmsg > 0)
		    sleep((sleepmsg + 1) / 2);
	      sprintf(cur_folder,"%s/%s", home, unedited_mail);
	      if (copy(temp_keep_file, cur_folder) != 0) {

		/* couldn't copy to special file either */
		err = errno;
		MoveCursor(LINES, 0);
		Raw(OFF);
		dprint(1, (debugfile, 
			"leavembox: couldn't copy to %s either!!  Help;", 
			cur_folder));
		dprint(1, (debugfile, "** %s **\n", error_description(err)));
		printf(catgets(elm_msg_cat, ElmSet, ElmLeaveCantCopyMailbox,
			"Can't copy folder, system trouble : contents preserved in %s\n"),temp_keep_file);
#ifdef SAVE_GROUP_MAILBOX_ID
	        if (folder_type == SPOOL)
		  setgid(groupid);
#endif
		unlock();
		fclose(mailfile);
		emergency_exit();
	      } else {
		dprint(1, (debugfile,
			"\nWoah! Confused - Saved mail in %s (leavembox)\n", 
			cur_folder));
		error1(catgets(elm_msg_cat, ElmSet, ElmLeaveSavedMailIn,
			"Saved mail in %s."), cur_folder);
		if (sleepmsg > 0)
		    sleep((sleepmsg + 1) / 2);
	      }
	    }
	  }

	  /* link or copy complete - remove temp keep file */
	  unlink(temp_keep_file);

	} else if(folder_type == SPOOL || keep_empty_files || resyncing) {

	  /* if this is an empty spool file, or if this is an empty non spool 
	   * file and we keep empty non spool files (we always keep empty
	   * spool files), create an empty file */

	  if(folder_type == NON_SPOOL)
	    error1(catgets(elm_msg_cat, ElmSet, ElmLeaveKeepingEmpty,
		"Keeping empty folder '%s'."), cur_folder);
	  temp = fopen(cur_folder, "w");
	  fclose(temp);
	}

	/*
	 * restore permissions and access times of folder only if not
	 * a symlink, as symlinks have no permissions, and not worth
	 * tracking down what it points to.
	 */

	if (!no_restore) {
	  if(restore_file_stats(cur_folder) != 1) {
	    error1(catgets(elm_msg_cat, ElmSet, ElmLeaveProblemsRestoringPerms,
		  "Problems restoring permissions of folder %s!"), cur_folder);
	    if (sleepmsg > 0)
		sleep(sleepmsg);
	  }
	}

#if defined(BSD) && !defined(UTIMBUF)
	utime_buffer[0]     = buf.st_atime;
	utime_buffer[1]     = buf.st_mtime;
#else
	utime_buffer.actime = buf.st_atime;
	utime_buffer.modtime= buf.st_mtime;
#endif

#if defined(BSD) && !defined(UTIMBUF)
	if (utime(cur_folder, utime_buffer) != 0) 
#else
	if (utime(cur_folder, &utime_buffer) != 0) 
#endif
	{
	  err = errno;
	  dprint(1, (debugfile, 
		 "Error: encountered error doing utime (leavmbox)\n"));
	  dprint(1, (debugfile, "** %s **\n", error_description(err)));
	  error2(catgets(elm_msg_cat, ElmSet, ElmLeaveChangingAccessTime,
		"Error %s trying to change file %s access time."), 
		   error_description(err), cur_folder);
	}
#ifdef SAVE_GROUP_MAILBOX_ID
	if (folder_type == SPOOL)
	    setgid(groupid);
#endif


	mailfile_size = bytes(cur_folder);
	unlock();	/* remove the lock on the file ASAP! */
	fclose(mailfile);
	unblock_signals();
	return(1);	
}

#ifdef HASSIGPROCMASK
	sigset_t	toblock, oldset;
#else  /* HASSIGPROCMASK */
#  ifdef HASSIGBLOCK
	int		toblock, oldset;
#  else /* HASSIGBLOCK */
#    ifdef HASSIGHOLD
	/* Nothing required */
#    else /* HASSIGHOLD */
	SIGHAND_TYPE	(*oldhup)();
	SIGHAND_TYPE	(*oldint)();
	SIGHAND_TYPE	(*oldquit)();
	SIGHAND_TYPE	(*oldstop)();
#    endif /* HASSIGHOLD */
#  endif /* HASSIGBLOCK */
#endif /* HASSIGPROCMASK */

/*
 * Block all keyboard generated signals.  Need to do this while
 * rewriting mailboxes to avoid inadvertant corruption.  In
 * particular, a SIGHUP (from logging out under /bin/sh), can
 * corrupt a spool mailbox during an elm autosync.
 */

block_signals()
{
	dprint(1,(debugfile, "block_signals\n"));
#ifdef HASSIGPROCMASK
	sigemptyset(&oldset);
	sigemptyset(&toblock);
	sigaddset(&toblock, SIGHUP);
	sigaddset(&toblock, SIGINT);
	sigaddset(&toblock, SIGQUIT);
#ifdef SIGTSTP
	sigaddset(&toblock, SIGTSTP);
#endif /* SIGTSTP */

	sigprocmask(SIG_BLOCK, &toblock, &oldset);

#else /* HASSIGPROCMASK */
#  ifdef HASSIGBLOCK
	toblock = sigmask(SIGHUP) | sigmask(SIGINT) | sigmask(SIGQUIT);
#ifdef SIGTSTP
	toblock |= sigmask(SIGTSTP);
#endif /* SIGTSTP */

	oldset = sigblock(toblock);

#  else /* HASSIGBLOCK */
#    ifdef HASSIGHOLD
	sighold(SIGHUP);
	sighold(SIGINT);
	sighold(SIGQUIT);
#ifdef SIGTSTP
	sighold(SIGTSTP);
#endif /* SIGTSTP */

#    else /* HASSIGHOLD */
	oldhup  = signal(SIGHUP, SIG_IGN);
	oldint  = signal(SIGINT, SIG_IGN);
	oldquit = signal(SIGQUIT, SIG_IGN);
#ifdef SIGTSTP
	oldstop = signal(SIGTSTP, SIG_IGN);
#endif /* SIGTSTP */
#    endif /* HASSIGHOLD */
#  endif /* HASSIGBLOCK */
#endif /* HASSIGPROCMASK */
}

/*
 * Inverse of the previous function.  Restore keyboard generated
 * signals.
 */
unblock_signals()
{
	dprint(1,(debugfile, "unblock_signals\n"));
#ifdef HASSIGPROCMASK
	sigprocmask(SIG_SETMASK, &oldset, (sigset_t *)0);

#else  /* HASSIGPROCMASK */
#  ifdef HASSIGBLOCK
	sigsetmask(oldset);

#  else /* HASSIGBLOCK */
#    ifdef HASSIGHOLD
	sigrelse(SIGHUP);
	sigrelse(SIGINT);
	sigrelse(SIGQUIT);
#ifdef SIGTSTP
	sigrelse(SIGTSTP);
#endif /* SIGTSTP */

#    else /* HASSIGHOLD */
	signal(SIGHUP, oldhup);
	signal(SIGINT, oldint);
	signal(SIGQUIT, oldquit);
#ifdef SIGTSTP
	signal(SIGTSTP, oldstop);
#endif /* SIGTSTP */

#    endif /* HASSIGHOLD */
#  endif /* HASSIGBLOCK */
#endif /* HASSIGPROCMASK */
}

