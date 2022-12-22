#include <string.h>
/* $Id: strpbrk.c,v 1.2 1993/12/21 02:37:01 polk Exp $ */

/*
 * strpbrk - find first occurrence of any char from breakat in s
 */

char *				/* found char, or NULL if none */
strpbrk(s, breakat)
const char *s;
const char *breakat;
{
	register const char *sscan;
	register const char *bscan;

	for (sscan = s; *sscan != '\0'; sscan++) {
		for (bscan = breakat; *bscan != '\0';)	/* ++ moved down. */
			if (*sscan == *bscan++)
				return(sscan);
	}
	return(NULL);
}
