#include <string.h>
/* $Id: strspn.c,v 1.2 1993/12/21 02:38:05 polk Exp $ */

/*
 * strspn - find length of initial segment of s consisting entirely
 * of characters from accept
 */

size_t
strspn(s, accept)
const char *s;
const char *accept;
{
	register const char *sscan;
	register const char *ascan;
	register size_t count;

	count = 0;
	for (sscan = s; *sscan != '\0'; sscan++) {
		for (ascan = accept; *ascan != '\0'; ascan++)
			if (*sscan == *ascan)
				break;
		if (*ascan == '\0')
			return(count);
		count++;
	}
	return(count);
}
