#include <string.h>
/* $Id: strcpy.c,v 1.2 1993/12/21 02:33:25 polk Exp $ */

/*
 * strcpy - copy string src to dst
 */
char *				/* dst */
strcpy(dst, src)
char *dst;
const char *src;
{
	register char *dscan;
	register const char *sscan;

	dscan = dst;
	sscan = src;
	while ((*dscan++ = *sscan++) != '\0')
		continue;
	return(dst);
}
