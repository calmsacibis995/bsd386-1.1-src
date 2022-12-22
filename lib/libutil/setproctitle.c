/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: setproctitle.c,v 1.1 1993/10/22 17:29:56 donn Exp $
 */

#include <sys/param.h>
#include <sys/exec.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

/*
 * Set the process status strings for the current process.
 * These default to the current argument strings.
 */
void
#ifdef __STDC__
setproctitle(const char *fmt, ...)
#else
setproctitle(va_alist)
	va_dcl
#endif
{
	va_list ap;
	struct ps_strings *psp = (struct ps_strings *)PS_STRINGS;
	int n;
	char c = '\0';
	char *buf = &c;
	static char *xargv[2];
#ifndef __STDC__
	char *fmt;

	va_start(ap);
	fmt = va_arg(ap, char *);
#else
	va_start(ap, fmt);
#endif

	n = vsnprintf(buf, 1, fmt, ap);
	va_end(ap);
	if ((buf = malloc(++n)) == 0)
		return;

	/* ANSI C says you have to re-start ap after calling a v*() function */
#ifndef __STDC__
	va_start(ap);
	fmt = va_arg(ap, char *);
#else
	va_start(ap, fmt);
#endif

	vsnprintf(buf, n, fmt, ap);
	va_end(ap);

	if (xargv[0])
		free(xargv[0]);
	xargv[0] = buf;

#ifdef OLDWAY
	psp->ps_nargvstr = 1;
	psp->ps_argvstr = buf;
#else
	psp->ps_argc = 1;
	psp->ps_argv = xargv;
#endif
}
