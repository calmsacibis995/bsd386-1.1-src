/*
 * Emulation of misc. ANSI C stdio functions
 */

/* $Id: stdio.c,v 1.2 1993/12/21 02:30:19 polk Exp $ */

#include <stdio.h>

#ifndef __STDC__
#define const
#define volatile
#endif

#if 1
int
remove(name)
	const char *name;
{
	return unlink(name);
}
#endif

#if _V7
int
rename(oname, nname)
	const char *oname, *nname;
{
	return link(oname, nname) == 0 && unlink(oname) == 0 ? 0 : -1;
}
#endif

