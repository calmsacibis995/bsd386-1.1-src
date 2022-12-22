/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
 
/*	BSDI errlib.c,v 1.1 1992/08/24 21:18:11 sanders Exp	*/

#ifndef lint
static char sccsid[] = "@(#)errlib.c        1.1 (BSDI) 08/24/92";
#endif

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include "errlib.h"

void
Perror(char *fmt, ...)
{
	int errcode = errno;
        va_list ap;

        va_start(ap, fmt);
        (void)fprintf(stderr, "%s: ", PROGNAME);
        (void)vfprintf(stderr, fmt, ap);
        va_end(ap);
	if (errcode != 0)
		(void)fprintf(stderr, ": %s", strerror(errcode));
        (void)fprintf(stderr, "\n");
        exit(1);
}

void
Pwarn(char *fmt, ...)
{
	int errcode = errno;
        va_list ap;

        va_start(ap, fmt);
	(void)fprintf(stderr, "%s: ", PROGNAME);
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
	if (errcode != 0)
		(void)fprintf(stderr, ": %s", strerror(errcode));
        (void)fprintf(stderr, "\n");
}

void
Perrmsg(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
}

void
__Pmsg(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	(void)vfprintf(stdout, fmt, ap);
	va_end(ap);
}
