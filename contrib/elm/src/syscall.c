
static char rcsid[] = "@(#)Id: syscall.c,v 5.8 1993/08/23 02:46:07 syd Exp ";

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
 * $Log: syscall.c,v $
 * Revision 1.2  1994/01/14  00:55:56  cp
 * 2.4.23
 *
 * Revision 5.8  1993/08/23  02:46:07  syd
 * Don't declare _exit() if <unistd.h> already did it.
 * From: decwrl!uunet.UU.NET!fin!chip (Chip Salzenberg)
 *
 * Revision 5.7  1993/01/20  03:48:08  syd
 * Fix not to use vfork if SY_ENV_SHELL is set, as this causes the
 * parent environment to be modified.
 * From: Syd
 *
 * Revision 5.6  1992/12/20  05:29:33  syd
 * Fixed where when doing ! or | and ti/te is enabled, one doesn't see the
 * "Press any key to return to ELM:" message. because the screens are
 * switched before the message is printed.
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.5  1992/12/11  02:05:26  syd
 * List_folder knew only about '=' but nothing about the rest
 * of [+=%] as one would have expected.
 * From: Jukka Antero Ukkonen <ukkonen@venus.csc.fi>
 *
 * Revision 5.4  1992/12/11  01:58:50  syd
 * Allow for use from restricted shell by putting SHELL=/bin/sh in the
 * environment of spawned mail transport program.
 * From: chip@tct.com (Chip Salzenberg)
 *
 * Revision 5.3  1992/11/07  20:45:39  syd
 * add no tite flag on options that should not use ti/te
 * Hack by Syd
 *
 * Revision 5.2  1992/11/07  19:37:21  syd
 * Enhanced printing support.  Added "-I" to readmsg to
 * suppress spurious diagnostic messages.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** These routines are used for user-level system calls, including the
    '!' command and the '|' commands...

**/

#include "headers.h"
#include "s_elm.h"

#include <errno.h>

#ifdef BSD
#  include <sys/wait.h>
#endif

#ifndef I_UNISTD
void _exit();
#endif

char *argv_zero();	

#ifdef ALLOW_SUBSHELL

int
subshell()
{
	/** spawn a subshell with either the specified command
	    returns non-zero if screen rewrite needed
	**/

	char command[SLEN];
	int  old_raw, helpful, ret;

	helpful = (user_level == 0);

	if (helpful)
	  PutLine0(LINES-3, COLUMNS-40, catgets(elm_msg_cat, ElmSet, ElmUseShellName,
		"(Use the shell name for a shell.)"));
	PutLine0(LINES-2, 0, catgets(elm_msg_cat, ElmSet, ElmShellCommand,
		"Shell command: "));
	CleartoEOS();
	command[0] = '\0';
	(void) optionally_enter(command, LINES-2, 15, FALSE, FALSE);
	if (command[0] == 0) {
	  if (helpful)
	    MoveCursor(LINES-3,COLUMNS-40);
	  else
	    MoveCursor(LINES-2,0);
	  CleartoEOS();
	  return 0;
	}

	MoveCursor(LINES,0);
	CleartoEOLN();

	if ((old_raw = RawState()) == ON)
	  Raw(OFF);
	softkeys_off();
	if (cursor_control)
	  transmit_functions(OFF);

	umask(original_umask);	/* restore original umask so users new files are ok */
	ret = system_call(command, SY_USER_SHELL|SY_ENAB_SIGINT|SY_DUMPSTATE);
	umask(077);		/* now put it back to private for mail files */

	SetXYLocation(0, 40);	/* a location not near the next request, so an absolute is used */
	PutLine0(LINES, 0, catgets(elm_msg_cat, ElmSet, ElmPressAnyKeyToReturn,
		"\n\nPress any key to return to ELM: "));
	Raw(ON | NO_TITE);
	(void) getchar();
	printf("\r\n");
	Raw(OFF | NO_TITE); /* Done even if old_raw == ON, to get ti/te right */
	if (old_raw == ON)
	  Raw(ON);

	softkeys_on();
	if (cursor_control)
	  transmit_functions(ON);

	if (ret)
	  error1(catgets(elm_msg_cat, ElmSet, ElmReturnCodeWas,
		"Return code was %d."), ret);

	return 1;
}

#endif /* ALLOW_SUBSHELL */

int system_call(string, options)
char *string;
int options;
{
	/** execute 'string', setting uid to userid... **/

	/** The following might be encoded into the "options" parameter:

	    SY_USER_SHELL	When set, we will use the user-defined
				"shell" instead of "/bin/sh" for the
				shell escape.

	    SY_ENV_SHELL	When set, put "SHELL=[name-of-shell]" in
				the child's environment.  This hack makes
				mail transport programs work right even
				for users with restricted shells.

	    SY_ENAB_SIGHUP	When set, we will set SIGHUP, SIGTSTP, and
				SIGCONT to their default behaviour during
				the shell escape rather than ignoring them.
				This is particularly important with stuff
				like `vi' so it can preserve the session on
				a SIGHUP and do its thing with job control.

	    SY_ENAB_SIGINT	This option implies SY_ENAB_SIGHUP.  In
				addition to the signals listed above, this
				option will also set SIGINT and SIGQUIT
				to their default behaviour rather than
				ignoring them.

	    SY_DUMPSTATE	Create a state file for use by the "readmsg"
				program.  This is so that if "readmsg" is
				invoked it can figure out what folder we are
				in and what message(s) are selected.
	**/

	int pfd[2], stat, pid, w, iteration;
	char *sh;
#if defined(BSD) && !defined(WEXITSTATUS)
	union wait status;
#else
	int status;
#endif
	register SIGHAND_TYPE (*istat)(), (*qstat)();
#ifdef SIGTSTP
	register SIGHAND_TYPE (*oldstop)(), (*oldstart)();
#endif
	extern int errno;

	/* flush any pending output */
	fflush(stdout);

	/* figure out what shell we are using here */
	sh = ((options & SY_USER_SHELL) ? shell : "/bin/sh");
	dprint(2, (debugfile, "System Call: %s\n\t%s\n", sh, string));

	/* if we aren't reading a folder then a state dump is meaningless */
	if (mail_only)
	    options &= ~SY_DUMPSTATE;

	/* see if we need to dump out the folder state */
	if (options & SY_DUMPSTATE) {
	    if (create_folder_state_file() != 0)
		return -1;
	}

	/*
	 * Note the neat trick with close-on-exec pipes.
	 * If the child's exec() succeeds, then the pipe read returns zero.
	 * Otherwise, it returns the zero byte written by the child
	 * after the exec() is attempted.  This is the cleanest way I know
	 * to discover whether an exec() failed.   --CHS
	 */

	if (pipe(pfd) == -1) {
	  perror("pipe");
	  return -1;
	}
	fcntl(pfd[0], F_SETFD, 1);
	fcntl(pfd[1], F_SETFD, 1);

	istat = signal(SIGINT, SIG_IGN);
	qstat = signal(SIGQUIT, SIG_IGN);
#ifdef SIGTSTP
	oldstop = signal(SIGTSTP, SIG_DFL);
	oldstart = signal(SIGCONT, SIG_DFL);
#endif

	stat = -1;		/* Assume failure. */

	for (iteration = 0; iteration < 5; ++iteration) {
	  if (iteration > 0)
	    sleep(2);

#ifdef VFORK
	  if (options&SY_ENV_SHELL)
	    pid = fork();
	  else
	    pid = vfork();
#else
	  pid = fork();
#endif

	  if (pid != -1)
	    break;
	}

	if (pid == -1) {
	  perror("fork");
	}
	else if (pid == 0) {
	  /*
	   * Set group and user back to their original values.
	   * Note that group must be set first.
	   */
	  setgid(groupid);
	  setuid(userid);

	  /*
	   * Program to exec may or may not be able to handle
	   * interrupt, quit, hangup and stop signals.
	   */
	  if (options&SY_ENAB_SIGINT)
		options |= SY_ENAB_SIGHUP;
	  (void) signal(SIGHUP,  (options&SY_ENAB_SIGHUP) ? SIG_DFL : SIG_IGN);
	  (void) signal(SIGINT,  (options&SY_ENAB_SIGINT) ? SIG_DFL : SIG_IGN);
	  (void) signal(SIGQUIT, (options&SY_ENAB_SIGINT) ? SIG_DFL : SIG_IGN);
#ifdef SIGTSTP
	  (void) signal(SIGTSTP, (options&SY_ENAB_SIGHUP) ? SIG_DFL : SIG_IGN);
	  (void) signal(SIGCONT, (options&SY_ENAB_SIGHUP) ? SIG_DFL : SIG_IGN);
#endif

	  /* Optionally override the SHELL environment variable. */
	  if (options&SY_ENV_SHELL) {
	    static char sheq[] = "SHELL=";
	    char *p = malloc(sizeof(sheq) + strlen(sh));
	    if (p) {
	      sprintf(p, "%s%s", sheq, sh);
	      putenv(p);
	    }
	  }

	  /* Go for it. */
	  if (string) execl(sh, argv_zero(sh), "-c", string, (char *) 0);
	  else execl(sh, argv_zero(sh), (char *) 0);

	  /* If exec fails, we write a byte to the pipe before exiting. */
	  perror(sh);
	  write(pfd[1], "", 1);
	  _exit(127);
	}
	else {
	  int rd;
	  char ch;

	  /* Try to read a byte from the pipe. */
	  close(pfd[1]);
	  rd = read(pfd[0], &ch, 1);
	  close(pfd[0]);

	  while ((w = wait(&status)) != pid)
	      if (w == -1 && errno != EINTR)
		  break;

	  /* If we read a byte from the pipe, the exec failed. */
	  if (rd > 0)
	    stat = -1;
	  else if (w == pid) {
#ifdef	WEXITSTATUS
	    stat = WEXITSTATUS(status);
#else
# ifdef	BSD
	    stat = status.w_retcode;
# else
	    stat = status;
# endif
#endif
	  }
  	}
  
	(void) signal(SIGINT, istat);
	(void) signal(SIGQUIT, qstat);
#ifdef SIGTSTP
	(void) signal(SIGTSTP, oldstop);
	(void) signal(SIGCONT, oldstart);
#endif

	/* cleanup any folder state file we made */
	if (options & SY_DUMPSTATE)
	    (void) remove_folder_state_file();

	return(stat);
}

int
do_pipe()
{
	/** pipe the current message or tagged messages to
	    the specified sequence.. **/

	char command[SLEN], buffer[SLEN], *prompt;
	register int  ret;
	int	old_raw;

	prompt = catgets(elm_msg_cat, ElmSet, ElmPipeTo, "Pipe to: ");
        PutLine0(LINES-2, 0, prompt);
	command[0] = '\0';
	(void) optionally_enter(command, LINES-2, strlen(prompt), FALSE, FALSE);
	if (command[0] == '\0') {
	  MoveCursor(LINES-2,0);
	  CleartoEOLN();
	  return(0);
	}

	MoveCursor(LINES,0);
	CleartoEOLN();
	if (( old_raw = RawState()) == ON)
	  Raw(OFF);

	if (cursor_control)
	  transmit_functions(OFF);
	
	sprintf(buffer, "%s -Ih|%s", readmsg, command);
	ret = system_call(buffer, SY_USER_SHELL|SY_ENAB_SIGINT|SY_DUMPSTATE);

	SetXYLocation(0, 40);	/* a location not near the next request, so an absolute is used */
	PutLine0(LINES, 0, catgets(elm_msg_cat, ElmSet, ElmPressAnyKeyToReturn,
		"\n\nPress any key to return to ELM: "));

	Raw(ON | NO_TITE);
	(void) getchar();
	printf("\r\n");
	Raw(OFF | NO_TITE); /* Done even if old_raw == ON, to get ti/te right */
	if (old_raw == ON)
	  Raw(ON);

	if (cursor_control)  transmit_functions(ON);

	if (ret != 0)
	  error1(catgets(elm_msg_cat, ElmSet, ElmReturnCodeWas,
		"Return code was %d."), ret);
	return(1);
}

int print_msg(pause_on_scroll)
int pause_on_scroll;
{
	/*
	 * Print the tagged messages, or the current message if none are
	 * tagged.  Message(s) are passed passed into the command specified
	 * by "printout".  An error is given if "printout" is undefined.
	 *
	 * Printing will be done through a pipe so we can print the number
	 * of lines output.  This is used to determine whether the screen
	 * got trashed by the print command.  One limitation is that only
	 * stdout lines are counted, not stderr output.  A nonzero result
	 * is returned if we think enough output was generated to trash
	 * the display, a zero result indicates the display is probably
	 * alright.  Further, if the display is trashed and "pause_on_scroll"
	 * is true then we'll give a "hit any key" prompt before returning.
	 *
	 * This routine has two modes of behavior, depending upon whether
	 * there is a "%s" embedded in the "printout" string.  If there,
	 * the old Elm behavior is used (a temp file is used, all output
	 * from the print command is chucked out).  If there isn't a "%s"
	 * then the new behavior is used (message(s) piped right into
	 * print command, output is left attached to the terminal).
	 *
	 * The old behaviour is bizarre.  I hope we can ditch it someday.
	 */

	char buffer[SLEN], filename[SLEN], printbuffer[SLEN];
	int  nlines, retcode, old_raw;
	FILE *fp;

	/*
	 * Make sure we know how to print.
	 */
	if (printout[0] == '\0') {
	    error(catgets(elm_msg_cat, ElmSet, ElmPrintDontKnowHow,
		"Don't know how to print - option \"printmail\" undefined!"));
	    return 0;
	}

	/*
	 * Temp file name used by "old style" printing.
	 */
        sprintf(filename,"%s%s%d", temp_dir, temp_print, getpid());

	/*
	 * Setup print command.  Select old or new behavior based
	 * upon the presence of "%s" in the print command string.
	 */
	if (in_string(printout, "%s")) {
	    sprintf(printbuffer, printout, filename);
	    sprintf(buffer,"(%s -Ip > %s; %s 2>&1) > /dev/null",
		readmsg, filename, printbuffer);
	} else {
	    sprintf(buffer,"%s -Ip | %s", readmsg, printout);
	}

	/*
	 * Create information for "readmsg" command.
	 */
	if (create_folder_state_file() != 0)
	    return 0;

	/*
	 * Put keyboard into normal state.
	 */
	if ((old_raw = RawState()) == ON)
	    Raw(OFF | NO_TITE);
	softkeys_off();
	if (cursor_control)
	    transmit_functions(OFF);

	/*
	 * Run the print command in a pipe and grab the output.
	 */
	putchar('\n');
	fflush(stdout);
	nlines = 0;
	if ((fp = popen(buffer, "r")) == NULL) {
	    error(catgets(elm_msg_cat, ElmSet, ElmPrintPipeFailed,
		"Cannot create pipe to print command."));
	    goto done;
	}
	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
	    fputs(buffer, stdout);
	    ++nlines;
	}

	/*
	 * See if there were enough lines printed to trash the screen.
	 */
	if (pause_on_scroll && nlines > 1) {
	    printf("\n%s ", catgets(elm_msg_cat, ElmSet, ElmPrintPressAKey,
		"Press any key to continue:"));
	    fflush(stdout);
	    Raw(ON | NO_TITE);
	    (void) getchar();
	}

	/*
	 * Display a status message.
	 */
	if ((retcode = pclose(fp)) == 0) {
	    error(catgets(elm_msg_cat, ElmSet, ElmPrintJobSpooled,
		"Print job has been spooled."));
	} else if ((retcode & 0xFF) == 0) {
	    error1(catgets(elm_msg_cat, ElmSet, ElmPrintFailCode,
		"Printout failed with return code %d."), (retcode>>8));
	} else {
	    error1(catgets(elm_msg_cat, ElmSet, ElmPrintFailStatus,
		"Printout failed with status 0x%04x."), (retcode>>8));
	}

	/*
	 * Hack alert:  The only place we use "pause_on_scroll" false is when
	 * printing while reading a mail message.  This newline prevents the
	 * above message from being wiped out by the command prompt.
	 */
	if (!pause_on_scroll)
		putchar('\n');

done:
	Raw(old_raw | NO_TITE);
	softkeys_on();
	if (cursor_control)
	    transmit_functions(ON);
	(void) unlink(filename);
	(void) remove_folder_state_file();
	return (nlines > 1);
}


list_folders(numlines, helpmsg, wildcard)
unsigned numlines;
char *helpmsg;
char *wildcard;
{
	/** list the folders in the users FOLDERHOME directory.  This is
	    simply a call to "ls -C" unless there is a wildcard, in
	    which case it's "ls -C wildcard".  Note that wildcards can
	    refer either to the folder directory (in which case they
	    start with an '=') or a general directory, in which case we
	    take them at face value.
	    Numlines is the number of lines to scroll afterwards. This is
	    useful when a portion of the screen needs to be cleared for
	    subsequent prompts, but you don't want to overwrite the
	    list of folders.
	    Helpmsg is what should be printed before the listing if not NULL.
	**/

	char buffer[SLEN];

	Raw(OFF | NO_TITE);
	ClearScreen();
	MoveCursor(LINES, 0);
	if(helpmsg)
	  printf(helpmsg);
	if ( NULL == wildcard )
	{
	  sprintf(buffer, "cd %s;ls -C", folders);
	  printf(catgets(elm_msg_cat, ElmSet, ElmContentsOfYourFolderDir,
		"\n\rContents of your folder directory:\n\r\n\r"));
	  (void) system_call(buffer, 0); 
	}
	else
	{
	  if (( *wildcard == '=' ) ||
	      ( *wildcard == '+' ) || ( *wildcard == '%' ))
	  {
	    sprintf(buffer, "cd %s;ls -C %s", folders, wildcard+1);
	    printf(catgets(elm_msg_cat, ElmSet, ElmFoldersWhichMatch,
		"\n\rFolders which match `%s':\n\r\n\r"), wildcard+1);
	  }
          else
	  {
	    sprintf(buffer, "ls -C %s", wildcard);
	    printf(catgets(elm_msg_cat, ElmSet, ElmFilesWhichMatch,
		"\n\rFiles which match `%s':\n\r\n\r"), wildcard);
	  }
	  (void) system_call(buffer, 0); 
	}
	while(numlines--)
	    printf("\n\r");
	Raw(ON | NO_TITE);
}


static char folder_state_env_param[SLEN], *folder_state_fname;

/*
 * Setup a folder state file for external utilities (e.g. "readmsg").
 * Returns zero if the file was created, -1 if an error occurred.  A
 * diagnostic will have been printed on an error return.
 *
 * The state file contains the following:
 *
 * - An "F" record with the pathname to the current folder.
 *
 * - An "N" record with a count of the number of messages in the folder.
 *
 * - A set of "I" records indicating the seek index of the messages
 *   in the folder.  The first "I" record will contain the seek index
 *   of message number one, and so on.  The "I" records will be in
 *   sorting order and not necessarily mbox order.  The number of "I"
 *   records will match the value indicated in the "N" record.
 *
 * - A "C" record with a count of the total number of messages selected.
 *
 * - A set of "S" records indicating message number(s) which have been
 *   selected.  If messages have been tagged then there will be one
 *   "S" record for each selected message.  If no messages have been
 *   tagged then either:  there will be a single "S" record with the
 *   current message number, or there will be no "S" records if the
 *   folder is empty.  The number of "S" records will match the value
 *   indicated in the "C" record.
 */
int create_folder_state_file()
{
    int count, i;
    FILE *fp;

    /* format an environ param with the state file and pick out file name */
    sprintf(folder_state_env_param, "%s=%s%s%d",
	FOLDER_STATE_ENV, default_temp, temp_state, getpid());
    folder_state_fname = folder_state_env_param + strlen(FOLDER_STATE_ENV) +1;

    /* open up the folder state file for writing */
    if ((fp = fopen(folder_state_fname, "w")) == NULL) {
	error1(catgets(elm_msg_cat, ElmSet, ElmCannotCreateFolderState,
		"Cannot create folder state file \"%s\"."), folder_state_fname);
	return -1;
    }

    /* write out the pathname of the folder */
    fprintf(fp, "F%s\n",
	(folder_type == NON_SPOOL ? cur_folder : cur_tempfolder));

    /* write out the folder size and message indices */
    fprintf(fp, "N%d\n", message_count);
    for (i = 0 ; i < message_count ; ++i)
	fprintf(fp, "I%ld\n", headers[i]->offset);

    /* count up the number of tagged messages */
    count = 0;
    for (i = 0 ; i < message_count ; i++)  {
	if (headers[i]->status & TAGGED)
		++count;
    }

    /* write out selected messages */
    if (count > 0) {
	/* we found tagged messages - write them out */
	fprintf(fp, "C%d\n", count);
	for (i = 0 ; i < message_count ; i++) {
	    if (headers[i]->status & TAGGED)
		fprintf(fp, "S%d\n", i+1);
	}
    } else if (current > 0) {
	/* no tagged messages - write out the selected message */
	fprintf(fp, "C1\nS%d\n", current);
    } else {
	/* hmmm...must be an empty mailbox */
	fprintf(fp, "C0\n");
    }

    /* file is done */
    (void) fclose(fp);

    /* put pointer to the file in the environment */
    if (putenv(folder_state_env_param) != 0) {
	error1(catgets(elm_msg_cat, ElmSet, ElmCannotCreateEnvParam,
	    "Cannot create environment parameter \"%s\"."), FOLDER_STATE_ENV);
	return -1;
    }

    return 0;
}


int remove_folder_state_file()
{
    /*
     * We simply leave the FOLDER_STATE_ENV environment variable set.
     * It's too much work trying to pull it out of the environment, and
     * the load_folder_state_file() does not mind if the environment
     * variable points to a non-existent file.
     */
    return unlink(folder_state_fname);
}

