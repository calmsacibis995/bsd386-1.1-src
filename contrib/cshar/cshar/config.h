/*
**  Configuration file for shar and friends.
**
**  This is known to work on Ultrix1.2 and Sun3.4 machines; it may work
**  on other BSD variants, too.
**
**  $Header: /bsdi/MASTER/BSDI_OS/contrib/cshar/cshar/config.h,v 1.1.1.1 1993/02/23 18:00:02 polk Exp $
*/


/*
**  Higher-level controls for which operating system we're running on.
*/
#define UNIX				/* Works			*/
/*efine MSDOS				/* Should work			*/
/*efine VMS				/* Doesn't work			*/


/*
**  A dense section of one-line compilation controls.  If you're confused,
**  your best bet is to search through the source to see where and how
**  each one of these is used.
*/
#define IDX		index		/* Maybe strchr?		*/
#define RDX		rindex		/* Maybe strrchr?		*/
/*efine NEED_MKDIR			/* Don't have mkdir(2)?		*/
/*efine NEED_QSORT			/* Don't have qsort(3)?		*/
/*efine NEED_RENAME			/* Don't have rename(2 or 3)?	*/
/*efine NEED_GETOPT			/* Need local getopt object?	*/
#define CAN_POPEN			/* Can invoke file(1) command?	*/
/*efine USE_MY_SHELL			/* Don't popen("/bin/sh")?	*/
/*efine BACKUP_PREFIX	"B-"		/* Instead of ".BAK" suffix?	*/
typedef void		 sigret_t;	/* What a signal handler returns */
typedef int		*align_t;	/* Worst-case alignment, for lint */
/* typedef long		time_t		/* Needed for non-BSD sites?	*/
/* typedef long		off_t		/* Needed for non-BSD sites?	*/
/*efine void		int		/* If you don't have void	*/
#define SYS_WAIT			/* Have <sys/wait.h> and vfork?	*/
/*efine USE_SYSTEM			/* Use system(3), not exec(2)?	*/
#define USE_SYSERRLIST			/* Have sys_errlist[], sys_nerr? */
#define USE_GETPWUID			/* Use getpwuid(3)?		*/
#define DEF_SAVEIT	1		/* Save headers by default?	*/
/*efine FMT02d				/* Need "%02.2d", not "%2.2d"?	*/
#define MAX_LEVELS	6		/* Levels for findsrc to walk	*/
#define THE_TTY		"/dev/tty"	/* Maybe "con:" for MS-DOS?	*/
#define RCSID				/* Compile in the RCS strings?	*/
#define USERNAME	"USER"		/* Your name, if not in environ	*/
/*efine PTR_SPRINTF			/* Need extern char *sprinf()?	*/
/*efine ANSI_HDRS			/* Use <stdlib.h>, etc.?	*/
#define REGISTER	register	/* Do you trust your compiler?	*/


/*
**  There are several ways to get current machine name.  Enable just one
**  of one of the following lines.
*/
#define GETHOSTNAME			/* Use gethostname(2) call	*/
/*efine UNAME				/* Use uname(2) call		*/
/*efine UUNAME				/* Invoke "uuname -l"		*/
/*efine	WHOAMI				/* Try /etc/whoami & <whoami.h>	*/
/*efine HOST		"SITE"		/* If all else fails		*/


/*
**  There are several different ways to get the current working directory.
**  Enable just one of the following lines.
*/
#define GETWD				/* Use getwd(3) routine		*/
/*efine GETCWD				/* Use getcwd(3) routine	*/
/*efine PWDPOPEN			/* Invoke "pwd"			*/
/*efine PWDGETENV	"PWD"		/* Get $PWD from environment	*/


/*
**  If you're a notes site, you might have to tweaks these two #define's.
**  If you don't care, then set them equal to something that doesn't
**  start with the comment-begin sequence and they'll be effectively no-ops
**  at the cost of an extra strcmp.  I've also heard of broken MS-DOS
**  compilers that don't ignore slash-star inside comments!  Anyhow, for
**  more details see unshar.c
*/
/*efine NOTES1		"/* Written "	/* This is what notes 1.7 uses	*/
/*efine NOTES2		"/* ---"	/* This is what notes 1.7 uses	*/
#define NOTES1		"$$"		/* This is a don't care		*/
#define NOTES2		"$$"		/* This is a don't care		*/


/*
**  The findsrc program uses the readdir() routines to read directories.
**  If your system doesn't have this interface, there are public domain
**  implementations available for Unix from the comp.sources.unix archives,
**  GNU has a VMS one inside EMACS, and this package comes with kits for
**  MS-DOS and the Amiga.  Help save the world and use or write a readdir()
**  package for your system!
*/

/* Now then, where did I put that header file?   Pick one. */
#define IN_SYS_DIR			/* <sys/dir.h>			*/
/*efine IN_SYS_NDIR			/* <sys/ndir.h>			*/
/*efine IN_DIR				/* <dir.h>			*/
/*efine IN_DIRECT			/* <direct.h>			*/
/*efine IN_NDIR				/* "ndir.h"			*/
/*efine IN_DIRENT			/* <dirent.h>			*/

/*  What readdir() returns.  Must be a #define because of #include order. */
#ifdef	IN_DIRENT
#define DIRENTRY	struct dirent
#else
#define DIRENTRY	struct direct
#endif	/* IN_DIRENT */

/*
**  Congratulations, you're done!
*/
