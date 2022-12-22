/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	`site.h' -- define site-dependancies here.
**
**	This one is specific to SPARCstations at UUNET Technologies Inc.
**	XXX -- oh no it isn't --vix
**
**	RCSID $Id: site.h,v 1.3 1994/01/31 04:43:39 donn Exp $
**
**	$Log: site.h,v $
 * Revision 1.3  1994/01/31  04:43:39  donn
 * The new params dir is /etc/uucp, which isn't writable by uucp.  Put SEQF
 * in the spool directory instead.
 *
 * Revision 1.2  1993/02/28  15:27:44  pace
 * Add recent uunet changes; make site.h more generic; add uuxqthook support.
 *
 * Revision 1.1.1.1  1992/09/28  20:08:34  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.2  1992/04/24  13:52:05  piers
 * cosmetic
 *
 * Revision 1.1  1992/04/14  21:48:05  piers
 * Initial revision
 *
*/

/*
**	Compilation configuration.
**
**	(Some are best defined in Makefile.)
*/

	/* Apollo UNIX is different */
	/* XXX - should depend on apollo-specific defines --vix */
/* #define	APOLLO		1 */

	/* Which version of BSD 4.? */
#define	BSD4		4

#if	BSD4 < 2
	/*	Select a busy-loop type:
	**	define FASTTIMER if you have the nap() system call;
	**	define FTIME if you have the ftime() system call;
	**	define BUSYLOOP if you must do a busy loop.
	**	
	**	Look at uucpdelay() in uucico/Diallers.c for details.
	*/
/*#define FASTTIMER /**/
/*#define FTIME /**/
/*#define BUSYLOOP /**/
#endif	/* BSD4 < 2 */

#if	BSD4 >= 4
	/* Use standard functions (e.g. from a shared library) */
#define	strccmp		strcasecmp
#define	strnccmp	strncasecmp
#endif

	/* Set to '1' if you're using an ANSI compiler (like gcc) */
	/* XXX - should depend on __STDC__ --vix */
#define	ANSI_C		1

	/* Set to `1' if cc thinks `enum' types are not `int' valued */
/* #define	ENUM_NOT_INT	1 */

	/* Set to `1' if the file <fcntl.h> exists */
#define	FCNTL		1

	/* Set to `1' if cc `switch` statement has to have int expressions
		(eg: not pointers) */
/* #define	INT_SWITCH	1 */

	/* Set to `1' if you wish `uucpd' to be able to run as a inetd-style listener */
	/* XXX - should be in CONFIG file --vix */
/* #define	NO_INETD	1 */

	/* Number of chars in last component of lockfile name */
#define	LOCKNAMESIZE	14

	/* Set to '1' if any uucp file-system is mounted via NFS
		because there are bugs with using rename(2) over NFS */
	/* XXX - should be in CONFIG file --vix */
/* #define	NFS		1 */

	/* Set to `1' if there is no `vfprintf(3)' routine in libc */
/* #define	NO_VFPRINTF	1 */

	/* Set to `1' if cc can't hack (void *) types */
/* #define	NO_VOID_STAR	1 */

	/* Location of global parameters file */
#define	PARAMSFILE	"/etc/uucp/CONFIG"

	/* Which version of SYSTEM V.? */
	/* XXX - should depend on system-specific include files/defines --vix */
/* #define	SYSV		3 */

	/* Set to `1' if the file <sys/mkdev.h> exists */
/* #define	SYSMKDEV		1 */

	/* Set to `1' if any file <sys/*vfs.h> exists */
/* #define	SYS_VFS		1 */

	/* Set to `1' to use system logging */
/* #define	USE_SYSLOG	1 */

	/* Size of local file-system efficient buffer size */
#define	FILEBUFFERSIZE	8192

	/* Node name if not in PARAMSFILE -- NULLSTR uses kernel name */
#define	NODENAME	NULLSTR			/* Portable */
#define	NODEUNKNOWN	"unknown"		/* If all else fails */
#define	NODENAMESIZE	7			/* Size used in old message filenames */
#define	NODENAMEMAXSIZE	14			/* Maximum matched length for NODENAME */
#if	!defined(BSD4) && !defined(SYSV)
#define	NODENAMEFILE	"/etc/uucpname"		/* What exists */
#endif	/* !defined(BSD4) && !defined(SYSV) */

	/* Valid bits in (unsigned) long, int, short, char */
	/* XXX - should depend on stddef.h and/or stdlib.h --vix */
#define	MAX_ULONG	0xffffffff
#define	MAX_LONG	0x7fffffff
#define	MAX_UINT	MAX_ULONG
#define	MAX_INT		MAX_LONG
#define	MAX_SHORT	0x7fff
#define	MAX_CHAR	0x7f

	/* Maximum length of a pathname */
#define	PATHNAMESIZE	MAXPATHLEN

#include <paths.h>

/*
**	Define the various kinds of connections to include.
**	The complete list is in the `condevs' array in "uucico/Diallers.c".
*/

/* #define ATT2224	1	/* AT&T 2224 */
/* #define BSDTCP	1	/* 4.2bsd TCP/IP */
/* #define CDS224	1	/* Concord Data Systems 2400 */
/* #define DATAKIT	1	/* ATT's datakit */
/* #define DF02		1	/* Dec's DF02/DF03 */
/* #define DF112	1	/* Dec's DF112 */
/* #define DN11		1	/* "standard" DEC dialer */
/* #define HAYES	1	/* Hayes' Smartmodem */
/* #define HAYES2400	1	/* Hayes' 2400 baud Smartmodem */
/* #define MICOM	1	/* Micom Mux port */
/* #define NOVATION	1	/* Novation modem */
/* #define PENRIL	1	/* PENRIL Dialer */
/* #define PNET		1	/* Purdue network */
/* #define RVMACS	1	/* Racal-Vadic MACS  820 dialer, 831 adaptor */
/* #define SYTEK	1	/* Sytek Local Area Net */
/* #define UNETTCP	1	/* 3Com's UNET */
/* #define USR2400	1	/* USRobotics Courier 2400 */
/* #define VA212	1	/* Racal-Vadic 212 */
/* #define VA811S	1	/* Racal-Vadic 811S dialer, 831 adaptor */
/* #define VA820	1	/* Racal-Vadic 820 dialer, 831 adaptor */
/* #define VADIC	1	/* Racal-Vadic 345x */
/* #define VENTEL	1	/* Ventel Dialer */
/* #define VMACS	1	/* Racal-Vadic MACS  811 dialer, 831 adaptor */
/* #define USE_X25	1	/* Enable X.25 code if X.25 available */
/* #define X25_PAD	1	/* X.25 PAD */

#if	defined(USE_X25) && !defined(X25_PAD)
#define X25_PAD		1	/*  */
#endif	/* USE_X25 && !X25_PAD */

#if	defined(USR2400) && !defined(HAYES)
#define HAYES		1	/*  */
#endif	/* USR2400 && !HAYES */

#if	defined(UNETTCP) || defined(BSDTCP)
#define TCPIP		1	/*  */
#endif	/* UNETTCP || BSDTCP */



/*
**	Run-time configurable parameter defaults.
**
**	Any defaults changed here also should be set
**	in the `Makefile' section `Installation parameters'.
**
**	These are run-time configurable
**	in the file named by PARAMSFILE.
**
**	Directory pathnames marked SPOOLDIR
**	that don't start with '/' will have SPOOLDIR pre-pended.
**
**	Directory pathnames marked PARAMSDIR
**	that don't start with '/' will have PARAMSDIR pre-pended.
**
**	Directory pathnames marked PROGDIR
**	that don't start with '/' will have PROGDIR pre-pended.
**
**	NB:	directory pathnames need a trailing '/'
**		(all parameters with names ending in `DIR').
*/

	/* Set to 1 (else 0) to allow uux to fetch remote files */
#define	ACCESS_REMOTE_FILES	0

	/* Set to unique filename if `uucico' should create it
		while active in its spool directory. */
#define	ACTIVEFILE	".active"

	/* Name of aliases file */
#define	ALIASFILE	"L.aliases"		/* PARAMSDIR */

	/* 1 if you want _all_ ACU lines to be DIALINOUT (cf: below) */
#define	ALLACUINOUT	0

	/* Set of (comma-separated) login names that are for `anonymous' use,
		and that shouldn't receive waiting messages */
/* #define	ANON_USERS	"uucp" */
#define	ANON_USERS	""

	/* Name of `legal commands' validation file */
#define	CMDSFILE	"L.cmds"		/* PARAMSDIR */

	/* Timeout for command file */
#define	CMDTIMEOUT	(HOUR*4)

	/* Name of file to hold connect accounting record
		(NULLSTR to turn off accounting) */
#define	CNNCT_ACCT_FILE	NULLSTR
/* #define	CNNCT_ACCT_FILE	"/var/log/uucp/connect" */	/* PARAMSDIR */

	/* Directory for corrupted command files */
#define	CORRUPTDIR	".Corrupt/"		/* SPOOLDIR */

	/* How long before untouched lock file is considered dead */
#define	DEADLOCKTIME	5400

	/* Default uucico turn-around time in seconds */
#define	DEFAULT_CICO_TURN	(30*60)

	/* Name of devices description file */
#define	DEVFILE		"L-devices"		/* PARAMSDIR */

	/* Name of dialcodes file */
#define	DIALFILE	"L-dialcodes"		/* PARAMSDIR */

	/* Pathname of ACU control program,
		if you want to use same modem for both dialin, and dialout. */
#define	DIALINOUT	"/usr/libexec/acucntrl"		/* PARAMSDIR */

	/* Directory check period for new work */
#define	DIRCHECKTIME	(60*15)

	/* Directory creation mode for work directories */
#define	DIR_MODE	0775

	/* Directory for program explanations 
		(for boolean program options etc.) */
#define	EXPLAINDIR	"explain/"		/* PARAMSDIR */

	/* Basic file creation mode */
#define	FILE_MODE	0666

	/* Lock file creation mode */
#define	LOCK_MODE	0444

	/* Set to 1 if lock files have names representing device number */
#define	LOCKNAMEISDEV	0

	/* Directory for locks */
#if	1 || LOCKNAMEISDEV		/* XXX - disable conditional (vix) */
#define	LOCKDIR		"/var/spool/uucp/"	/* SPOOLDIR */
#else	/* LOCKNAMEISDEV */
#define	LOCKDIR		_PATH_VARRUN		/* SPOOLDIR */
#endif	/* LOCKNAMEISDEV */

	/* Set to 1 if lockfiles have ASCII pid (otherwise int) */
#define	LOCKPIDISSTR	0

	/* Lock file pre-fix */
#if	0 && LOCKNAMEISDEV		/* XXX - disable conditional (vix) */
#define	LOCKPRE		"LK."
#else	/* LOCKNAMEISDEV */
#define	LOCKPRE		"LCK.."
#endif	/* LOCKNAMEISDEV */

	/* Directory for program log files */
#define	LOGDIR		"/var/log/uucp/"	/* SPOOLDIR */

	/* Non-null if you want to guarantee that the site they claim to be
		is who you expect them to be (list are exceptions). */
#define LOGIN_MUST_MATCH	""

	/* Log file creation mode */
#define	LOG_MODE	0644

	/* Set to '1' if you want to log unknown system names */
#define	LOG_BAD_SYS_NAME	0

	/* Internally generated mail delivery program and args */
#define	MAILPROG	_PATH_SENDMAIL
#define	MAILPROGARGS	NULLSTR	/* "-f&F!&O" */

	/* Max. number of simultaneous uuxqts */
#define	MAXUUSCHEDS	4

	/* Max. number of simultaneous uuxqts */
#define	MAXXQTS		15

	/* Minimum space that should be available on SPOOLDIR f/s */
#define	MINSPOOLFSFREE	100			/* Kbytes */

	/* File to request a `uuxqt' scan */
#define NEEDUUXQT        ".NeedUuxqt"		/* SPOOLDIR */

	/* Node conversation sequence numbers:
		define and create NODESEQFILE if you want
		node conversation sequence numbers checked */
#define	NODESEQFILE	NULLSTR			/* PARAMSDIR */
/*#define	NODESEQFILE	"SQFILE"	/* PARAMSDIR */
#define	NODESEQTEMP	"SQTMP"			/* PARAMSDIR */
#define	NODESEQLOCK	"SQ"			/* PARAMSDIR */

	/* Pathname of file to prevent further activity */
#define	NOLOGIN		_PATH_NOLOGIN

	/* Non-null if you want to deny access to sites
		that are not in your SYSFILE (list are exceptions). */
#define NOSTRANGERS	""

	/* Set to `1` to emulate original UUCP */
#define	ORIG_UUCP	1

	/* Directory for UUCP parameter files */
#define	PARAMSDIR	"/etc/uucp/"

	/* Directory for UUCP programs */
#define	PROGDIR		"/usr/bin/"

	/* Directory for program specific parameter files */
#define	PROG_PARAMSDIR	"params/"		/* PARAMSDIR */

	/* Location of public files directory */
#define	PUBDIR		"/var/spool/uucppublic/"/* SPOOLDIR */

	/* Name of work file sequence number file */
#define	SEQFILE		"SEQF"			/* SPOOLDIR */

	/* Bourne shell pathname */
#define	SHELL		_PATH_BSHELL

	/* Spool arena for uucp */
#define	SPOOLDIR	"/var/spool/uucp/"

	/* Alternative spool areas for uucp (`,' separated list) */
#define	SPOOLALTDIRS	NULLSTR
/* #define	SPOOLALTDIRS	"/var/spool/nuucp/" */

	/* Directory for status files */
#define	STATUSDIR	"/var/log/uucp/status/"	/* SPOOLDIR */

	/* Define pathname of optional command to be run
		if NOSTRANGERS/LOGIN_MUST_MATCH tests fail */
#define STRANGERSCMD	"remote.unknown"	/* PARAMSDIR */

	/* Name of system file */
#define	SYSFILE		"L.sys"			/* PARAMSDIR */

	/* Pathname for `telnetd(8)' if uucpd should login non-uucico users */
#define	TELNETD		"/usr/libexec/telnetd"

	/* Directory for (very) temporary work files */
#define	TMPDIR		_PATH_TMP

	/* Name of system file */
#define	USERFILE	"USERFILE"		/* PARAMSDIR */

	/* Installed path name of `uucico' and default args */
#define	UUCICO		"/usr/libexec/uucico"		/* PROGDIR */
#define	UUCICOARGS	"-cr1"			/* Initial params for UUCICO */

	/* Set to `1' if you wish uucpd to only allow logins to accounts with shell==UUCICO
		(Otherwise, it starts up `rlogind(8)' for non-UUCICO accounts.) */
#define	UUCICO_ONLY	0

	/* Installed path name of `uucp' and default args */
#define	UUCP		"uucp"			/* PROGDIR */
#define	UUCPARGS	NULLSTR			/* Initial params for UUCP */

	/* User/group for UUCP */
#define	UUCPGROUP	"uucp"
#define	UUCPUSER	"uucp"

	/* Installed path name of `uusched' and default args */
#define	UUSCHED		"/usr/libexec/uusched"		/* PROGDIR */
#define	UUSCHEDARGS	NULLSTR			/* Initial params for UUSCHED */

	/* Installed path name of `uuxqt' and default args */
#define	UUX		"uux"			/* PROGDIR */
#define	UUXARGS		NULLSTR			/* Initial params for UUX */

	/* Installed path name of `uuxqt' and default args and hook */
#define	UUXQT		"/usr/libexec/uuxqt"			/* PROGDIR */
#define	UUXQTARGS	NULLSTR			/* Initial params for UUXQT */
#define	UUXQTHOOK	"uuxqt_hook"		/* PARAMD */

	/* Set to `1' to verify incoming TCP addresses */
#define	VERIFY_TCP_ADDRESS	0

	/* Set to '1' if you want warnings about otherwise acceptable long message names */
#define	WARN_NAME_TOO_LONG	0

	/* File creation mask (umask) for temporary work files */
#define	WORK_FILE_MASK	0137

	/* Directory where uuxqt places temp files */
#define	XQTDIR		".Xqtdir/"		/* SPOOLDIR */


/*
**	Sanity checks
*/

#if	BSD4 >= 2 || SYSV >= 3
#if	NFS == 0
#undef	RENAME_2
#define	RENAME_2	1
#endif	/* NFS == 0 */
#undef	MKDIR_2
#define	MKDIR_2		1
#endif	/* BSD4 >= 2 || SYSV >= 3 */

#if	BSD4 >= 3 || SYSV >= 1
#undef	PGRP
#define	PGRP		1
#endif	/* BSD4 >= 3 || SYSV >= 1 */

#ifndef	SYS_ACCT
#define	sysaccf(A)
#define	sysacct(A,B)
#endif	/* SYS_ACCT */
