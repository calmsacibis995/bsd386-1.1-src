#include <string.h>
/* $Id: strchr.c,v 1.2 1993/12/21 02:32:24 polk Exp $ */

/*
 * strchr - find first occurrence of a character in a string
 */

char *				/* found char, or NULL if none */
strchr(s, charwanted)
const char *s;
register char charwanted;
{
	register const char *scan;

	/*
	 * The odd placement of the two tests is so NUL is findable.
	 */
	for (scan = s; *scan != charwanted;)	/* ++ moved down for opt. */
		if (*scan++ == '\0')
			return(NULL);
	return(scan);
}
