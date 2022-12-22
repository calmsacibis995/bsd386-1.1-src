
static char rcsid[] = "@(#)Id: savecopy.c,v 5.12 1993/05/31 19:35:24 syd Exp ";

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
 * $Log: savecopy.c,v $
 * Revision 1.2  1994/01/14  00:55:41  cp
 * 2.4.23
 *
 * Revision 5.12  1993/05/31  19:35:24  syd
 * Move the actual file saving code out of "save_copy()" into a seperate
 * routine called "append_copy_to_file()" to make it globally available.
 * In particular the "Canceled.mail" handling is going to be rewritten
 * to use it.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.11  1993/05/08  20:25:33  syd
 * Add sleepmsg to control transient message delays
 * From: Syd
 *
 * Revision 5.10  1993/04/12  02:34:36  syd
 * I have now added a parameter which controls whether want_to clears the
 * line and centers the question or behaves like it did before. I also
 * added a 0 at the end of the parameter list to all the other calls to
 * want_to where a centered question on a clean line is not desirable.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.9  1993/02/03  17:12:53  syd
 * move more declarations to defs.h, including sleep
 * From: Syd
 *
 * Revision 5.8  1993/01/19  04:55:10  syd
 * fix which file name is used on confirm messages
 * From: vogt@isa.de (Gerald Vogt)
 *
 * Revision 5.7  1993/01/19  03:55:39  syd
 * exitprog.c makes a reference to a null character pointer, savecopy.c
 * tries to reference an uninitialized variable, and the previous patch to
 * src/lock.c to get rid of an uninitialized variable compiler message
 * needed to be put in filter/lock.c as well.
 * From: wdh@grouper.mkt.csd.harris.com (W. David Higgins)
 *
 * Revision 5.6  1992/12/25  00:18:10  syd
 * Remove editing garbage from end
 * From: Syd
 *
 * Revision 5.5  1992/12/24  21:42:01  syd
 * Fix messages and nls messages to match.  Plus use want_to
 * where appropriate.
 * From: Syd, via prompting from Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.4  1992/11/26  00:46:50  syd
 * Fix how errno is used so err is inited and used instead
 * as errno gets overwritten by print system call
 * From: Syd
 *
 * Revision 5.3  1992/10/30  21:01:49  syd
 * More changes to folder creation confirmation
 * From: Larry Philps <larryp@sco.COM>
 *
 * Revision 5.2  1992/10/24  13:25:41  syd
 * In our global elm.rc I keep the four options as below
 *
 * 	confirmappend = OFF	Don't confirm every append to any file.
 * 	confirmcreate = ON	Confirm creation of every new file.
 * 	confirmfiles = ON	Confirm append to non folder files though.
 * 	confirmfolders = ON	In case someone does not want to be asked
 * 				every time when creating a new file try
 * 				to confirm creation of folders though.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** Save a copy of the specified message in a folder.

**/

#include "headers.h"
#include "s_elm.h"
#ifdef I_TIME
#  include <time.h>
#endif
#ifdef I_SYSTIME
#  include <sys/time.h>
#endif

#include <errno.h>

char *format_long(), *error_description(), *ctime();

extern int errno;

#ifdef MIME
extern int msg_is_multipart;
#endif
extern long C_L_Position[2];   /*To Remember position of the Content-Length*/
extern long C_StartData[2];    /*To Remember length of Header Area */
extern long C_EndData[2];   /* To Remeber the End of the Data */


/*
 * save_copy() - Append a copy of the message contained in "filename" to
 * the file specified by "copy_file".  This routine simply gets all of
 * the filenames right, and then invokes "append_copy_to_file()" to do
 * the dirty work.
 */
save_copy(to, cc, bcc, filename, copy_file, form)
char *to, *cc, *bcc, *filename, *copy_file;
int form;
{
	char  buffer[SLEN],	/* read buffer 		       */
	      savename[SLEN],	/* name of file saving into    */
	      msg_buffer[SLEN], answer;
	register int
	     is_ordinary_file;
	int  err;
  
	/* presume copy_file is okay as is for now */
	strcpy(savename, copy_file);

	/* if save-by-name wanted */
	if((strcmp(copy_file, "=") == 0)  || (strcmp(copy_file, "=?") == 0)) {

	  get_return_name(to, buffer, TRUE);	/* determine 'to' login */
	  if (strlen(buffer) == 0) {

	    /* can't get file name from 'to' -- use sent_mail instead */
	    dprint(3, (debugfile,
		"Warning: get_return_name couldn't break down %s\n", to));
	    error1(catgets(elm_msg_cat, ElmSet, ElmCannotDetermineToName,
"Cannot determine `to' name to save by! Saving to \"sent\" folder %s instead."),
	      sent_mail);
	    strcpy(savename, "<");
	    if (sleepmsg > 0)
		sleep(sleepmsg);
	  } else
	    sprintf(savename, "=%s", buffer);		/* good! */
	}

	expand_filename(savename, TRUE);	/* expand special chars */

	/*
	 *  If saving conditionally by logname but folder doesn't
	 *  exist save to sent folder instead.
	 */
	if((strcmp(copy_file, "=?") == 0)
	      && (access(savename, ACCESS_EXISTS) != 0)) {
	  dprint(5, (debugfile,
	    "Conditional save by name: file %s doesn't exist - using \"<\".\n",
	    savename));
	  strcpy(savename, "<");
	  expand_filename(savename, TRUE);
	}

	/*
	 *  Allow options
	 *  confirm_files, confirm_folders,
	 *  confirm_append and confirm_create
	 *  to control where the actual copy
	 *  should be saved.
	 */
	is_ordinary_file = strncmp (savename, folders, strlen(folders));

        if (access(savename, ACCESS_EXISTS)== 0) {	/* already there!! */
	    if (confirm_append || (confirm_files && is_ordinary_file)) {
		/*
		 *  OK in batch mode it may be impossible
		 *  to ask the user to confirm. So we have
		 *  to use sent_mail anyway.
		 */
		if (batch_only) {
		    strcpy(savename, sent_mail);
		}
		else {
		    if (is_ordinary_file)
		      MCsprintf(msg_buffer, catgets(elm_msg_cat, ElmSet, ElmConfirmFilesAppend,
			  "Append to an existing file `%s'? (%c/%c) "),
			savename, *def_ans_yes, *def_ans_no);
		    else
		      MCsprintf(msg_buffer, catgets(elm_msg_cat, ElmSet, ElmConfirmFolderAppend,
			  "Append to mail folder `%s'? (%c/%c) "),
			savename, *def_ans_yes, *def_ans_no);

		    answer = want_to(msg_buffer, *def_ans_no, LINES-2, 1);

		    if (answer != *def_ans_yes) {
			strcpy(savename, sent_mail);
			PutLine1 (LINES-2, 0, catgets(elm_msg_cat, ElmSet, ElmSavingToInstead,
				  "Alright - saving to `%s' instead"),
				  savename);
			if (sleepmsg > 0)
				sleep(sleepmsg);
			ClearLine (LINES-2);
		    }
		}
	    }
	}
        else {
            if (confirm_create || (confirm_folders && !is_ordinary_file)) {
		/*
		 *  OK in batch mode it may be impossible
		 *  to ask the user to confirm. So we have
		 *  to use sent_mail anyway.
		 */
		if (batch_only) {
		    strcpy(savename, sent_mail);
		}
		else {
		    if (is_ordinary_file)
		      MCsprintf(msg_buffer, catgets(elm_msg_cat, ElmSet, ElmConfirmFilesCreate,
			  "Create a new file `%s'? (%c/%c) "),
			savename, *def_ans_yes, *def_ans_no);
		    else
		      MCsprintf(msg_buffer, catgets(elm_msg_cat, ElmSet, ElmConfirmFolderCreate,
			  "Create a new mail folder `%s'? (%c/%c) "),
			savename, *def_ans_yes, *def_ans_no);

		    answer = want_to(msg_buffer, *def_ans_no, LINES-2, 1);

		    if (answer != *def_ans_yes) {
			strcpy(savename, sent_mail);
			PutLine1 (LINES-2, 0, catgets(elm_msg_cat, ElmSet, ElmSavingToInstead,
				  "Alright - saving to `%s' instead"),
				  savename);
			if (sleepmsg > 0)
				sleep(sleepmsg);
			ClearLine (LINES-2);
		    }
		}
	    }
	}

	if ((err = can_open(savename, "a"))) {
	  dprint(2, (debugfile,
	  "Error: attempt to autosave to a file that can't be appended to!\n"));
	  dprint(2, (debugfile, "\tfilename = \"%s\"\n", savename));
	  dprint(2, (debugfile, "** %s **\n", error_description(err)));

	  /* Lets try sent_mail before giving up */
	  if(strcmp(sent_mail, savename) == 0) {
	    /* we are ALREADY using sent_mail! */
	    error1(catgets(elm_msg_cat, ElmSet, ElmCannotSaveTo,
			"Cannot save to %s!"), savename);
	    if (sleepmsg > 0)
		    sleep(sleepmsg);
	    return(FALSE);
	  }

	  if ((err = can_open(sent_mail, "a"))) {
	    dprint(2, (debugfile,
	  "Error: attempt to autosave to a file that can't be appended to!\n"));
	    dprint(2, (debugfile, "\tfilename = \"%s\"\n", sent_mail));
	    dprint(2, (debugfile, "** %s **\n", error_description(err)));
	    error2(catgets(elm_msg_cat, ElmSet, ElmCannotSaveToNorSent,
		    "Cannot save to %s nor to \"sent\" folder %s!"),
		    savename, sent_mail);
	    if (sleepmsg > 0)
		    sleep(sleepmsg);
	    return(FALSE);
	  }
	  error2(catgets(elm_msg_cat, ElmSet, ElmCannotSaveToSavingInstead,
		"Cannot save to %s! Saving to \"sent\" folder %s instead."),
	        savename, sent_mail);
	  if (sleepmsg > 0)
		sleep(sleepmsg);
	  strcpy(savename, sent_mail);
	}

	return (append_copy_to_file(to, cc, bcc, savename, filename, form)==0);
}

char *
cf_english(fn)
char *fn;
{
    /** Return "English" expansion for special copy file name abbreviations
	or just the file name  **/

    if(!*fn)
      return(catgets(elm_msg_cat, ElmSet, ElmNoSave, "<no save>"));
    else if(!fn[1]) {
      if(*fn == '=')
	return(catgets(elm_msg_cat, ElmSet, ElmUncondSaveByName, "<unconditionally save by name>"));
      else if(*fn == '<')
	return(catgets(elm_msg_cat, ElmSet, ElmSentFolder, "<\"sent\" folder>"));
    } else if ((fn[0] == '=') && (fn[1] == '?'))
      return(catgets(elm_msg_cat, ElmSet, ElmCondSaveByName, "<conditionally save by name>"));

    return(fn);
}

static char *ncf_prompt = NULL;
int
name_copy_file(fn)
char *fn;
{
    /** Prompt user for name of file for saving copy of outbound msg to.
	Return true if we need a redraw. **/

    int redraw = 0;	/* set when we ask for help = need redraw */
    char buffer[SLEN], origbuffer[SLEN];
    static char helpmsg[LONG_STRING];


    if (ncf_prompt == NULL) {
	ncf_prompt = catgets(elm_msg_cat, ElmSet, ElmSaveCopyInPrompt,
			"Save copy in (use '?' for help/to list folders): ");
    }
    /* expand passed copy file name into English */
    strcpy(buffer, cf_english(fn));

    /* prepare screen with instructions */
    MoveCursor(LINES-2, 0);
    CleartoEOS();
    PutLine0(LINES-2, 0, ncf_prompt);

    while(1) {

      /* get file name from user input */
      strcpy(origbuffer, buffer);
      optionally_enter(buffer, LINES-2, strlen(ncf_prompt), FALSE, FALSE);

      if(strcmp(buffer, "?") != 0) { /* got what we wanted - non-help choice */

	if(strcmp(origbuffer, buffer) != 0)
	  /* user changed from our English expansion 
	   * so we'd better copy user input to fn
	   */
	  strcpy(fn, buffer);

	/* else user presumably left our English expansion - no change in fn */

	/* display English expansion of new user input a while */
	PutLine1(LINES-2, strlen(ncf_prompt), cf_english(fn));
	MoveCursor(LINES, 0);
	if (sleepmsg > 0)
		sleep((sleepmsg + 1) / 2);
	MoveCursor(LINES-2, 0);
	CleartoEOS();

	return(redraw);
      }

      /* give help and list folders */
      redraw = TRUE;
      if(!*helpmsg) {	/* help message not yet formulated */
	strcpy(helpmsg, catgets(elm_msg_cat, ElmSet, ElmListFoldersHelp1,
"Enter: <nothing> to not save a copy of the message,\n\
\r       '<'       to save in your \"sent\" folder ("));
	strcat(helpmsg, sent_mail);
	strcat(helpmsg, catgets(elm_msg_cat, ElmSet, ElmListFoldersHelp2,
"),\n\
\r       '='       to save by name (the folder name depends on whom the\n\
\r                     message is to, in the end),\n\
\r       '=?'      to save by name if the folder already exists,\n\
\r                     and if not, to your \"sent\" folder,\n\
\r       or a filename (a leading '=' denotes your folder directory).\n\r\n\r"));
      }

      list_folders(4, helpmsg, NULL);
      PutLine0(LINES-2, 0, ncf_prompt);

      /* restore as default to English version of the passed copy file name */
      strcpy(buffer, cf_english(fn));

    }
}


int
append_copy_to_file(to, cc, bcc, fname_copy, fname_mssg, form)
char *to;
char *cc;
char *bcc;
char *fname_copy;
char *fname_mssg;
int form;
{
    int err;
    FILE *fp_copy, *fp_mssg;
    extern FILE *write_header_info();

    /* open up the file with the message */
    if ((fp_mssg = fopen(fname_mssg, "r")) == NULL) {
	err = errno;
	dprint(1, (debugfile,
	    "Error: Couldn't open \"%s\" for read in append_copy_to_file\n",
	    fname_mssg));
	dprint(1, (debugfile, "** %s **\n", error_description(err)));
	error1(catgets(elm_msg_cat, ElmSet, ElmAppendCopyCouldntReadFile,
	    "Couldn't read file \"%s\"!"), fname_mssg);
	if (sleepmsg > 0)
	    sleep(sleepmsg);
	return -1;
    }

    /* dump the header to the end of the copy file */
    save_file_stats(fname_copy);
    fp_copy = write_header_info(fname_copy, to, cc, bcc, (form == YES), TRUE);
    if (fp_copy == NULL) {
	(void) fclose(fp_mssg);
	return -1;
    }
    restore_file_stats(fname_copy);
    C_StartData[0] = ftell(fp_copy);

    /* dump the contents of the message to the end of the copy file */
    copy_message_across(fp_mssg, fp_copy, TRUE);

#ifdef MIME
    if (!form != NO && msg_is_multipart) {
	fprintf(fp_copy, "--%%#%%record%%#%%--\n");
	if (C_L_Position[1] != 0L) {
	    C_EndData[1] = ftell(fp_copy);
	    C_L_Position[1] = fseek(fp_copy, C_L_Position[1], 0);
	    fprintf(fp_copy, "%d", C_EndData[1] - C_StartData[1]);
	    fseek(fp_copy, C_EndData[1], 0);
	}
    }
#endif
    C_EndData[0] = ftell(fp_copy) ;

#ifdef MMDF
    /*
     * Actually, the C_EndData just calculated is wrong for MMDF.
     * Because we are saving a copy instead of handing off to
     * submit, copy_message_across will have added the trailing
     * MMDF MSG_SEPARATOR to the end of the saved message to ensure
     * a valid mailbox format.  We *must not* count that
     * MSG_SEPARATOR when calculating the size of the message for
     * Content-Length header!  In order to keep the hack for this
     * localized to this function, we will just subtract off the
     * length of the MSG_SEPARATOR.
     */
    C_EndData[0] -= strlen(MSG_SEPARATOR);
#endif /* MMDF */

    /* go fixup the content length header */
    fseek(fp_copy, C_L_Position[0], 0);
    fprintf(fp_copy, "%d", C_EndData[0] - C_StartData[0]);

    /* copy successfully done */
    fclose(fp_copy);
    fclose(fp_mssg);
    return 0;
}

