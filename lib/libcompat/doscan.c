/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: doscan.c,v 1.1 1993/10/22 16:56:11 donn Exp $
 */

#include <stdarg.h>
#include <stdio.h>

/*
 * Modelled after _doprnt(), documented in the 4.3 BSD UPM.
 * Hope it works right.
 */

int
_doscan(char *format, va_list ap, FILE *stream)
{

	return (vfscanf(stream, format, ap));
}
