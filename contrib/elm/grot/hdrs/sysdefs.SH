case $CONFIG in
'')
    if test ! -f config.sh; then
	ln ../config.sh . || \
	ln ../../config.sh . || \
	ln ../../../config.sh . || \
	(echo "Can't find config.sh."; exit 1)
	echo "Using config.sh from above..."
    fi
    . ./config.sh
    ;;
esac
case "$0" in
*/*) cd `expr X$0 : 'X\(.*\)/'` ;;
esac

echo "Extracting hdrs/sysdefs.h (with variable substitutions)"
sed <<!GROK!THIS! >sysdefs.h -e 's!^#undef!/\*#undef!'
/* $Id: sysdefs.SH,v 5.4 1992/12/24 19:24:10 syd Exp  */
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
 * $Log: sysdefs.SH,v $
# Revision 1.2  1994/01/14  00:52:12  cp
# 2.4.23
#
 * Revision 5.4  1992/12/24  19:24:10  syd
 * EDITOROPTS and DEFEDITOR appear to be unused but here is
 * a patch to make use of $editoropts.
 * From: "Robert L. Howard" <robert.howard@matd.gatech.edu>
 *
 * Revision 5.3  1992/11/22  01:14:20  syd
 * Allow SCO MMDF to use the mmdf library for mailer via execmail.
 * From: Larry Philps <larryp@sco.com>
 *
 * Revision 5.2  1992/10/24  13:44:41  syd
 * There is now an additional elmrc option "displaycharset", which
 * sets the charset supported on your terminal. This is to prevent
 * elm from calling out to metamail too often.
 * Plus a slight documentation update for MIME composition (added examples)
 * From: Klaus Steinberger <Klaus.Steinberger@Physik.Uni-Muenchen.DE>
 *
 * Revision 5.1  1992/10/03  22:34:39  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/**  System level, configurable, defines for the ELM mail system.  **/


#define MAX_IN_WEEDLIST 150	/* max headers to weed out               */

#define MAX_HOPS	35	/* max hops in return addr to E)veryone  */

#define DEFAULT_BATCH_SUBJECT  "$defbatsub"

#define DEFAULT_DOMAIN  "$mydomain"  /* if mydomain file is missing */

/** If you want to implement 'site hiding' in the mail, then you'll need to
    uncomment the following lines and set them to reasonable values.  See 
    the configuration guide for more details....(actually these are undoc-
    umented because they're fairly dangerous to use.  Just ignore 'em and
    perhaps one day you'll find out what they do, ok?)
**/

/****************************************************************************

#define   SITE_HIDING
#define   HIDDEN_SITE_NAME	"fake-machine-name"
#define   HIDDEN_SITE_USERS	"/usr/mail/lists/hidden_site_users"

****************************************************************************/

#define system_text_file        "$lib/aliases.text"
#define system_data_file        "$lib/aliases"
#define system_rc_file		"$lib/elm.rc"

#define ALIAS_TEXT		".elm/aliases.text"
#define ALIAS_DATA		".elm/aliases"

#define hostdomfile             "$lib/domain"

/** where to put the output of the elm -d command... (in home dir) **/
#define DEBUGFILE	"ELM:debug.info"
#define OLDEBUG		"ELM:debug.last"

#define	default_temp   	"$tmpdir/"
#define temp_file	"snd."
#define temp_form_file	"form."
#define temp_mbox	"mbox."
#define temp_print      "print."
#define temp_edit	"elm-edit"
#define temp_uuname	"uuname."
#define temp_state	"est."

#define emacs_editor	"$emacs"
#define vi_editor	"$vi"

#define default_editor	"$defeditor $editoropts"
#define mailhome	"$maildir/"

#define default_shell	"$prefshell"
#define default_pager	"$pager"
#define default_printout	"$cat %s | $linepr"

#define default_charset	"$defcharset"
#define default_display_charset	"$defdispcharset"
#define default_encoding	"$defencoding"
#define execmail	"$execmail"
#define emflags		""
#define emflagsv	"-d"		/* Verbose voyuer mode */
#define emflagmt	"-m"		/* metoo copy to sender */

#define sendmail	"$sendmail"
#define smflags		"-oi -oem"	/* ignore dots and mail back errors */
#define smflagsv	"-oi -oem -v"	/* Verbose voyuer mode */
#define smflagmt	" -om"		/* metoo copy to sender */
#define submitmail	"$submit"
#define submitflags	"-mlrnvxto,cc*"
#define submitflags_s	"-mlrnv"
#define mailer		"$mailer"

#define mailx		"$mailx"

#define helphome	"$lib"
#define helpfile	"elm-help"

#define ELMRC_INFO	"$lib/elmrc-info"

#define elmrcfile	".elm/elmrc"
#define old_elmrcfile	".elm/elmrc.old"
#define mailheaders	".elm/elmheaders"
#define dead_letter	"Canceled.mail"

#define unedited_mail	"emergency.mbox"

#define newalias	"newalias 1>&2 > /dev/null"
#define readmsg		"readmsg"

#define remove_cmd	"$rm -f"	/* how to remove a file */
#define cat		"$cat"		/* how to display files */
#define sed_cmd		"$sed"		/* how to access sed */
#define move_cmd	"$mv"		/* how to access sed */
#define uuname		"$uuname"	/* how to get a uuname  */

#define MSG_SEPARATOR	"\001\001\001\001\n"	/* mmdf message seperator */

#ifdef CRAY
/* avoid conflict with typedef'd word */
#define word wrd   
#endif /* CRAY */

#ifndef STRSTR
char *strstr();
#endif

!GROK!THIS!
