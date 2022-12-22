#include <string.h>
/* $Id: strlen.c,v 1.2 1993/12/21 02:35:08 polk Exp $ */

/*
 * strlen - length of string (not including NUL)
 */
size_t
strlen(s)
const char *s;
{
	register const char *scan;
	register size_t count;

	count = 0;
	scan = s;
	while (*scan++ != '\0')
		count++;
	return(count);
}
