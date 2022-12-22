/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: gcvt.c,v 1.1 1993/10/22 16:57:22 donn Exp $
 */

#include <stdio.h>

char *
gcvt(value, ndigit, buf)
	double value;
	int ndigit;
	char *buf;
{

	sprintf(buf, "%.*g", ndigit, value);
	return (buf);
}
