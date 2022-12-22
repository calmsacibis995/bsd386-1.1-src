
static char rcsid[] = "@(#)Id: init.c,v 5.19 1993/09/27 01:51:38 syd Exp ";

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
 * $Log: init.c,v $
 * Revision 1.2  1994/01/14  00:55:06  cp
 * 2.4.23
 *
 * Revision 5.19  1993/09/27  01:51:38  syd
 * Add elm_chown to consolidate for Xenix not allowing -1
 * From: Syd
 *
 * Revision 5.18  1993/08/23  03:26:24  syd
 * Try setting group id separate from user id in chown to
 * allow restricted systems to change group id of file
 * From: Syd
 *
 * Revision 5.17  1993/08/03  19:28:39  syd
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
 * Revision 5.16  1993/04/12  03:30:23  syd
 * On AIX, __STDC__ is not defined but it does use unistd.h, etc.  In
 * hdrs/def.h, ANS_C already gets defined if __STDC__ or _AIX.  But this
 * variable then needs to be used in src/init.c and hdrs/filter.h in place
 * of the current test for __STDC__.
 * From:	rstory@elegant.com (Robert Story)
 *
 * Revision 5.15  1993/04/12  03:07:57  syd
 * The "window is too small..." message gets printed on the wrong screen. The
 * message also needs \r\n at the end.
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.14  1993/04/12  01:52:31  syd
 * Initialize safe_malloc() failure trap just to play it safe.  Although
 * Elm doesn't currently use these routines, do this just in case somebody
 * someday adds a call to a library routine that does use them.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.13  1993/02/03  19:08:03  syd
 * fix extra declares
 * From: syd
 *
 * Revision 5.12  1993/01/20  03:43:37  syd
 * Some systems dont have SIGBUS, make it optional
 *
 * Revision 5.11  1993/01/20  03:02:19  syd
 * Move string declarations to defs.h
 * From: Syd
 *
 * Revision 5.10  1993/01/19  05:07:05  syd
 * Trim erroreous extra log entry
 * From: Syd
 *
 * Revision 5.9  1993/01/19  04:47:12  syd
 * Significant changes to provide consistent Date and From_ header
 * cracking.  Overhauled date utilities and moved into library.  Moved
 * real_from() into library.  Modified frm, newmail, and readmsg utilities
 * to use library version of real_from().  Moved get_word() from Elm
 * source into library.  Added new library routines atonum() and strfcpy().
 * Fixed trailing backslash bug in len_next().
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.8  1992/12/07  02:41:21  syd
 * This implements the use of SIGUSR1 and SIGUSR2 as discussed on the
 * mailing list recently, and adds them to the documentation.
 * From: scs@lokkur.dexter.mi.us (Steve Simmons)
 *
 * Revision 5.7  1992/11/26  01:26:04  syd
 * only enter raw mode if not batch and not elm -c, to avoid
 * screen swap and cursor control output
 *
 * Revision 5.6  1992/11/26  00:46:13  syd
 * changes to first change screen back (Raw off) and then issue final
 * error message.
 * From: Syd
 *
 * Revision 5.5  1992/10/30  21:12:40  syd
 * Make patchlevel a text string to allow local additions to the variable
 * From: syd via a request from Dave Wolfe
 *
 * Revision 5.4  1992/10/27  01:34:12  syd
 * It fixes the problem where ti/te was being used (by Raw()),
 * AFTER the command line args had been read but BEFORE the rc files had.
 * From: Graham Hudspith <gwh@inmos.co.uk>
 *
 * Revision 5.3  1992/10/25  02:01:58  syd
 * Here are the patches to support POSIX sigaction().
 * From: tom@osf.org
 *
 * Revision 5.2  1992/10/24  13:35:39  syd
 * changes found by using codecenter on Elm 2.4.3
 * From: Graham Hudspith <gwh@inmos.co.uk>
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/***** Initialize - read in all the defaults etc etc 
*****/

#include "headers.h"
#include "patchlevel.h"
#include "s_elm.h"

#ifdef TERMIOS
#  include <termios.h>
   typedef struct termios term_buff;
#else
# ifdef TERMIO
#  include <termio.h>
#  define tcgetattr(fd,buf)	ioctl((fd),TCGETA,(buf))
   typedef struct termio term_buff;
# else
#  include <sgtty.h>
#  define tcgetattr(fd,buf)	ioctl((fd),TIOCGETP,(buf))
   typedef struct sgttyb term_buff;
# endif
#endif

#ifdef PWDINSYS
#  include <sys/pwd.h>
#else
#  include <pwd.h>
#endif

#ifdef I_TIME
#  include <time.h>
#endif
#ifdef I_SYSTIME
#  include <sys/time.h>
#endif

#include <errno.h>

extern int errno;		/* system error number on failure */
extern char version_buff[];

char *error_description();

#ifndef ANSI_C /* items are in unistd.h which is included if __STDC__ */
char *getlogin();
unsigned short getgid(), getuid(); 
#endif

#ifndef	_POSIX_SOURCE
struct passwd *getpwuid();
#endif
char *get_full_name();

SIGHAND_TYPE
#ifdef SIGTSTP
	sig_user_stop(), sig_return_from_user_stop(),
#endif
#ifdef SIGWINCH
	winch_signal(),
#endif
	quit_signal(), term_signal(), ill_signal(),
	fpe_signal(), segv_signal(),
#ifdef SIGBUS
	bus_signal(),
#endif
	alarm_signal(), pipe_signal(), hup_signal(),
	usr1_signal(), usr2_signal() ;

extern void init_opts_menu();
extern void malloc_failed_exit();

initialize(requestedmfile)
char *requestedmfile;	/* first mail file to open, empty if the default */
{
	/** initialize the whole ball of wax.
	**/
	struct passwd *pass, *getpwnam();
	register int init_scr_return, hostlen, domlen; 

#if defined(SIGVEC) & defined(SV_INTERRUPT)
	struct sigvec alarm_vec;
#endif /* defined(SIGVEC) & defined(SV_INTERRUPT) */
	char     buffer[SLEN], *cp;

	sprintf(version_buff, "%s PL%s", VERSION, PATCHLEVEL);
	def_ans_yes = catgets(elm_msg_cat, ElmSet, ElmYes, "y");
	def_ans_no = catgets(elm_msg_cat, ElmSet, ElmNo, "n");
	nls_deleted = catgets(elm_msg_cat, ElmSet, ElmTitleDeleted, "[deleted]");
	nls_form = catgets(elm_msg_cat, ElmSet, ElmTitleForm, "Form");
	nls_message = catgets(elm_msg_cat, ElmSet, ElmTitleMessage, "Message");
	nls_to = catgets(elm_msg_cat, ElmSet, ElmTitleTo, "To");
	nls_from = catgets(elm_msg_cat, ElmSet, ElmTitleFrom, "From");
	nls_page = catgets(elm_msg_cat, ElmSet, ElmTitlePage, "  Page %d");
	change_word = catgets(elm_msg_cat, ElmSet, ElmChange, "change");
	save_word = catgets(elm_msg_cat, ElmSet, ElmSave, "save");
	copy_word = catgets(elm_msg_cat, ElmSet, ElmCopy, "copy");
	cap_save_word = catgets(elm_msg_cat, ElmSet, ElmCapSave, "Save");
	cap_copy_word = catgets(elm_msg_cat, ElmSet, ElmCapCopy, "Copy");
	saved_word = catgets(elm_msg_cat, ElmSet, ElmSaved, "saved");
	copied_word = catgets(elm_msg_cat, ElmSet, ElmCopied, "copied");
	strcpy(item, catgets(elm_msg_cat, ElmSet, Elmitem, "message"));
	strcpy(items, catgets(elm_msg_cat, ElmSet, Elmitems, "messages"));
	strcpy(Item, catgets(elm_msg_cat, ElmSet, ElmItem, "Message"));
	strcpy(Items, catgets(elm_msg_cat, ElmSet, ElmItems, "Messages"));
	strcpy(Prompt, catgets(elm_msg_cat, ElmSet, ElmPrompt, "Command: "));

	/*
	 * Install the error trap to take if xmalloc() or friends fail.
	 */
	safe_malloc_fail_handler = malloc_failed_exit;

	init_opts_menu();
	init_scr_return = InitScreen();

	/* save original user and group ids */
	userid  = getuid();
	groupid = getgid();	
#ifdef SAVE_GROUP_MAILBOX_ID
	mailgroupid = getegid();
	setgid(groupid);
#endif

	/* make all newly created files private */
	original_umask = umask(077);

	/* Get username (logname), home (login directory), and full_username
	 * (part of GCOS) field from the password entry for this user id.
	 * Full_username will get overridden by fullname in elmrc, if defined.
	 *
	 * For those sites that have various user names with the same user
	 * ID, use the passwd entry corresponding to the user name as long 
	 * as it matches the user ID.  Otherwise fall back on the entry 
	 * associated with the user ID alone.
	 */

	if((cp = getenv("LOGNAME")) == NULL)
		cp = getenv("USER");
	if(cp != NULL && (pass = getpwnam(cp)) != NULL &&
	    pass->pw_uid == userid) {
	  ;  /* Null body */
	} else if((pass = getpwuid(userid)) == NULL) {
	  error(catgets(elm_msg_cat, ElmSet, ElmYouHaveNoPasswordEntry,
		"You have no password entry!\r\n"));
	  exit(1);
	}
	strcpy(username, pass->pw_name);
	strcpy(home, pass->pw_dir);

	if((cp = get_full_name(username)) != NULL)
	  strcpy(full_username, cp);
	else
	  strcpy(full_username, username);	/* fall back on logname */

#ifdef DEBUG
	if (debug) {		/* setup for dprint() statements! */
	  char newfname[SLEN], filename[SLEN];

	  sprintf(filename, "%s/%s", home, DEBUGFILE);
	  if (access(filename, ACCESS_EXISTS) == 0) {	/* already one! */
	    sprintf(newfname,"%s/%s", home, OLDEBUG);
	    (void) rename(filename, newfname);
	  }

	  /* Note what we just did up there: we always save the old
	     version of the debug file as OLDEBUG, so users can mail
	     copies of bug files without trashing 'em by starting up
	     the mailer.  Dumb, subtle, but easy enough to do!
 	  */

	  if ((debugfile = fopen(filename, "w")) == NULL) {
	    debug = 0;	/* otherwise 'leave' will try to log! */
	    leave(fprintf(stderr, catgets(elm_msg_cat, ElmSet, ElmCouldNotOpenDebugFile,
			"Could not open file %s for debug output!\n"),
		  filename));
	  }
	  (void) elm_chown(filename, userid, groupid); /* file owned by user */

	  fprintf(debugfile, 
     "Debug output of the ELM program (at debug level %d).  Version %s\n\n",
		  debug, version_buff);
	}
#endif

	/*
	 * If debug level is fairly low, ignore keyboard signals
	 * until the screen is set up.
	 */
	if (debug < 5) {
	  signal(SIGINT,  SIG_IGN);
	  signal(SIGQUIT, SIG_IGN);
	}

	if(!check_only && !batch_only) {
	  if (init_scr_return < 0) {
	    if (init_scr_return == -1) {
	      printf(catgets(elm_msg_cat, ElmSet, ElmNoTerm,
"Sorry, but you must specify what type of terminal you're on if you want to\n\
run the \"elm\" program. (You need your environment variable \"TERM\" set.)\n"));
	      dprint(1,(debugfile,"No $TERM variable in environment!\n"));
	    }
	    else if (init_scr_return == -2) {
	      printf(catgets(elm_msg_cat, ElmSet, ElmBadTerm,
"You need a cursor-addressable terminal to run \"elm\" and I can't find any\n\
kind of termcap entry for \"%s\" - check your \"TERM\" setting...\n"),
		   getenv("TERM"));
	      dprint(1,
		(debugfile,"$TERM variable is an unknown terminal type!\n"));
	    } else {
	      printf(catgets(elm_msg_cat, ElmSet, ElmTermInitFailed,
		"Failed trying to initialize your terminal entry: unknown return code %d\n"), init_scr_return);
	      dprint(1, (debugfile, "Initscreen returned unknown code: %d\n",
		  init_scr_return));
	    }
	    exit(1);	/* all the errors share this exit statement */
	  }
	}

	if (debug < 5) {	/* otherwise let the system trap 'em! */
	  signal(SIGQUIT, quit_signal);		/* Quit signal 	            */
	  signal(SIGTERM, term_signal); 	/* Terminate signal         */
	  signal(SIGILL,  ill_signal);		/* Illegal instruction      */
	  signal(SIGFPE,  fpe_signal);		/* Floating point exception */
#ifdef SIGBUS
	  signal(SIGBUS,  bus_signal);		/* Bus error  		    */
#endif
	  signal(SIGSEGV, segv_signal);		/* Segmentation Violation   */
	  signal(SIGHUP,  hup_signal);		/* HangUp (line dropped)    */
	  signal(SIGUSR1, usr1_signal);		/* User request 1	    */
	  signal(SIGUSR2, usr2_signal);		/* User request 2	    */
	}
	else {
	  dprint(3,(debugfile,
  "\n*** Elm-Internal Signal Handlers Disabled due to debug level %d ***\n\n",
		    debug));
	}
#if !defined(POSIX_SIGNALS) & defined(SIGVEC) & defined(SV_INTERRUPT)
	bzero((char *) &alarm_vec, sizeof(alarm_vec));
	alarm_vec.sv_handler = alarm_signal;
	alarm_vec.sv_flags = SV_INTERRUPT;
	sigvec (SIGALRM, &alarm_vec, (struct sigvec *)0);	/* Process Timer Alarm	    */
#else /* defined(SIGVEC) & defined(SV_INTERRUPT) */
	signal(SIGALRM, alarm_signal);		/* Process Timer Alarm      */
#endif /* defined(SIGVEC) & defined(SV_INTERRUPT) */
	signal(SIGPIPE, pipe_signal);		/* Illegal Pipe Operation   */
#ifdef SIGTSTP
	signal(SIGTSTP, sig_user_stop);		/* Suspend signal from tty  */
	signal(SIGCONT, sig_return_from_user_stop);	/* Continue Process */
#endif
#ifdef SIGWINCH
	signal(SIGWINCH, winch_signal);		/* change screen size */
#endif

	get_term_chars();

	/*
	 * Get the host name as per configured behavior.
	 */
#ifdef HOSTCOMPILED
	strfcpy(hostname, HOSTNAME, sizeof(hostname));
#else
	gethostname(hostname, sizeof(hostname));
#endif

	/*
	 * now get the domain name, used to build the full name
	 */
	gethostdomain(hostdomain, sizeof(hostdomain));

	/*
	 * now the tough part:
	 *	we need to make three variables out of this stuff:
	 *	hostname = just the hostname, as in bangpaths,
	 *		this is whatever the user gave us so far,
	 *		we wont change this one
	 *	hostdomain = this is the domain considered local to this
	 *		machine, and should be what we got above.
	 *	hostfullname = this is the full FQDN of this machine,
	 *		and is a strange combination of the first two.
	 *	if tail(hostname) == hostdomain
	 *		then hostfullname = hostname
	 *			ie: node.ld.domain.type, ld.domain.type -> node.ld.domain.type
	 *	else if hostname == hostdomain + 1
	 *		then hostfullname = hostname
	 *			ie: domain.type, .domain.type -> domain.type
	 *	
	 *	else hostfullname = hostname + hostdomain
	 *			ie: host, .domain.type -> host.domain.type
	 * lost yet?
	 */
	hostlen = strlen(hostname);
	domlen = strlen(hostdomain);
	if (hostlen >= domlen) {
	  if (istrcmp(&hostname[hostlen - domlen], hostdomain) == 0)
	    strcpy(hostfullname, hostname);
	  else {
	    strcpy(hostfullname, hostname);
	    strcat(hostfullname, hostdomain);
	  }
	} else {
	  if (istrcmp(hostname, hostdomain + 1) == 0)
	    strcpy(hostfullname, hostname);
	  else {
	    strcpy(hostfullname, hostname);
	    strcat(hostfullname, hostdomain);
	  }
	}

	/* Determine the default mail file name.
	 * 
	 * First look for an environment variable MAIL, then
	 * use then mailhome if it is not found
	 */
	if ((cp = getenv("MAIL")) == NULL)
		sprintf(defaultfile, "%s%s", mailhome, username);
	else
		strcpy(defaultfile, cp);

	/* Determine options that might be set in the .elm/elmrc */
	read_rc_file();

	/* Now that we've read the rc file we can enter RAW mode */
	if (!batch_only && !check_only)
	    Raw(ON);

	/* Determine the mail file to read */
	if (*requestedmfile == '\0')
	  strcpy(requestedmfile, defaultfile);
	else if(!expand_filename(requestedmfile, FALSE)) {
	    Raw(OFF);
	    exit(0);
        }
	if (check_size)
	  if(check_mailfile_size(requestedmfile) != 0) {
	      Raw(OFF);
	      exit(0);
	  }

	/* check for permissions only if not send only mode file */
	if (! mail_only) {
	  if ((errno = can_access(requestedmfile, READ_ACCESS)) != 0) {
	    if (strcmp(requestedmfile, defaultfile) != 0 || errno != ENOENT) {
	      dprint(1, (debugfile,
		    "Error: given file %s as folder - unreadable (%s)!\n", 
		    requestedmfile, error_description(errno)));
	      fprintf(stderr, catgets(elm_msg_cat, ElmSet, ElmCantOpenFolderRead,
			"Can't open folder '%s' for reading!\n"),
		    requestedmfile);
	      Raw(OFF);
	      exit(1);
	    }
	  }
	}

	/** check to see if the user has defined a LINES or COLUMNS
	    value different to that in the termcap entry (for
	    windowing systems, of course!) **/

	ScreenSize(&LINES, &COLUMNS);

	if ((cp = getenv("LINES")) != NULL && isdigit(*cp)) {
	  sscanf(cp, "%d", &LINES);
	  LINES -= 1;	/* kludge for HP Window system? ... */
	}

	if ((cp = getenv("COLUMNS")) != NULL && isdigit(*cp))
	  sscanf(cp, "%d", &COLUMNS);

	/** fix the shell if needed **/

	if (shell[0] != '/') {
	   sprintf(buffer, "/bin/%s", shell);
	   strcpy(shell, buffer);
	}

	if (! mail_only && ! check_only) {

	  /* get the cursor control keys... */

	  cursor_control = FALSE;

	  if ((cp = return_value_of("ku")) != NULL) {
	    strcpy(up, cp);
	    if ((cp = return_value_of("kd")) != NULL) {
	      strcpy(down, cp);
	      if ((cp = return_value_of("kl")) != NULL) {
		strcpy(left, cp);
		if ((cp = return_value_of("kr")) != NULL) {
		  strcpy(right, cp);
		  cursor_control = TRUE;
		  transmit_functions(ON);
		}
	      }
	    }
	  }
	  if (!arrow_cursor) 	/* try to use inverse bar instead */
	    if (return_value_of("so") != NULL && return_value_of("se") != NULL)
	        has_highlighting = TRUE;
	}

	/** clear the screen **/
	if(!check_only && !batch_only)
	  ClearScreen();

	if (! mail_only && ! check_only) {
	  if (mini_menu)
	    headers_per_page = LINES - 13;
	  else
	    headers_per_page = LINES -  8;	/* 5 more headers! */

	  if (headers_per_page <= 0) {
	    Raw(OFF);
	    error1(catgets(elm_msg_cat, ElmSet, ElmWindowSizeTooSmall,
		 "The window is too small to run Elm.  It must have at least %d rows"),
		 1 + LINES - headers_per_page);
	    exit(1);
	  }
	  newmbox(requestedmfile, FALSE);	/* read in the folder! */
	}

#ifdef DEBUG
	if (debug >= 2 && debug < 10) {
	  fprintf(debugfile,
"hostname = %-20s \tusername = %-20s \tfullname = %-20s\n",
	         hostname, username, full_username);

	  fprintf(debugfile,
"home     = %-20s \teditor   = %-20s \trecvd_mail  = %-20s\n",
		 home, editor, recvd_mail);

	  fprintf(debugfile,
"cur_folder   = %-20s \tfolders  = %-20s \tprintout = %-20s\n",
		 cur_folder, folders, printout);
	
	  fprintf(debugfile,
"sent_mail = %-20s \tprefix   = %-20s \tshell    = %-20s\n\n",
		sent_mail, prefixchars, shell);
	
	  if (local_signature[0])
	    fprintf(debugfile, "local_signature = \"%s\"\n",
			local_signature);
	  if (remote_signature[0])
	    fprintf(debugfile, "remote_signature = \"%s\"\n",
			remote_signature);
	  if (local_signature[0] || remote_signature[0])
	    fprintf(debugfile, "\n");
	}
#endif
}

get_term_chars()
{
	/** This routine sucks out the special terminal characters
	    ERASE and KILL for use in the input routine.  The meaning 
            of the characters are (dare I say it?) fairly obvious... **/

	term_buff term_buffer;

	if (tcgetattr(STANDARD_INPUT,&term_buffer) == -1) {
	  dprint(1, (debugfile,
		   "Error: %s encountered on ioctl call (get_term_chars)\n", 
		   error_description(errno)));
	  /* set to defaults for terminal driver */
	  backspace = BACKSPACE;
	  kill_line = ctrl('U');
	}
	else {
#if defined(TERMIO) || defined(TERMIOS)
	  backspace = term_buffer.c_cc[VERASE];
	  kill_line = term_buffer.c_cc[VKILL];
#else
	  backspace = term_buffer.sg_erase;
	  kill_line = term_buffer.sg_kill;
#endif
	}
}
