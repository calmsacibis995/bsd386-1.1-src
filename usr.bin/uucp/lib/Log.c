/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Log actions.
**
**	RCSID $Id: Log.c,v 1.1.1.1 1992/09/28 20:08:41 trent Exp $
**
**	$Log: Log.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:41  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	FILE_CONTROL
#define	NO_VA_FUNC_DECLS
#define	STDIO

#include	"global.h"

char *		LogNode;	/* Global */
bool		NoLog;		/* Global */

static FILE *	Efd;
static FILE *	Lfd;
static FILE *	Sfd;
static bool	InLog;
static char *	Syserrs;
static int	Syserrt;

static FILE *	LogOpen(char *);
static void	LogS(FILE *);
static void	LogVS(FILE *, va_list);
static void	LogVF(FILE *, va_list);

#define	Flush	(void)fflush
#define	Fprintf	(void)fprintf



/*
**	Standard Log.
*/

void
Log(va_alist)
	va_dcl
{
	va_list	vp;

	if ( InLog )
		return;

	if ( Lfd == (FILE *)0 )
		Lfd = LogOpen(Name);

	LogS(Lfd);
	va_start(vp);
	LogVF(Lfd, vp);
	va_end(vp);

	Fprintf(Lfd, "\n");
	Flush(Lfd);

	InLog = false;
}

/*
**	Close node-specific log file (if node changes).
*/

void
LogClose()
{
	if ( Lfd == (FILE *)0 )
		return;

	Debug((1, "close log file %d", fileno(Lfd)));

	(void)fclose(Lfd);
	Lfd = (FILE *)0;
}

/*
**	Return fd used by Log().
*/

int
LogFd()
{
	if ( Lfd == (FILE *)0 )
		Lfd = LogOpen(Name);

	return fileno(Lfd);
}

/*
**	Error log interfaces for Error/SysError/SysWarn/Warn etc.
*/

void
ErrorLog(va_alist)
	va_dcl
{
	va_list	vp;
#	ifdef	USE_SYSLOG
	char *	errs;

	va_start(vp);
	errs = newvprintf(vp);
	va_end(vp);
	Debug((1, errs));
	syslog(LOG_WARNING, errs);
	free(errs);

#	else	/* USE_SYSLOG */

	if ( InLog )
		return;

	if ( Efd == NULL )
		Efd = LogOpen(NULLSTR);

	LogS(Efd);
	va_start(vp);
	LogVF(Efd, vp);
	va_end(vp);

	Fprintf(Efd, "\n");
	Flush(Efd);

	InLog = false;

#	endif	/* USE_SYSLOG */
}

/*
**	Must be called *after* `ErrorLogV'.
*/

void
ErrorLogN(va_alist)
	va_dcl
{
	va_list	vp;
#	ifdef	USE_SYSLOG
	char *	errs;

	if ( !InLog )
		abort();

	va_start(vp);
	errs = newvprintf(vp);
	va_end(vp);

	Debug((1, "%s%s", Syserrs, errs));
	syslog(Syserrt, "%s%s", Syserrs, errs);
	free(errs);
	free(Syserrs);

#	else	/* USE_SYSLOG */

	if ( !InLog || Efd == (FILE *)0 )
		abort();

	va_start(vp);
	LogVF(Efd, vp);
	va_end(vp);

	Fprintf(Efd, "\n");
	Flush(Efd);

#	endif	/* USE_SYSLOG */

	InLog = false;
}

/*
**	First part of 2-part ErrorLog.
*/

bool
ErrorLogV(err, vp)
	char *	err;
	va_list	vp;
{
	if ( InLog || NoLog )
		return false;

#	ifdef	USE_SYSLOG

	InLog = true;
	Syserrs = newvprintf(vp);
	Syserrt = (strstr(err, "error") == NULLSTR) ? LOG_WARNING
			: (strstr(err, "INTERNAL") == NULLSTR) ? LOG_ERR
			: LOG_ALERT ;

#	else	/* USE_SYSLOG */

	if ( Efd == NULL )
		Efd = LogOpen(NULLSTR);

	LogS(Efd);
/*	Fprintf(Efd, "%s -- ", err);
*/	LogVF(Efd, vp);

#	endif	/* USE_SYSLOG */

	return true;
}

/*
**	Open file for log.
*/

static FILE *
LogOpen(name)
	char *	name;
{
	int	fd;
	char *	file;

	InLog = true;

	for ( ;; )
	{
		if ( name == NULLSTR || LogNode == NULLSTR )
		{
			name = NULLSTR;
			file = concat(LOGDIR, "errors", NULLSTR);
		}
		else
			file = concat(LOGDIR, name, Slash, LogNode, NULLSTR);

		for ( ;; )
		{
			if ( (fd = open(file, O_WRONLY|O_CREAT|O_APPEND, 0664)) != SYSERROR )
			{
				(void)chmod(file, LOG_MODE);
				free(file);
				Debug((1, "open log file %d <= %s", fd, file));
				return fdopen(fd, "a");
			}

			if ( !CheckDirs(file) )
				break;
		}

		(void)SysWarn(CouldNot, OpenStr, file);

		free(file);

		if ( name == NULLSTR )
			return stderr;

		name = NULLSTR;
	}
}

/*
**	Start log output.
*/

static void
LogS(fd)
	FILE *	fd;
{
	char *	cp;
	char *	user;
	char *	node;
	char	tbuf[ISODATETIMESTRLEN];

	InLog = true;

#	ifndef	F_SETFL
	(void)fseek(Lfd, (long)0, 2);
#	endif	/* !F_SETFL */

	(void)SetTimes();

	if ( ORIG_UUCP )
		(void)UUCPDateTimeStr(Time, tbuf);
	else
		(void)ISODateTimeStr(Time, Timeusec, tbuf);

	if ( (user = UserName) == NULLSTR )
		user = Invoker;
	if ( (node = LogNode) == NULLSTR )
		node = NODENAME;

	Fprintf(fd, "%s %s (%s-%d) ", user, node, tbuf, Pid);
}

#if	0
/*
**	Print spaces between strings.
*/

static void
LogVS(fd, vp)
	FILE *	fd;
	va_list	vp;
{
	char *	cp;

	if ( (cp = va_arg(vp, char *)) != NULLSTR )
	{
		Fprintf(fd, " %s", cp);

		if ( (cp = va_arg(vp, char *)) != NULLSTR )
			Fprintf(fd, " %s", cp);
	}
}
#endif	/* 0 */

/*
**	Pass format and args to `vfprintf'.
*/

static void
LogVF(fd, vp)
	FILE *	fd;
	va_list	vp;
{
	char *	fmt;
#	if	NO_VFPRINTF
	int	i;
	char *	a[10];
#	endif	/* NO_VFPRINTF */

	if ( Debugflag && fd != Sfd )
	{
		char *	str = newvprintf(vp);

		Debug((1, "log %d: %s", fileno(fd), str));
		free(str);
	}

	if ( (fmt = va_arg(vp, char *)) == NULLSTR )
		return;

#	if	NO_VFPRINTF

	for ( i = 0 ; i < 10 ; i++ )
		a[i] = va_arg(vp, char *);

	(void)fprintf(fd, fmt, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9]);

#	else	/* NO_VFPRINTF */

	(void)vfprintf(fd, fmt, vp);

#	endif	/* NO_VFPRINTF */
}

/*
**	Statistics Log.
*/

void
LogStats(va_alist)
	va_dcl
{
	va_list	vp;

	if ( InLog )
		return;

	if ( Sfd == (FILE *)0 )
		Sfd = LogOpen("xferstats");

	LogS(Sfd);
	if ( ORIG_UUCP )
		Fprintf(Sfd, "(%lu.%02u) ", Time, Timeusec/10000);
	va_start(vp);
	LogVF(Sfd, vp);
	va_end(vp);

	Fprintf(Sfd, "\n");
	Flush(Sfd);

	InLog = false;
}
