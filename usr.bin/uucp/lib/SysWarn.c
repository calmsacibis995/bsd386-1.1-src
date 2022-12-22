/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	System error/warning on ErrorFd.
**
**	SysWarn returns true if should try again.
**
**	RCSID $Id: SysWarn.c,v 1.2 1993/02/28 15:28:48 pace Exp $
**
**	$Log: SysWarn.c,v $
 * Revision 1.2  1993/02/28  15:28:48  pace
 * Add recent uunet changes
 *
 * Revision 1.1.1.1  1992/09/28  20:08:37  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	ERRNO
#define	NO_VA_FUNC_DECLS
#define	SETJMP
#define	STDIO
#define	SYSEXITS

#include	"global.h"

#define		MAXZERO		3
static int	ZeroErrnoCount;	/* Count incidences of errno==0 */
static char	SysErrStr[]	= english("system error");
static char	SysWrnStr[]	= english("system warning");

#define	Fprintf	(void)fprintf
#define	Fflush	(void)fflush

extern char *	sys_errlist[];
extern int	sys_nerr;



/*
**	Return string representing <errno>.
*/

char *
ErrnoStr(en)
	int		en;
{
	static char	errs[6+10+1];

	if ( en < sys_nerr && en > 0 )
		return sys_errlist[en];

	(void)sprintf(errs, english("Error %d"), en);
	return errs;
}



/*
**	Write error record to ErrorFd, and ErrorLog.
**
**	Return true if recoverable resource error, else false.
*/

static bool
sys_common(type, en, vp)
	char *		type;
	int		en;
	va_list		vp;
{
	bool		val;
	static char	errs[]	= ": %s";

	if ( (val = SysRetry(en)) == true && en == 0 )
		return true;

	MesgV(type, vp);

	Fprintf(ErrorFd, errs, ErrnoStr(en));
	(void)fputc('\n', ErrorFd);
	Fflush(ErrorFd);

#	ifdef	USE_SYSLOG
	if ( en == ENOMEM )
		syslog(LOG_ERR, "No memory");
	else
#	endif	/* USE_SYSLOG */
	if ( ErrorLogV(type, vp) )
		ErrorLogN(errs, ErrnoStr(en));

	return val;
}



/*
**	Report system error, terminate if non-recoverable, else return.
*/

void
SysError(va_alist)
	va_dcl
{
	va_list		vp;
	int		en;

	if ( (en = errno) == EINTR )
		return;

	va_start(vp);

	FreeStr(&ErrString);
	if ( en != ENOMEM )
		ErrString = newvprintf(vp);

	if ( sys_common(SysErrStr, en, vp) )
	{
		va_end(vp);
		errno = en;
		return;
	}

	va_end(vp);

	if ( ErrFlag != ert_exit )
		finish(EX_OSERR);

	exit(EX_OSERR);
}



/*
**	Test if error is retry-able, and if so, sleep, return true.
*/

bool
SysRetry(
	int	en
)
{
	switch ( errno = en )
	{
	default:
		ZeroErrnoCount = 0;
		return false;

	case 0:
		if ( ++ZeroErrnoCount > MAXZERO )
			return false;
		(void)sleep(10);
		break;

	case EINTR:
		ZeroErrnoCount = 0;
		return true;

	case ENFILE:
	case ENOSPC:
	case EAGAIN:

#	ifdef	EALLOCFAIL
	case EALLOCFAIL:
#	endif	/* EALLOCFAIL */

#	ifdef	ENOBUFS
	case ENOBUFS:
#	endif	/* ENOBUFS */

		ZeroErrnoCount = 0;
		(void)sleep(20);
		break;
	}

	errno = en;
	return true;
}



/*
**	Report system error, return true if recoverable, else false.
*/

bool
SysWarn(va_alist)
	va_dcl
{
	va_list		vp;
	int		en;
	bool		val;

	if ( (en = errno) == EINTR )
		return true;

	va_start(vp);

	val = sys_common(SysWrnStr, en, vp);

	va_end(vp);

	errno = en;

	return val;
}
