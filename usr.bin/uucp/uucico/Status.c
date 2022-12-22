/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Routines to manipulate call status files.
**
**	RCSID $Id: Status.c,v 1.2 1994/01/31 01:26:42 donn Exp $
**
**	$Log: Status.c,v $
 * Revision 1.2  1994/01/31  01:26:42  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:58:30  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:42:44  vixie
 * Initial revision
 *
 * Revision 1.1.1.1  1992/09/28  20:08:54  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	ERRNO
#define	FILES
#define	FILE_CONTROL
#define	NO_VA_FUNC_DECLS
#define	SETJMP
#define	SYS_TIME

#include	"global.h"
#include	"cico.h"

/*
**	Files each have 2 lines:
**		node "in"  line oktime nowtime retrytime state count text...
**		node "out" line oktime nowtime retrytime state count text...
*/

static char	WriteFmt[]	= "%s %s %s %lu %lu %lu %d %d %s";
static char	ReadFmt[]	= "%*s %*s %*s %lu %*lu %lu %d %d %*s";
static char	NL[]		= "\n";

/*
**	Local variables.
*/

static char *	StatusFile;
static int	StatusFd	= SYSERROR;
static bool	StatusMode;

/*
**	Local routines.
*/

static void	close_status(void);
static bool	open_status(char *);


/*
**	Find out if call can be made.
*/

StatusType
CallOK(
	char *	node
)
{
	int	count;
	char *	indata;
	Time_t	lasttime;
	int	mode;
	int	oldmode;
	char *	outdata;
	Time_t	retrytime;

	Trace((1, "CallOK(%s)", node));

	mode = (int)SS_OK;

	if ( !open_status(node) )
		return SS_INPROGRESS;	/* LOCKED */

	while ( (indata = ReadFd(StatusFd)) != NULLSTR )
	{
		if
		(
			(outdata = strchr(indata, '\n')) == NULLSTR
			||
			strchr(++outdata, '\n') != NULLSTR
		)
		{
			DebugT(1, (1,
				"CallOK bad status format:\n%s",
				ExpandString(indata, RdFileSize)
			));
			(void)unlink(StatusFile);
			free(indata);
			break;
		}

		Trace((3, "scan line: %s", outdata));

		(void)sscanf(outdata, ReadFmt,
			     &lasttime, &retrytime, &oldmode, &count);

		switch ( (StatusType)oldmode )
		{
		default:
			break;

		case SS_WRONGTIME:
		case SS_FAIL:
			(void)SetTimes();
			if ( (Time - lasttime) >= retrytime )
				break;

			Debug((1, "NO CALL: RETRY TIME (%lu) NOT REACHED",
			       retrytime));

			if ( Debugflag )
				Debug((1, "debugging: continuing anyway"));
			else
				mode = (int)SS_WRONGTIME;
			break;
		}

		free(indata);
		break;
	}

	if ( mode != (int)SS_OK )
		close_status();

	Debug((2, "CallOK => %d", mode));
	return (StatusType)mode;
}

/*
**	Close status file.
*/

static void
close_status()
{
	Trace((1,
		"close_status() fd=%d [%s]",
		StatusFd,
		(StatusFile==NULLSTR) ? NullStr : StatusFile
	));

	FreeStr(&StatusFile);

	if ( StatusFd == SYSERROR )
		return;

	(void)close(StatusFd);
	StatusFd = SYSERROR;
}

/*
**	Open status file, and lock.
*/

static bool
open_status(
	char *	name
)
{
	Trace((3, "open_status(%s)", (name==NULLSTR) ? NullStr : name));

	if
	(
		name == NULLSTR
		||
		name[0] == '\0'
		||
		strcmp(name, NODENAME) == STREQUAL
	)
		return false;

	if ( StatusFd != SYSERROR )
	{
		(void)lseek(StatusFd, (off_t)0, 0);
		return true;
	}

	StatusFile = concat(STATUSDIR, name, NULLSTR);

	while ( (StatusFd = open(StatusFile, O_CREAT|O_RDWR, 0664)) == SYSERROR )
		if ( !CheckDirs(StatusFile) )
			SysError(CouldNot, CreateStr, StatusFile);

	Trace((2, "open_status(%s) => %s", name, StatusFile));

	while ( flock(StatusFd, LOCK_EX|LOCK_NB) == SYSERROR )
	{
		if ( errno == EWOULDBLOCK )
		{
			close_status();
			return false;
		}

		SysError(CouldNot, LockStr, StatusFile);
	}

	fioclex(StatusFd);

	return true;
}

/*
**	Write status entry.
**
**	Status(StatusType, char *node, char *fmt, ...)
*/

bool
Status(va_alist)
	va_dcl
{
	register va_list vp;

	char *		buf;
	int		count;
	char *		data;
	char *		indata;
	int		mode;
	char *		node;
	char *		outdata;
	Time_t		retrytime;
	char *		str;
	char *		ttyn;
	StatusType	type;

	static Time_t	lasttime;

	va_start(vp);

	if ( (type = va_arg(vp, StatusType)) == SS_CLOSE )
	{
		close_status();
		va_end(vp);
		return true;
	}

	node = va_arg(vp, char *);
	str = newvprintf(vp);
	va_end(vp);

	Trace((1, "Status(%s, %s, %d)", node, str, type));

	if ( !open_status(node) )
	{
		free(str);
		return false;	/* LOCKED */
	}

	outdata = indata = EmptyStr;
	count = 0;
	mode = SS_OK;

	while ( (data = ReadFd(StatusFd)) != NULLSTR )
	{
		if ( (outdata = strchr(data, '\n')) == NULLSTR )
		{
			outdata = EmptyStr;
			break;	/* Bad format */
		}

		*outdata++ = '\0';
		indata = data;

		if ( InitialRole == MASTER )
			buf = outdata;
		else
			buf = indata;

		Trace((3, "scan line: %s", buf));

		(void)sscanf(buf, ReadFmt, &lasttime, &retrytime, &mode, &count);

		if ( type == SS_WRONGTIME && mode != (int)SS_INPROGRESS )
		{
			free(data);
			free(str);
			return true;	/* Preserve old mode */
		}

		if ( count < 0 )
			count = 0;
		break;
	}

	if ( type != SS_CALLING )
	{
		if ( type == SS_OK )
			retrytime = SysRetryTime;

		/** Exponential backoff **/

		if ( type == SS_FAIL &&
		     ++count > 5 &&
		     (retrytime *= count - 5) > DAY/2 )
			retrytime = DAY/2;

		if ( (ttyn = TtyName) == NULLSTR || ttyn[0] == '\0' )
			ttyn = DFLTTTYNAME;

		(void)SetTimes();

		if ( type == SS_OK )
		{
			lasttime = Time;
			count = 0;
		}

		buf = newprintf
			(
				WriteFmt,
				RemoteNode,
				(InitialRole == MASTER) ? "out" : "in",
				ttyn,
				(Ulong)lasttime,
				(Ulong)Time,
				(Ulong)retrytime,
				(int)type,
				(int)count,
				str
			);
	
		if ( InitialRole == MASTER )
			outdata = buf;
		else
			indata = buf;

		Debug((1, "status => %s", buf));
	}
	else
		buf = NULLSTR;

	outdata = concat(indata, NL, outdata, NL, NULLSTR);

	(void)lseek(StatusFd, (off_t)0, 0);

	/** NB: if no `ftruncate(2)', then we must create/rename a temporary file. **/

	while ( ftruncate(StatusFd, (off_t)0) == SYSERROR )
		SysError(CouldNot, "truncate", StatusFile);

	while ( write(StatusFd, outdata, strlen(outdata)) == SYSERROR )
	{
		SysError(CouldNot, WriteStr, StatusFile);
		(void)lseek(StatusFd, (off_t)0, 0);
	}

	free(outdata);
	FreeStr(&buf);
	FreeStr(&data);
	free(str);

	return true;
}
