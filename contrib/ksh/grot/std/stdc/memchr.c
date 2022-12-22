/* $Id: memchr.c,v 1.2 1993/12/21 02:25:16 polk Exp $ */

#include <string.h>

void *
memchr(ap, c, n)
	const void *ap;
	register int c;
	register size_t n;
{
	register char *p = ap;

	if (n++ > 0)
		while (--n > 0)
			if (*p++ == c)
				return --p;
	return NULL;
}

