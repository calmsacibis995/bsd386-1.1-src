/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: uname.c,v 1.2 1993/03/08 06:46:04 polk Exp $
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/kinfo.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
uname(u)
	struct utsname *u;
{
	struct sysinfo *s;
	int len;

	/* try not to segfault on bad buffer addresses */
	if (gethostname(u->nodename, sizeof u->nodename) == -1)
		return (-1);
	if ((len = getkerninfo(KINFO_SYSINFO, NULL, NULL, 0)) == -1)
		return (-1);
	if ((s = malloc(len)) == NULL)
		return (-1);
	if (getkerninfo(KINFO_SYSINFO, (char *)s, &len, 0) == -1)
		return (-1);

#define	s2u(sm, um) \
	if (s->sm) \
		strncpy(u->um, (char *)s + (int)s->sm, sizeof u->um); \
	else \
		u->um[0] = '\0'

	s2u(sys_ostype, sysname);
	s2u(sys_osrelease, release);
	s2u(sys_machine, machine);

	snprintf(u->version, sizeof u->version, "%d", s->sys_posix1_version);

	free(s);

	return (0);
}
