/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
 
/*	BSDI $Id: z.c,v 1.2 1993/02/23 18:20:06 polk Exp $	*/

/*
 * z checks assertions and prints appropriate messages
 *  before aborting.
 */

#include	<varargs.h>
#include 	"installsw.h"

extern int curseson;

z(va_alist) va_dcl
{
	va_list pvar;		/* var args traverser */
	int checkz;		/* whether we should quit or not */
	char *format;		/* printf message */
	int i;

	va_start(pvar);
	checkz = va_arg(pvar, int);

	if (!checkz)
		return;

	if (curseson)
		cleanup();

	format = va_arg(pvar, char *);
	(void) fprintf(stdout, "%s Fatal Error:  ", progname);
	(void) vfprintf(stdout, format, pvar);
	(void) fprintf(stdout, "\n");
	va_end(pvar);

	(void) printf("\n");
	exit(1);
}
