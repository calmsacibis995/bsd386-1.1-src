
static char rcsid[] = "@(#)Id: edit.c,v 5.7 1993/05/08 20:25:33 syd Exp ";

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
 * $Log: edit.c,v $
 * Revision 1.2  1994/01/14  00:54:43  cp
 * 2.4.23
 *
 * Revision 5.7  1993/05/08  20:25:33  syd
 * Add sleepmsg to control transient message delays
 * From: Syd
 *
 * Revision 5.6  1993/02/03  17:12:53  syd
 * move more declarations to defs.h, including sleep
 * From: Syd
 *
 * Revision 5.5  1993/01/20  03:37:16  syd
 * Nits and typos in the NLS messages and corresponding default messages.
 * From: dwolfe@pffft.sps.mot.com (Dave Wolfe)
 *
 * Revision 5.4  1992/12/07  14:53:21  syd
 * Fix typos in edit.c
 * From: Bo.Asbjorn.Muldbak <bam@jutland.ColumbiaSC.NCR.COM>
 *
 * Revision 5.3  1992/11/26  00:46:13  syd
 * changes to first change screen back (Raw off) and then issue final
 * error message.
 * From: Syd
 *
 * Revision 5.2  1992/11/14  21:53:49  syd
 * When elm copies the temp mailbox back to the mail spool to resync or
 * quit, it changes to the mailgroup before attempting to diddle in the
 * mail spool, but when it copies the temp mailbox back to the mail spool
 * after editing, it forgets to change to mailgroup.  This patch appears
 * to work, but I haven't exhaustively checked for some path that leaves
 * the gid set
 * wrong.  From: dwolfe@pffft.sps.mot.com (Dave Wolfe)
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** This routine is for allowing the user to edit their current folder
    as they wish.

**/

#include "headers.h"
#include "s_elm.h"
#include <errno.h>

extern int errno;

char   *error_description();
long   bytes();

#ifdef ALLOW_MAILBOX_EDITING

static void copy_failed_emergency_exit(cur_folder, edited_file, linked)
char *cur_folder, *edited_file;
int linked;
{
	int err = errno;

	MoveCursor(LINES, 0);
	Raw(OFF);

	if (linked)
		MCprintf(catgets(elm_msg_cat, ElmSet, ElmCouldntLinkMailfile,
			"\nCouldn't link %s to mail file %s!\n"),
			edited_file, cur_folder);
	else
		MCprintf(catgets(elm_msg_cat, ElmSet, ElmCouldntCopyMailfile,
			"\nCouldn't copy %s to mail file %s!\n"),
			cur_folder, edited_file);

	printf(catgets(elm_msg_cat, ElmSet, ElmCheckOutMail,
		"\nYou'll need to check out %s for your mail.\n"),
		edited_file);
	printf("** %s. **\n", error_description(err));
	unlock();					/* ciao!*/
	emergency_exit();
}

edit_mailbox()
{
	/** Allow the user to edit their folder, always resynchronizing
	    afterwards.   Due to intense laziness on the part of the
	    programmer, this routine will invoke $EDITOR on the entire
	    file.  The mailer will ALWAYS resync on the folder
	    even if nothing has changed since, not unreasonably, it's
	    hard to figure out what occurred in the edit session...

	    Also note that if the user wants to edit their incoming
	    mailbox they'll actually be editing the tempfile that is
	    an exact copy.  More on how we resync in that case later
	    in this code.
	**/

	FILE	*real_folder, *temp_folder;
	char	edited_file[SLEN], buffer[SLEN];
	int	len;

	if(folder_type == SPOOL) {
	  if(save_file_stats(cur_folder) != 0) {
	    error1(catgets(elm_msg_cat, ElmSet, ElmPermFolder,
	      "Problems saving permissions of folder %s!"), cur_folder);
	    Raw(ON);
	    if (sleepmsg > 0)
		sleep(sleepmsg);
	    return(0);
	  }
	}

	strcpy(edited_file,
	    (folder_type == NON_SPOOL ? cur_folder : cur_tempfolder));
	if (edit_a_file(edited_file) == 0) {
	    return (0);
	}

	if (folder_type == SPOOL) {	/* uh oh... now the toughie...  */

	  if (bytes(cur_folder) != mailfile_size) {

	     /* SIGH.  We've received mail since we invoked the editor
		on the folder.  We'll have to do some strange stuff to
	        remedy the problem... */

	     PutLine0(LINES, 0, catgets(elm_msg_cat, ElmSet, ElmWarnNewMailRecv,
	       "Warning: new mail received..."));
	     CleartoEOLN();

	     if ((temp_folder = fopen(edited_file, "a")) == NULL) {
	       dprint(1, (debugfile,
		    "Attempt to open \"%s\" to append failed in %s\n",
		    edited_file, "edit_mailbox"));
	       set_error(catgets(elm_msg_cat, ElmSet, ElmCouldntReopenTemp,
		 "Couldn't reopen temp file. Edit LOST!"));
	       return(1);
	     }
	     /** Now let's lock the folder up and stream the new stuff
		 into the temp file... **/

	     lock(OUTGOING);
	     if ((real_folder = fopen(cur_folder, "r")) == NULL) {
	       dprint(1, (debugfile,
	           "Attempt to open \"%s\" for reading new mail failed in %s\n",
 		   cur_folder, "edit_mailbox"));
	       sprintf(buffer, catgets(elm_msg_cat, ElmSet, ElmCouldntOpenFolder,
		 "Couldn't open %s for reading!  Edit LOST!"), cur_folder);
	       set_error(buffer);
	       unlock();
	       return(1);
	     }
	     if (fseek(real_folder, mailfile_size, 0) == -1) {
	       dprint(1, (debugfile,
			"Couldn't seek to end of cur_folder (offset %ld) (%s)\n",
			mailfile_size, "edit_mailbox"));
	       set_error(catgets(elm_msg_cat, ElmSet, ElmCouldntSeekEnd,
		 "Couldn't seek to end of folder.  Edit LOST!"));
	       unlock();
	       return(1);
	     }

	     /** Now we can finally stream the new mail into the tempfile **/

	     while ((len = mail_gets(buffer, SLEN, real_folder)) != 0)
	       if (fwrite(buffer, 1, len, temp_folder) != len) {
	         copy_failed_emergency_exit(cur_folder, edited_file, FALSE);
	       }

	     fclose(real_folder);
	     if (fclose(temp_folder)) {
	       copy_failed_emergency_exit(cur_folder, edited_file, FALSE);
	     }

 	   } else lock(OUTGOING);

#ifdef SAVE_GROUP_MAILBOX_ID
      	   setgid(mailgroupid);
#endif

	   /* remove real mail_file and then
	    * link or copy the edited mailfile to real mail_file */

	   (void)unlink(cur_folder);

	   if (link(edited_file, cur_folder) != 0)  {
	     if (errno == EXDEV || errno == EEXIST) {
	       /* attempt to link across file systems */
   	       if (copy(edited_file, cur_folder) != 0) {
	         copy_failed_emergency_exit(cur_folder, edited_file, FALSE);
	       }
	     } else {
	         copy_failed_emergency_exit(cur_folder, edited_file, TRUE);
	     }
	   }

	   /* restore file permissions before removing lock */

	   if(restore_file_stats(cur_folder) != 1) {
	     error1(catgets(elm_msg_cat, ElmSet, ElmProblemsRestoringPerms,
	       "Problems restoring permissions of folder %s!"), cur_folder);
	     Raw(ON);
	     if (sleepmsg > 0)
		sleep(sleepmsg);
	   }

#ifdef SAVE_GROUP_MAILBOX_ID
	   setgid(groupid);
#endif

	   unlock();
	   unlink(edited_file);	/* remove the edited mailfile */
	   error(catgets(elm_msg_cat, ElmSet, ElmChangesIncorporated,
	     "Changes incorporated into new mail..."));

	} else
	  error(catgets(elm_msg_cat, ElmSet, ElmResyncingNewVersion,
	    "Resynchronizing with new version of folder..."));

	if (sleepmsg > 0)
		sleep(sleepmsg);
	ClearScreen();
	newmbox(cur_folder, FALSE);
	showscreen();
	return(1);
}

#endif

int
edit_a_file(editfile)
char *editfile;
{
	/** Edit a file.  This routine is used by edit_mailbox()
	    and edit_aliases_text().  It gets all the editor info
	    from the elmrc file.
	**/

	char     buffer[SLEN];

	PutLine0(LINES-1,0, catgets(elm_msg_cat, ElmSet, ElmInvokeEditor,
	  "Invoking editor..."));

	if (strcmp(editor, "builtin") == 0 || strcmp(editor, "none") == 0) {
	  if (in_string(alternative_editor, "%s"))
	    sprintf(buffer, alternative_editor, editfile);
	  else
	    sprintf(buffer, "%s %s", alternative_editor, editfile);
	} else {
	  if (in_string(editor, "%s"))
	    sprintf(buffer, editor, editfile);
	  else
	    sprintf(buffer, "%s %s", editor, editfile);
	}

	Raw(OFF);

	if (system_call(buffer, SY_ENAB_SIGHUP) == -1) {
	  error1(catgets(elm_msg_cat, ElmSet, ElmProblemsInvokingEditor,
	    "Problems invoking editor %s!"), alternative_editor);
	  Raw(ON);
	  if (sleepmsg > 0)
		sleep(sleepmsg);
	  return(0);
	}

	Raw(ON);
	/* a location not near the next request, so an absolute is used */
	SetXYLocation(0, 40);

	return(1);
}
