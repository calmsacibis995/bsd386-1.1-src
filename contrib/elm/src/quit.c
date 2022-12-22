
static char rcsid[] = "@(#)Id: quit.c,v 5.4 1992/11/26 00:46:13 syd Exp ";

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
 * $Log: quit.c,v $
 * Revision 1.2  1994/01/14  00:55:30  cp
 * 2.4.23
 *
 * Revision 5.4  1992/11/26  00:46:13  syd
 * changes to first change screen back (Raw off) and then issue final
 * error message.
 * From: Syd
 *
 * Revision 5.3  1992/10/24  13:35:39  syd
 * changes found by using codecenter on Elm 2.4.3
 * From: Graham Hudspith <gwh@inmos.co.uk>
 *
 * Revision 5.2  1992/10/17  22:47:09  syd
 * adds the function bytemap() and the macros MAPIN and MAPOUT from the file
 * lib/ndbz.c in the file src/alias.c.
 *
 * prevent elm from exiting when resyncing the empty incoming mailbox.
 * From: vogt@isa.de (Gerald Vogt)
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** quit: leave the current folder and quit the program.
  
**/

#include "headers.h"
#include "s_elm.h"
#include <errno.h>

extern int errno;		/* system error number on failure */

long bytes();
char *error_description();
extern void init_helpmsg();

quit(prompt)
int prompt;
{
	/* a wonderfully short routine!! */

	if (leave_mbox(FALSE, TRUE, prompt) == -1)
	  /* new mail - leave not done - can't change to another file yet
	   * check for change in mailfile_size in main() will do the work
	   * of calling newmbox to add in the new messages to the current
	   * file and fix the sorting sequence that leave_mbox may have
	   * changed for its own purposes */
	  return;

	leave(0);
}

int
resync()
{
	/** Resync on the current folder. Leave current and read it back in.
	    Return indicates whether a redraw of the screen is needed.
	 **/
	int  err;

	  if(leave_mbox(TRUE, FALSE, TRUE) ==-1)
	    /* new mail - leave not done - can't change to another file yet
	     * check for change in mailfile_size in main() will do the work
	     * of calling newmbox to add in the new messages to the current
	     * file and fix the sorting sequence that leave_mbox may have
	     * changed for its own purposes */
	    return(FALSE);

	  if ((errno = can_access(cur_folder, READ_ACCESS)) != 0) {
	    if (strcmp(cur_folder, defaultfile) != 0 || errno != ENOENT) {
	      err = errno;
	      MoveCursor(LINES, 0);
	      Raw(OFF);
	      dprint(1, (debugfile,
		     "Error: given file %s as folder - unreadable (%s)!\n", 
		     cur_folder, error_description(err)));
	      fprintf(stderr, catgets(elm_msg_cat, ElmSet, ElmCantOpenFolderRead,
			"Can't open folder '%s' for reading!\n"), cur_folder);
	      leave(0);
	    }
	  }

	  newmbox(cur_folder, FALSE);
	  return(TRUE);
}

char helpmsg[VERY_LONG_STRING];

change_file()
{
	  /* Prompt user for name of folder to change to.
	   * If all okay with that folder, leave the current folder.
	   * If leave goes okay (i.e. no new messages in current folder),
	   * change to the folder that the user specified.
	   *
	   * Return value indicates whether a redraw is needed.
	   */

	  int redraw = FALSE;
	  char newfile[SLEN];

	  char	*nameof();


	  /* get new file name */

	  MoveCursor(LINES-3, 30);
	  CleartoEOS();
	  PutLine0(LINES-3, 38, catgets(elm_msg_cat, ElmSet, ElmUseForHelp,
		"(Use '?' for help)"));
	  PutLine0(LINES-2,0, catgets(elm_msg_cat, ElmSet, ElmChangeToWhichFolder,
		"Change to which folder: "));
	  while(1) {
	    newfile[0] = '\0';
	    (void) optionally_enter(newfile, LINES-2, 24, FALSE, FALSE);
	    clear_error();

	    if(*newfile == '\0') {	/* if user didn't enter a file name */
	      MoveCursor(LINES-3, 30);	/* abort changing file process */
	      CleartoEOS();
	      return(redraw);

	    }
	    if (strcmp(newfile, "?") == 0) {

	      /* user wants to list folders */
	      if(!*helpmsg) 	/* format helpmsg if not yet done */
		init_helpmsg( helpmsg, change_word, FALSE );
	      ClearScreen();
	      printf( helpmsg ) ;
	      PutLine0(LINES-2,0,catgets(elm_msg_cat, ElmSet, ElmChangeToWhichFolder,
		"Change to which folder: "));	/* reprompt */
	      redraw = TRUE;		/* we'll need to clean the screen */
	      continue ;
	    }

	    /* if user entered wildcard, list expansions and try again */
	    if ( has_wildcards( newfile ) ) {
	      list_folders( 4, NULL, newfile ) ;
	      PutLine0(LINES-2,0,catgets(elm_msg_cat, ElmSet, ElmChangeToWhichFolder,
		"Change to which folder: "));	/* reprompt */
	      redraw = TRUE ;
	      continue ;  
	    }

	    /* user entered a file name - expand it */
	    if (! expand_filename(newfile, TRUE))
	      continue;	/* prompt again */

	    /* don't accept the same file as the current */
	    if (strcmp(newfile, cur_folder) == 0) {
	      error(catgets(elm_msg_cat, ElmSet, ElmAlreadyReadingThatFolder,
		"Already reading that folder!"));
	      continue;	/* prompt again */
	    }

	    /* Make sure this is a file the user can open, unless it's the
	     * default mailfile, which is openable even if empty */
	    if ((errno = can_access(newfile, READ_ACCESS)) != 0 ) {
	      if (strcmp(newfile, defaultfile) != 0 || errno != ENOENT) {
		error1(catgets(elm_msg_cat, ElmSet, ElmCantOpenFolderReadNONL,
			"Can't open folder '%s' for reading!"), newfile);
		continue; 	/* prompt again */
	      }
	    }
	    break;	/* exit loop - we got the name of a good file */
	  }

	  /* All's clear with the new file to go ahead and leave the current. */
	  MoveCursor(LINES-3, 30);
	  CleartoEOS();

	  if(leave_mbox(FALSE, FALSE, TRUE) ==-1) {
	    /* new mail - leave not done - can't change to another file yet
	     * check for change in mailfile_size in main() will do the work
	     * of calling newmbox to add in the new messages to the current
	     * file and fix the sorting sequence that leave_mbox may have
	     * changed for its own purposes */
	    return(redraw);
	  }

	  redraw = 1;
	  newmbox(newfile, FALSE);
	  return(redraw);
}
