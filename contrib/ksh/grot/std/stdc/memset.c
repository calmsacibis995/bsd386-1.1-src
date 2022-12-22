/* $Id: memset.c,v 1.2 1993/12/21 02:27:24 polk Exp $ */

#include <string.h>

void *
memset(ap, c, n)
	void *ap;
	register int c;
	register size_t n;
{
	register char *p = ap;

	if (n++ > 0)
		while (--n > 0)
			*p++ = c;
	return ap;
}

