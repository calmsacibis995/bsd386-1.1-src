
/* $Id: defs.h,v 5.33 1993/09/19 23:40:48 syd Exp  */

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
 * $Log: defs.h,v $
 * Revision 1.2  1994/01/14  00:52:32  cp
 * 2.4.23
 *
 * Revision 5.33  1993/09/19  23:40:48  syd
 * Defince SEEK_SET in one of our headers as a last resort
 * From: Syd
 *
 * Revision 5.32  1993/08/23  02:46:51  syd
 * Test ANSI_C, not __STDC__ (which is not set on e.g. AIX).
 * From: decwrl!uunet.UU.NET!fin!chip (Chip Salzenberg)
 *
 * Revision 5.31  1993/08/23  02:45:29  syd
 * The macro ctrl(c) did not work correctly for a DEL character
 * neither did it make the backward mapping from a control char
 * to the letter that is normally used with an up-arrow prefix
 * to represent the control character.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.30  1993/08/03  19:28:39  syd
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
 * Revision 5.29  1993/08/03  19:05:33  syd
 * When STDC is used on Convex the feof() function is defined as
 * a true library routine in the header files and moreover the
 * library routine also leaks royally. It returns always 1!!
 * So we have to use a macro. Convex naturally does not provide
 * you with one though if you are using a STDC compiler. So we
 * have to include one.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.28  1993/07/20  02:59:53  syd
 * Support aliases both on 64 bit and 32 bit machines at the same time.
 * From: Dan Mosedale <mosedale@genome.stanford.edu>
 *
 * Revision 5.27  1993/05/08  19:41:13  syd
 * make it now depend on mallocvoid
 *
 * Revision 5.26  1993/04/16  04:16:24  syd
 * For convex, #if-defed memcpy, memset and sleep.
 * From: rzm@oso.chalmers.se (Rafal Maszkowski)
 *
 * Revision 5.25  1993/04/12  03:30:23  syd
 * On AIX, __STDC__ is not defined but it does use unistd.h, etc.  In
 * hdrs/def.h, ANS_C already gets defined if __STDC__ or _AIX.  But this
 * variable then needs to be used in src/init.c and hdrs/filter.h in place
 * of the current test for __STDC__.
 * From:	rstory@elegant.com (Robert Story)
 *
 * Revision 5.24  1993/04/12  03:25:26  syd
 * Use I_UNISTD instead of UNISTD_H
 *
 * Revision 5.23  1993/04/12  03:22:49  syd
 * Add UNISTD_H to check for unistd.h include
 * From: Syd
 *
 * Revision 5.22  1993/04/12  01:51:42  syd
 * Added safe_malloc(), safe_realloc(), and safe_strdup().  They
 * will be used in the new elmalias utility.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.21  1993/02/09  15:31:10  syd
 * Make the str declare symbol ANSI_C, our contrived symbol
 * so IBM compiles work ok.
 * From: Syd
 *
 * Revision 5.20  1993/02/07  15:12:50  syd
 * fix declaration of fseek, it was of the wrong type (sb int, was long)
 *
 * Revision 5.19  1993/02/03  17:12:53  syd
 * move more declarations to defs.h, including sleep
 * From: Syd
 *
 * Revision 5.18  1993/02/03  16:56:24  syd
 * add unistd.h for include of lseek/fseek on stdc compilers
 * From: Syd via prompt from mfvargo@netcom.com (Michael Vargo)
 *
 * Revision 5.17  1993/02/03  16:18:17  syd
 * add strtokq
 * From: Syd
 *
 * Revision 5.16  1993/01/20  03:02:19  syd
 * Move string declarations to defs.h
 * From: Syd
 *
 * Revision 5.15  1993/01/20  02:56:38  syd
 * hide index and rindex declarations inside __STDC__ so AIX doesnt see it
 *
 * Revision 5.14  1993/01/19  05:07:05  syd
 * Trim erroreous extra log entry
 * From: Syd
 *
 * Revision 5.13  1993/01/19  04:47:12  syd
 * Significant changes to provide consistent Date and From_ header
 * cracking.  Overhauled date utilities and moved into library.  Moved
 * real_from() into library.  Modified frm, newmail, and readmsg utilities
 * to use library version of real_from().  Moved get_word() from Elm
 * source into library.  Added new library routines atonum() and strfcpy().
 * Fixed trailing backslash bug in len_next().
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.12  1993/01/05  03:36:10  syd
 * remove leading 0s from access() defs as it confuses some
 * compilers (and I know it shouldnt)
 * From: Syd
 *
 * Revision 5.11  1992/12/11  01:58:50  syd
 * Allow for use from restricted shell by putting SHELL=/bin/sh in the
 * environment of spawned mail transport program.
 * From: chip@tct.com (Chip Salzenberg)
 *
 * Revision 5.10  1992/12/11  01:29:27  syd
 * add include of sys/types.h for time_t usage
 *
 * Revision 5.9  1992/12/07  02:47:45  syd
 * fix time variables that are not declared time_t
 * From: Syd via prompting from Jim Brown
 *
 * Revision 5.8  1992/11/26  01:46:26  syd
 * add Decode option to copy_message, convert copy_message to
 * use bit or for options.
 * From: Syd and bjoerns@stud.cs.uit.no (Bjoern Stabell)
 *
 * Revision 5.7  1992/11/22  01:22:29  syd
 * This mod fixes overlapping prototypes for strchr and index on
 * Convex.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.6  1992/11/07  20:40:27  syd
 * flag for no tite for curses calls
 *
 * Revision 5.5  1992/11/07  20:05:52  syd
 * change to use header_cmp to allow for linear white space around the colon
 * From: Syd
 *
 * Revision 5.4  1992/10/27  01:40:08  syd
 * fix compilation error on posix_signals for non STDC
 * From: tom@osf.org
 *
 * Revision 5.3  1992/10/25  02:01:58  syd
 * Here are the patches to support POSIX sigaction().
 * From: tom@osf.org
 *
 * Revision 5.2  1992/10/25  01:47:45  syd
 * fixed a bug were elm didn't call metamail on messages with a characterset,
 * which could be displayed by elm itself, but message is encoded with QP
 * or BASE64
 * From: Klaus Steinberger <Klaus.Steinberger@Physik.Uni-Muenchen.DE>
 *
 * Revision 5.1  1992/10/03  22:34:39  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/**  define file for ELM mail system.  **/


#include <sys/types.h>	/* for fundamental types */
#include <stdio.h>	/* Must get the _IOEOF flag for feof() on Convex */
#include "../config.h"
#include "sysdefs.h"	/* system/configurable defines */


# define VERSION	"2.4"				/* Version number... */
# define VERS_DATE	"October 1, 1992"		/* for elm -v option */
# define WHAT_STRING	\
	"@(#) Version 2.4, USENET supported version, released October 1, 1992"

#if defined(__STDC__) || defined(_AIX)
# define ANSI_C 1
#endif

#define KLICK		25

#define SLEN		256	    /* long for ensuring no overwrites... */
#define SHORT		10	    /* super short strings!		  */
#define NLEN		48	    /* name length for aliases            */
#define WLEN		20
#define STRING		128	/* reasonable string length for most..      */
#define LONG_STRING	512	/* even longer string for group expansion   */
#define VERY_LONG_STRING 2560	/* huge string for group alias expansion    */
#define MAX_LINE_LEN	5120	/* even bigger string for "filter" prog..   */

#define BREAK		'\0'  		/* default interrupt    */
#define BACKSPACE	'\b'     	/* backspace character  */
#define TAB		'\t'            /* tab character        */
#define RETURN		'\r'     	/* carriage return char */
#define LINE_FEED	'\n'     	/* line feed character  */
#define FORMFEED	'\f'     	/* form feed (^L) char  */
#define COMMA		','		/* comma character      */
#define SPACE		' '		/* space character      */
#define DOT		'.'		/* period/dot character */
#define BANG		'!'		/* exclaimation mark!   */
#define AT_SIGN		'@'		/* at-sign character    */
#define PERCENT		'%'		/* percent sign char.   */
#define COLON		':'		/* the colon ..		*/
#define BACKQUOTE	'`'		/* backquote character  */
#define TILDE_ESCAPE	'~'		/* escape character~    */
#define ESCAPE		'\033'		/* the escape		*/

#define NO_OP_COMMAND	'\0'		/* no-op for timeouts   */

#define STANDARD_INPUT  0		/* file number of stdin */

#ifndef TRUE
#define TRUE		1
#define FALSE		0
#endif

#define NO		0
#define YES		1
#define	NO_TITE		2		/* ti/te or in flag 	   */
#define MAYBE		2		/* a definite define, eh?  */
#define FORM		3		/*      <nevermind>        */
#define PREFORMATTED	4		/* forwarded form...       */

#define SAME_PAGE	1		/* redraw current only     */
#define NEW_PAGE	2		/* redraw message list     */
#define ILLEGAL_PAGE	0		/* error in page list, punt */

#define PAD		0		/* for printing name of    */
#define FULL		1		/*   the sort we're using  */

#define OUTGOING	0		/* defines for lock file   */
#define INCOMING	1		/* creation..see lock()    */

/* options to the system_call() procedure */
#define SY_USER_SHELL	(1<<0)		/* use user shell instead of /bin/sh */
#define SY_ENV_SHELL	(1<<1)		/* put SHELL=[shell] into environ    */
#define SY_ENAB_SIGHUP	(1<<2)		/* pgm to exec can handle signals    */
#define SY_ENAB_SIGINT	(1<<3)		/*  ...and it can handle SIGINT too  */
#define SY_DUMPSTATE	(1<<4)		/* create folder state dump file     */

/* options to the copy_message() procedure */
#define CM_REMOVE_HEADER	(1<<0)	/* skip header of message */
#define CM_REMOTE		(1<<1)	/* append remote from hostname to first line */
#define CM_UPDATE_STATUS	(1<<2)	/* Update Status: Header  */
#define CM_MMDF_HEAD		(1<<3)	/* strip mmdf message seperator */
#define CM_REMAIL		(1<<4)	/* Add Sender: and Orig-To: headers */
#define CM_DECODE		(1<<5)	/* prompt for key if message is encrypted */

#define EXECUTE_ACCESS	1		/* These five are 	   */
#define WRITE_ACCESS	2		/*    for the calls	   */
#define READ_ACCESS	4		/*       to access()       */
#define ACCESS_EXISTS	0		/*           <etc>         */
#define EDIT_ACCESS	6		/*  (this is r+w access)   */

#define BIG_NUM		999999		/* big number!             */
#define BIGGER_NUM	9999999 	/* bigger number!          */

#define START_ENCODE	"[encode]"
#define END_ENCODE	"[clear]"

#define DONT_SAVE	"[no save]"
#define DONT_SAVE2	"[nosave]"

#define alias_file	".aliases"
#define group_file	".groups"
#define system_file	".systems"

#define default_folders		"Mail"
#define default_recvdmail	"=received"
#define default_sentmail	"=sent"

/* environment variable with name of folder state dump file */
#define FOLDER_STATE_ENV	"ELMSTATE"

/** some defines for the 'userlevel' variable... **/

#define RANK_AMATEUR	0
#define AMATEUR		1
#define OKAY_AT_IT	2
#define GOOD_AT_IT	3
#define EXPERT		4
#define SUPER_AT_IT	5

/** some defines for the "status" field of the header and alias record **/

#define ACTION		1		/* bit masks, of course */
#define CONFIDENTIAL	2
#define DELETED		4
#define EXPIRED		8
#define FORM_LETTER	16
#define NEW		32
#define PRIVATE		64
#define TAGGED		128
#define URGENT		256
#define VISIBLE		512
#define UNREAD		1024
#define STATUS_CHANGED	2048
#define MIME_MESSAGE	4096	/* indicates existence of MIME Header */
#define MIME_NEEDDECOD	8192	/* indicates that we need to call mmdecode */
#define	MIME_NOTPLAIN	16384	/* indicates that we have a content-type,
				   for which we need metamail anyway. */

/** some defines for the "type" field of the alias record **/

#define SYSTEM		1		/* bit masks, of course */
#define USER		2
#define PERSON		4
#define GROUP		8
#define DUPLICATE	16		/* system aliases only */

/** some defines to aid in the limiting of alias displays **/

#define BY_NAME		64
#define BY_ALIAS	128

#define UNDELETE	0		/* purely for ^U function... */

/** values for headers exit_disposition field */
#define UNSET	0
#define KEEP	1
#define	STORE	2
#define DELETE	3

/** some months... **/

#define JANUARY		0			/* months of the year */
#define FEBRUARY	1
#define MARCH		2
#define APRIL		3
#define MAY		4
#define JUNE		5
#define JULY		6
#define AUGUST		7
#define SEPTEMBER	8
#define OCTOBER		9
#define NOVEMBER	10
#define DECEMBER	11

#define equal(s,w)	(strcmp(s,w) == 0)
#define min(a,b)	(a) < (b) ? (a) : (b)
/*
 *  Control character mapping like "c - 'A' + 1" does not work
 *  correctly for a DEL. Neither does it allow mapping from
 *  a control character to the letter that is normally used with
 *  an up-arrow prefix to represent the control char.
 *  The correct mapping should be done like this...
 */
#define ctrl(c)	        (((c) + '@') & 0x7f)

#define plural(n)	n == 1 ? "" : "s"
#define lastch(s)	s[strlen(s)-1]
#define ifmain(a,b)	(inalias ? b : a)

#ifdef MEMCPY

#  ifdef I_MEMORY
#   include <memory.h>
#  else /* I_MEMORY */
#   ifndef ANSI_C   /* ANSI puts these in string.h */
#if defined(__convexc__)
extern void *memcpy(), *memset();
#else
extern char *memcpy(), *memset();
#endif
extern int memcmp();
#   endif /* ANSI_C */
#  endif /* I_MEMORY */

#define bcopy(s1,s2,l) memcpy(s2,s1,l)
#define bcmp(s1,s2,l) memcmp(s1,s2,l)
#define bzero(s,l) memset(s,0,l)
#endif /* MEMCPY */

#ifdef MALLOCVOID
typedef	void *	malloc_t;
#else
typedef	char *	malloc_t;
#endif

#ifdef I_STDLIB
# include <stdlib.h>
#else
extern	malloc_t	calloc();
extern	int		free();
extern	malloc_t	malloc();
extern	malloc_t	realloc();
extern	void		exit();
extern char		*getenv();
#endif

/* find tab stops preceding or following a given column position 'a', where
 * the column position starts counting from 1, NOT 0!
 * The external integer "tabspacing" must be declared to use this. */
#define prev_tab(a)	(((((a-1)/tabspacing))*tabspacing)+1)
#define next_tab(a)	(((((a-1)/tabspacing)+1)*tabspacing)+1)

#define movement_command(c)	(c == 'j' || c == 'k' || c == ' ' || 	      \
				 c == BACKSPACE || c == ESCAPE || c == '*' || \
				 c == '-' || c == '+' || c == '=' ||          \
				 c == '#' || c == '@' || c == 'x' || 	      \
				 c == 'a' || c == 'q')

#define no_ret(s)	{ register int xyz; /* varname is for lint */	      \
		          for (xyz=strlen(s)-1; xyz >= 0 && 		      \
				(s[xyz] == '\r' || s[xyz] == '\n'); )	      \
			     s[xyz--] = '\0';                                 \
			}
			  
#define first_word(s,w) (strncmp(s,w, strlen(w)) == 0)
#define first_word_nc(s,w) (strincmp(s,w, strlen(w)) == 0)
#define ClearLine(n)	MoveCursor(n,0); CleartoEOLN()
#define whitespace(c)	(c == ' ' || c == '\t')
#define ok_rc_char(c)	(isalnum(c) || c == '-' || c == '_')
#define ok_alias_char(c) (isalnum(c) || c == '-' || c == '_' || c == '.')
#define onoff(n)	(n == 0 ? "OFF" : "ON")

/** The next function is so certain commands can be processed from the showmsg
    routine without rewriting the main menu in between... **/

#define special(c)	(c == 'j' || c == 'k')

/** and a couple for dealing with status flags... **/

#define ison(n,mask)	(n & mask)
#define isoff(n,mask)	(!ison(n, mask))

#define setit(n,mask)		n |= mask
#define clearit(n, mask)	n &= ~mask

/** a few for the usage of function keys... **/

#define f_key1	1
#define f_key2	2
#define f_key3	3
#define f_key4	4
#define f_key5	5
#define f_key6	6
#define f_key7	7
#define f_key8	8

#define MAIN	0
#define ALIAS   1
#define YESNO	2
#define CHANGE  3
#define READ	4

#define MAIN_HELP    0
#define OPTIONS_HELP 1
#define ALIAS_HELP   2
#define PAGER_HELP   3

/** types of folders **/
#define NO_NAME		0		/* variable contains no file name */
#define NON_SPOOL	1		/* mailfile not in mailhome */
#define SPOOL		2		/* mailfile in mailhome */

/* the following is true if the current mailfile is the user's spool file*/
#define USERS_SPOOL	(strcmp(cur_folder, defaultfile) == 0)

/** some possible sort styles... **/

#define REVERSE		-		/* for reverse sorting           */
#define SENT_DATE	1		/* the date message was sent     */
#define RECEIVED_DATE	2		/* the date message was received */
#define SENDER		3		/* the name/address of sender    */
#define SIZE		4		/* the # of lines of the message */
#define SUBJECT		5		/* the subject of the message    */
#define STATUS		6		/* the status (deleted, etc)     */
#define MAILBOX_ORDER	7		/* the order it is in the file   */

/** some possible sort styles...for aliases **/

#define ALIAS_SORT	1		/* the name of the alias         */
#define NAME_SORT	2		/* the actual name for the alias */
#define TEXT_SORT	3		/* the order of aliases.text     */
#define LAST_ALIAS_SORT	TEXT_SORT

/* some stuff for our own malloc call - pmalloc */

#define PMALLOC_THRESHOLD	256	/* if greater, then just use malloc */
#define PMALLOC_BUFFER_SIZE    2048	/* internal [memory] buffer size... */

/** the following macro is as suggested by Larry McVoy.  Thanks! **/

# ifdef DEBUG
#  define   dprint(n,x)		{ 				\
				   if (debug >= n)  {		\
				     fprintf x ; 		\
				     fflush(debugfile);         \
				   }				\
				}
# else
#  define   dprint(n,x)
# endif

/* some random structs... */

struct header_rec {
	int  lines;		/** # of lines in the message	**/
	int  status;		/** Urgent, Deleted, Expired?	**/
	int  index_number;	/** relative loc in file...	**/
	int  encrypted;		/** whether msg has encryption	**/
	int  exit_disposition;	/** whether to keep, store, delete **/
	int  status_chgd;	/** whether became read or old, etc. **/
	long content_length;	/** content_length in bytes from message header	**/
	long offset;		/** offset in bytes of message	**/
	time_t received_time;	/** when elm received here	**/
	char from[STRING];	/** who sent the message?	**/
	char to[STRING];	/** who it was sent to		**/
	char messageid[STRING];	/** the Message-ID: value	**/
	char time_zone[12];	/**                incl. tz	**/
	time_t time_sent;	/** gmt when sent for sorting	**/
	char time_menu[SHORT];	/** just the month..day for menu **/
	time_t tz_offset;	/** offset to gmt of time sent	**/
	char subject[STRING];   /** The subject of the mail	**/
	char mailx_status[WLEN];/** mailx status flags (RO...)	**/
       };

#ifdef __alpha
#define int32 int
#else
#define int32 long
#endif

struct alias_disk_rec {
	int32 status;			/* DELETED, TAGGED, VISIBLE, ...     */
	int32 alias;			/* alias name                        */
	int32 last_name;		/* actual personal (last) name       */
	int32 name;			/* actual personal name (first last) */
	int32 comment;			/* comment, doesn't show in headers  */
	int32 address;			/* non expanded address              */
	int32 type;			/* mask-- sys/user, person/group     */
	int32 length;			/* length of alias data on file      */
       };

struct alias_rec {
	int   status;			/* DELETED, TAGGED, VISIBLE, ...     */
	char  *alias;			/* alias name                        */
	char  *last_name;		/* actual personal (last) name       */
	char  *name;			/* actual personal name (first last) */
	char  *comment;			/* comment, doesn't show in headers  */
	char  *address;			/* non expanded address              */
	int   type;			/* mask-- sys/user, person/group     */
	long  length;			/* length of alias data on file      */
       };

struct addr_rec {
	 char   address[NLEN];	/* machine!user you get mail as      */
	 struct addr_rec *next;	/* linked list pointer to next       */
	};

/*
 * Filled in by "load_folder_state_file()".  This allows an external program
 * (e.g. "readmsg") to receive information on the current Elm state.
 */
struct folder_state {
	char *folder_name;	/* full pathname to current folder	*/
	int num_mssgs;		/* number of messages in the folder	*/
	long *idx_list;		/* index of seek offsets for messages	*/
	int num_sel;		/* number of messages selected		*/
	int *sel_list;		/* list of selected message numbers	*/
};

#ifdef SHORTNAMES	/* map long names to shorter ones */
# include <shortname.h>
#endif

/** Let's make sure that we're not going to have any annoying problems with 
    int pointer sizes versus char pointer sizes by guaranteeing that every-
    thing vital is predefined... (Thanks go to Detlev Droege for this one)
**/

#ifdef STRINGS
#  include <strings.h>
#else
#  if defined(_CONVEX_SOURCE) && defined(index)
#    undef _CONVEX_SOURCE
#    include <string.h>     /* Now there is no proto for index. */
#    define _CONVEX_SOURCE
#  else
#    include <string.h>
#  endif
#endif

#ifdef	__convex__
/*
 *  Nice work Convex people! Thanks a million!
 *  When STDC is used feof() is defined as a true library routine
 *  in the header files and moreover the library routine also leaks
 *  royally. (It returns always 1!!) Consequently this macro is
 *  unavoidable.)
 */
#  ifndef   feof
#    define   feof(p)	((p)->_flag&_IOEOF)
#  endif
#endif

#ifndef ANSI_C   /* ANSI puts these in string.h */
char *index(), *rindex(); /* names will be traslated by define in config.h */
char *strtok(), *strcpy(), *strcat(), *strncpy(); /* more in string.h in ANSI */
long lseek();
int fseek();
#if defined(__convexc__)
unsigned sleep();
#else
unsigned long sleep();
#endif
#else
#  ifdef I_UNISTD /* unistd.h available */
#    include <unistd.h> /* ansi C puts sleep, lseek and fseek in unistd.h */
#  else /* I_UNISTD */
long lseek();
int fseek();
unsigned long sleep();
#  endif /* I_UNISTD */
#endif
char *strtokq(); /* our own quote minding strtok */

#ifndef STRSTR
char *strstr();
#endif

#ifdef I_LOCALE
#include <locale.h>
#endif

#ifdef I_NL_TYPES
#include <nl_types.h>
#else
#include "../hdrs/nl_types.h"
#endif

#ifndef	USENLS
#define MCprintf printf
#define MCfprintf fprintf
#define MCsprintf sprintf
#endif

#ifdef POSIX_SIGNALS
#define signal posix_signal
#if ANSI_C
extern SIGHAND_TYPE (*posix_signal(int, SIGHAND_TYPE (*)(int)))(int);
#else	/* ANSI_C */
extern SIGHAND_TYPE (*posix_signal())();
#endif	/* ANSI_C */
#else	/* POSIX_SIGNALS */
#ifdef SIGSET
#define signal sigset
#ifdef _AIX
extern SIGHAND_TYPE (*sigset(int sig, SIGHAND_TYPE (*func)(int)))(int);
#endif
#endif /* SIGSET */
#endif /* POSIX_SIGNALS */

/*
 * Some of the old BSD ctype conversion macros corrupted characters.
 * We will substitute our own versions if required.
 */
#include <ctype.h>
#ifdef BROKE_CTYPE
# undef  toupper
# define toupper(c)	(islower(c) ? ((c) - 'a' + 'A') : (c))
# undef  tolower
# define tolower(c)	(isupper(c) ? ((c) - 'A' + 'a') : (c))
#endif

/*
 *	if the seek constants arent set in an include file
 *	lets define them ourselves
 */
#ifndef SEEK_SET
#define	SEEK_SET	0	/* Set file pointer to "offset" */
#define	SEEK_CUR	1	/* Set file pointer to current plus "offset" */
#define	SEEK_END	2	/* Set file pointer to EOF plus "offset" */
#endif


/*
 * The "safe_malloc_fail_handler" vector points to a routine that is invoked
 * if one of the safe_malloc() routines fails.  At startup, this will point
 * to the default handler that prints a diagnostic message and aborts.  The
 * vector may be changed to install a different error handler.
 */
extern void (*safe_malloc_fail_handler)();

char *argv_zero();
char *bounce_off_remote();
char *ctime();
char *error_description();
char *expand_system();
char *format_long();
char *get_alias_address();
char *get_arpa_date();
char *get_ctime_date();
char *get_date();
char *get_token();
char *getlogin();
char *level_name();
char *shift_lower();
char *strip_commas();
char *strip_parens();
char *strpbrk();
char *qstrpbrk();
char *strfcpy();
char *strtok();
char *tail_of_string();
char *tgetstr();
char *pmalloc();
char *header_cmp();
char *safe_strdup();

malloc_t safe_malloc();
malloc_t safe_realloc();

long times();
long ulimit();
