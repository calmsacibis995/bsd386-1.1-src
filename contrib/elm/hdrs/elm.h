
/* $Id: elm.h,v 5.10 1993/08/10 18:49:32 syd Exp  */

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
 * $Log: elm.h,v $
 * Revision 1.2  1994/01/14  00:52:33  cp
 * 2.4.23
 *
 * Revision 5.10  1993/08/10  18:49:32  syd
 * When an environment variable was given as the tmpdir definition the src
 * and dest overlapped in expand_env.  This made elm produce a garbage
 * expansion because expand_env cannot cope with overlapping src and
 * dest.  I added a new variable raw_temp_dir to keep src and dest not to
 * overlap.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.9  1993/05/08  20:03:12  syd
 * add sleepmsg to list
 *
 * Revision 5.8  1993/05/08  18:56:16  syd
 * created a new elmrc variable named "readmsginc".  This specifies an
 * increment by which the message count is updated.  If this variable is
 * set to, say, 25, then the message count will only be updated every 25
 * messages, displaying 0, 25, 50, 75, and so forth.  The default value
 * of 1 will cause Elm to behave exactly as it currently does in PL21.
 * From: Eric Peterson <epeterso@encore.com>
 *
 * Revision 5.7  1993/01/20  17:37:56  syd
 * fix spacing of =- to avoid compiler warnings
 *
 * Revision 5.6  1993/01/20  04:01:07  syd
 * Adds a new integer parameter builtinlines.
 * if (builtinlines < 0) and (the length of the message < LINES on
 *       screen + builtinlines) use internal.
 * if (builtinlines > 0) and (length of message < builtinlines)
 * 	use internal pager.
 * if (builtinlines = 0) or none of the above conditions hold, use the
 * external pager if defined.
 * From: "John P. Rouillard" <rouilj@ra.cs.umb.edu>
 *
 * Revision 5.5  1992/10/25  02:38:27  syd
 * Add missing new flags for new elmrc options for confirm
 * From: Syd
 *
 * Revision 5.4  1992/10/24  13:44:41  syd
 * There is now an additional elmrc option "displaycharset", which
 * sets the charset supported on your terminal. This is to prevent
 * elm from calling out to metamail too often.
 * Plus a slight documentation update for MIME composition (added examples)
 * From: Klaus Steinberger <Klaus.Steinberger@Physik.Uni-Muenchen.DE>
 *
 * Revision 5.3  1992/10/24  13:35:39  syd
 * changes found by using codecenter on Elm 2.4.3
 * From: Graham Hudspith <gwh@inmos.co.uk>
 *
 * Revision 5.2  1992/10/17  22:58:57  syd
 * patch to make elm use (or in my case, not use) termcap/terminfo ti/te.
 * From: Graham Hudspith <gwh@inmos.co.uk>
 *
 * Revision 5.1  1992/10/03  22:34:39  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/**  Main header file for ELM mail system.  **/


#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include "../hdrs/curses.h"
#include "../hdrs/defs.h"

#ifdef	BSD
#include <setjmp.h>
#endif

/******** static character string containing the version number  *******/

static char ident[] = { WHAT_STRING };

/******** and another string for the copyright notice            ********/

static char copyright[] = { 
		"@(#)          (C) Copyright 1986,1987, Dave Taylor\n@(#)          (C) Copyright 1988-1992, The Usenet Community Trust\n" };

/******** global variables accessable by all pieces of the program *******/

int check_size = 0;		/* don't start mailer if no mail */
int current = 0;		/* current message number  */
int header_page = 0;     	/* current header page     */
int inalias = 0;		/* TRUE if in the alias menu */
int last_current = -1;		/* previous current message */
int last_header_page = -1;     	/* last header page        */
int message_count = 0;		/* max message number      */
int headers_per_page = 0;	/* number of headers/page  */
int original_umask = 0;		/* original umask, for restore before subshell */
int sendmail_verbose = 0;       /* Extended mail debugging */
int redraw = 0,	 		/* do we need to rewrite the entire screen? */
    nucurr = 0,	 		/* change list or just the current pointer...   */
    nufoot = 0,	 		/* clear lines 16 thru bottom and new menu  */
    readmsginc = 1,		/* increment of msg cnt when reading new mbox */
    sleepmsg = 2;		/* time to sleep for messages being overwritten on screen */
#ifdef MIME
char charset[SLEN] = {0};		/* name of character set */
char display_charset[SLEN] = {0};	/* the charset, the display supports */
char charset_compatlist[SLEN] = {0};	/* list of charsets which are a
					   superset of us-ascii */
char text_encoding[SLEN] = {0};	/* default encoding for text/plain */
#endif
char cur_folder[SLEN] = {0};	/* name of current folder */
char cur_tempfolder[SLEN] = {0};/* name of temp folder open for a mailbox */
char defaultfile[SLEN] = {0};	/* name of default folder */
char temp_dir[SLEN] = {0};      /* name of temp directory */
char raw_temp_dir[SLEN] = {0};  /* unexpanded name of temp directory */
char hostname[SLEN] = {0};	/* name of machine we're on*/
char hostdomain[SLEN] = {0};	/* name of domain we're in */
char hostfullname[SLEN] = {0};	/* name of FQDN we're in */
char item[WLEN] = "message";	/* either "message" or "alias" */
char items[WLEN] = "messages";	/* plural: either "messages" or "aliases" */
char Item[WLEN] = "Message";	/* CAP: either "Message" or "Alias" */
char Items[WLEN] = "Messages";	/* CAP-plural: either "Messages" or "Aliases" */
char Prompt[WLEN] = "Command: ";	/* Menu Prompt: either "Command" or "Alias" */
char username[SLEN] = {0};	/* return address name!    */
char full_username[SLEN] = {0};	/* Full username - gecos   */
char home[SLEN] = {0};		/* home directory of user  */
char folders[SLEN] = {0};	/* folder home directory   */
char raw_folders[SLEN] = {0};	/* unexpanded folder home directory   */
char recvd_mail[SLEN] = {0};	/* folder for storing received mail     */
char raw_recvdmail[SLEN] = {0};	/* unexpanded recvd_mail name */
char editor[SLEN] = {0};	/* editor for outgoing mail*/
char raw_editor[SLEN] = {0};	/* unexpanded editor for outgoing mail*/
char alternative_editor[SLEN] = {0};	/* alternative editor...   */
char printout[SLEN] = {0};	/* how to print messages   */
char raw_printout[SLEN] = {0};	/* unexpanded how to print messages   */
char sent_mail[SLEN] = {0};	/* name of file to save copies to */
char raw_sentmail[SLEN] = {0};	/* unexpanded name of file to save to */
char calendar_file[SLEN] = {0};	/* name of file for clndr  */
char raw_calendar_file[SLEN] = {0};	/* unexpanded name of file for clndr  */
char attribution[SLEN] = {0};	/* attribution string for replies     */
char prefixchars[SLEN] = "> ";	/* prefix char(s) for msgs */
char shell[SLEN] = {0};		/* current system shell    */
char raw_shell[SLEN] = {0};	/* unexpanded current system shell    */
char pager[SLEN] = {0};		/* what pager to use       */
char raw_pager[SLEN] = {0};	/* unexpanded what pager to use       */
char batch_subject[SLEN] = {0};	/* subject buffer for batchmail */
char included_file[SLEN] = {0};	/* prepared file to include in the edit buf */
char local_signature[SLEN] = {0};	/* local msg signature file     */
char raw_local_signature[SLEN] = {0};	/* unexpanded local msg signature file     */
char remote_signature[SLEN] = {0};	/* remote msg signature file    */
char raw_remote_signature[SLEN] = {0};	/* unexpanded remote msg signature file    */
char version_buff[NLEN] = {0};	/* version buffer */
char e_editor[SLEN] = {0};	/* "~e" editor...   */
char v_editor[SLEN] = {0};	/* "~v" editor...   */
char config_options[SLEN] = {0};	/* which options are in o)ptions */
char allowed_precedences[SLEN] = {0};	/* list of precedences user may specify */
 
char *def_ans_yes;		/* default yes answer - single char, lc	*/
char *def_ans_no;		/* default no answer - single char, lc	*/
char *nls_deleted;		/* [deleted] */
char *nls_form;			/* Form */
char *nls_message;		/* Message */
char *nls_to;			/* To */
char *nls_from;			/* From */
char *nls_page;			/* Page */
char *change_word;		/* change */
char *save_word;		/* save */
char *copy_word;		/* copy */
char *cap_save_word;		/* Save */
char *cap_copy_word;		/* Copy */
char *saved_word;		/* saved */
char *copied_word;		/* copied */

char backspace,			/* the current backspace char */
     escape_char = TILDE_ESCAPE,/* '~' or something else..    */
     kill_line;			/* the current kill-line char */

char up[SHORT] = {0},		/* cursor control seq's    */
     down[SHORT] = {0},
     left[SHORT] = {0},
     right[SHORT] = {0};
int  cursor_control = FALSE;	/* cursor control avail?   */

int  has_highlighting = FALSE;	/* highlighting available? */

char *weedlist[MAX_IN_WEEDLIST] = {0};
int  weedcount;

int allow_forms = NO;		/* flag: are AT&T Mail forms okay?  */
int mini_menu = 1;		/* flag: menu specified?	    */
int metoo = 0;			/* flag: copy me on mail to alias?  */
int prompt_after_pager = 1;	/* flag: prompt after pager exits   */
int folder_type = 0;		/* flag: type of folder		    */
int auto_copy = 0;		/* flag: automatically copy source? */
int filter = 1;			/* flag: weed out header lines?	    */
int resolve_mode = 1;		/* flag: delete saved mail?	    */
int auto_cc = 0;		/* flag: mail copy to user?	    */
int noheader = 1;		/* flag: copy + header to file?     */
int title_messages = 1;		/* flag: title message display?     */
int forwarding = 0;		/* flag: are we forwarding the msg? */
int hp_terminal = 0;		/* flag: are we on HP term?	    */
int hp_softkeys = 0;		/* flag: are there softkeys?        */
int save_by_name = 1;		/* flag: save mail by login name?   */
int force_name = 0;		/* flag: save by name forced?	    */
int mail_only = 0;		/* flag: send mail then leave?      */
int check_only = 0;		/* flag: check aliases then leave?  */
int batch_only = 0;		/* flag: send without prompting?    */
int move_when_paged = 0;	/* flag: move when '+' or '-' used? */
int point_to_new = 1;		/* flag: start pointing at new msg? */
int builtin_lines= -3;		/* int: if < 0 use builtin if message
					shorter than LINES+builtin_lines
					else use pager. If > 0 use builtin
					if message has fewer than # of lines */
int bounceback = 0;		/* flag: bounce copy off remote?    */
int always_keep = 1;		/* flag: always keep unread msgs?   */
int always_store = 0;		/* flag: always store read msgs?    */
int always_del = 0;		/* flag: always delete marked msgs? */
int arrow_cursor = 0;		/* flag: use "->" cursor regardless?*/
int debug = 0; 			/* flag: default is no debug!       */
int user_level = 0;		/* flag: how good is the user?      */
int selected = 0;		/* flag: used for select stuff      */
int names_only = 1;		/* flag: display user names only?   */
int question_me = 1;		/* flag: ask questions as we leave? */
int keep_empty_files = 0;	/* flag: leave empty folder files? */
int clear_pages = 0;		/* flag: act like "page" (more -c)? */
int prompt_for_cc = 1;		/* flag: ask user for "cc:" value?  */
int sig_dashes = 1;		/* flag: include dashes above sigs? */
int use_tite = 1;		/* flag: use termcap/terminfo ti/te?*/
int confirm_append = 0;		/* flag: confirm append to folder?  */
int confirm_create = 0;		/* flag: confirm create new folder? */
int confirm_files = 0;		/* flag: confirm files for append?  */
int confirm_folders = 0;	/* flag: confirm folders for create?*/

int sortby = REVERSE SENT_DATE;	/* how to sort incoming mail...     */
int alias_sortby = NAME_SORT;	/* how to sort aliases...           */

long timeout = 600L;		/* timeout (secs) on main prompt    */

/** set up some default values for a 'typical' terminal *snicker* **/

int LINES=23;			/** lines per screen      **/
int COLUMNS=80;			/** columns per page      **/
#ifdef SIGWINCH
int resize_screen = 0;		/** SIGWINCH occured?	  **/
#endif

long size_of_pathfd;		/** size of pathfile, 0 if none **/

FILE *mailfile;			/* current folder	    */
FILE *debugfile;		/* file for debug output    */
FILE *pathfd;			/* path alias file          */
FILE *domainfd;			/* domain file		    */
nl_catd elm_msg_cat = 0;	/* message catalog	    */

long mailfile_size;		/* size of current mailfile */

int   max_headers = 0;		/* number of headers allocated */

struct header_rec **headers;    /* array of header structure pointers */

struct alias_rec **aliases;     /* for the alias menu */
int   max_aliases = 0;		/* number of aliases allocated */

struct addr_rec *alternative_addresses;	/* how else do we get mail? */

int system_data = -1;		/* fileno of system data file */
int user_data = -1;		/* fileno of user data file   */

int userid;			/* uid for current user	      */
int groupid;			/* groupid for current user   */
#ifdef SAVE_GROUP_MAILBOX_ID
int mailgroupid;		/* groupid for current user   */
#endif

#ifdef	BSD
jmp_buf GetPromptBuf;		/* setjmp buffer */
int InGetPrompt;		/* set if in GetPrompt() in read() */
#endif
