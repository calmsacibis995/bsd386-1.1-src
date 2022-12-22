
/* @(#)Id: save_opts.h,v 5.10 1993/08/10 18:49:32 syd Exp  */

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
 * $Log: save_opts.h,v $
 * Revision 1.2  1994/01/14  00:52:52  cp
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
 * Revision 5.9  1993/06/12  05:28:06  syd
 * Missing checkin
 *
 * Revision 5.8  1993/05/08  18:56:16  syd
 * created a new elmrc variable named "readmsginc".  This specifies an
 * increment by which the message count is updated.  If this variable is
 * set to, say, 25, then the message count will only be updated every 25
 * messages, displaying 0, 25, 50, 75, and so forth.  The default value
 * of 1 will cause Elm to behave exactly as it currently does in PL21.
 * From: Eric Peterson <epeterso@encore.com>
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
 * Revision 5.6  1992/10/25  02:43:50  syd
 * fix typo
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
 * Revision 5.3  1992/10/17  22:58:57  syd
 * patch to make elm use (or in my case, not use) termcap/terminfo ti/te.
 * From: Graham Hudspith <gwh@inmos.co.uk>
 *
 * Revision 5.2  1992/10/17  22:42:24  syd
 * Add flags to read_rc to support command line overrides of the option.
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.1  1992/10/03  22:34:39  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/*
 *	Defines for the storage of options portion of the Elm system.
 */

typedef struct {
    char letter;		/* menu letter on options screen */
    char *menu;			/* menu prompt */
    int  menu_msg;		/* NLS message number of menu prompt */
    char *parm;			/* parameter to modify */
    int (*post)();		/* post processing function */
    char *one_liner;		/* one line help message */
    int  one_liner_msg; 	/* NLS message number of one line help message */
    } opts_menu;

#define DT_SYN 0 /* synonym entry (old name) */
#define DT_STR 1 /* string */
#define DT_NUM 2 /* number */
#define DT_BOL 3 /* ON/OFF (boolean) */
#define DT_CHR 4 /* character */
#define DT_WEE 5 /* weed list */
#define DT_ALT 6 /* alternate addresses list */
#define DT_SRT 7 /* sort-by code */
#define DT_MLT 8 /* multiple destinations for data */
#define DT_ASR 9 /* sort-by code */
#define DT_MASK 037 /* mask for data type */
#define FL_LOCAL 0040          /* flag if changed */
#define FL_NOSPC 0100          /* flag if preserve blanks as "_" */
#define FL_SYS   0200          /* flag if only valid in system RC */
#define FL_OR    0400          /* flag if boolean value may have been set */
#define FL_AND  01000          /* flag if boolean value may have been unset */

typedef struct { 
	char 	name[NLEN]; 	/* name of instruction */
	long 	offset;		/* offset into elmrc-info file */
        int	flags;	/* DT_STR, DT_NUM, DT_BOL, etc */
        union {
          char 	*str;
          int 	*num;
          int 	*bol;
          char 	*chr;
          char 	**weed;
          struct addr_rec **alts;
          int 	*sort;
          } val;
	} save_info_recs;

/*
 *	since many C compilers cannot init a union as a static
 *	init, make the same structure with just the char * for
 *	the union pointer.
 */
typedef struct { 
	char 	name[NLEN]; 	/* name of instruction */
	long 	offset;		/* offset into elmrc-info file */
        int	flags;	/* DT_STR, DT_NUM, DT_BOL, etc */
        char 	*str;
	} save_info_recs_init;

#define SAVE_INFO_STR(x) (save_info[x].val.str)
#define SAVE_INFO_NUM(x) (save_info[x].val.num)
#define SAVE_INFO_BOL(x) (save_info[x].val.bol)
#define SAVE_INFO_CHR(x) (save_info[x].val.chr)
#define SAVE_INFO_WEE(x) (save_info[x].val.weed)
#define SAVE_INFO_ALT(x) (save_info[x].val.alts)
#define SAVE_INFO_SRT(x) (save_info[x].val.sort)
#define SAVE_INFO_ASR(x) (save_info[x].val.sort)
#define SAVE_INFO_SYN(x) ((char *)save_info[x].val.str)
#define SAVE_INFO_MLT(x) ((char **)save_info[x].val.weed)

#ifdef SAVE_OPTS

/* "lists" for DT_MLT.  These and DT_SYN could be eliminated if support
   of the old parameter names was dropped.
*/
char *SIGS[]={"remotesignature","localsignature",NULL},
	*ALWAYS[]={"alwayskeep","alwaysstore",NULL};

save_info_recs_init save_info_data[] = {
{"aliassortby",		-1L,DT_ASR,(char *)&alias_sortby},
{"alteditor",		-1L,DT_STR,alternative_editor},
{"alternatives",	-1L,DT_ALT,(char *)&alternative_addresses},
{"alwaysdelete",	-1L,DT_BOL,(char *)&always_del},
{"alwayskeep",		-1L,DT_BOL,(char *)&always_keep},
{"alwaysleave",		-1L,DT_MLT,(char *)ALWAYS},
{"alwaysstore",		-1L,DT_BOL,(char *)&always_store},
{"arrow",		-1L,DT_BOL|FL_OR,(char *)&arrow_cursor},
{"ask",			-1L,DT_BOL,(char *)&question_me},
{"askcc",		-1L,DT_BOL,(char *)&prompt_for_cc},
{"attribution",		-1L,DT_STR,attribution},
{"auto_cc",		-1L,DT_SYN,"copy"},
{"autocopy",		-1L,DT_BOL,(char *)&auto_copy},
{"bounce",		-1L,DT_SYN,"bounceback"},
{"bounceback",		-1L,DT_NUM,(char *)&bounceback},
{"builtinlines",	-1L,DT_NUM,(char *)&builtin_lines},
{"calendar",		-1L,DT_STR,raw_calendar_file},
{"cc",			-1L,DT_SYN,"askcc"},
#ifdef MIME
{"charset",		-1L,DT_STR,charset},
{"compatcharsets",		-1L,DT_STR,charset_compatlist},
#endif
{"configoptions",	-1L,DT_STR,config_options},
{"confirmappend",	-1L,DT_BOL,(char *)&confirm_append},
{"confirmcreate",	-1L,DT_BOL,(char *)&confirm_create},
{"confirmfiles",	-1L,DT_BOL,(char *)&confirm_files},
{"confirmfolders",	-1L,DT_BOL,(char *)&confirm_folders},
{"copy",		-1L,DT_BOL,(char *)&auto_cc},
{"delete",		-1L,DT_SYN,"alwaysdelete"},
#ifdef MIME
{"displaycharset",	-1L,DT_STR,display_charset},
#endif
{"easyeditor",		-1L,DT_STR,e_editor},
{"editor",		-1L,DT_STR,raw_editor},
{"escape",		-1L,DT_CHR,(char *)&escape_char},
{"folders",		-1L,DT_SYN,"maildir"},
{"forcename",		-1L,DT_BOL,(char *)&force_name},
{"form",		-1L,DT_SYN,"forms"},
{"forms",		-1L,DT_BOL,(char *)&allow_forms},
{"fullname",		-1L,DT_STR,full_username},
{"hostdomain",		-1L,DT_STR|FL_SYS,hostdomain},
{"hostfullname",	-1L,DT_STR|FL_SYS,hostfullname},
{"hostname",		-1L,DT_STR|FL_SYS,hostname},
{"hpkeypad",		-1L,DT_SYN,"keypad"},
{"hpsoftkeys",		-1L,DT_SYN,"softkeys"},
{"keep",		-1L,DT_SYN,"keepempty"},
{"keepempty",		-1L,DT_BOL,(char *)&keep_empty_files},
{"keypad",		-1L,DT_BOL|FL_OR,(char *)&hp_terminal},
{"localsignature",	-1L,DT_STR,raw_local_signature},
{"mailbox",		-1L,DT_SYN,"receivedmail"},
{"maildir",		-1L,DT_STR,raw_folders},
{"mailedit",		-1L,DT_SYN,"editor"},
{"menu",		-1L,DT_BOL|FL_AND,(char *)&mini_menu},
{"menus",		-1L,DT_SYN,"menu"},
{"metoo",		-1L,DT_BOL,(char *)&metoo},
{"movepage",		-1L,DT_BOL,(char *)&move_when_paged},
{"movewhenpaged",	-1L,DT_SYN,"movepage"},
{"name",		-1L,DT_SYN,"fullname"},
{"names",		-1L,DT_BOL,(char *)&names_only},
{"noheader",		-1L,DT_BOL,(char *)&noheader},
{"page",		-1L,DT_SYN,"pager"},
{"pager",		-1L,DT_STR,raw_pager},
{"pointnew",		-1L,DT_BOL,(char *)&point_to_new},
{"pointtonew",		-1L,DT_SYN,"pointnew"},
{"precedences",		-1L,DT_STR,allowed_precedences},
{"prefix",		-1L,DT_STR|FL_NOSPC,prefixchars},
{"print",		-1L,DT_STR,raw_printout},
{"printmail",		-1L,DT_SYN,"print"},
{"promptafter",		-1L,DT_BOL,(char *)&prompt_after_pager},
{"question",		-1L,DT_SYN,"ask"},
{"readmsginc",		-1L,DT_NUM,(char *)&readmsginc},
{"receivedmail",	-1L,DT_STR,raw_recvdmail},
{"remotesignature",	-1L,DT_STR,raw_remote_signature},
{"resolve",		-1L,DT_BOL,(char *)&resolve_mode},
{"savebyname",		-1L,DT_SYN,"savename"},
{"savemail",		-1L,DT_SYN,"sentmail"},
{"savename",		-1L,DT_BOL,(char *)&save_by_name},
{"saveto",		-1L,DT_SYN,"sentmail"},
{"sentmail",		-1L,DT_STR,raw_sentmail},
{"shell",		-1L,DT_STR,raw_shell},
{"sigdashes",		-1L,DT_BOL,(char *)&sig_dashes},
{"signature",		-1L,DT_MLT,(char *)SIGS},
{"sleepmsg",		-1L,DT_NUM,(char *)&sleepmsg},
{"softkeys",		-1L,DT_BOL|FL_OR,(char *)&hp_softkeys},
{"sort",		-1L,DT_SYN,"sortby"},
{"sortby",		-1L,DT_SRT,(char *)&sortby},
{"store",		-1L,DT_SYN,"alwaysstore"},
#ifdef MIME
{"textencoding", -1L,DT_STR,text_encoding},
#endif
{"timeout",		-1L,DT_NUM,(char *)&timeout},
{"titles",		-1L,DT_BOL,(char *)&title_messages},
{"tmpdir",		-1L,DT_STR,raw_temp_dir},
{"userlevel",		-1L,DT_NUM,(char *)&user_level},
{"username",		-1L,DT_SYN,"fullname"},
{"usetite",		-1L,DT_BOL|FL_AND,(char *)&use_tite},
{"visualeditor",	-1L,DT_STR,v_editor},
{"weed",		-1L,DT_BOL,(char *)&filter},
{"weedout",		-1L,DT_WEE,(char *)weedlist},
};
int NUMBER_OF_SAVEABLE_OPTIONS=(sizeof(save_info_data)/sizeof(save_info_recs_init));
save_info_recs *save_info = (save_info_recs *) save_info_data;
#else
extern save_info_recs *save_info;
extern int NUMBER_OF_SAVEABLE_OPTIONS;
#endif

