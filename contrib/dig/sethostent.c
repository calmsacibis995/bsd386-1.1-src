
/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
** Distributed with 'dig' version 2.0 from University of Southern
** California Information Sciences Institute (USC-ISI). 9/1/90
*/

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)sethostent.c	6.3 (Berkeley) 4/10/86";
#endif LIBC_SCCS and not lint

#include "hfiles.h"

#include <sys/types.h>
#include NAMESERH
#ifndef T_TXT
#define T_TXT 16
#endif T_TXT
#include <netinet/in.h>
#include RESOLVH

sethostent(stayopen)
{
	if (stayopen)
		_res.options |= RES_STAYOPEN | RES_USEVC;
}

endhostent()
{
	_res.options &= ~(RES_STAYOPEN | RES_USEVC);
	_res_close();
}

sethostfile(name)
char *name;
{
#ifdef lint
name = name;
#endif
}
