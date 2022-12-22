/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: doprnt.c,v 1.1 1993/10/22 16:55:37 donn Exp $
 */

#include <stdarg.h>
#include <stdio.h>

/*
 * Based on the (unfortunate) description of the interface
 * in the 4.3 BSD UPM, on the printf(3S) manual page.
 */

int
_doprnt(char *format, va_list ap, FILE *stream)
{

	return (vfprintf(stream, format, ap));
}
