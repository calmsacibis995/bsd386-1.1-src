
/* $Id: headers.h,v 5.11 1993/08/10 20:49:40 syd Exp  */

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
 * $Log: headers.h,v $
 * Revision 1.2  1994/01/14  00:52:38  cp
 * 2.4.23
 *
 * Revision 5.11  1993/08/10  20:49:40  syd
 * Add raw_temp_dir
 *
 * Revision 5.10  1993/05/08  20:03:12  syd
 * add sleepmsg to list
 *
 * Revision 5.9  1993/05/08  18:56:16  syd
 * created a new elmrc variable named "readmsginc".  This specifies an
 * increment by which the message count is updated.  If this variable is
 * set to, say, 25, then the message count will only be updated every 25
 * messages, displaying 0, 25, 50, 75, and so forth.  The default value
 * of 1 will cause Elm to behave exactly as it currently does in PL21.
 * From: Eric Peterson <epeterso@encore.com>
 *
 * Revision 5.8  1993/04/21  01:24:54  syd
 * It's very non-portable, and fairly dangerous, to ass_u_me that you
 * know what's inside a FILE.  So don't #define clearerr().
 * From: decwrl!uunet.UU.NET!fin!chip (Chip Salzenberg)
 *
 * Revision 5.7  1993/01/20  04:01:07  syd
 * Adds a new integer parameter builtinlines.
 * if (builtinlines < 0) and (the length of the message < LINES on
 *       screen + builtinlines) use internal.
 * if (builtinlines > 0) and (length of message < builtinlines)
 * 	use internal pager.
 * if (builtinlines = 0) or none of the above conditions hold, use the
 * external pager if defined.
 * From: "John P. Rouillard" <rouilj@ra.cs.umb.edu>
 *
 * Revision 5.6  1992/10/30  21:47:30  syd
 * Use copy_message in mime shows to get encode processing
 * From: bjoerns@stud.cs.uit.no (Bjoern Stabell)
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

/**  This is the header file for ELM mail system.  **/


#include <stdio.h>
#include <fcntl.h>
#include <signal.h>

#include "curses.h"
#include "defs.h"

#ifdef	BSD
#include <setjmp.h>
#endif

#ifdef MIME
#include "mime.h"
#endif

/******** global variables accessable by all pieces of the program *******/

extern int check_size;		/* don't start mailer if no mail */
extern int current;		/* current message number  */
extern int header_page;         /* current header page     */
extern int inalias;		/* TRUE if in the alias menu */
extern int last_current;	/* previous current message */
extern int last_header_page;    /* last header page        */
extern int message_count;	/* max message number      */
extern int headers_per_page;	/* number of headers/page  */
extern int original_umask;	/* original umask, for restore before subshell */
extern int sendmail_verbose;    /* Allow extended debugging on sendmail */
extern int redraw, 		/** do we need to rewrite the entire screen? **/
           nucurr, 		/** change list or just the current pointer  **/
           nufoot, 		/** clear lines 16 thru bottom and new menu  **/
	   readmsginc,		/* msg cnt increment during new mbox read */
	   sleepmsg;		/* time to sleep for messages being overwritten on screen */
#ifdef MIME
extern char charset[SLEN];	/* name of character set */
extern char display_charset[SLEN];	/* name of character set */
extern char charset_compatlist[SLEN];	/* list of charsets which are a
					   superset of us-ascii */
extern char text_encoding[SLEN];	/* default encoding for text/plain */
#endif
extern char cur_folder[SLEN];	/* name of current folder */
extern char cur_tempfolder[SLEN]; /* name of temp folder open for a mailbox */
extern char defaultfile[SLEN];	/* name of default folder */
extern char temp_dir[SLEN];     /* name of temp directory */
extern char raw_temp_dir[SLEN]; /* unexpanded name of temp directory */
extern char hostname[SLEN];	/* name of machine we're on*/
extern char hostdomain[SLEN];	/* name of domain we're in */
extern char hostfullname[SLEN]; /* name of FQDN we're in */
extern char item[WLEN];		/* either "message" or "alias" */
extern char items[WLEN];	/* plural: either "messages" or "aliases" */
extern char Item[WLEN];		/* CAP: either "Message" or "Alias" */
extern char Items[WLEN];	/* CAP-plural: either "Messages" or "Aliases" */
extern char Prompt[WLEN];	/* Menu Prompt: either "Command" or "Alias" */
extern char username[SLEN];	/* return address name!    */
extern char full_username[SLEN];/* Full username - gecos   */
extern char home[SLEN];		/* home directory of user  */
extern char folders[SLEN];	/* folder home directory   */
extern char raw_folders[SLEN];	/* unexpanded folder home directory   */
extern char recvd_mail[SLEN];	/* folder for storing received mail	*/
extern char raw_recvdmail[SLEN];/* unexpanded recvd_mail name */
extern char editor[SLEN];	/* default editor for mail */
extern char raw_editor[SLEN];	/* unexpanded default editor for mail */
extern char alternative_editor[SLEN];/* the 'other' editor */
extern char printout[SLEN];	/* how to print messages   */
extern char raw_printout[SLEN];	/* unexpanded how to print messages   */
extern char sent_mail[SLEN];	/* name of file to save copies to */
extern char raw_sentmail[SLEN];	/* unexpanded name of file to save to */
extern char calendar_file[SLEN];/* name of file for clndr  */
extern char raw_calendar_file[SLEN];/* unexpanded name of file for clndr  */
extern char attribution[SLEN];  /* attribution string for replies     */
extern char prefixchars[SLEN];	/* prefix char(s) for msgs */
extern char shell[SLEN];	/* default system shell    */
extern char raw_shell[SLEN];	/* unexpanded default system shell    */
extern char pager[SLEN];	/* what pager to use...    */
extern char raw_pager[SLEN];	/* unexpanded what pager to use...    */
extern char batch_subject[SLEN];/* subject buffer for batchmail */
extern char included_file[SLEN];/* name of file to place in edit buf */
extern char local_signature[SLEN];/* local msg signature file   */
extern char raw_local_signature[SLEN];/* unexpanded local msg signature file */
extern char remote_signature[SLEN];/* remote msg signature file */
extern char raw_remote_signature[SLEN];/* unexpanded remote msg signature file*/
extern char version_buff[NLEN]; /* version buffer */
extern char e_editor[SLEN];	/* "~e" editor...   */
extern char v_editor[SLEN];	/* "~v" editor...   */
extern char config_options[SLEN];	/* which options are in o)ptions */
extern char allowed_precedences[SLEN];	/* list of precedences user may specify */
extern char *def_ans_yes;	/* default yes answer - single char, lc	*/
extern char *def_ans_no;	/* default no answer - single char, lc	*/
extern char *nls_deleted;	/* [deleted] */
extern char *nls_form;		/* Form */
extern char *nls_message;	/* Message */
extern char *nls_to;		/* To */
extern char *nls_from;		/* From */
extern char *nls_page;		/* Page */
extern char *change_word;	/* change */
extern char *save_word;		/* save */
extern char *copy_word;		/* copy */
extern char *cap_save_word;	/* Save */
extern char *cap_copy_word;	/* Copy */
extern char *saved_word;	/* saved */
extern char *copied_word;	/* copied */

extern char backspace,		/* the current backspace char  */
	    escape_char,	/* '~' or something else...    */
	    kill_line;		/* the current kill_line char  */

extern char up[SHORT], 
	    down[SHORT],
	    left[SHORT],
	    right[SHORT];	/* cursor control seq's    */
extern int  cursor_control;	/* cursor control avail?   */

extern int  has_highlighting;	/* highlighting available? */

/** the following two are for arbitrary weedout lists.. **/

extern char *weedlist[MAX_IN_WEEDLIST];
extern int  weedcount;		/* how many headers to check?        */

extern int  allow_forms;	/* flag: are AT&T Mail forms okay?    */
extern int  prompt_after_pager;	/* flag: prompt after pager exits     */
extern int  mini_menu;		/* flag: display menu?     	      */
extern int  metoo;		/* flag: copy me on mail to alias?    */
extern int  folder_type;	/* flag: type of folder		      */
extern int  auto_copy;		/* flag: auto copy source into reply? */
extern int  filter;		/* flag: weed out header lines?	      */
extern int  resolve_mode;	/* flag: resolve before moving mode?  */
extern int  auto_cc;		/* flag: mail copy to yourself?       */
extern int  noheader;		/* flag: copy + header to file?       */
extern int  title_messages;	/* flag: title message display?       */
extern int  forwarding;		/* flag: are we forwarding the msg?   */
extern int  hp_terminal;	/* flag: are we on an hp terminal?    */
extern int  hp_softkeys;	/* flag: are there softkeys?          */
extern int  save_by_name;  	/* flag: save mail by login name?     */
extern int  force_name;		/* flag: save by name forced?	      */
extern int  mail_only;		/* flag: send mail then leave?        */
extern int  check_only;		/* flag: check aliases and leave?     */
extern int  batch_only;		/* flag: send without prompting?      */
extern int  move_when_paged;	/* flag: move when '+' or '-' used?   */
extern int  point_to_new;	/* flag: start pointing at new msgs?  */
extern int  builtin_lines;	/* int use builtin pager?             */
extern int  bounceback;		/* flag: bounce copy off remote?      */
extern int  always_keep;	/* flag: always keep unread msgs?     */
extern int  always_store;	/* flag: always store read mail?      */
extern int  always_del;		/* flag: always delete marked msgs?   */
extern int  arrow_cursor;	/* flag: use "->" regardless?	      */
extern int  debug;		/* flag: debugging mode on?           */
extern int  user_level;		/* flag: how knowledgable is user?    */
extern int  selected;		/* flag: used for select stuff        */
extern int  names_only;		/* flag: display names but no addrs?  */
extern int  question_me;	/* flag: ask questions as we leave?   */
extern int  keep_empty_files;	/* flag: keep empty files??	      */
extern int  clear_pages;	/* flag: clear screen w/ builtin pgr? */
extern int  prompt_for_cc;	/* flag: prompt user for 'cc' value?  */
extern int  sig_dashes;		/* flag: put dashes above signature?  */
extern int  use_tite;		/* flag: use termcap/terminfo ti/te?  */
extern int  confirm_append;	/* flag: confirm append to folder?    */
extern int  confirm_create;	/* flag: confirm create new folder?   */
extern int  confirm_files;	/* flag: confirm files for append?    */
extern int  confirm_folders;	/* flag: confirm folders for create?  */

extern int  sortby;		/* how to sort folders	      */
extern int  alias_sortby;	/* how to sort aliases        */

extern long timeout;		/* seconds for main level timeout     */

extern int LINES;		/** lines per screen    **/
extern int COLUMNS;		/** columns per line    **/
#ifdef SIGWINCH
extern int resize_screen;	/** SIGWINCH occured?   **/
#endif

extern long size_of_pathfd;	/** size of pathfile, 0 if none **/

extern FILE *mailfile;		/* current folder 	    */
extern FILE *debugfile;		/* file for debut output    */
extern FILE *pathfd;		/* path alias file          */
extern FILE *domainfd;		/* domains file 	    */
extern nl_catd elm_msg_cat;	/* message catalog	    */

extern long mailfile_size;	/* size of current mailfile */

extern int  max_headers;	/* number of headers currently allocated */

extern struct header_rec **headers; /* array of header structure pointers */

extern struct alias_rec **aliases; /* for the alias menu */
extern int    max_aliases;	/* number of aliases allocated */

extern struct addr_rec *alternative_addresses;	/* how else do we get mail? */

extern int system_data;		/* fileno of system data file */
extern int user_data;		/* fileno of user data file   */

extern int userid;		/* uid for current user	      */
extern int groupid;		/* groupid for current user   */
#ifdef SAVE_GROUP_MAILBOX_ID
extern int mailgroupid;		/* groupid for current user   */
#endif

#ifdef	BSD
extern jmp_buf GetPromptBuf;	/* setjmp buffer */
extern int InGetPrompt;		/* set if in GetPrompt() in read() */
#endif
