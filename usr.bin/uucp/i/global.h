/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Include local peculiarities here.
**
**	RCSID $Id: global.h,v 1.4 1994/01/31 01:25:48 donn Exp $
**
**	$Log: global.h,v $
 * Revision 1.4  1994/01/31  01:25:48  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:57:22  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:41:51  vixie
 * Initial revision
 *
 * Revision 1.3  1993/02/28  15:27:42  pace
 * Add recent uunet changes; make site.h more generic; add uuxqthook support.
 *
 * Revision 1.2  1992/11/20  19:40:43  trent
 * update from ziegast
 *
 * Revision 1.1.1.1  1992/09/28  20:08:33  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:48:05  piers
 * Initial revision
 *
*/

#include	"site.h"

/*
**	Global definitions for magic numbers.
*/

#define	DATESTRLEN	13			/* Size of buffer for DateString() */
#define	DATETIMESTRLEN	18			/* Size of buffer for DateTimeStr() */
#define	ISODATETIMESTRLEN	18		/* Size of buffer for ISODateTimeStr() */
#define	MAXWORKFILES	10			/* Maximum number of work files needed */
#define	NUMERICARGLEN	(ULONG_SIZE+2)		/* Size of buffer for NumericArg() */
#define	TIMESTRLEN	8			/* Size of buffer for TimeString() */
#define	TIME_SIZE	10			/* Maximum number of digits in a decimal time */
#define	ULONG_SIZE	11			/* Maximum bytes in an ASCII ``%lu'' field */
#define	WORKNAMESIZE	14			/* Last component of work file name */

/*
**	Global definitions for C programming.
*/

#define	ARR_SIZE(A)	((sizeof A)/(sizeof A[0]))
#define	BYTE(A)		(((Uchar *)&(A))[0])
#define	english(S)	S			/* Replace with language specific string */
#define	Extern		extern			/* #define to <null> to declare a global object */
#define	NULLFUNCP	(Funcp)0
#define	NULLSTR		(char*)0
#define	NULLVFUNCP	(vFuncp)0
#define	STREQUAL	0			/* Value returned by strcmp and equiv */
#define	SYSERROR	(-1)			/* System call error return */
#define	Talloc(T)	(T*)Malloc(sizeof(T))	/* Typed new memory arena */
#define	Talloc0(T)	(T*)Malloc0(sizeof(T))	/* Zero-filled typed new memory arena */

#define	VC_SHIFT	6			/* Bits to shift for ValidChars[] */
#define	VC_MASK		077			/* Mask for VC_SHIFT */

/*
**	Type definition for variable argument parameters.
*/

#define	MAXVARARGS	512

typedef struct
{
	int	va_count;
	char *	va_args[MAXVARARGS+1];
}
			VarArgs;

#define	ARG(A,N)	(A)->va_args[N]
#define	FIRSTARG(A)	(A)->va_args[(A)->va_count=1,0]
#define	LASTARG(A)	(A)->va_args[(A)->va_count-1]
#define	NARGS(A)	(A)->va_count
#define	NEXTARG(A)	(A)->va_args[(A)->va_count++]

/*
**	System include file control.
*/

#include	<varargs.h>	/* Always include for function prototypes */

#ifdef __bsdi__
# ifndef _BSDI_VERSION
#  include <sys/param.h>	/* try to set this */
# endif
# ifndef _BSDI_VERSION
#  define _BSDI_VERSION 199303
# endif
#endif /* __bsdi__ */

#ifdef	CTYPE
#include	<ctype.h>
#endif	/* CTYPE */

#ifdef	ERRNO
#include	<errno.h>
#endif	/* ERRNO */

#ifdef	NDIR
#ifndef	_TYPES_
#include	<sys/types.h>
#undef	_TYPES_
#define	_TYPES_
#endif	/* _TYPES_ */
#if	SYSV >= 3
#define	NDIRLIB	2
#endif	/* SYSV >= 3 */
#if	NDIRLIB >= 1
#if	NDIRLIB == 2
#include	<dirent.h>
#define	direct	dirent
#else	/* NDIRLIB == 2 */
#include	<ndir.h>
#endif	/* NDIRLIB == 2 */
#ifndef	MAXNAMLEN
#define	MAXNAMLEN	255
#endif	/* MAXNAMLEN */
#else	/* NDIRLIB >= 1*/
#if	BSD4 >= 2
#include	<sys/dir.h>
#else	/* BSD4 >= 2 */
	WHERE IS READDIR(3)?
#endif	/* BSD4 >= 2 */
#endif	/* NDIRLIB >= 1 */
#ifndef	rewinddir
#define rewinddir(dirp)	seekdir((dirp), 0L)
#endif
#endif	/* NDIR */

#ifdef	PASSWD
#include	<pwd.h>
#endif	/* PASSWD */

#ifdef	SETJMP
#include	<setjmp.h>

typedef enum { ert_finish, ert_here, ert_return, ert_exit } ERC_t;

extern jmp_buf		ErrBuf;
extern ERC_t		ErrFlag;

#define	Recover(T)	(ErrFlag=T,((T==ert_here)?setjmp(ErrBuf):0))
#endif	/* SETJMP */

#ifdef	SIGNALS
#include	<signal.h>
#endif	/* SIGNALS */

#ifdef	STDIO
#include	<stdio.h>
#endif	/* STDIO */

#ifdef	SYSLOG
#ifdef	USE_SYSLOG
#include	<syslog.h>
#if	BSD4 >= 3
#define	OpenLog(A,B,C)	openlog(A,B,C)
#else	/* BSD4 >= 3 */
#define	OpenLog(A,B,C)	openlog(A,B)
#endif	/* BSD4 >= 3 */
#else	/* USE_SYSLOG */
#define	OpenLog(A,B,C)
#endif	/* USE_SYSLOG */
#endif	/* SYSLOG */

#ifdef	SYS_STAT
#ifndef	_TYPES_
#include	<sys/types.h>
#undef	_TYPES_
#define	_TYPES_
#endif	/* _TYPES_ */
#include	<sys/stat.h>
#endif	/* SYS_STAT */

#ifdef	SYS_TIME
#ifndef	_TYPES_
#include	<sys/types.h>
#undef	_TYPES_
#define	_TYPES_
#endif	/* _TYPES_ */
#include	<sys/time.h>
#if	BSD4 >= 2
#include	<sys/timeb.h>
#else	/* BSD4 > 2 */
struct timeb
{
	time_t		time;
	unsigned short	millitm;
	short		timezone;
	short		dstflag;
};
#endif	/* BSD4 > 2 */
typedef struct timeb	Timeb;
#endif	/* SYS_TIME */

#ifdef	SYS_WAIT
#ifndef	_TYPES_
#include	<sys/types.h>
#undef	_TYPES_
#define	_TYPES_
#endif	/* _TYPES_ */
#include	<sys/wait.h>
#endif	/* SYS_WAIT */

#ifdef	SYSEXITS
#include	<sysexits.h>

#define	EX_FDERR	(EX__BASE+20)	/* Beyond the others? */
#endif	/* SYSEXITS */

#ifdef	TERMIOCTL
# ifdef	USE_TERMIOS
#  include <termios.h>
#  include <unistd.h>
#  include <sys/ioctl.h>
# else
#  include <sgtty.h>
# endif
#endif	/* TERMIOCTL */

#ifdef	TCP_IP
#ifndef	_TYPES_
#include	<sys/types.h>
#undef	_TYPES_
#define	_TYPES_
#endif	/* _TYPES_ */
#if	SYSV >= 3
#include	<sys/utsname.h>
#endif	/* SYSV >= 3 */
#if	BSD4 >= 2 || BSD_IP == 1
#ifdef	M_XENIX
#include	<sys/types.tcp.h>
#endif	/* M_XENIX */
#include	<sys/socket.h>
#include	<netinet/in.h>
#else	/* BSD4 >= 2 || BSD_IP == 1 */
#include	<socket.h>
#include	<in.h>
#endif	/* BSD4 >= 2 || BSD_IP == 1 */
#include	<netdb.h>
#endif	/* TCP_IP */

#ifdef	TIME
#include	<time.h>
#endif	/* TIME */

#ifdef	LOCKING
#undef	FILE_CONTROL
#define	FILE_CONTROL
#endif

#ifdef	FILE_CONTROL
#ifndef	_TYPES_
#include	<sys/types.h>
#undef	_TYPES_
#define	_TYPES_
#endif	/* _TYPES_ */
#if	FCNTL == 1
#include	<fcntl.h>	/* O_XXX definitions may live here */
#if	BSD4 >= 2
#include	<sys/file.h>	/* O_XXX definitions live here */
#endif	/* BSD4 >= 2 */
#else	/* FCNTL == 1 */
#if	BSD4 > 0
#ifndef	O_RDWR
#include	<sys/file.h>	/* O_XXX definitions live here */
#endif	/* O_RDWR */
#if	BSD4 < 2
#undef	O_EXCL			/* Does the wrong thing */
#endif	/* BSD4 < 2 */
#else	/* BSD4 > 0 */
#define	O_RDONLY	0
#define	O_WRONLY	1
#define	O_RDWR		2
#endif	/* BSD4 > 0 */
#endif	/* FCNTL == 1 */
#endif	/* FILE_CONTROL */

#ifndef MAXPATHLEN
#include <sys/param.h>
#endif /* MAXPATHLEN */
/*
**	All source gets debugging.
*/

extern int	Debugflag;	/* Functional debug level */

#define	Debug(A)	_Debug A
#define	DebugT(L,A)	if(Debugflag>=(L))_Debug A
#ifdef	STDIO
extern FILE *		DebugFd;
#endif	/* STDIO */

extern int	Traceflag;	/* Program/routine tracing */

#define	TRACEONFILE	"TRACEON"
#define	TRACEOFFFILE	"TRACEOFF"

#ifdef	EBUG
#define	DEBUG	1
#endif

#if	DEBUG
#define	Trace(A)	_Trace A
#define	TraceT(L,A)	if(Traceflag>=(L))_Trace A
#define	DODEBUG(A)	A
#define	free(A)		Free(A)
#ifdef	STDIO
extern FILE *		TraceFd;
#endif	/* STDIO */
#ifndef	_Trace
#ifdef	NO_VA_FUNC_DECLS
extern void		_Trace();
#else	/* NO_VA_FUNC_DECLS */
extern void		_Trace(int, char *, ...);
#endif	/* NO_VA_FUNC_DECLS */
#endif	/* !_Trace */
#else	/* DEBUG */
#define	Trace(A)
#define	TraceT(L,A)
#define	DODEBUG(A)
#endif	/* DEBUG */

/*
**	Include type, function and string definitions here.
*/

#include	"typedefs.h"
#include	"functions.h"
#include	"strings.h"

/*
**	Local include file control.
*/

#ifdef	ARGS
#include	"args.h"
#endif	/* ARGS */

#ifdef	EXECUTE
#include	"exec.h"
#endif	/* EXECUTE */

#ifdef	FILES
#include	"files.h"
#endif	/* FILES */

#ifdef	PARAMS
#include	"params.h"
#endif	/* PARAMS */
