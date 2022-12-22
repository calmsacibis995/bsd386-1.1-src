/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
 
/*	BSDI queuejob.c,v 1.1 1992/08/24 21:18:11 sanders Exp	*/

#ifndef lint
static char sccsid[] = "@(#)queuejob.c	1.1 (BSDI) 08/24/92";
#endif

/*
 * Processes data in global structure: job
 */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#include "pathnames.h"
#include "at.h"
#include "errlib.h"

/*
 * Handle locking and cleanup on abort
 */

static	int needs_cleanup = 0;
static	void (*int_func)();
static	void (*quit_func)();
static	void (*term_func)();

static	void
cleanup()
{
	if (needs_cleanup == 0)
		return;			/* calling exit from an atexit() */
	needs_cleanup = 0;

	Pmsg(("cleaning up %s\n", job->fn?job->fn:"(null)"));
	if (job->fn)
		unlink(job->fn);
	exit(1);
}

static void
Flock()
{
	char	*lockid = AT_LOCKID;
	int	len = strlen(lockid);

	if (atexit(cleanup) < 0)
		Perror("Can't register cleanup function");
	int_func = signal(SIGINT, cleanup);
	quit_func = signal(SIGQUIT, cleanup);
	term_func = signal(SIGTERM, cleanup);

	if ((job->fn = strdup(qfpath(job))) == NULL)
		Perror("Can't strdup jobname");

	if ((job->fd = open(job->fn, O_WRONLY|O_CREAT|O_EXCL, 0600)) < 0)
		Perror("Can't open %s", job->fn);

	needs_cleanup = 1;
	if (write(job->fd, lockid, len) != len)
		Perror("Can't lock %s", job->fn);
	fsync(job->fd);
}

static void
Funlock()
{
	char	*unlockid = AT_VERSION;
	int	len = strlen(unlockid);

	if (lseek(job->fd, 0, SEEK_SET) < 0)
		Perror("Can't seek %s", job->fn);
	if (write(job->fd, unlockid, len) != len)
		Perror("Can't unlock %s", job->fn);
	close(job->fd);

	needs_cleanup = 0;			/* turn off cleanup on exit */
	signal(SIGINT, int_func);
	signal(SIGQUIT, quit_func);
	signal(SIGTERM, term_func);
}

/*
 * Insert a job into the queue directory
 */

char *
queuejob(int *argc, char **argv[], char *envp[])
{
        int	ac = *argc;
        char	**av = *argv;
	char	**ep;
#	define	IOBUF 4096
	char	iobuf[IOBUF];
	FILE	*ifile = stdin;
	char	*cwd;
	int	i;

	job->id = getjobid();

	Flock();				/* delete files if aborted */

	swrite(job->fd, "# jobid: %d\n", job->id);
	swrite(job->fd, "# owner: %s\n", username(job->owner));

	if (ac > 0) {
		swrite(job->fd, "# jobname: %s\n", *av);
		Pmsg(("opening script file: ``%s''\n", *av));
		ifile = fopen(*av, "r");
		if (ifile == NULL)
			Perror("Can't open %s", *av);
		ac--, av++;
	} else
		swrite(job->fd, "# jobname: (stdin)\n");
	
	swrite(job->fd, "# shell: %s\n", job->shell);
	swrite(job->fd, "# notify by mail: %s\n",
		job->notify ? "yes" : "no");
	swrite(job->fd, "# groups:");
	if ((job->ngroups = getgroups(NGROUPS, job->groups)) < 0)
		Perror("Can't getgroups()");
	for (i = 0; i < job->ngroups; i++)
		swrite(job->fd, " %d", job->groups[i]);

	swrite(job->fd, "\n\n");

	for (ep = envp; *ep; ep++) {
		char *s = index(*ep, '=') + 1;
		write(job->fd, *ep, s - *ep);
		swrite(job->fd, "'%s'\n", shellesc(s));
		swrite(job->fd, "export %.*s\n", s - *ep - 1, *ep);
	}
	
	swrite(job->fd, "$SHELL << '...rest is shell input'\n\n");

	if ((cwd = getcwd(NULL, MAXPATHLEN)) == NULL)
		Perror("Can't get cwd");
	swrite(job->fd, "cd '%s'\n", shellesc(cwd));
	free(cwd);

	swrite(job->fd, "umask 0%o\n", umask(077));

	swrite(job->fd, "\n# begin user input\n");

	errno = 0;
	while (fgets(iobuf, IOBUF, ifile) != NULL)
		write(job->fd, iobuf, strlen(iobuf));
	if (ferror(ifile))
		Perror("Can't read %s", job->fn);

	if (fchown(job->fd, job->owner, getgid()) < 0)
		Perror("Can't chown %s", job->fn);

	fclose(ifile);

	/*
	 * ready to rock and roll
	 */
	Funlock();

	/*
	 * Ack job queued for user
	 */
	printf("job %d at ", job->id);
	print_time(job->when);
	printf("\n");

        *argc = ac;
        *argv = av;
}
