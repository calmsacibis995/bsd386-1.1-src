/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI $Id: iso9660_util.c,v 1.1 1992/09/23 23:48:19 karels Exp $
 */

#include "param.h"

/*
 * These routines decode numbers that are stored in various formats
 * on the disk.  The last part of the function name (e.g. 733) tells
 * the chapter and section number of the ISO-9660 spec where that
 * format is defined.
 *
 * Note that there are no alignment guarantees - a 32 bit number 
 * could start at any address.
 */

/* unsigned 8 bit */
int
iso9660num_711(p)
	u_char *p;
{

	return (*p & 0xff);
}

/* signed 8 bit */
int
iso9660num_712(p)
	u_char *p;
{
	int val;

	val = *p;
	if (val & 0x80)
		val |= (-1 << 8);
	return (val);
}

/* unsigned 16 bit, little endian */
int
iso9660num_721(p)
	u_char *p;
{
	return ((p[0] & 0xff) | ((p[1] & 0xff) << 8));
}

/* unsigned 16 bit, big endian */
int
iso9660num_722(p)
	u_char *p;
{

	return (((p[0] & 0xff) << 8) | (p[1] & 0xff));
}

/* unsigned 16 bit, little endian, followed by big endian */
int
iso9660num_723(p)
	u_char *p;
{

	return ((p[0] & 0xff) | ((p[1] & 0xff) << 8));
}

/* signed 32 bit little endian */
int
iso9660num_731(p)
	u_char *p;
{

	return ((p[0] & 0xff) | ((p[1] & 0xff) << 8) |
	    ((p[2] & 0xff) << 16) | ((p[3] & 0xff) << 24));
}

/* signed 32 bit big endian */
int
iso9660num_732(p)
	u_char *p;
{

	return (((p[0] & 0xff) << 24) | ((p[1] & 0xff) << 16) |
	    ((p[2] & 0xff) << 8) | (p[3] & 0xff));
}

/* signed 32 bit little endian, followed by big endian */
int
iso9660num_733 (p)
u_char *p;
{
	return ((p[0] & 0xff)
		| ((p[1] & 0xff) << 8)
		| ((p[2] & 0xff) << 16)
		| ((p[3] & 0xff) << 24));
}

		
	
