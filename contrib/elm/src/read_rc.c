
static char rcsid[] = "@(#)Id: read_rc.c,v 5.25 1993/08/23 02:58:04 syd Exp ";

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
 * $Log: read_rc.c,v $
 * Revision 1.2  1994/01/14  00:55:31  cp
 * 2.4.23
 *
 * Revision 5.25  1993/08/23  02:58:04  syd
 * Call to expand_env for pager should be call to do_expand_env.
 * temp_dir not initialized.
 * From: Jan.Djarv@sa.erisoft.se (Jan Djarv)
 *
 * Revision 5.24  1993/08/10  18:49:32  syd
 * When an environment variable was given as the tmpdir definition the src
 * and dest overlapped in expand_env.  This made elm produce a garbage
 * expansion because expand_env cannot cope with overlapping src and
 * dest.  I added a new variable raw_temp_dir to keep src and dest not to
 * overlap.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.23  1993/08/03  19:28:39  syd
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
 * Revision 5.22  1993/08/03  18:57:21  syd
 * default_weedlist() can't use pmalloc, because weedout() calls free when
 * it encounters *clear-weed-list*.
 * From: Jan.Djarv@sa.erisoft.se (Jan Djarv)
 *
 * Revision 5.21  1993/08/03  18:55:42  syd
 * Spaces in weedout strings wasn't preserved ("From " became "From").
 * From: Jan.Djarv@sa.erisoft.se (Jan Djarv)
 *
 * Revision 5.20  1993/07/20  02:41:24  syd
 * Three changes to expand_env() in src/read_rc.c:  make it non-destructive,
 * have it return an error code instead of bailing out, and add a buffer
 * size argument to avoid overwritting the destination.  The first is to
 * avoid all of the gymnastics Elm needed to go through (and occasionally
 * forgot to go through) to protect the value handed to expand_env().
 * The second is because expand_env() was originally written to support
 * "elmrc" and bailing out was a reasonable thing to do there -- but not
 * in the other places where it has since been used.  The third is just
 * a matter of practicing safe source code.
 *
 * This patch changes all invocations to expand_env() to eliminate making
 * temporary copies (now that the routine is non-destructive) and to pass
 * in a destination length.  Since expand_env() no longer bails out on
 * error, a do_expand_env() routine was added to src/read_rc.c handle
 * this.  Moreover, the error message now gives some indication of what
 * the problem is rather than just saying "can't expand".
 *
 * Gratitous change to src/editmsg.c renaming filename variables to
 * clarify the purpose.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.19  1993/06/10  03:08:38  syd
 * I have been repetedly crashing elm 2.4.22 until I discovered that a
 * test for failing to open the system elmrc file is mising. This could
 * usefully be added in read_rc.c. Version info and patch follows:
 * From: "C.R. Ritson" <C.R.Ritson@newcastle.ac.uk>
 *
 * Revision 5.18  1993/06/10  03:07:39  syd
 * This fixes a bug in the MIME code.  Include_Part() uses expand_env()
 * to expand the include file name, but since expand_env() is destructive
 * [it uses strtok()] the file name gets corrupted, and the "Content-Name"
 * header can contain a bogus value.  The easy fix would be a one-line
 * hack to Include_Part to use a temporary buffer.  This patch does not
 * implement the easy fix.  *Every* place expand_env() is used, its side
 * effects cause problems.  I think the right fix is to make expand_env()
 * non-destructive (i.e. have it duplicate the input to a temporary buffer
 * and work from there).  The attached patch modifies expand_env() in
 * that manner, and eliminates all of the `copy to a temporary buffer'
 * calls that precede it throughout elm.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.17  1993/06/10  02:55:34  syd
 * Write options to elmrc even if their values are empty strings.
 * Rationalize code that reads and writes weedouts and alternates.
 * From: chip%fin@myrddin.sybus.com
 *
 * Revision 5.16  1993/05/16  20:59:14  syd
 * fix not closing file
 * From: Syd
 *
 * Revision 5.15  1993/05/08  20:25:33  syd
 * Add sleepmsg to control transient message delays
 * From: Syd
 *
 * Revision 5.14  1993/05/08  20:06:15  syd
 * add sleepinterval to valid options
 *
 * Revision 5.13  1993/05/08  18:56:16  syd
 * created a new elmrc variable named "readmsginc".  This specifies an
 * increment by which the message count is updated.  If this variable is
 * set to, say, 25, then the message count will only be updated every 25
 * messages, displaying 0, 25, 50, 75, and so forth.  The default value
 * of 1 will cause Elm to behave exactly as it currently does in PL21.
 * From: Eric Peterson <epeterso@encore.com>
 *
 * Revision 5.12  1993/02/03  19:06:31  syd
 * Remove extra strchr/strcat/strcpy et al declarations
 * From: Syd
 *
 * Revision 5.11  1993/01/20  04:01:07  syd
 * Adds a new integer parameter builtinlines.
 * if (builtinlines < 0) and (the length of the message < LINES on
 *       screen + builtinlines) use internal.
 * if (builtinlines > 0) and (length of message < builtinlines)
 * 	use internal pager.
 * if (builtinlines = 0) or none of the above conditions hold, use the
 * external pager if defined.
 * From: "John P. Rouillard" <rouilj@ra.cs.umb.edu>
 *
 * Revision 5.10  1992/11/26  00:46:13  syd
 * changes to first change screen back (Raw off) and then issue final
 * error message.
 * From: Syd
 *
 * Revision 5.9  1992/11/24  01:44:18  syd
 * Add raw/no tite stuff around directory create questions
 * From: Syd via bug report from ade@clark.edu
 *
 * Revision 5.8  1992/11/07  16:27:33  syd
 * Fix where elm duplicates the entry's from the global elm.rc.
 * It will however still copy the global weedout headers to your local
 * elmrc if they where not already in it.
 * From: janw@fwi.uva.nl (Jan Wortelboer)
 *
 * Revision 5.7  1992/10/24  13:44:41  syd
 * There is now an additional elmrc option "displaycharset", which
 * sets the charset supported on your terminal. This is to prevent
 * elm from calling out to metamail too often.
 * Plus a slight documentation update for MIME composition (added examples)
 * From: Klaus Steinberger <Klaus.Steinberger@Physik.Uni-Muenchen.DE>
 *
 * Revision 5.6  1992/10/24  13:35:39  syd
 * changes found by using codecenter on Elm 2.4.3
 * From: Graham Hudspith <gwh@inmos.co.uk>
 *
 * Revision 5.5  1992/10/17  22:58:57  syd
 * patch to make elm use (or in my case, not use) termcap/terminfo ti/te.
 * From: Graham Hudspith <gwh@inmos.co.uk>
 *
 * Revision 5.4  1992/10/17  22:42:24  syd
 * Add flags to read_rc to support command line overrides of the option.
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.3  1992/10/17  22:25:29  syd
 * TEXT_SORT was supported, but UNRECOGNIZED
 * From: ls@dmicvx.dmi.min.dk  (Lennart Sorth)
 *
 * Revision 5.2  1992/10/11  00:59:39  syd
 * Fix some compiler warnings that I receive compiling Elm on my SVR4
 * machine.
 * From: Tom Moore <tmoore@fievel.DaytonOH.NCR.COM>
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** This file contains programs to allow the user to have a .elm/elmrc file
    in their home directory containing any of the following: 

	fullname= <username string>
	maildir = <directory>
	tmpdir  = <directory>
	sentmail = <file>
	editor  = <editor>
	receviedmail= <file>
	calendar= <calendar file name>
	shell   = <shell>
	print   = <print command>
	weedout = <list of headers to weed out>
	prefix  = <copied message prefix string>
	pager   = <command to use for displaying messages>
	configoptions = <list of letters to use for options menu>
	precedences = <list of delivery precedences>
	
	escape  = <single character escape, default = '~' >

--
	signature = <.signature file for all outbound mail>
OR:
	localsignature = <.signature file for local mail>
	remotesignature = <.signature file for non-local mail>
--

	bounceback= <hop count threshold, or zero to disable>
	readmsginc = <new folder message count display increment>
	sleepmsg = time to 'sleep' while displaying transient messages
	timeout = <seconds for main menu timeout or zero to disable>
	userlevel = <0=amateur, 1=okay, 2 or greater = expert!>

	sortby  = <sent, received, from, size, subject, mailbox, status>
	alias_sortby  = <alias, name>

	alternatives = <list of addresses that forward to us>

	builtinlines = <use builtin pager if message shorter than # lines>

    and/or the logical arguments:
	
	autocopy    [on|off]
	askcc	    [on|off]
	copy        [on|off]	
	resolve     [on|off]
	weed        [on|off]
	noheader    [on|off]
	titles      [on|off]
	savebyname  [on|off]
	forcename   [on|off]
	movepage    [on|off]
	pointnew    [on|off]
	hpkeypad    [on|off]
	hpsoftkeys  [on|off]
	alwayskeep  [on|off]
	alwaysstore [on|off]
	alwaysdel   [on|off]
	arrow	    [on|off]
	menus	    [on|off]
	metoo	    [on|off]
	forms	    [on|off]
	names	    [on|off]
	ask	    [on|off]
	keepempty   [on|off]
	promptafter [on|off]
	sigdashes   [on|off]
	usetite	    [on|off]


    Lines starting with '#' are considered comments and are not checked
    any further!

**/

#define SAVE_OPTS
#include "headers.h"
#include "save_opts.h"
#include "s_elm.h"
#include <errno.h>

extern opts_menu *find_cfg_opts();
extern int errno;

char  *error_description(), *shift_lower();

#define  metachar(c)	(c == '+' || c == '%' || c == '=')
#ifndef ok_rc_char
#define ok_rc_char(c)   (isalnum(c) || c == '-' || c == '_')
#endif


#define ASSIGNMENT      0
#define WEEDOUT		1
#define ALTERNATIVES	2

#define SYSTEM_RC	0
#define LOCAL_RC	1

static int lineno = 0;
static int errors = 0;


static void
do_expand_env(descr, dest, src, destlen)
    char	*descr, *dest, *src;
    unsigned	destlen;
{
    if (expand_env(dest, src, destlen) != 0) {
	printf(catgets(elm_msg_cat, ElmSet, ElmCannotInitErrorExpanding,
	    "\r\nCannot initialize \"%s\" - error expanding \"%s\".\r\n"),
	    descr, src);
	leave(0);
    }
}


read_rc_file()
{
	/** this routine does all the actual work of reading in the
	    .rc file... **/

	FILE *file;
	char buffer[SLEN], filename[SLEN], *cp, 
	     	temp[SLEN]; /* for when an option is run through expandenv */
	int  i, ch, len, err;

	/* Establish some defaults in case elmrc is incomplete or not there.
	 * Defaults for other elmrc options were established in their
	 * declaration - in elm.h.  And defaults for sent_mail and recvd_mail
	 * are established after the elmrc is read in since these default
	 * are based on the folders directory name, which may be given
	 * in the emrc.
	 * Also establish alternative_editor here since it is based on
	 * the default editor and not on the one that might be given in the
	 * elmrc.
	 */
	 
	default_weedlist();

	errors = 0;
	alternative_addresses = NULL; 	/* none yet! */

	raw_local_signature[0] = raw_remote_signature[0] =
		local_signature[0] = remote_signature[0] =
		raw_recvdmail[0] = raw_sentmail[0] = 
		allowed_precedences[0] = '\0';
		/* no defaults for those */


	strcpy(raw_shell, ((cp = getenv("SHELL")) == NULL)? default_shell : cp);
	do_expand_env("shell", shell, raw_shell, sizeof(shell));

	strcpy(raw_pager, ((cp = getenv("PAGER")) == NULL)? default_pager : cp);
	do_expand_env("pager", pager, raw_pager, sizeof(pager));

	strcpy(raw_temp_dir, (cp = getenv("TMPDIR")) ? cp : default_temp);
	do_expand_env("temp_dir", temp_dir, raw_temp_dir, sizeof(temp_dir));
	if (temp_dir[strlen (temp_dir)-1] != '/')
	    strcat(temp_dir, "/");

	strcpy(raw_editor, ((cp = getenv("EDITOR")) == NULL)? default_editor:cp);
	strcpy(alternative_editor, raw_editor);
	do_expand_env("editor", editor, raw_editor, sizeof(editor));

	strcpy(raw_printout, default_printout);
	do_expand_env("printout", printout, raw_printout, sizeof(printout));

	sprintf(raw_folders, "~/%s", default_folders);
	do_expand_env("folders", folders, raw_folders, sizeof(folders));

	sprintf(raw_calendar_file, "~/%s", dflt_calendar_file);
	do_expand_env("calendar_file", calendar_file, raw_calendar_file,
	    sizeof(calendar_file));

	strcpy(e_editor, emacs_editor);
	strcpy(v_editor, default_editor);

	strcpy(raw_printout, default_printout);
	strcpy(printout, raw_printout);

	sprintf(raw_folders, "%s/%s", home, default_folders);
	strcpy(folders, raw_folders);

	sprintf(raw_calendar_file, "%s/%s", home, dflt_calendar_file);
	strcpy(calendar_file, raw_calendar_file);

#ifdef MIME
	strcpy(charset, default_charset);
	strcpy(charset_compatlist, COMPAT_CHARSETS);
	strcpy(display_charset, default_display_charset);
	strcpy(text_encoding, default_encoding);
#endif

	/* see if the user has a $HOME/.elm directory */
	sprintf(filename, "%s/.elm", home);
	if (access(filename, 00) == -1) {
	  if(batch_only)  {
	    printf(catgets(elm_msg_cat, ElmSet, ElmBatchDirNotice, "\nNotice:\
\nThis version of ELM requires the use of a .elm directory to store your\
\nelmrc and alias files. I'd like to create the directory .elm for you\
\nand set it up, but I can't in \"batch mode\".\
\nPlease run ELM in \"normal mode\" first.\n"));
	    exit(0);
	  }

	  Raw(ON | NO_TITE);
	  MCprintf(catgets(elm_msg_cat, ElmSet, ElmDirNotice, "\n\rNotice:\
\n\rThis version of ELM requires the use of a .elm directory in your home\
\n\rdirectory to store your elmrc and alias files. Shall I create the\
\n\rdirectory .elm for you and set it up (%c/%c/q)? %c%c"),
		*def_ans_yes, *def_ans_no, *def_ans_no, BACKSPACE);

	  fflush(stdout);
	  ch=getchar();
	  ch = tolower(ch);
	  if (ch == '\n' || ch == '\r') /* they took the default by pressing cr */
		ch = *def_ans_no;

	  if (ch == *def_ans_no) {
	    printf(catgets(elm_msg_cat, ElmSet, ElmDirNoticeNo,
"No.\n\rVery well. I won't create it.\n\rBut, you may run into difficulties later.\n\r"));
	    if (sleepmsg > 0)
		sleep(sleepmsg * 2);
	  }
	  else if (ch == *def_ans_yes) {
	    printf(catgets(elm_msg_cat, ElmSet, ElmDirNoticeYes,
		"Yes.\n\rGreat! I'll do it now.\n\r"));
	    create_new_elmdir();
	  }
	  else {
	    printf(catgets(elm_msg_cat, ElmSet, ElmDirNoticeQuit,
		    "Quit.\n\rOK.  Bailing out of ELM.\n\r"));
	    Raw(OFF | NO_TITE);
	    exit(0);
	  }
	  Raw(OFF | NO_TITE);
	}

	/* try system-wide rc file */
	file = fopen(system_rc_file, "r");
	if ( file != NULL ) {
	  do_rc(file, SYSTEM_RC);
	  fclose(file);
	}

	/* Look for the elmrc file */
	sprintf(filename, "%s/%s", home, elmrcfile);
	if ((file = fopen(filename, "r")) == NULL) {
	  dprint(2, (debugfile, "Warning:User has no \".elm/elmrc\" file\n\n"));

	  /* look for old-style .elmrc file in $HOME */
	  sprintf(filename, "%s/.elmrc", home);
	  if (access(filename, 00) != -1) {
	    move_old_files_to_new();

	    /* try to open elmrc file again */
	    sprintf(filename, "%s/%s", home, elmrcfile);
	    if((file = fopen(filename, "r")) == NULL) {
	      err = errno;
	      dprint(2, (debugfile,
		"Warning: could not open new \".elm/elmrc\" file.\n"));
	      dprint(2, (debugfile, "** %s **\n", error_description(err)));
	      printf(catgets(elm_msg_cat, ElmSet, ElmCouldNotOpenNewElmrc,
		"Warning: could not open new \".elm/elmrc\" file! Using default parameters.\n\r"));
	      if (sleepmsg > 0)
		    sleep(sleepmsg * 2);
	    }
	  }
	}
	if (file != NULL) {
  	  do_rc(file, LOCAL_RC);
	  fclose(file);
	}

/* validate/correct config_options string */

	if (config_options[0]) {
	    register char *s, *t;
	    register opts_menu *o;
	    s = shift_lower(config_options);
	    for (t = config_options; *s; ++s) {
		if (*s == '_' || *s == '^') {
		    *t++ = *s;
		    continue;
		}
		o = find_cfg_opts(*s);
		if (o != NULL)
		    *t++ = *s; /* silently remove invalid options */
		}
	    *t = '\0';
	}
	do_expand_env("folders", folders, raw_folders, sizeof(folders));

	do_expand_env("temp_dir", temp_dir, raw_temp_dir, sizeof(temp_dir));
  	if (temp_dir[strlen (temp_dir)-1] != '/')
  	    strcat(temp_dir, "/");
  
	do_expand_env("shell", shell, raw_shell, sizeof(shell));

	do_expand_env("editor", editor, raw_editor, sizeof(editor));

	do_expand_env("calendar_file", calendar_file, raw_calendar_file,
	    sizeof(calendar_file));

	do_expand_env("printout", printout, raw_printout, sizeof(printout));

	do_expand_env("pager", pager, raw_pager, sizeof(pager));
	if (equal(pager, "builtin+") || equal(pager, "internal+"))
		clear_pages++;

	do_expand_env("local_signature", local_signature,
	    raw_local_signature, sizeof(local_signature));
	do_expand_env("remote_signature", remote_signature,
	    raw_remote_signature, sizeof(remote_signature));

	if (equal(local_signature, remote_signature) &&
	    (equal(shift_lower(local_signature), "on") ||
	    equal(shift_lower(local_signature), "off"))) {
	    errors++;

	    printf(catgets(elm_msg_cat, ElmSet, ElmSignatureObsolete,
	"\"signature\" used in obsolete way in .elm/elmrc file. Ignored!\n\r\
\t(Signature should specify the filename to use rather than on/off.)\n\r\n"));

	    raw_local_signature[0] = raw_remote_signature[0] =
		local_signature[0] = remote_signature[0] = '\0';
	}

	if (hp_softkeys) hp_terminal=TRUE;	/* must be set also! */

	allow_forms = (allow_forms?MAYBE:NO);
	if (bounceback > MAX_HOPS) {
	    errors++;
	    printf(catgets(elm_msg_cat, ElmSet, ElmBouncebackGTMaxhops,
	"Warning: bounceback is set to greater than %d (max-hops). Ignored.\n\r"),
		     MAX_HOPS);
	    bounceback = 0;
	}

	if ((timeout != 0) && (timeout < 10)) {
	    errors++;
	    printf(catgets(elm_msg_cat, ElmSet, ElmTimeoutLTTen,
		 "Warning: timeout is set to less than 10 seconds. Ignored.\n\r"));
	    timeout = 0;
	}

	if (readmsginc < 1) {
		errors++;
		printf(catgets(elm_msg_cat, ElmSet, ElmReadMessageIncrement,
		"Warning: readmsginc is set to less than 1.  Ignored.\n\r"));
		readmsginc = 1;
	}

	if (sleepmsg < 0) {
		errors++;
		printf(catgets(elm_msg_cat, ElmSet, ElmSleepMessageInvalid,
		"Warning: sleepmsg is set to less than 0.  Setting to 0.\n\r"));
		sleepmsg = 0;
	}

	/* see if the user has a folders directory */
	if (access(folders, 00) == -1) {
	  if(batch_only)  {
	    printf(catgets(elm_msg_cat, ElmSet, ElmBatchNoticeFoldersDir, "\n\
Notice:\n\
ELM requires the use of a folders directory to store your mail folders in.\n\
I'd like to create the directory %s for you,\n\
but I can't in \"batch mode\". Please run ELM in \"normal mode\" first.\n"),
		folders);
	    exit(0);
	  }

	  Raw( ON | NO_TITE );
	  MCprintf(catgets(elm_msg_cat, ElmSet, ElmNoticeFoldersDir,"\n\rNotice:\n\r\
ELM requires the use of a folders directory to store your mail folders in.\n\r\
Shall I create the directory %s for you (%c/%c/q)? %c%c"),
		folders, *def_ans_yes, *def_ans_no, *def_ans_yes, BACKSPACE);

	  fflush(stdout);
	  ch=getchar();
	  ch = tolower(ch);
	  if (ch == '\n' || ch == '\r') /* they took the default by pressing cr */
		ch = *def_ans_yes;

	  if (ch == *def_ans_no) {
	    printf(catgets(elm_msg_cat, ElmSet, ElmDirNoticeNo,
"No.\n\rVery well. I won't create it.\n\rBut, you may run into difficulties later.\n\r"));
	    if (sleepmsg > 0)
		    sleep(sleepmsg * 2);
	  }
	  else if (ch == *def_ans_yes) {
	    printf(catgets(elm_msg_cat, ElmSet, ElmDirNoticeYes,
		"Yes.\n\rGreat! I'll do it now.\n\r"));
	    create_new_folders();
	  }
	  else {
	    printf(catgets(elm_msg_cat, ElmSet, ElmDirNoticeQuit,
		    "Quit.\n\rOK.  Bailing out of ELM.\n\r"));
	    Raw(OFF | NO_TITE);
	    exit(0);
	  }
	  Raw( OFF | NO_TITE );
	}

	/* If recvd_mail or sent_mail havent't yet been established in
	 * the elmrc, establish them from their defaults.
	 * Then if they begin with a metacharacter, replace it with the
	 * folders directory name.
	 */
	if(*raw_recvdmail == '\0') {
	  strcpy(raw_recvdmail, default_recvdmail);
	}

	do_expand_env("recvd_mail", recvd_mail, raw_recvdmail,
	    sizeof(recvd_mail));

	if(metachar(recvd_mail[0])) {
	  strcpy(buffer, &recvd_mail[1]);
	  sprintf(recvd_mail, "%s/%s", folders, buffer);
	}

	if(*raw_sentmail == '\0') {
	  sprintf(raw_sentmail, default_sentmail);
	  sprintf(sent_mail, default_sentmail);
	}

	do_expand_env("sent_mail", sent_mail, raw_sentmail, sizeof(sent_mail));

	if(metachar(sent_mail[0])) {
	  strcpy(buffer, &sent_mail[1]);
	  sprintf(sent_mail, "%s/%s", folders, buffer);
	}

	if (debug > 10) 	/** only do this if we REALLY want debug! **/
	  dump_rc_results();

}


do_rc(file, lcl)
FILE *file;
int lcl;
{
	static int prev_type = 0;
	int x;
	char buffer[SLEN], word1[SLEN], word2[SLEN];

	if (!file) return;
	lineno=0;

	while (x = mail_gets(buffer, SLEN, file)) {
	    lineno++;
	    no_ret(buffer);	 	/* remove return */
	    if (buffer[0] == '#'        /* comment       */
	     || x < 2)     /* empty line    */
	      continue;

	    if(breakup(buffer, word1, word2) == -1)
	        continue;		/* word2 is null - let default value stand */

	    if(strcmp(word1, "warnings") == 0)
		continue;		/* grandfather old keywords */

	    strcpy(word1, shift_lower(word1));	/* to lower case */
	    x = do_set(file, word1, word2, lcl);

	    if (x == 0) {
		if (prev_type == DT_ALT) {
		    alternatives(buffer);
		} else if (prev_type == DT_WEE) {
		    weedout(buffer);
		} else {
		    errors++;
		    printf(catgets(elm_msg_cat, ElmSet, ElmBadSortKeyInElmrc,
      "I can't understand sort key \"%s\" in line %d in your \".elm/elmrc\" file\n\r"),
			word1, lineno);
		}
	    } else
		prev_type = x;
	}
}

/*
 * set the named parameter according to save_info structure.
 * This routine may call itself (DT_SYN or DT_MLT).
 * Also tags params that were set in "local" (personal) RC file
 * so we know to save them back out in "o)ptions" screen.
 * Uses an internal table to decode sort-by params...should be coupled
 * with sort_name(), etc...but...
 */

do_set(file, word1, word2, lcl)
FILE *file;
int lcl;
char *word1, *word2;
{
	register int x, y;

	for (x=0; x < NUMBER_OF_SAVEABLE_OPTIONS; ++x) {
	    y = strcmp(word1, save_info[x].name);
	    if (y <= 0)
		break;
	}

	if (y != 0)
	    return(0);

	if (save_info[x].flags & FL_SYS && lcl == LOCAL_RC)
	    return(0);

	if (lcl == LOCAL_RC)
	    save_info[x].flags |= FL_LOCAL;

	switch (save_info[x].flags & DT_MASK) {
	    case DT_SYN:
		return(do_set(file, SAVE_INFO_SYN(x), word2, lcl));

	    case DT_MLT:
		y=0;
		{ register char **s;
		for (s = SAVE_INFO_MLT(x); *s; ++s)
		    y |= do_set(file, *s, word2, lcl);
		}

/* a kludge that should be part of the "machine", but... */
		if (equal(save_info[x].name, "alwaysleave")) {
		    always_store = !always_store;
		}
		return(y); /* we shouldn't "or" the values into "y" */

	    case DT_STR:
		strcpy(SAVE_INFO_STR(x), word2);
		if (save_info[x].flags & FL_NOSPC) {
		    register char *s;
		    for (s = SAVE_INFO_STR(x); *s; ++s)
			if (*s == '_') *s=' ';
		    }
		break;

	    case DT_CHR:
		*SAVE_INFO_CHR(x) = word2[0];
		break;

	    case DT_NUM:
		*SAVE_INFO_NUM(x) = atoi(word2);
		break;

	    case DT_BOL:
		if (save_info[x].flags & FL_OR)
		    *SAVE_INFO_BOL(x) |= is_it_on(word2);
		else if (save_info[x].flags & FL_AND)
		    *SAVE_INFO_BOL(x) &= is_it_on(word2);
		else
		    *SAVE_INFO_BOL(x) = is_it_on(word2);
		break;

	    case DT_SRT:
		{ static struct { char *kw; int sv; } srtval[]={
		    {"sent", SENT_DATE},
		    {"received", RECEIVED_DATE},
		    {"recieved", RECEIVED_DATE},
		    {"rec", RECEIVED_DATE},
		    {"from", SENDER},
		    {"sender", SENDER},
		    {"size", SIZE},
		    {"lines", SIZE},
		    {"subject", SUBJECT},
		    {"mailbox", MAILBOX_ORDER},
		    {"folder", MAILBOX_ORDER},
		    {"status", STATUS},
		    {NULL, 0} };
		    char *s = word2;
		    int f;

		    f = 1;
		    strcpy(word2, shift_lower(word2));
		    if (strncmp(s, "rev-", 4) == 0 ||
			strncmp(s, "reverse-", 8) == 0) {
			f = -f;
			s = index(s, '-') + 1;
		    }

		    for (y= 0; srtval[y].kw; y++) {
			if (equal(s, srtval[y].kw))
			    break;
		    }
		    if (srtval[y].kw) {
			*SAVE_INFO_SRT(x) = f > 0 ? srtval[y].sv : -srtval[y].sv;
		    } else {
			errors++;
			printf(catgets(elm_msg_cat, ElmSet, ElmBadSortKeyInElmrc,
	  "I can't understand sort key \"%s\" in line %d in your \".elm/elmrc\" file\n\r"),
			    word2, lineno);
		    }
		}
		break;

	    case DT_ASR:
		{ static struct { char *kw; int sv; } srtval[]={
		    {"alias", ALIAS_SORT},
		    {"name", NAME_SORT},
		    {"text", TEXT_SORT},
		    {NULL, 0} };
		    char *s = word2;
		    int f;

		    f = 1;
		    strcpy(word2, shift_lower(word2));
		    if (strncmp(s, "rev-", 4) == 0 ||
			strncmp(s, "reverse-", 8) == 0) {
			f = -f;
			s = index(s, '-') + 1;
		    }

		    for (y= 0; srtval[y].kw; y++) {
			if (equal(s, srtval[y].kw))
			    break;
		    }
		    if (srtval[y].kw) {
			*SAVE_INFO_SRT(x) = f > 0 ? srtval[y].sv : -srtval[y].sv;
		    } else {
			errors++;
			printf(catgets(elm_msg_cat, ElmSet, ElmBadAliasSortInElmrc,
	"I can't understand alias sort key \"%s\" in line %d in your \".elm/elmrc\" file\n\r"),
			    word2, lineno);
		    }
		}
		break;

	    case DT_ALT:
		alternatives(word2);
		break;

	    case DT_WEE:
		weedout(word2);
		break;
	    }

	return(save_info[x].flags & DT_MASK);
}
	
weedout(string)
char *string;
{
	/** This routine is called with a list of headers to weed out.   **/

	char *strptr, *header, *p;
	int finished;

	finished = FALSE;
	strptr = string;
	while (!finished && (header = strtokq(strptr, "\t ,", TRUE)) != NULL) {
	  strptr = NULL;

	  if (!*header)
	    continue;

	  for (p = header; *p; ++p) {
	    if (*p == '_')
	      *p = ' ';
	  }

	  if (! istrcmp(header, "*end-of-user-headers*"))
	    break;

	  if (! istrcmp(header, "*end-of-defaults*"))
	    finished = TRUE;

	  if (! istrcmp(header, "*clear-weed-list*")) {
	    while (weedcount)
	      free(weedlist[--weedcount]);
	    header = "*end-of-defaults*";
	  }

	  if (matches_weedlist(header))
	    continue;

	  if (weedcount > MAX_IN_WEEDLIST) {
	    printf(catgets(elm_msg_cat, ElmSet, ElmTooManyWeedHeaders,
		   "Too many weed headers!  Leaving...\n\r"));
	    exit(1);
	  }
	  if ((p = malloc(strlen(header) + 1)) == NULL) {
	    printf(catgets(elm_msg_cat, ElmSet, ElmTooManyWeedPmalloc,
		   "Too many weed headers!  Out of memory!  Leaving...\n\r"));
	    exit(1);
	  }
	  strcpy(p, header);
	  weedlist[weedcount++] = p;
	}
}


alternatives(string)
char *string;
{
	/** This routine is called with a list of alternative addresses
	    that you may receive mail from (forwarded) **/

	char *strptr, *address;
	struct addr_rec *current_record, *previous_record;

	previous_record = alternative_addresses;	/* start 'er up! */
	/* move to the END of the alternative addresses list */

	if (previous_record != NULL)
	  while (previous_record->next != NULL)
	    previous_record = previous_record->next;

	strptr = (char *) string;

	while ((address = strtok(strptr, "\t ,\"'")) != NULL) {
	  if (previous_record == NULL) {
	    previous_record = (struct addr_rec *) pmalloc(sizeof 
		*alternative_addresses);

	    strcpy(previous_record->address, address);
	    previous_record->next = NULL;
	    alternative_addresses = previous_record;
	  }
	  else {
	    current_record = (struct addr_rec *) pmalloc(sizeof 
		*alternative_addresses);
	  
	    strcpy(current_record->address, address);
	    current_record->next = NULL;
	    previous_record->next = current_record;
	    previous_record = current_record;
	  }
	  strptr = (char *) NULL;
	}
}

default_weedlist()
{
	/** Install the default headers to weed out!  Many gracious 
	    thanks to John Lebovitz for this dynamic method of 
	    allocation!
	**/

	static char *default_list[] = { ">From", "In-Reply-To:",
		       "References:", "Newsgroups:", "Received:",
		       "Apparently-To:", "Message-Id:", "Content-Type:",
		       "Content-Length", "MIME-Version",
		       "Content-Transfer-Encoding",
		       "From", "X-Mailer:", "Status:",
		       "*end-of-defaults*", NULL
		     };

	for (weedcount = 0; default_list[weedcount] != (char *) 0;weedcount++){
	  if ((weedlist[weedcount] = 
	      malloc(strlen(default_list[weedcount]) + 1)) == NULL) {
	    printf(catgets(elm_msg_cat, ElmSet, ElmNoMemDefaultWeed,
		"\nNot enough memory for default weedlist. Leaving.\n"));
	    leave(1);
	  }
	  strcpy(weedlist[weedcount], default_list[weedcount]);
	}
}

int
matches_weedlist(buffer)
char *buffer;
{
	/** returns true iff the first 'n' characters of 'buffer' 
	    match an entry of the weedlist **/
	
	register int i;

	for (i=0;i < weedcount; i++)
	  if (strincmp(buffer, weedlist[i], strlen(weedlist[i])) == 0) 
	    return(1);

	return(0);
}

int
breakup(buffer, word1, word2)
char *buffer, *word1, *word2;
{
	/** This routine breaks buffer down into word1, word2 where 
	    word1 is alpha characters only, and there is an equal
	    sign delimiting the two...
		alpha = beta
	    For lines with more than one 'rhs', word2 is set to the
	    entire string.
	    Return -1 if word 2 is of zero length, else 0.
	**/

	register int i;
	
	for (i=0;buffer[i] != '\0' && ok_rc_char(buffer[i]); i++)
	  if (buffer[i] == '_')
	    word1[i] = '-';
	  else
	    word1[i] = tolower(buffer[i]);

	word1[i++] = '\0';	/* that's the first word! */

	/** spaces before equal sign? **/

	while (whitespace(buffer[i])) i++;
	if (buffer[i] == '=') i++;

	/** spaces after equal sign? **/

	while (whitespace(buffer[i])) i++;

	if (buffer[i] != '\0')
	  strcpy(word2, (char *) (buffer + i));
	else
	  word2[0] = '\0';

	/* remove trailing spaces from word2! */
	i = strlen(word2) - 1;
	while(i && (whitespace(word2[i]) || word2[i] == '\n'))
	  word2[i--] = '\0';

	return(*word2 == '\0' ? -1 : 0 );

}


/*
 * expand_env() - Perform environment expansion on a pathname.  Also
 * replaces "~" at the front of the path with the user's home directory.
 * Environment expansion occurs at the path component boundaries, e.g.
 * "/foo/$BAR/baz" is subject to expansion but "/foo/zzz$BAR/baz" is not.
 * Returns 0 if expansion successful, -1 if an error occurs (result too
 * long, cannot get home directory, or environment expansion failed).
 */
expand_env(dest, src, destlen)
char *dest;		/* pointer to space to hold the result	*/
char *src;		/* pointer to string to expand		*/
unsigned destlen;	/* size of the destination buffer	*/
{
    char envname_buf[SLEN], *envname_front, *expval;
    int check_for_env, len, ret;

    --destlen;		/* reserve space for trailing '\0' */
    ret = 0;		/* assume success */

    /*
     * Replace "~" at front with user's home directory.
     */
    if (src[0] == '~' && (src[1] == '\0' || src[1] == '/')) {
	if (home[0] == '\0') {
	    expval = "~";
	    ret = -1;
	} else {
	    expval = home;
	}
	if ((len = strlen(expval)) > destlen)
	    len = destlen;
	strfcpy(dest, expval, len+1);
	dest += len;
	destlen -= len;
	++src;
    }

    /*
     * Copy through the rest, performing $NAME expansion where appropriate.
     */
    check_for_env = TRUE;
    while (destlen > 0 && *src != '\0') {

	/*
	 * Check for "$NAME" at the start of every path component.
	 */
	if (check_for_env && *src == '$') {

	    /*
	     * Get the environment parameter name into "envname_buf"
	     * and advance "src" to the next path component.
	     */
	    envname_front = ++src;
	    if ((len = strcspn(src, "/")) == 0)
		len = strlen(src);
	    src += len;
	    if (len > sizeof(envname_buf)-1)
		len = sizeof(envname_buf)-1;
	    strfcpy(envname_buf, envname_front, len+1);

	    /*
	     * Copy over the environment expansion.  If the environment
	     * parameter is undefined then copy over unchanged and set
	     * a fail return status.
	     */
	    if ((expval = getenv(envname_buf)) == NULL) {
		*dest++ = '$';
		--destlen;
		expval = envname_buf;
		ret = -1;
	    }
	    if ((len = strlen(expval)) > destlen)
		len = destlen;
	    strfcpy(dest, expval, len+1);
	    dest += len;
	    destlen -= len;
	    check_for_env = FALSE;

	} else {

	    check_for_env = (*src == '/');
	    *dest++ = *src++;
	    --destlen;

	}

    }

    *dest = '\0';
    if (destlen <= 0)
	ret = -1;
    return ret;
}

#define on_off(s)	(s == 1? "ON " : "OFF")
dump_rc_results()
	{
	register int i, j, len = 0;
	char buf[SLEN], *s;

	for (i = 0; i < NUMBER_OF_SAVEABLE_OPTIONS; i++) {
	    extern char *sort_name(), *alias_sort_name();

	    switch (save_info[i].flags & DT_MASK) {
		case DT_SYN:
		case DT_MLT:
		    break;
		case DT_ALT:
		    break; /* not dumping addresses to debug file */
		case DT_WEE:
		    fprintf(debugfile, "\nAnd we're skipping the following headers:\n\t");

		    for (len = 8, j = 0; j < weedcount; j++) {
			if (weedlist[j][0] == '*') continue;	/* skip '*end-of-defaults*' */
			if (len + strlen(weedlist[j]) > 80) {
			    fprintf(debugfile, " \n\t");
			    len = 8;
			}
			fprintf(debugfile, "%s  ", weedlist[j]);
			len += strlen(weedlist[j]) + 3;
		    }
		    fprintf(debugfile, "\n\n");
		    break;

		default:
		    switch (save_info[i].flags&DT_MASK) {
	
		    case DT_STR:
			s = SAVE_INFO_STR(i);
			break;

		    case DT_NUM:
			sprintf(buf, "%d", *SAVE_INFO_NUM(i));
			s = buf;
			break;

		    case DT_CHR:
			sprintf(buf, "%c", *SAVE_INFO_CHR(i));
			s = buf;
			break;

		    case DT_BOL:
			s = on_off(*SAVE_INFO_BOL(i));
			break;

		    case DT_SRT:
			s = sort_name(SHORT);
			break;

		    case DT_ASR:
			s = alias_sort_name(SHORT);
			break;
			}

		    fprintf(debugfile, "%s = %s\n", save_info[i].name, s);
		    break;
		}
	    }
	fprintf(debugfile, "\n\n");
}

is_it_on(word)
char *word;
{
	/** Returns TRUE if the specified word is either 'ON', 'YES'
	    or 'TRUE', and FALSE otherwise.   We explicitly translate
	    to lowercase here to ensure that we have the fastest
	    routine possible - we really DON'T want to have this take
	    a long time or our startup will be major pain each time.
	**/

	static char mybuffer[NLEN];
	register int i, j;

	for (i=0, j=0; word[i] != '\0'; i++)
	  mybuffer[j++] = tolower(word[i]);
	mybuffer[j] = '\0';

	return(  (strncmp(mybuffer, "on",   2) == 0) ||
		 (strncmp(mybuffer, "yes",  3) == 0) ||
		 (strncmp(mybuffer, "true", 4) == 0)
	      );
}
