/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

/*	BSDI utils.c,v 1.1 1992/08/24 21:18:11 sanders Exp */

#ifndef lint
static char sccsid[] = "@(#)utils.c	1.1 (BSDI) 08/24/92";
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <pwd.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>

#include "pathnames.h"
#include "at.h"
#include "errlib.h"

/*
 * Cache a small number of usernames by uid.
 * Most likely we are going to be looking up the same one over and over
 * For a hash function we simply take (uid % CACHESIZE)
 *
 * You can override the default CACHESIZE in the Makefile
 */

#ifndef CACHESIZE
#define CACHESIZE 41
#endif

static	struct	usernamecache {
	uid_t	uid;
	char	*un;
}	usernamecache[CACHESIZE];

char *
username(uid_t uid)
{
	register struct usernamecache *p = usernamecache + (uid % CACHESIZE);
	struct	passwd *pw;

	if (p->un && p->uid == uid)
		return p->un;		/* we have a match */

	if (p->un) free(p->un);		/* free the old name */

	if ((pw = getpwuid(uid)) == NULL)
		Perror("getpwuid(%d)", uid);

	p->uid = uid;
	if ((p->un = strdup(pw->pw_name)) == NULL)
		Perror("strdup username");
	return p->un;
}

char *
qfpath(JobQ p)
{
	static char buf[MAXPATHLEN];
	sprintf(buf, "%s%d.%d", AT_DIR, p->id, p->when);
	return buf;
}

char *
xfpath(JobQ p)
{
	static char buf[MAXPATHLEN];
	sprintf(buf, "%s%s/%d.%d", AT_DIR, AT_LOCKDIR, p->id, p->when);
	return buf;
}

char *
outputpath(JobQ p)
{
	static char buf[MAXPATHLEN];
	sprintf(buf, "%s%s/%d.output", AT_DIR, AT_LOCKDIR, p->id);
	return buf;
}

char *
escstr(char *s, char match, char *repl)
{
	static char buf[ARG_MAX];
	int rlen = strlen(repl);
	char *end = buf + ARG_MAX - 1;
	char *p = buf;

	while (*s && end >= p)
		if (*s == match)
			strncpy(p, repl, end-p), p += rlen, s++;
		else
			*p++ = *s++;
	*p++ = '\0';
	if (p > end)
		Perror("environment string %s too long", s);
	return buf;
}

char *
shellesc(char *s)
{
	return escstr(s, '\'', "'\\''");
}

void
swrite(int fd, char *fmt, ...)
{
	char buf[ARG_MAX];
	va_list ap;

	va_start(ap, fmt);
	if (vsnprintf(buf, ARG_MAX, fmt, ap) >= ARG_MAX-1)
		Perror("string %s too long", buf);
	if (write(fd, buf, strlen(buf)) <= 0)
		Perror("write in swrite failed on fd=%d", fd);
	va_end(ap);
}

#ifdef NEED_USLEEP
void
usleep(long usec)
{
	struct	timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = usec;
	select(0, NULL, NULL, NULL, &tv);
}
#endif

/*
 * Lcck the at directory and get the next job number
 */

static int
atlock()
{
	char	lockfile[MAXPATHLEN];
	int	lockid;
	mode_t	mode = O_RDWR|O_EXLOCK;

	sprintf(lockfile, "%s%s", AT_DIR, AT_LOCK);
	while(1) {
		errno = 0;
		lockid = open(lockfile, mode, 0600);
		if (lockid >= 0) break;
		if (errno == ENOENT) {
			mode |= O_CREAT;
			errno = ENOLCK;
		}
		if (errno != ENOLCK)
			Perror("lockfile %s", lockfile);
		usleep(50000);
	}

	/* create and init */
	if (mode & O_CREAT) {
		swrite(lockid, "1\n");
		lseek(lockid, 0, SEEK_SET);
	}
	return lockid;
}

static void
atunlock(int fd)
{
	close(fd);
}

long
getjobid()
{
#define	JOBLEN 12
	char buf[JOBLEN+1];
	int fd;
	long jobid;

	fd = atlock();

	if (read(fd, buf, JOBLEN) < 1)
		Perror("read at lockfile");
	buf[JOBLEN] = '\0';
	if ((jobid = atol(buf)) == 0)
		Perror("lockfile job# invalid");
	if (lseek(fd, 0, SEEK_SET) < 0)
		Perror("seek on at lockfile");
	swrite(fd, "%d\n", jobid + 1);
	
	atunlock(fd);
	return jobid;
#undef	JOBLEN
}
