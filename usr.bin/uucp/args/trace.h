/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Conditional tracing
**
**	RCSID $Id: trace.h,v 1.1.1.1 1992/09/28 20:08:32 trent Exp $
**
**	$Log: trace.h,v $
 * Revision 1.1.1.1  1992/09/28  20:08:32  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:48:05  piers
 * Initial revision
 *
*/

#if	TRACE

#ifndef	BUFSIZ
#include	<stdio.h>
#endif
#ifndef	_VARARGS_
#include	<varargs.h>
#endif

extern int	Traceflag;

static void
_trace(va_alist)
	va_dcl
{
	va_list	p;
	char *	f;
	int	l;
	va_start(p);
	l = va_arg(p, int);
	if (Traceflag >= l) {
		f = va_arg(p, char *);
		(void)vfprintf(stderr,f,p);
		(void)putc('\n', stderr);
		(void)fflush(stderr);
	}
	va_end(p);
}

#define	Trace(A)	_trace A

#else	TRACE

#define	Trace(A)

#endif	TRACE
