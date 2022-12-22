
static char rcsid[] = "@(#)Id: utils.c,v 5.15 1993/09/27 01:51:38 syd Exp ";

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
 * $Log: utils.c,v $
 * Revision 1.2  1994/01/14  00:55:58  cp
 * 2.4.23
 *
 * Revision 5.15  1993/09/27  01:51:38  syd
 * Add elm_chown to consolidate for Xenix not allowing -1
 * From: Syd
 *
 * Revision 5.14  1993/09/20  19:21:12  syd
 * make fflush conditional on the unit being open
 * From: Syd
 *
 * Revision 5.13  1993/09/19  23:37:29  syd
 * I found a few places more where the code was missing a call
 * to fflush() before it called unlock() and fclose()/exit()
 * right after unlocking the mail drop.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.12  1993/08/23  03:26:24  syd
 * Try setting group id separate from user id in chown to
 * allow restricted systems to change group id of file
 * From: Syd
 *
 * Revision 5.11  1993/08/03  19:28:39  syd
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
 * Revision 5.10  1993/04/12  03:11:50  syd
 * nameof() didn't check that the character after the common string was /, thus
 * (if Mail is the folderdir) Maildir/x was made to be =dir/x.
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.9  1993/04/12  03:08:40  syd
 * Added check if headers_per_page is zero in get_page().
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.8  1993/04/12  01:52:31  syd
 * Initialize safe_malloc() failure trap just to play it safe.  Although
 * Elm doesn't currently use these routines, do this just in case somebody
 * someday adds a call to a library routine that does use them.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.7  1993/01/20  03:37:16  syd
 * Nits and typos in the NLS messages and corresponding default messages.
 * From: dwolfe@pffft.sps.mot.com (Dave Wolfe)
 *
 * Revision 5.6  1993/01/05  03:40:45  syd
 * Protect TSTP for those systems without it.
 * From: kevin@cfctech.cfc.com (Kevin Darcy)
 *
 * Revision 5.5  1992/12/24  22:05:11  syd
 * Some OS's, especially ULTRIX create extra continue signals
 * that confuse Elm on exit
 * From: Syd via patch from Bob Mason
 *
 * Revision 5.4  1992/12/11  01:45:04  syd
 * remove sys/types.h include, it is now included by defs.h
 * and this routine includes defs.h or indirectly includes defs.h
 * From: Syd
 *
 * Revision 5.3  1992/12/07  04:30:37  syd
 * fix missing brace on do_cursor calls
 * From: Syd
 *
 * Revision 5.2  1992/11/26  00:46:13  syd
 * changes to first change screen back (Raw off) and then issue final
 * error message.
 * From: Syd
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** Utility routines for ELM

**/

#include "headers.h"
#include "s_elm.h"
#include <sys/stat.h>
#include <errno.h>

extern int errno;

create_new_folders()
{
	/* this creates a new folders directory */

#ifdef MKDIR
	(void) mkdir(folders, 0700);
#else
	char com[SLEN];

	/** Some systems don't have a mkdir call - how inconvienient! **/

	sprintf(com, "mkdir %s", folders);
	(void) system_call(com, 0);
	sprintf(com, "chmod 700 %s", folders);
	(void) system_call(com, 0);
#endif /* MKDIR */

	(void) elm_chown(folders, userid, groupid);
}

create_new_elmdir()
{
	/** this routine is just for allowing new users who don't have the
	    old elm files to create a new .elm directory **/

	char source[SLEN];
#ifdef MKDIR
	sprintf(source, "%s/.elm", home);
	(void) mkdir(source, 0700);
#else
	char com[SLEN];

	/** Some systems don't have a mkdir call - how inconvienient! **/

	sprintf(com, "mkdir %s/.elm", home);
	(void) system_call(com, 0);
	sprintf(com, "chmod 700 %s/.elm", home);
	(void) system_call(com, 0);
#endif /* MKDIR */

	(void) elm_chown( source, userid, groupid);
}

move_old_files_to_new()
{
	/** this routine is just for allowing people to transition from
	    the old Elm, where things are all kept in their $HOME dir,
	    to the new one where everything is in $HOME/.elm... **/

	char source[SLEN], dest[SLEN], temp[SLEN];
	char com[SLEN];

	/** simply go through all the files... **/

	sprintf(source, "%s/.alias_text", home);
	if (access(source, ACCESS_EXISTS) != -1) {
	  sprintf(dest,   "%s/%s", home, ALIAS_TEXT);
	  MCprintf(catgets(elm_msg_cat, ElmSet, ElmCopyingFromCopyingTo,
		"\n\rCopying from: %s\n\rCopying to:   %s\n\r"),
		source, dest);

	  sprintf(temp, "/tmp/%d", getpid());
	  sprintf(com, "%s -e 's/:/=/g' %s > %s\n", sed_cmd, source, temp);
	  (void) system_call(com, 0);
	  sprintf(com, "%s %s %s\n", move_cmd, temp, dest);
	  (void) system_call(com, 0);
	  (void) system_call("newalias", 0);
	}

	sprintf(source, "%s/.elmheaders", home);
	if (access(source, ACCESS_EXISTS) != -1) {
	  sprintf(dest,   "%s/%s", home, mailheaders);
	  MCprintf(catgets(elm_msg_cat, ElmSet, ElmCopyingFromCopyingTo,
		"\n\rCopying from: %s\n\rCopying to:   %s\n\r"),
	  	source, dest);
	  copy(source, dest);
	}

	sprintf(source, "%s/.elmrc", home);
	if (access(source, ACCESS_EXISTS) != -1) {
	  sprintf(dest,   "%s/%s", home, elmrcfile);
	  MCprintf(catgets(elm_msg_cat, ElmSet, ElmCopyingFromCopyingTo,
		"\n\rCopying from: %s\n\rCopying to:   %s\n\r"),
	  	source, dest);
	  copy(source, dest);
	}

	printf(catgets(elm_msg_cat, ElmSet, ElmWelcomeToNewElm,
	"\n\rWelcome to the new version of ELM!\n\n\rHit return to continue."));
	getchar();
}


/*
 * The initialize() procedure sets the "xalloc_fail_handler" vector to
 * point here in the event that xmalloc() or friends fail.
 */
/*ARGSUSED*/
void malloc_failed_exit(proc, len)
{
    MoveCursor(LINES,0);
    Raw(OFF);
    fprintf(stderr, catgets(elm_msg_cat, ElmSet, ElmCouldntMallocBytes,
	"\n\nCouldn't malloc %d bytes!!\n\n"), len);
    emergency_exit();
}


emergency_exit()
{
	/** used in dramatic cases when we must leave without altering
	    ANYTHING about the system... **/
	int do_cursor = RawState();

	char *mk_lockname();

/*
 *	some OS's get extra cont signal, so once this far into the
 *	exit, ignore those signals (Especially Ultrix)
 */
#ifdef SIGTSTP
	signal(SIGTSTP,SIG_IGN);
#endif
#ifdef SIGSTOP
	signal(SIGSTOP,SIG_IGN);
#endif
#ifdef SIGCONT
	signal(SIGCONT,SIG_IGN);
#endif

	dprint(1, (debugfile,
     "\nERROR: Something dreadful is happening!  Taking emergency exit!!\n\n"));
	dprint(1, (debugfile,
	     "  possibly leaving behind the following files;\n"));
	dprint(1, (debugfile,
	     "     The mailbox temp file : %s\n", cur_tempfolder));
	if(folder_type == SPOOL) dprint(1, (debugfile,
	     "     The mailbox lock file: %s\n", mk_lockname(cur_folder)));
	dprint(1, (debugfile,
	     "     The composition file : %s%s%d\n", temp_dir, temp_file, getpid()));

	if (cursor_control)  transmit_functions(OFF);
	if (hp_terminal)     softkeys_off();

	if (do_cursor) {
	  Raw(OFF);
	  MoveCursor(LINES, 0);
	  }

	printf(catgets(elm_msg_cat, ElmSet, ElmEmergencyExitTaken,
		"\nEmergency exit taken! All temp files intact!\n\n"));

	exit(1);
}
rm_temps_exit()
{
	char buffer[SLEN];
	int do_cursor = RawState();

/*
 *	some OS's get extra cont signal, so once this far into the
 *	exit, ignore those signals (Especially Ultrix)
 */
#ifdef SIGTSTP
	signal(SIGTSTP,SIG_IGN);
#endif
#ifdef SIGSTOP
	signal(SIGSTOP,SIG_IGN);
#endif
#ifdef SIGCONT
	signal(SIGCONT,SIG_IGN);
#endif

	PutLine0(LINES, 0, catgets(elm_msg_cat, ElmSet, ElmWriteFailedExitingIntact,
	 "\nWrite to temp file failed, exiting leaving mailbox intact!\n\n"));
	dprint(2, (debugfile, "\nrm_temps_exit, deleteing temp files\n"));

	if (cursor_control)  transmit_functions(OFF);
	if (hp_terminal)     softkeys_off();

	sprintf(buffer,"%s%d",temp_file, getpid());  /* editor buffer */
	(void) unlink(buffer);

	if (folder_type == SPOOL) {
	    (void) unlink(cur_tempfolder);
	}

	unlock();                               /* remove lock file if any */

	if(do_cursor) {
	    MoveCursor(LINES,0);
	    NewLine();
	    Raw(OFF);
	}

	exit(1);
}

/*ARGSUSED*/
/*VARARGS0*/

leave(val)
int val;	/* not used, placeholder for signal catching! */
{
	char buffer[SLEN];
	int do_cursor = RawState();

/*
 *	some OS's get extra cont signal, so once this far into the
 *	exit, ignore those signals (Especially Ultrix)
 */
#ifdef SIGTSTP
	signal(SIGTSTP,SIG_IGN);
#endif
#ifdef SIGSTOP
	signal(SIGSTOP,SIG_IGN);
#endif
#ifdef SIGCONT
	signal(SIGCONT,SIG_IGN);
#endif

	dprint(2, (debugfile, "\nLeaving mailer normally (leave)\n"));

	if (cursor_control)  transmit_functions(OFF);
	if (hp_terminal)     softkeys_off();

	sprintf(buffer,"%s%s%d", temp_dir, temp_file, getpid());  /* editor buffer */
	(void) unlink(buffer);

	if (folder_type == SPOOL) {
	  (void) unlink(cur_tempfolder);
	}

	if (mailfile)
	    fflush (mailfile);

	unlock();				/* remove lock file if any */

	if (do_cursor) {
	  MoveCursor(LINES,0);
	  NewLine();
	  Raw(OFF);
	}

	exit(0);
}

silently_exit()
{
	/** This is the same as 'leave', but it doesn't remove any non-pid
	    files.  It's used when we notice that we're trying to create a
	    temp mail file and one already exists!!
	**/
	char buffer[SLEN];
	int do_cursor = RawState();

/*
 *	some OS's get extra cont signal, so once this far into the
 *	exit, ignore those signals (Especially Ultrix)
 */
#ifdef SIGTSTP
	signal(SIGTSTP,SIG_IGN);
#endif
#ifdef SIGSTOP
	signal(SIGSTOP,SIG_IGN);
#endif
#ifdef SIGCONT
	signal(SIGCONT,SIG_IGN);
#endif

	dprint(2, (debugfile, "\nLeaving mailer quietly (silently_exit)\n"));

	if (cursor_control)  transmit_functions(OFF);
	if (hp_terminal)     softkeys_off();

	sprintf(buffer,"%s%s%d", temp_dir, temp_file, getpid());  /* editor buffer */
	(void) unlink(buffer);

	if (do_cursor) {
	  MoveCursor(LINES,0);
	  NewLine();
	  Raw(OFF);
	}

	if (mailfile)
	    fflush (mailfile);
	unlock ();

	exit(0);
}

/*ARGSUSED0*/

#ifndef REMOVE_AT_LAST
leave_locked(val)
int val;	/* not used, placeholder for signal catching! */
{
	/** same as leave routine, but don't disturb lock file **/

	char buffer[SLEN];
        int do_cursor = RawState();

/*
 *	some OS's get extra cont signal, so once this far into the
 *	exit, ignore those signals (Especially Ultrix)
 */
#ifdef SIGTSTP
	signal(SIGTSTP,SIG_IGN);
#endif
#ifdef SIGSTOP
	signal(SIGSTOP,SIG_IGN);
#endif
#ifdef SIGCONT
	signal(SIGCONT,SIG_IGN);
#endif

        dprint(3, (debugfile,
	    "\nLeaving mailer due to presence of lock file (leave_locked)\n"));

	if (cursor_control)  transmit_functions(OFF);
	if (hp_terminal)     softkeys_off();

	sprintf(buffer,"%s%s%d", temp_dir, temp_file, getpid());  /* editor buffer */
	(void) unlink(buffer);

	(void) unlink(cur_tempfolder);			/* temp mailbox */

	if (mailfile)
	    fflush (mailfile);

	if (do_cursor) {
	  MoveCursor(LINES,0);
	  NewLine();
	  Raw(OFF);
	}

	exit(0);
}
#endif

int
get_page(msg_pointer)
int msg_pointer;
{
	/** Ensure that 'current' is on the displayed page,
	    returning NEW_PAGE iff the page changed! **/

	register int first_on_page, last_on_page;

	if (headers_per_page == 0)
	  return(SAME_PAGE); /* What else can I do ? */

	first_on_page = (header_page * headers_per_page) + 1;

	last_on_page = first_on_page + headers_per_page - 1;

	if (selected)	/* but what is it on the SCREEN??? */
	  msg_pointer = compute_visible(msg_pointer);

	if (selected && msg_pointer > selected)
	  return(SAME_PAGE);	/* too far - page can't change! */

	if (msg_pointer > last_on_page) {
	  header_page = (int) (msg_pointer-1)/ headers_per_page;
	  return(NEW_PAGE);
	}
	else if (msg_pointer < first_on_page) {
	  header_page = (int) (msg_pointer-1) / headers_per_page;
	  return(NEW_PAGE);
	}
	else
	  return(SAME_PAGE);
}

char *nameof(filename)
char *filename;
{
	/** checks to see if 'filename' has any common prefixes, if
	    so it returns a string that is the same filename, but
	    with '=' as the folder directory, or '~' as the home
	    directory..
	**/

	static char buffer[STRING];
	register int i = 0, iindex = 0, len;

	len = strlen(folders);
	if (strncmp(filename, folders, len) == 0 &&
	    len > 0 && (filename[len] == '/' || filename[len] == '\0')) {
	  buffer[i++] = '=';
	  iindex = len;
	  if(filename[iindex] == '/')
	    iindex++;
	}
	else
	{
	  len = strlen(home);
	  if (strncmp(filename, home, len) == 0 &&
	      len > 1 && (filename[len] == '/' || filename[len] == '\0')) {
	    buffer[i++] = '~';
	    iindex = len;
	  }
	  else iindex = 0;
	}

	while (filename[iindex] != '\0')
	  buffer[i++] = filename[iindex++];
	buffer[i] = '\0';

	return( (char *) buffer);
}

int elm_chown(file, userid, groupid)
char *file;
int userid, groupid;
{
#ifdef CHOWN_NEG1
	int status;

	status = chown(file, -1, groupid);
	chown(file, userid, -1);

	return(status);
#else
	return(chown(file, userid, groupid));
#endif
}
