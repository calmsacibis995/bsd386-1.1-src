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
echo "Extracting config.h (with variable substitutions)"
sed <<!GROK!THIS! >config.h -e 's!^#undef!/\*#undef!'
/* config.h
 * This file was produced by running the config.h.SH script, which
 * gets its values from config.sh, which is generally produced by
 * running Configure.
 *
 * Feel free to modify any of this as the need arises.  Note, however,
 * that running config.h.SH again will wipe out any changes you've made.
 * For a more permanent change edit config.sh and rerun config.h.SH.
 */


/* BIN:
 *	This symbol holds the name of the directory in which the user wants
 *	to put publicly executable images for the package in question.  It
 *	is most often a local directory such as /usr/local/bin.
 */
#define BIN "$bin"             /**/

/* CPPSTDIN:
 *	This symbol contains the first part of the string which will invoke
 *	the C preprocessor on the standard input and produce to standard
 *	output.	 Typical value of "cc -E" or "/lib/cpp".
 */
/* CPPMINUS:
 *	This symbol contains the second part of the string which will invoke
 *	the C preprocessor on the standard input and produce to standard
 *	output.  This symbol will have the value "-" if CPPSTDIN needs a minus
 *	to specify standard input, otherwise the value is "".
 */
#define CPPSTDIN "$cppstdin"
#define CPPMINUS "$cppminus"

/* GETOPT:
 *	This symbol, if defined, indicates that the getopt() routine exists.
 */
#$d_getopt	GETOPT		/**/

/* MEMCPY:
 *	This symbol, if defined, indicates that the memcpy routine is available
 *	to copy blocks of memory.  Otherwise you should probably use bcopy().
 *	If neither is defined, roll your own.
 */
#$d_memcpy	MEMCPY		/**/

/* MKDIR:
 *	This symbol, if defined, indicates that the mkdir routine is available
 *	to create directories.  Otherwise you should fork off a new process to
 *	exec /bin/mkdir.
 */
#$d_mkdir	MKDIR		/**/

/* RENAME:
 *	This symbol, if defined, indicates that the rename routine is available
 *	to rename files.  Otherwise you should do the unlink(), link(), unlink()
 *	trick.
 */
#$d_rename	RENAME		/**/

/* SYMLINK:
 *	This symbol, if defined, indicates that the symlink routine is available
 *	to create symbolic links.
 */
#$d_symlink	SYMLINK		/**/

/* WHOAMI:
 *	This symbol, if defined, indicates that the program may include
 *	whoami.h.
 */
#$d_whoami	WHOAMI		/**/

/* PREFSHELL:
 *	This symbol contains the full name of the preferred user shell on this
 *	system.  Usual values are /bin/csh, /bin/ksh, /bin/sh.
 */
#define PREFSHELL "$prefshell"		/**/

/* EUNICE:
 *	This symbol, if defined, indicates that the program is being compiled
 *	under the EUNICE package under VMS.  The program will need to handle
 *	things like files that don't go away the first time you unlink them,
 *	due to version numbering.  It will also need to compensate for lack
 *	of a respectable link() command.
 */
/* VMS:
 *	This symbol, if defined, indicates that the program is running under
 *	VMS.  It is currently only set in conjunction with the EUNICE symbol.
 */
#$d_eunice	EUNICE		/**/
#$d_eunice	VMS		/**/

/* CONFIGURE_DATE
 *	This symbol contains the last date that configure was run for elm -v output.
 */
#define		CONFIGURE_DATE	"$c_date"

/* ASCII_CTYPE:
 *	This symbol, if defined, indicates that the ctype functions and
 *	macros are ASCII specific and not 8-bit clean.
 */
#$d_ascii	ASCII_CTYPE	/**/

/* BROKE_CTYPE:
 *	This symbol, if defined, indicates that toupper and tolower
 *	routines/macros are not safe to use.
 */
#$d_broke_ctype	BROKE_CTYPE	/**/

/* ENABLE_CALENDAR:
 *	This symbol, if defined, indicates that the calendar feature
 *	should be supported.
 */
#$d_calendar	ENABLE_CALENDAR	/**/
#define dflt_calendar_file	"$calendar"	

/* CHOWN_NEG1:
 */
#$d_chown_neg1	CHOWN_NEG1	/**/

/* DONT_ESCAPE_MESSAGES:
 *	This symbol, if defined, indicates that a binary capable MTA is in use
 *	that honors the content-length header and no message constructs need
 *	to be escaped.
 */
#$d_content	DONT_ESCAPE_MESSAGES /**/

/* CRYPT:
 *	This symbol, if defined, indicates that the crypt routine is available
 *	to encrypt passwords and the like.
 */
#$d_crypt	CRYPT		/**/

/* HAS_CUSERID:
 *	This symbol, if defined, means not to include our own cuserid().
 */
#$d_cuserid HAS_CUSERID		/**/

/* DISP_HOST:
 *	This symbol, if defined, indicates that elm should display the
 *	host name on the index screen.
 */
#$d_disphost	DISP_HOST /**/

/* GETDOMAINNAME
 *	This symbol, if defined, indicates that the getdomainname system call
 *	is available to obtain the domain name.  Note that the $LIB/domain
 *	file overrides the value of getdomainname().  If getdomainname() exists,
 *	MYDOMAIN will be ignored.
 */
/* USEGETDOMAINNAME
 *	This symbol, if defined, indicates that the getdomainname system call
 *	should be used to obtain the domain name.  Note that the $LIB/domain
 *	file overrides the value of getdomainname().  If this variable is set
 *	MYDOMAIN (and therefore DEFAULT_DOMAIN) will be ignored.
 */
#$d_domname	GETDOMAINNAME		/**/

#$d_usegetdom	USEGETDOMAINNAME		/**/

/* ERRLST:
 *	This symbol, if defined, indicates that the sys_errlist and sys_nerr
 *	symbols exist.
 */
#$d_errlst	ERRLST	/**/

/* USE_FLOCK_LOCKING
 *	This symbol, if defined, indicates that the flock mailbox locking should be used.
 */
/* USE_DOTLOCK_LOCKING
 *	This symbol, if defined, indicates that the .lock mailbox locking should be used.
 */
/* USE_FCNTL_LOCKING
 *	This symbol, if defined, indicates that SYSV style fcntl file locking should be used.
 */
/* LOCK_DIR
 *	This symbol is the name of the lock directory for access (not mailbox) locks.
 *	It will be /usr/spool/locks or /usr/spool/uucp
 */
#$d_flock	USE_FLOCK_LOCKING		/**/

#$d_dotlock	USE_DOTLOCK_LOCKING		/**/

#$d_fcntlock	USE_FCNTL_LOCKING	/**/

#define		LOCK_DIR	"$lock_dir"	/**/

/* FTRUNCATE:
 *	This symbol, if defined, indicates that the ftruncate() routine exists.
 */
#$d_ftruncate	FTRUNCATE		/**/

/* GETHOSTNAME:
 *	This symbol, if defined, indicates that the C program may use the
 *	gethostname() routine to derive the host name.  See also DOUNAME.
 */
/* DOUNAME:
 *	This symbol, if defined, indicates that the C program may use the
 *	uname() routine to derive the host name.  See also GETHOSTNAME.
 */
/* HOSTCOMPILED:
 *	This symbol, if defined, indicated that the host name is compiled
 *	in from the string hostname
 */
#$d_gethname	GETHOSTNAME	/**/
#$d_douname	DOUNAME		/**/
#$d_host_comp	HOSTCOMPILED	/**/

/* HAVETERMLIB:
 *	This symbol, when defined, indicates that termlib-style routines
 *	are available.  There is nothing to include.
 */
#$d_havetlib	HAVETERMLIB	/**/

/* index:
 *	This preprocessor symbol is defined, along with rindex, if the system
 *	uses the strchr and strrchr routines instead.
 */
/* rindex:
 *	This preprocessor symbol is defined, along with index, if the system
 *	uses the strchr and strrchr routines instead.
 */
#$d_index	index strchr	/* cultural */
#$d_index	rindex strrchr	/*  differences? */

/* INTERNET:
 *	This symbol, if defined, indicates that there is a mailer available
 *	which supports internet-style addresses (user@site.domain).
 */
#$d_internet	INTERNET	/**/

/* ISPELL:
 *	This symbol, if defined, indicates that the GNU ispell
 *	spelling checker that is available to Elm.
 */
/* ISPELL_PATH:
 *	This symbol contains the path to the GNU ispell
 *	spelling checker that is available to Elm.
 */
/* ISPELL_OPTIONS:
 *	This symbol contains the options to the GNU ispell
 *	spelling checker that is available to Elm.
 */
#$d_ispell	ISPELL	/**/
#define		ISPELL_PATH	"$ispell_path" /**/
#define		ISPELL_OPTIONS	"$ispell_options" /**/

/* I_LOCALE:
 *	This symbol, if defined, indicates that the file locale.h
 *	should be included
 */
/* MSGCAT:
 *	This symbol, if defined, indicates that the MSGCAT NLS libraries
 *	are available.
 */
/* USENLS:
 *	This symbol, if defined, indicates that the Elm NLS libraries
 *	are being used instead of the system NLS libraries.
 */
#$d_locale	I_LOCALE	/**/
#$d_nl_types	I_NL_TYPES	/**/
#$d_msgcat	MSGCAT		/**/
#$d_usenls	USENLS		/**/

/* MALLOCVOID:
 *	This symbol, if defined, indicates that the malloc_t type should
 *	be void * instead of char *
 */
#$d_mallocvoid	MALLOCVOID	/**/

/* ALLOW_MAILBOX_EDITING:
 *	This symbol, if defined, indicates that the E)dit mailbox
 *	function is to be allowed.
 */
#$d_mboxedit	ALLOW_MAILBOX_EDITING	/**/

/* MIME:
 *	This symbol, if defined, indicates that the MIME mail
 *	extension utilities are available
 */
#$d_mime	MIME	/**/

/* MMDF:
 *	This symbol, if defined, indicates that mailboxes are in
 *	the MMDF format.
 */
#$d_mmdf	MMDF	/**/

/* AUTO_BACKGROUND:
 *	This symbol, if defined, indicates that newmail should go to
 *	the background automatically.
 */
#$d_newauto AUTO_BACKGROUND /**/

/* DONT_ADD_FROM:
 *	This symbol, if defined, indicates that elm should not add
 *	the From: header
 */
/* USE_DOMAIN:
 *	This symbol, if defined, indicates that elm should add
 *	the domain name to our address
 */
#$d_noaddfrom	DONT_ADD_FROM /**/
#$d_usedomain	USE_DOMAIN /**/

/* NO_XHEADER:
 *	This symbol, if defined, will not automatically add "X-Mailer:"
 *	headers.
 */
#$d_noxheader	NO_XHEADER	/**/

/* PIDCHECK:
 *	This symbol, if defined, means that the kill(pid, 0) will
 *	check for an active pid.
 */
#$d_pidcheck PIDCHECK		/**/

/* PTEM:
 *	This symbol, if defined, indicates that the sys/ptem.h include file is
 *	needed for window sizing.
 */
#$d_ptem	PTEM		/**/

/* PUTENV:
 *	This symbol, if defined, indicates that putenv() exists.
 */
#$d_putenv	PUTENV	/**/

/* REMOVE_AT_LAST:
 *	This symbol, if defined, tells the C code to remove the lock
 *	file on lock failure.
 */
/* MAX_ATTEMPTS:
 *	This symbol defines to the C code the number of times to try
 *	locking the mail file.
 */
#$d_remlock REMOVE_AT_LAST	/**/
#define MAX_ATTEMPTS	$maxattempts

/* SAVE_GROUP_MAILBOX_ID:
 *	This symbol, if defined, indicates that Elm needs to restore the
 *	group id of the file, as it is running setgid.
 */
#$d_savegrpmboxid SAVE_GROUP_MAILBOX_ID	/**/

/* SIGVEC:
 *	This symbol, if defined, indicates that BSD reliable signals routine
 *	sigvec is available.
 */
/* SIGVECTOR:
 *	This symbol, if defined, indicates that the sigvec() routine is called
 *	sigvector() instead, and that sigspace() is provided instead of
 *	sigstack().  This is probably only true for HP-UX.
 */
/* SIGSET:
 *	This symbol, if defined, indicates that BSD reliable signal routine
 *	sigset is available.
 */
/* POSIX_SIGNALS:
 *	This symbol, if defined, indicated that POSIX sigaction
 *	routine is available.
 */
/* HASSIGHOLD:
 *	This symbol, if defined, indicates that sighold routine is
 *	available.
 */
/* HASSIGBLOCK
 *	This symbol, if defined, indicates that the sigblock routine is
 *	available.
 */
/* HASSIGPROCMASK:
 *	This symbol, if defined, indicates that POSIX sigprocmask
 *	routine is available.
 */
#$d_sigvec	SIGVEC		/**/

#$d_sigvectr	SIGVECTOR	/**/

#$d_sigset	SIGSET	/**/

#$d_sigaction	POSIX_SIGNALS	/**/

#$d_sighold	HASSIGHOLD	/**/

#$d_sigblock	HASSIGBLOCK	/**/

#$d_sigprocmask	HASSIGPROCMASK	/**/

/* STRSPN:
 *	This symbol, if defined, indicates that the strspn() routine exists.
 */
/* STRCSPN:
 *	This symbol, if defined, indicates that the strcspn() routine exists.
 */
/* STRPBRK:
 *	This symbol, if defined, indicates that the strpbrk() routine exists.
 */
#$d_strspn	STRSPN		/**/

#$d_strcspn	STRCSPN		/**/

#$d_strpbrk	STRPBRK		/**/

/* STRERROR:
 *	This symbol, if defined, indicates that strerror() exists.
 */
#$d_strerror	STRERROR	/**/

/* STRFTIME:
 *	This symbol, if defined, indicates that strftime() exists.
 */
#$d_strftime	STRFTIME	/**/

/* STRINGS:
 *	This symbol, if defined, indicates that the file strings.h
 *	should be included not string.h
 */
/* PWDINSYS:
 *	This symbol, if defined, indicates that the file pwd.h
 *	is in the sys sub directory
 */
#$d_strings	STRINGS		/**/
#$d_pwdinsys	PWDINSYS	/**/

/* STRSTR:
 *	This symbol, if defined, indicates that strstr() exists.
 */
#$d_strstr	STRSTR	/**/

/* STRTOK:
 *	This symbol, if defined, indicates that strtok() exists.
 */
#$d_strtok	STRTOK	/**/

/* ALLOW_SUBSHELL:
 *	This symbol, if defined, indicates that the '!' subshell
 *	function is to be allowed at various places.
 */
#$d_subshell	ALLOW_SUBSHELL	/**/

/* TEMPNAM:
 *	This symbol, if defined, indicates that the tempnam() routine exists.
 */
#$d_tempnam	TEMPNAM		/**/

/* TERMIOS:
 *	This symbol, if defined, indicates that the program should include
 *	termios.h rather than sgtty.h or termio.h.  There are also differences
 *	in the ioctl() calls that depend on the value of this symbol.
 */
/* TERMIO:
 *	This symbol, if defined, indicates that the program should include
 *	termio.h rather than sgtty.h.  There are also differences in the
 *	ioctl() calls that depend on the value of this symbol.
 */
#$d_termios	TERMIOS		/**/

#$d_termio	TERMIO		/**/

/* USE_EMBEDDED_ADDRESSES:
 *	This symbol, if defined, indicates that replyto: and from:
 *	headers can be trusted.
 */
#$d_useembed USE_EMBEDDED_ADDRESSES	 /**/

/* UTIMBUF:
 *	This symbol is defined if this system defines struct utimbuf.
 */
#$d_utimbuf UTIMBUF		/**/

/* VFORK:
 *	This symbol, if defined, indicates that vfork() exists.
 */
#$d_vfork	VFORK	/**/

/* DEFEDITOR:
 *	This symbol contains the name of the default editor.
 */
/* EDITOROPTS:
 *	This symbol contains the command line options for the default editor.
 */
#define DEFEDITOR "$defeditor"		/**/

#define EDITOROPTS "$editoropts"		/**/

/* HOSTNAME:
 *	This symbol contains name of the host the program is going to run on.
 *	The domain is not kept with hostname, but must be gotten from MYDOMAIN.
 *	The dot comes with MYDOMAIN, and need not be supplied by the program.
 *	If gethostname() or uname() exist, HOSTNAME may be ignored.
 */
/* MYDOMAIN:
 *	This symbol contains the domain of the host the program is going to
 *	run on.  The domain must be appended to HOSTNAME to form a complete
 *	host name.  The dot comes with MYDOMAIN, and need not be supplied by
 *	the program.  If the host name is derived from PHOSTNAME, the domain
 *	may or may not already be there, and the program should check.
 */
#define HOSTNAME "$hostname"		/**/
#define MYDOMAIN "$mydomain"		/**/

/* I_MEMORY:
 *	This symbol, if defined, indicates that the file memory.h
 *	should be included instead of declaring the memory routines.
 */
#$i_memory	I_MEMORY	/**/

/* I_STDARG:
 *	This symbol, if defined, indicates that the file stdarg.h
 *	should be included instead of varargs
 */
#$i_stdarg	I_STDARG	/**/

/* I_STDLIB:
 *	This symbol, if defined, indicates that the file stdlib.h
 *	should be included instead of declaring the stdlib routines.
 */
#$i_stdlib	I_STDLIB	/**/

/* I_TIME:
 *	This symbol is defined if the program should include <time.h>.
 */
/* I_SYSTIME:
 *	This symbol is defined if the program should include <sys/time.h>.
 */
/* I_SYSTIMEKERNEL:
 *	This symbol is defined if the program should include <sys/time.h>
 *	with KERNEL defined.
 */
#$i_time		I_TIME	 	/**/
#$i_systime	I_SYSTIME 	/**/
#$d_systimekernel	SYSTIMEKERNEL 	/**/

/* I_UNISTD:
 *	This symbol, if defined, indicates that the file unistd.h
 *	should be included instead of declaring the unistd routines.
 */
#$i_unistd	I_UNISTD	/**/

/* I_UTIME:
 *	This symbol, if defined, indicates that the file utime.h
 *	should be included instead of declaring our own utimbuf.
 */
/* I_USYSTIME:
 *	This symbol, if defined, indicates that the file sys/utime.h
 *	should be included instead of declaring our own utimbuf.
 */
#$i_utime	I_UTIME	/**/
#$i_sysutime	I_SYSUTIME	/**/

/* PASSNAMES:
 *	This symbol, if defined, indicates that full names are stored in
 *	the /etc/passwd file.
 */
/* BERKNAMES:
 *	This symbol, if defined, indicates that full names are stored in
 *	the /etc/passwd file in Berkeley format (name first thing, everything
 *	up to first comma, with & replaced by capitalized login ID, yuck).
 */
/* USGNAMES:
 *	This symbol, if defined, indicates that full names are stored in
 *	the /etc/passwd file in USG format (everything after - and before ( is
 *	the name).
 */
#$d_passnames	PASSNAMES /*  (undef to take name from ~/.fullname) */
#$d_berknames	BERKNAMES /* (that is, ":name,stuff:") */
#$d_usgnames	USGNAMES  /* (that is, ":stuff-name(stuff):") */

/* SIG_TYPE:
 *	This symbol contains the type name of the signal handler functions.
 */
#define	SIGHAND_TYPE	$sigtype

/* TZMINS_USE_xxxxxx:
 *	Specify how to get timezone offset.  Possible values are:
 *		 TZMINS_USE_TM_TZADJ	use (struct tm *)->tm_tzadj
 *		 TZMINS_USE_TM_GMTOFF	use (struct tm *)->tm_gmtoff
 *		 TZMINS_USE_TZAZ_GLOBAL	use "timezone, altzone" externals
 *		 TZMINS_USE_TZ_GLOBAL	use "timezone" external
 *		 TZMINS_USE_FTIME	use ftime() function
 *		 TZMINS_USE_TIMEOFDAY	use gettimeofday() function
 */
/* TZNAME_USE_xxxxxx:
 *	Specify how to get timezone name.  Possible values are:
 *		 TZNAME_USE_TM_NAME	use (struct tm *)->tm_name
 *		 TZNAME_USE_TM_ZONE	use (struct tm *)->tm_zone
 *		 TZNAME_USE_TZNAME	use "tzname[]" external
 *		 TZNAME_USE_TIMEZONE	use timezone() function
 */
#define TZMINS_USE_$tzmins_handling
#define TZNAME_USE_$tzname_handling

/* XENIX:
 *	This symbol, if defined, indicates this is a Xenix system,
 *	for knocking  out the far keyword in selected places.
 */
/* BSD:
 *	This symbol, if defined, indicates this is a BSD type system,
 */
#$d_xenix	XENIX	/**/
#$d_bsd	BSD	/**/

!GROK!THIS!
