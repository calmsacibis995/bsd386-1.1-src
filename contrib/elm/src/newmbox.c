
static char rcsid[] = "@(#)Id: newmbox.c,v 5.34 1993/09/27 01:51:38 syd Exp ";

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
 * $Log: newmbox.c,v $
 * Revision 1.2  1994/01/14  00:55:21  cp
 * 2.4.23
 *
 * Revision 5.34  1993/09/27  01:51:38  syd
 * Add elm_chown to consolidate for Xenix not allowing -1
 * From: Syd
 *
 * Revision 5.33  1993/09/19  23:38:16  syd
 * Erroneous Content-Length:s that ended up beyond the end of the folder
 * wasn't checked for, so the rest of the folder became one big message.
 * From: Jan.Djarv@sa.erisoft.se (Jan Djarv)
 *
 * Revision 5.32  1993/09/19  23:32:35  syd
 * Fix a code portability problem with Convex.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.31  1993/09/19  23:15:28  syd
 * Changed a few buffers from LONG_STRING (512) to VERY_LONG_STRING
 * to avoid long header lines overflowing the allocated space. At
 * least 1024 bytes should be allowed in any header line/field.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.30  1993/08/23  03:26:24  syd
 * Try setting group id separate from user id in chown to
 * allow restricted systems to change group id of file
 * From: Syd
 *
 * Revision 5.29  1993/08/23  02:46:51  syd
 * Test ANSI_C, not __STDC__ (which is not set on e.g. AIX).
 * From: decwrl!uunet.UU.NET!fin!chip (Chip Salzenberg)
 *
 * Revision 5.28  1993/08/03  19:28:39  syd
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
 * Revision 5.27  1993/07/20  15:32:36  syd
 * no dprint should be at level 0
 * From: Syd
 *
 * Revision 5.26  1993/05/14  03:55:13  syd
 * The recent feature addition for the "readmsginc" variable did not modify
 * the output statement that is used when MMDF is defined.  The following
 * patch makes the variable work on MMDF systems.
 * From: Larry Philps <larryp@sco.COM>
 *
 * Revision 5.25  1993/05/08  20:25:33  syd
 * Add sleepmsg to control transient message delays
 * From: Syd
 *
 * Revision 5.24  1993/05/08  18:56:16  syd
 * created a new elmrc variable named "readmsginc".  This specifies an
 * increment by which the message count is updated.  If this variable is
 * set to, say, 25, then the message count will only be updated every 25
 * messages, displaying 0, 25, 50, 75, and so forth.  The default value
 * of 1 will cause Elm to behave exactly as it currently does in PL21.
 * From: Eric Peterson <epeterso@encore.com>
 *
 * Revision 5.23  1993/04/16  04:03:18  syd
 * *** empty log message ***
 *
 * Revision 5.22  1993/02/09  19:36:52  syd
 * STDC declares rewind
 * From: Syd
 *
 * Revision 5.21  1993/02/03  17:12:53  syd
 * move more declarations to defs.h, including sleep
 * From: Syd
 *
 * Revision 5.20  1993/02/03  15:26:13  syd
 * protect atol in ifndef __STDC__ as some make it a macro, and its in stdlib.h
 *
 * Revision 5.19  1993/01/27  20:45:55  syd
 * When elm exits because a spool mailbox is used, it should do a Raw(OFF) so
 * that the error message get's printed on the right screen.
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.18  1993/01/27  20:44:27  syd
 * Correct MMDF syntax problems and newmail botches.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.17  1993/01/20  03:37:16  syd
 * Nits and typos in the NLS messages and corresponding default messages.
 * From: dwolfe@pffft.sps.mot.com (Dave Wolfe)
 *
 * Revision 5.16  1993/01/20  03:02:19  syd
 * Move string declarations to defs.h
 * From: Syd
 *
 * Revision 5.15  1993/01/19  05:20:32  syd
 * On a resync, the temp file was being reopened in *append*
 * mode, thus the fseek that tried to back up the file pointer had no
 * effect.
 * From: Larry Philps <larryp@sco.COM>
 *
 * Revision 5.14  1993/01/19  05:12:22  syd
 * read_headers() in src/newmbox.c assumes that Mime-Version preceeds
 * Content-Type in the headers. I removed that assumption.
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.13  1993/01/19  05:07:05  syd
 * Trim erroreous extra log entry
 * From: Syd
 *
 * Revision 5.12  1993/01/19  04:47:12  syd
 * Significant changes to provide consistent Date and From_ header
 * cracking.  Overhauled date utilities and moved into library.  Moved
 * real_from() into library.  Modified frm, newmail, and readmsg utilities
 * to use library version of real_from().  Moved get_word() from Elm
 * source into library.  Added new library routines atonum() and strfcpy().
 * Fixed trailing backslash bug in len_next().
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.11  1992/12/11  01:45:04  syd
 * remove sys/types.h include, it is now included by defs.h
 * and this routine includes defs.h or indirectly includes defs.h
 * From: Syd
 *
 * Revision 5.10  1992/12/07  04:31:38  syd
 * Fix typo
 * From: Syd
 *
 * Revision 5.9  1992/11/26  00:46:13  syd
 * changes to first change screen back (Raw off) and then issue final
 * error message.
 * From: Syd
 *
 * Revision 5.8  1992/11/22  00:08:45  syd
 * I was playing with the metamail stuff and Sun's Openwindows Mailtool
 * and discovered that I was able to 'display' messages generated with
 * Mailtool in elm using metamail.  This marks the Sun Attachments as
 * needing metamail.
 * From: Lutz Brunke <brunke@dkrz-hamburg.dbp.de>
 *
 * Revision 5.7  1992/11/14  21:49:42  syd
 * I think that the code in 'newmbox.c' which handles bad 'Content-length'
 * entries is incomplete. The file-ptr for the mail file is
 * backed up, but the file-ptr of the temp file WAS LEFT UNMODIFIED !
 * From langesw.ssw.de!root Wed Nov 11 14:28:57 1992
 *
 * Revision 5.6  1992/11/07  20:05:52  syd
 * change to use header_cmp to allow for linear white space around the colon
 * From: Syd
 *
 * Revision 5.5  1992/11/07  16:08:52  syd
 * add tolerance for incorrect content length line
 * From: Syd
 *
 * Revision 5.4  1992/10/31  18:52:51  syd
 * Corrections to Unix date parsing and time zone storage
 * From: eotto@hvlpa.att.com
 *
 * Revision 5.3  1992/10/25  01:47:45  syd
 * fixed a bug were elm didn't call metamail on messages with a characterset,
 * which could be displayed by elm itself, but message is encoded with QP
 * or BASE64
 * From: Klaus Steinberger <Klaus.Steinberger@Physik.Uni-Muenchen.DE>
 *
 * Revision 5.2  1992/10/24  12:51:34  syd
 * Fixes SysV style forwarding
 * From: Baruch Cochavy <bcochavy@ilux39.intel.com>
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/**  read new folder **/

#include "headers.h"
#include "s_elm.h"

#include <sys/stat.h>
#include <errno.h>

#ifdef I_TIME
#  include <time.h>
#endif
#ifdef I_SYSTIME
#  include <sys/time.h>
#endif

extern int errno;

char *error_description();
long bytes();
#ifndef ANSI_C /* avoid problems with systems that declare atol as a macro */
extern void rewind();
extern long atol();
#endif

int
newmbox(new_file, adds_only)
char *new_file;
int adds_only;
{
	/** Read a folder.

	    new_file	- name of folder  to read. It is up to the calling
			  function to make sure that the file can be
			  read by the user. This is not checked in this
			  function. The reason why it is not checked here
			  is due to the situation where the user wants to
			  change folders: the new folder must be checked
			  for access *before* leaving the old one, which
			  is before this function gets called.
	    adds_only	- set if we only want to read newly added messages to
				same old folder.

	**/

	int  same_file;
	int  new_folder_type;
	int err;
	char new_tempfile[SLEN];

	/* determine type of new mailfile and calculate temp file name */
	if((new_folder_type = get_folder_type(new_file)) == SPOOL)
	  mk_temp_mail_fn(new_tempfile, new_file);
	else
	  *new_tempfile = '\0';

	/* determine whether we are changing files */
	same_file = !(strcmp(new_file, cur_folder));

	/* If we are changing files and we are changing to a spool file,
	 * make sure there isn't a temp file for it, because if
	 * there is, someone else is using ELM to read the new file,
	 * and we don't want to be reading it at the same time.
	 */
	if((new_folder_type == SPOOL) && (!same_file)) {
	  if (access(new_tempfile, ACCESS_EXISTS) != -1) {
	    Raw(OFF);
	    ClearScreen();
	    Centerline(15, catgets(elm_msg_cat, ElmSet, ElmAlreadyRunning1,
		"You seem to have ELM already reading this mail!"));
	    Centerline(17, catgets(elm_msg_cat, ElmSet, ElmAlreadyRunning2,
		"You may not have two copies of ELM running simultaneously.  -- Exiting --"));
	    Centerline(18, catgets(elm_msg_cat, ElmSet, ElmAlreadyRunning3,
		"If this is in error, then you'll need to remove the following file:"));
	    Centerline(19, new_tempfile);
	    MoveCursor(LINES, 0);  /* so shell prompt upon exit is on newline */
	    silently_exit();
	  }
	}

	/* If we were reading a spool file and we are not just reading
	 * in the additional new messages to the same file, we need to
	 * remove the corresponding tempfile.
	 */

	if((folder_type == SPOOL) && !adds_only) {
	  if (access(cur_tempfolder, ACCESS_EXISTS) != -1) {
	    if (unlink(cur_tempfolder) != 0) {
	      error2(catgets(elm_msg_cat, ElmSet, ElmSorryCantUnlinkTemp,
		"Sorry, can't unlink the temp file %s [%s]!\n\r"),
	        cur_tempfolder, error_description(errno));
	      silently_exit();
	    }
	  }
	}

	/* Okay! Now establish this new file as THE file */
	strcpy(cur_folder, new_file);
	folder_type = new_folder_type;
	strcpy(cur_tempfolder, new_tempfile);

	clear_error();
	clear_central_message();

	if (mailfile != NULL)
	  (void) fclose(mailfile);  /* close it first, to avoid too many open */

	if ((mailfile = fopen(cur_folder,"r")) == NULL)  {
	  if (errno != ENOENT ) { /* error on anything but file not exist */
	    err = errno;
	    MoveCursor(LINES,0);
	    Raw(OFF);
	    printf(catgets(elm_msg_cat, ElmSet, ElmFailOnOpenNewmbox,
		    "\nfail on open in newmbox, open %s failed!!\n"),
		    cur_folder);
	    printf("** %s. **\n", error_description(err));
	    dprint(1, (debugfile, "fail on open in newbox, file %s!!\n",
		    cur_folder));
	    rm_temps_exit();
	  }
	  else {
	    mailfile_size = 0;         /* must non-existant folder */
	    message_count = 0;
	    selected = 0;
	  }
	} else {                          /* folder exists, read headers */
	  read_headers(adds_only);
	}

	if(!same_file)		/* limit mode off if this is a new file */
	  selected = 0;
	if (!adds_only)		/* limit mode off if recreating headers */
	  selected = 0;		/* because we loose the 'Visible' flag */

	dprint(1, (debugfile,
	  "New folder %s type %s temp file %s (%s)\n", cur_folder, 
	  (folder_type == SPOOL ? "spool" : "non-spool"),
	  (*cur_tempfolder ? cur_tempfolder : "none"), "newmbox"));

	return(0);
}

int
get_folder_type(filename)
char *filename;
{
	/** returns the type of mailfile filename is
	    NO_NAME = no name
	    SPOOL = consisting only of mailhome plus base file name
		    (no intervening directory name)
	    NON_SPOOL = a name that is not SPOOL type above
	 **/

	char *last_slash;

	/* if filename is null or is of zero length */
	if((filename == NULL) || (*filename == '\0'))
	  return(NO_NAME);

	/* if filename begins with mailhome,
	 * and there is a slash in filename,
	 * and there is a filename after it (i.e. last slash is not last char),
	 * and the last character of mailhome is last slash in filename,
	 * it's a spool file .
	 */
	if((first_word(filename, mailhome)) &&
	  ((last_slash = rindex(filename, '/')) != NULL) &&
	  (*(last_slash+1) != '\0') &&
	  (filename + strlen(mailhome) - 1 == last_slash))
	    return(SPOOL);
	/* if file name == default mailbox, its a spool file also
	 * even if its not in the spool directory. (SVR4)
	 */
	if (strcmp(filename, defaultfile) == 0)
	    return(SPOOL);

	return(NON_SPOOL);
}

mk_temp_mail_fn(tempfn, mbox)
char *tempfn, *mbox;
{
	/** create in tempfn the name of the temp file corresponding to
	    mailfile mbox. Mbox is presumed to be a file in mailhome;
	    Strangeness may result if it is not!
	 **/

	char *cp;

	sprintf(tempfn, "%s%s", default_temp, temp_mbox);
	if((cp = rindex(mbox, '/')) != NULL) {
	  cp++;
	  if (strcmp(cp, "mbox") == 0 || strcmp(cp, "mailbox") == 0 ||
		strcmp(cp, "inbox") == 0 || *cp == '.')
	    strcat(tempfn, username);
	  else
	    strcat(tempfn, cp);
	}
}

int
read_headers(add_new_only)
int add_new_only;
{
	/** Reads the headers into the headers[] array and leaves the
	    file rewound for further I/O requests.   If the file being
	    read is a mail spool file (ie incoming) then it is copied to
	    a temp file and closed, to allow more mail to arrive during 
	    the elm session.  If 'add_new_only' is set, the program will copy
	    the status flags from the previous data structure to the new 
	    one if possible and only read in newly added messages.
	**/

	FILE *temp;
	struct header_rec *current_header = NULL;
	char buffer[VERY_LONG_STRING], tbuffer[VERY_LONG_STRING], *c;
	long fbytes = 0L, line_bytes = 0L, content_start = 0L,
	  content_remaining = -1L, lines_start = 0L;
	register long line = 0;
	register int count = 0, another_count,
	  subj = 0, copyit = 0, in_header = FALSE;
	int count_x, count_y = 17, err;
	int in_to_list = FALSE, forwarding_mail = FALSE, first_line = TRUE,
	  content_length_found = FALSE;

	static int first_read = 0;
#ifdef MMDF
        int newheader = 0;
#endif /* MMDF */

	if (folder_type == SPOOL) {
	  lock(INCOMING);	/* ensure no mail arrives while we do this! */
	  if (! add_new_only) {
	    if (access(cur_tempfolder, ACCESS_EXISTS) != -1) {
	      /* Hey!  What the hell is this?  The temp file already exists? */
	      /* Looks like a potential clash of processes on the same file! */
	      unlock();				     /* so remove lock file! */
	      error(catgets(elm_msg_cat, ElmSet, ElmWhatsThisTempExists,
		"What's this?  The temp folder already exists??"));
	      if (sleepmsg > 0)
		    sleep(sleepmsg);
	      error(catgets(elm_msg_cat, ElmSet, ElmIGiveUp,
		"Ahhhh... I give up."));
	      silently_exit();	/* leave without tampering with it! */
	    }
	    if ((temp = fopen(cur_tempfolder,"w")) == NULL) {
	     err = errno;
	     unlock();	/* remove lock file! */
	     MoveCursor(LINES, 0);
	     Raw(OFF);
	     printf(catgets(elm_msg_cat, ElmSet, ElmCouldntOpenForTemp,
		     "\nCouldn't open file %s for use as temp file.\n"),
		     cur_tempfolder);
	     printf("** %s. **\n", error_description(err));
	     dprint(1, (debugfile,
                "Error: Couldn't open file %s as temp mbox.  errno %s (%s)\n",
	         cur_tempfolder, error_description(err), "read_headers"));
	     rm_temps_exit();
	    }
	   copyit++;
	   (void) elm_chown(cur_tempfolder, userid, groupid);
	   chmod(cur_tempfolder, 0700);	/* shut off file for other people! */
	 }
	 else {
	   /*
	    * Used to open this file for append, however with the new
	    * content length stuff, we might want to fseek backwards
	    * in the file.  This will fail if append mode is set, so
	    * we can not do that anymore.  Since there is no way to
	    * fopen a file for write-only without either truncating it
	    * or putting it into append mode (neither of which is right
	    * for us), we do a "r+" open (also opens the file for reading),
	    * then fseek to the end of the file.
	    */
	   if ((temp = fopen(cur_tempfolder,"r+")) == NULL) {
	     err = errno;
	     unlock();	/* remove lock file! */
	     MoveCursor(LINES,0);
	     Raw(OFF);
	     printf(catgets(elm_msg_cat, ElmSet, ElmCouldntReopenForTemp,
		     "\nCouldn't reopen file %s for use as temp file.\n"),
		     cur_tempfolder);
	     printf("** %s. **\n", error_description(err));
	     dprint(1, (debugfile, 
                "Error: Couldn't reopen file %s as temp mbox.  errno %s (%s)\n",
	         cur_tempfolder, error_description(err), "read_headers"));
	     rm_temps_exit();
	    }
	   if (fseek(temp, 0, 2) == -1) {
	     err = errno;
	     unlock();	/* remove lock file! */
	     fclose(temp);
	     MoveCursor(LINES,0);
	     Raw(OFF);
	     printf(catgets(elm_msg_cat, ElmSet, ElmCouldntSeekTempEnd,
		     "\nCouldn't fseek to end of reopened temp mbox.\n"));
	     printf("** %s. **\n", error_description(err));
	     dprint(1, (debugfile, 
                "Error: Couldn't fseek to end of reopened temp mbox.  errno %s (%s)\n",
	         error_description(err), "read_headers"));
	     rm_temps_exit();
	    }
	   copyit++;
	  }
	}

	if (! first_read++) {
	  ClearLine(LINES-1);
	  ClearLine(LINES);
	  if (add_new_only)
	    PutLine2(LINES, 0, catgets(elm_msg_cat, ElmSet, ElmReadingInMessage,
		     "Reading in %s, message: %d"),
		     cur_folder, message_count);
	  else
	    PutLine1(LINES, 0, catgets(elm_msg_cat, ElmSet, ElmReadingInMessage0,
		     "Reading in %s, message: 0"), cur_folder);
	  count_x = LINES;
          count_y = 22 + strlen(cur_folder);
	}
	else {
	  count_x = LINES-2;
	  PutLine0(LINES-2, 0, catgets(elm_msg_cat, ElmSet, ElmReadingMessage0,
		"Reading message: 0"));
	}

	if (add_new_only) {
	   if (fseek(mailfile, mailfile_size, 0) == -1) {
	     err = errno;
	     MoveCursor(LINES, 0);
	     Raw(OFF);
	     MCprintf(catgets(elm_msg_cat, ElmSet, ElmCouldntSeekEndFolder,
		"\nCouldn't seek to %ld (end of folder) in %s!\n"),
		mailfile_size, cur_folder);	
	     printf("** %s. **\n", 1, error_description(err));
	     dprint(1, (debugfile,
     "Error: Couldn't seek to end of folder %s: (offset %ld) Errno %s (%s)\n",
	        cur_folder, mailfile_size, error_description(err), "read_headers"));
	     emergency_exit();
	   }
	   count = message_count;		/* next available  */
	   fbytes = mailfile_size;		/* start correctly */
	}

	/** find the size of the folder then unlock the file **/

	mailfile_size = bytes(cur_folder);
	unlock();

	/** now let's copy it all across accordingly... **/

	while (fbytes < mailfile_size) {

	  if ((line_bytes = mail_gets(buffer, VERY_LONG_STRING, mailfile)) == 0)
	    break;

	  if (copyit)
	    if (fwrite(buffer, 1, line_bytes, temp) != line_bytes) {
		err = errno;
		MoveCursor(LINES, 0);
		Raw(OFF);
		printf(catgets(elm_msg_cat, ElmSet, ElmWriteToTempFailed,
				"\nWrite to temp file %s failed!!\n"),
				cur_tempfolder);
		printf("** %s. **\n", error_description(err));
		dprint(1, (debugfile, "Can't write to temp file %s!!\n",
			   cur_tempfolder));
		rm_temps_exit();
	    }

	  /* Fix below to increment line count ONLY if we got a full line.
	   * Input lines longer than the mail_gets buffer size would
	   * get counted each time a subsequent part of them was
	   * read in. This meant that when the faulty line count was used
	   * to display the message, part of the next message
	   * was displayed at the end of the message.
	   */
	  if(buffer[line_bytes - 1] == '\n') line++;

	  if (fbytes == 0L || first_line) { 	/* first line of file... */	
	    if (folder_type == SPOOL) {
	      if (first_word_nc(buffer, "forward to ")) {
	        set_central_message(catgets(elm_msg_cat, ElmSet, ElmMailBeingForwardTo,
			"Mail being forwarded to %s"), 
                   	(char *) (buffer + 11));
	        forwarding_mail = TRUE;
	      }
	    }

	    /** flush leading blank lines before next test... **/
	    if (line_bytes == 1) {
	      fbytes++;
	      continue;	
	    }
	    else
	      first_line = FALSE;
	      	
#ifdef MMDF
	    if (!forwarding_mail && strcmp(buffer, MSG_SEPARATOR) != 0 ) {
#else
	    if (! first_word(buffer, "From ") && !forwarding_mail) {
#endif /* MMDF */
	      MoveCursor(LINES,0);
	      Raw(OFF);
	      printf(catgets(elm_msg_cat, ElmSet, ElmFolderCorrupt,
		  "\nFolder is corrupt!!  I can't read it!!\n\n"));
	      fflush(stderr);
	      dprint(1, (debugfile, 
			   "\n**** First mail header is corrupt!! ****\n"));
	      dprint(1, (debugfile, "Line is;\n\t%s\n\n", buffer));
              leave(0);
	    }
	  }

	  if (content_remaining <= 0) {
#ifdef MMDF
	    if (strcmp(buffer, MSG_SEPARATOR) == 0) {
              newheader = !newheader;
#else
	    if (first_word(buffer,"From ")) {
#endif /* MMDF */
	      /** allocate new header pointers, if needed... **/

	      if (count >= max_headers) {
		struct header_rec **new_headers;
		int new_max;

		new_max = max_headers + KLICK;
		if (max_headers == 0) {
		  new_headers = (struct header_rec **)
		    malloc(new_max * sizeof(struct header_rec *));
		}
		else {
		  new_headers = (struct header_rec **)
		    realloc((char *) headers,
			new_max * sizeof(struct header_rec *));
		}
		if (new_headers == NULL) {
	          MoveCursor(LINES,0);
	          Raw(OFF);
		  printf(catgets(elm_msg_cat, ElmSet, ElmCouldntAllocMemory,
	"\n\nCouldn't allocate enough memory! Message #%d.\n\n"),
			  count);
		  leave(0);
		}
		headers = new_headers;
		while (max_headers < new_max)
		  headers[max_headers++] = NULL;
	      }
	      
	      /** allocate new header structure, if needed... **/

	      if (headers[count] == NULL) {
		struct header_rec *h;

		if ((h = (struct header_rec *)
			  malloc(sizeof(struct header_rec))) == NULL) {
	          MoveCursor(LINES,0);
	          Raw(OFF);
		  printf(catgets(elm_msg_cat, ElmSet, ElmCouldntAllocMemory,
	"\n\nCouldn't allocate enough memory! Message #%d.\n\n"),
			  count);
		  leave(0);
		}
		headers[count] = h;
	      }

	      dprint(1, (debugfile, 
		   "\n**** Calling real_from for \"From_\" ****\n"));
	      if (real_from(buffer, headers[count])) {

	        dprint(1, (debugfile, "\ncontent_remaining = %ld, content_start = %ld, lines_start = %ld, fbytes = %ld\n",
			content_remaining, content_start, lines_start, fbytes));

		current_header = headers[count];

		current_header->offset = (long) fbytes;
		current_header->content_length = -1; /* not found yet */
		current_header->index_number = count+1;
		content_length_found = FALSE;
		/* set default status - always 'visible'  - and
		 * if a spool file, presume 'new', otherwise
		 * 'read', for the time being until overridden
		 * by a Status: header.
		 * We presume 'read' for nonspool mailfile messages
		 * to be compatible messages stored with older versions of elm,
		 * which didn't support a Status: header.
		 */
		if(folder_type == SPOOL)
		  current_header->status = VISIBLE | NEW | UNREAD;
		else
		  current_header->status = VISIBLE;

		strcpy(current_header->subject, "");	/* clear subj    */
		strcpy(current_header->to, "");		/* clear to    */
		strcpy(current_header->mailx_status, "");	/* clear status flags */
		strcpy(current_header->time_menu, "");	/* clear menu date */
		strcpy(current_header->messageid, "<no.id>"); /* set no id into message id */
		current_header->encrypted = 0;		/* clear encrypted */
		current_header->exit_disposition = UNSET;
		current_header->status_chgd = FALSE;

		/* Set the number of lines for the _preceding_ message,
		 * but only if there was a preceding message and
		 * only if it wasn't calculated already. It would
		 * have been calculated already if we are only
		 * reading headers of new messages that have just arrived,
		 * and the preceding message was one of the old ones.
		 */
		if ((count) && (!add_new_only || count > message_count)) {
		  headers[count-1]->lines = line;
		  if (headers[count-1]->content_length < 0)
		    headers[count-1]->content_length = fbytes - content_start;
		}

		count++;
		subj = 0;
		line = 0;
		in_header = TRUE;
		if (count % readmsginc == 0)
		  PutLine1(count_x, count_y, "%d", count);
#ifdef MMDF
	      } else if (newheader) {
		current_header = headers[count];

		current_header->offset = (long) fbytes;
		current_header->content_length = -1; /* not found yet */
		current_header->index_number = count+1;

		/* set default status - always 'visible'  - and
		 * if a spool file, presume 'new', otherwise
		 * 'read', for the time being until overridden
		 * by a Status: header.
		 * We presume 'read' for nonspool mailfile messages
		 * to be compatible messages stored with older versions of elm,
		 * which didn't support a Status: header.
		 */
		if(folder_type == SPOOL)
		  current_header->status = VISIBLE | NEW | UNREAD;
		else
		  current_header->status = VISIBLE;

		strcpy(current_header->from, "");		/* clear from    */
		strcpy(current_header->subject, "");	/* clear subj    */
		strcpy(current_header->to, "");		/* clear to    */
		strcpy(current_header->time_zone, "");	/* clear time zone name */
		strcpy(current_header->time_menu, "");	/* clear menu date */
		strcpy(current_header->mailx_status, "");	/* clear status flags */
		strcpy(current_header->messageid, "<no.id>"); /* set no id into message id */
		current_header->encrypted = 0;		/* clear encrypted */
		current_header->exit_disposition = UNSET;
		current_header->status_chgd = FALSE;

		/* Set the number of lines for the _preceding_ message,
		 * but only if there was a preceding message and
		 * only if it wasn't calculated already. It would
		 * have been calculated already if we are only
		 * reading headers of new messages that have just arrived,
		 * and the preceding message was one of the old ones.
		 */
		if (count && (!add_new_only || (count > message_count))) {
		  headers[count-1]->lines = line;
		  if (headers[count-1]->content_length < 0)
		    headers[count-1]->content_length = fbytes - content_start;
		}

		count++;
		subj = 0;
		line = 0;
		in_header = TRUE;
		if (count % readmsginc == 0)
		  PutLine1(count_x, count_y, "%d", count);
		dprint(1, (debugfile, 
			     "\n**** Added header record ****\n"));
#endif /* MMDF */
	      }
	    } else if (!forwarding_mail && count == 0) {
	      /* if this is the first "From" in file but the "From" line is
	       * not of the proper format, we've got a corrupt folder.
	       */
	      MoveCursor(LINES,0);
	      Raw(OFF);
	      printf(catgets(elm_msg_cat, ElmSet, ElmFolderCorrupt,
		  "\nFolder is corrupt!!  I can't read it!!\n\n"));
	      fflush(stderr);
	      dprint(1, (debugfile, 
			   "\n**** First mail header is corrupt!! ****\n"));
	      dprint(1, (debugfile, "Line is;\n\t%s\n\n", buffer));
	      leave(0);
	    } else if (in_header == FALSE && content_length_found == TRUE && line_bytes > 1) {
		/* invalid content length, skip back to beginning of
		 * this messages text and ignore the content length
		 * field.  This requires restoring the current position
		 * in the spool file and the number of lines in the
		 * message.
		 */
	      if (fseek(mailfile, content_start, 0) == -1) {
		err = errno;
		MoveCursor(LINES, 0);
		Raw(OFF);
		printf(catgets(elm_msg_cat, ElmSet, ElmCouldntSeekBytesIntoFolder,
		   "\nCouldn't seek %ld bytes into folder.\n"),
		   mailfile_size);	
		printf("** %s. **\n", 1, error_description(err));
		dprint(1, (debugfile,
      "Error: Couldn't seek folder %s: (offset %ld) Errno %s (%s)\n",
		   cur_folder, mailfile_size, error_description(err), "reset - read_headers"));
		emergency_exit();
	      }
	      if (copyit)
		if (fseek(temp, content_start, 0) == -1) {
		  err = errno;
		  MoveCursor(LINES, 0);
		  Raw(OFF);
		  printf(catgets(elm_msg_cat, ElmSet, ElmCouldntSeekBytesIntoTempFile,
		     "\nCouldn't seek %ld bytes into temp file.\n"),
		     mailfile_size);
		  printf("** %s. **\n", error_description(err));
		  dprint(1, (debugfile,
	"Error: Couldn't seek temp file %s: (offset %ld) Errno %s (%s)\n",
		     cur_tempfolder, mailfile_size, error_description(err), "reset - read_headers"));
		  emergency_exit();
		}
	      fbytes = content_start;
	      line = lines_start;
	      content_length_found = FALSE;
	      current_header->content_length = -1; /* mark as if not found yet */
	      line_bytes = 0;
	    }
	  }

	  if (in_header == TRUE) {
#ifdef MMDF
	    if (strcmp(buffer, MSG_SEPARATOR) == 0) {
	      /*
	       * This is disgusting, but the code below "knows" that
	       * headers all have colons in them.  However, the MMDF
	       * message separator is a header without a colon.  So,
	       * we put one there to make the rest of the logic work.
	       */
	      buffer[0] = ':';
	    } else if (first_word(buffer,"From ")) {
	      dprint(1, (debugfile, 
		   "\n**** Calling real_from for \"From_\" ****\n"));
	      real_from(buffer, current_header);
	    } else
#endif /* MMDF */
	    if (header_cmp(buffer,">From", NULL))
	      parse_arpa_who(buffer, current_header->from, FALSE);
	    else if (first_word_nc(buffer,">From")) 
	      forwarded(buffer, current_header); /* return address */
	    else if (header_cmp(buffer,"Subject", NULL) ||
		     header_cmp(buffer,"Subj", NULL) ||
		     header_cmp(buffer,"Re", NULL)) {
	      if (! subj++) {
		strfcpy(tbuffer, buffer, sizeof(tbuffer));
	        remove_header_keyword(tbuffer);
	        copy_sans_escape(current_header->subject, tbuffer, STRING);
		remove_possible_trailing_spaces(current_header->subject);
	      }
	    }
	    else if (header_cmp(buffer,"From", NULL)) {
	      dprint(1, (debugfile, 
		   "\n**** Calling parse_arpa_who for \"From\" ****\n"));
#ifdef MMDF
	      parse_arpa_who(buffer, current_header->from, TRUE);
#else
	      parse_arpa_who(buffer, current_header->from, FALSE);
#endif /* MMDF */

	    }
	    else if (header_cmp(buffer, "Message-Id", NULL)) {
	      buffer[line_bytes - 1] = '\0';
	      strcpy(current_header->messageid,
		     (char *) buffer + 12);
	    }

	    else if (header_cmp(buffer, "Content-Length", NULL)) {
	      buffer[line_bytes - 1] = '\0';
	      current_header->content_length = atol((char *) buffer + 15);
	      /* Check if content_length is > remaining size of file */
	      if (current_header->content_length > mailfile_size-fbytes)
		current_header->content_length = -1;
	      else
	        content_length_found = TRUE;

	    }

	    else if (header_cmp(buffer, "Expires", NULL))
	      process_expiration_date((char *) buffer + 9, 
				      &(current_header->status));
	    
	    /** when it was sent... **/

	    else if (header_cmp(buffer, "Date", NULL)) {
	      dprint(1, (debugfile, 
		      "\n**** Calling parse_arpa_date for \"Date\" ****\n"));
	      strfcpy(tbuffer, buffer, sizeof(tbuffer));
	      remove_header_keyword(tbuffer);
	      parse_arpa_date(tbuffer, current_header);
	    }

	    /** some status things about the message... **/

	    else if ((header_cmp(buffer, "Priority", NULL) ||
		     header_cmp(buffer, "Importance", "2")) &&
		   !(header_cmp(buffer, "priority", "normal") ||
		     header_cmp(buffer, "priority", "non-urgent")))
	      current_header->status |= URGENT;
	    else if (header_cmp(buffer, "Sensitivity", "2"))
	      current_header->status |= PRIVATE;
	    else if (header_cmp(buffer, "Sensitivity", "3"))
	      current_header->status |= CONFIDENTIAL;
	    else if (header_cmp(buffer, "Content-Type", "mailform"))
	      current_header->status |= FORM_LETTER;
	    else if (header_cmp(buffer, "Action", NULL))
	      current_header->status |= ACTION;
#ifdef	MIME
	    else if (header_cmp(buffer, MIME_HEADER_NAME, MIME_HEADER_VERSION))
	      current_header->status |= MIME_MESSAGE;
	    /* Next two lines for backward compatability to old drafts */
	    else if (header_cmp(buffer, MIME_HEADER_NAME, MIME_HEADER_OLDVERSION))
	      current_header->status |= MIME_MESSAGE;
	    /* Next two lines for minimal support for mail generated by
	     * Sun's Openwindows mailtool */
	    else if (header_cmp(buffer, "Content-Type", "X-"))
	      current_header->status |= MIME_MESSAGE|MIME_NOTPLAIN;
	    else if (header_cmp(buffer, MIME_HEADER_CONTENTTYPE, NULL)) {
	      if (! (current_header->status & MIME_NOTPLAIN) &&
		  notplain(buffer+strlen(MIME_CONTENTTYPE)))
		current_header->status |= MIME_NOTPLAIN;
	    }
	    else if (header_cmp(buffer, MIME_HEADER_CONTENTENCOD, NULL)) {
	      if (needs_mmdecode(buffer+strlen(MIME_CONTENTENCOD)))
		current_header->status |= MIME_NEEDDECOD;
	    }
#endif /* MIME */

	    /** next let's see if it's to us or not... **/

	    else if (header_cmp(buffer, "To", NULL)) {
	      in_to_list = TRUE;
	      current_header->to[0] = '\0';	/* nothing yet */
	      figure_out_addressee((char *) buffer +3, 
				   current_header->to);
	    }
	    else if (header_cmp(buffer, "Status", NULL)) {
	      strfcpy(tbuffer, buffer, sizeof(tbuffer));
	      remove_header_keyword(tbuffer);
	      strfcpy(current_header->mailx_status, tbuffer, WLEN);

	      c = index(current_header->mailx_status, '\n');
	      if (c != NULL)
		*c = '\0';
	      remove_possible_trailing_spaces(current_header->mailx_status);

	      /* Okay readjust the status. If there's an 'R', message
	       * is read; if there is no 'R' but there is an 'O', message
	       * is unread. In any case it isn't new because a new message
	       * wouldn't have a Status: header.
	       */
	      if (index(current_header->mailx_status, 'R') != NULL)
		current_header->status &= ~(NEW | UNREAD);
	      else if (index(current_header->mailx_status, 'O') != NULL) {
		current_header->status &= ~NEW;
		current_header->status |= UNREAD;
	      }
	    }

	    else if (buffer[0] == LINE_FEED || buffer[0] == '\0') {
	      in_header = FALSE;	/* in body of message! */
	      in_to_list = FALSE;
	      content_remaining = current_header->content_length;
	      content_start = fbytes + 1;
	      lines_start = line;
	    }
	    if (in_header == TRUE) {
	       if ((!whitespace(buffer[0])) && index(buffer, ':') == NULL) {
	        in_header = FALSE;	/* in body of message! */
	        in_to_list = FALSE;
	        content_remaining = current_header->content_length;
	        content_start = fbytes;
	        lines_start = line;
	      }
	    }
	    if (in_to_list == TRUE) {
	      if (whitespace(buffer[0]))
	        figure_out_addressee(buffer, current_header->to);
	      else in_to_list = FALSE;
	    }
	  }
	  if (in_header == FALSE && first_word(buffer, START_ENCODE))
	    current_header->encrypted = 1;
	  fbytes += (long) line_bytes;
	  content_remaining -= (long) line_bytes;
	}

	if (count) {
	  headers[count-1]->lines = line + 1;
	  if (headers[count-1]->content_length < 0)
	    headers[count-1]->content_length = fbytes - content_start;
	}

	if (folder_type == SPOOL) {
	  unlock();	/* remove lock file! */
	  if ((ferror(mailfile)) || (fclose(mailfile) == EOF)) {
	      err = errno;
	      MoveCursor(LINES, 0);
	      Raw(OFF);
	      printf(catgets(elm_msg_cat, ElmSet, ElmCloseOnFolderFailed,
			      "\nClose on folder %s failed!!\n"),
			      cur_folder);
	      printf("** %s. **\n", error_description(err));
	      dprint(1, (debugfile, "Can't close on folder %s!!\n",
			 cur_folder));
	      rm_temps_exit();
	  }
	  if ((ferror(temp)) || (fclose(temp) == EOF)) {
	      err = errno;
	      MoveCursor(LINES, 0);
	      Raw(OFF);
	      printf(catgets(elm_msg_cat, ElmSet, ElmCloseOnTempFailed,
			      "\nClose on temp file %s failed!!\n"), 
			      cur_tempfolder);
	      printf("** %s. **\n", error_description(err));
	      dprint(1, (debugfile, "Can't close on temp file %s!!\n",
			 cur_tempfolder));
	      rm_temps_exit();
	  }
	  /* sanity check on append - is resulting temp file longer??? */
	  if ( bytes(cur_tempfolder) != mailfile_size) {
	     MoveCursor(LINES, 0);
	     Raw(OFF);
	     printf(catgets(elm_msg_cat, ElmSet, ElmLengthNESpool,
	       "\nnewmbox - length of mbox. != spool mailbox length!!\n"));
	    dprint(1, (debugfile, "newmbox - mbox. != spool mail length"));
	    rm_temps_exit();
	  }
	  if ((mailfile = fopen(cur_tempfolder,"r")) == NULL) {
	    err = errno;
	    MoveCursor(LINES,0);
	    Raw(OFF);
	    printf(catgets(elm_msg_cat, ElmSet, ElmCouldntReopenForTemp,
		   "\nCouldn't reopen file %s for use as temp file.\n"),
	           cur_tempfolder);
	    printf("** %s. **\n", error_description(err));
	    dprint(1, (debugfile,
		  "Error: Reopening %s as temp file failed!  errno %s (%s)\n",
	           cur_tempfolder, error_description(err), "read_headers"));
	    leave(0);
	  }
	}
	else 
          rewind(mailfile);

	/* Sort folder *before* we establish the current message, so that
	 * the current message is based on the post-sort order.
	 * Note that we have to set the global variable message_count
	 * before the sort for the sort to correctly keep the correct
	 * current message if we are only adding new messages here. */

	message_count = count;
	sort_mailbox(count, 1);

	/* Now lets figure what the current message should be.
	 * If we are only reading in newly added messages from a mailfile
	 * that already had some messages, current should remain the same.
	 * If we have a folder of no messages, current should be zero.
	 * Otherwise, if we have point_to_new on then the current message
	 * is the first message of status NEW if there is one.
	 * If we don't have point_to_new on or if there are no messages of
	 * of status NEW, then the current message is the first message.
	 */
	if(!(add_new_only && current != 0)) {
	  if(count == 0)
	    current = 0;
	  else {
	    current = 1;
	    if (point_to_new) {
	      for(another_count = 0; another_count < count; another_count++) {
		if(ison(headers[another_count]->status, NEW)) {
		  current = another_count+1;
		  goto found_new;
		}
	      }
	      for(another_count = 0; another_count < count; another_count++) {
		if(ison(headers[another_count]->status, UNREAD)) {
		  current = another_count+1;
		  goto found_new;
		}
	      }
	      switch (sortby) {
		case SENT_DATE:
		case RECEIVED_DATE:
		case MAILBOX_ORDER:
		  current = count;
	      }
	      found_new: ;
	    }
	  }
	}
        get_page(current);
	return(count);
}
