/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
 
/*	BSDI at.c,v 1.1 1992/08/24 21:18:11 sanders Exp	*/

#ifndef lint
static char sccsid[] = "@(#)at.c	1.1 (BSDI) 08/24/92";
#endif

/*-
 * at.c
 *
 * Usage:	at [-csm] time [date] [+increment] [script]
 *		at -l [job ...] | atq
 *		at -r job [...] | atrm
 *
 *		at | atrun		When invoked by root w/no args,
 *					runs one scheduler pass
 *					Meant for use in cron (like atrun)
 *
 * Function:	Allows user to specify when commands should be executed
 *		(with a limited granularity).
 *
 * Author:	Tony Sanders
 * Date:	08/17/92
 *
 * Remarks:
 * History:	08/17/92 Tony Sanders -- creation
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

#include "pathnames.h"
#include "at.h"
#include "when.h"
#include "errlib.h"

JobQ	job;
char	*progname;

static void
usage() {
	Perrmsg("usage: %s [-csm] time [date] [+increment] [script]\n",
			progname);
	Perrmsg("       %s -r job [...]\n", progname);
	Perrmsg("       %s -l [job ...]\n", progname);
	exit(1);
}

int
main(argc, argv, envp)
	int argc;
	char *argv[];
	char *envp[];
{
	int     ch;
	char	*envshell = getenv("SHELL");

	progname = (progname = rindex(*argv, '/')) ? progname++ : *argv;

	if ((job = (JobQ)calloc(1, sizeof(*job))) == NULL)
		Perror("calloc");
	job->shell = envshell != NULL ? envshell : AT_DEFSHELL;
	job->owner = getuid();

	if (setuid(0) < 0)
                Pwarn("setuid");
	errno = 0;

	/*
	 * for backwards compatibility
	 */
	if (strcmp(progname, "atq") == 0) {
		argc--, argv++;
		listjobs(&argc, &argv);
		Perror("listjobs failed");
	} else if (strcmp(progname, "atrm") == 0) {
		argc--, argv++;
		removejobs(&argc, &argv);
		Perror("removejobs failed");
	}

        /*
         * if invoked by root with no args run the scheduler()
         */
	if ((argc == 1 && job->owner == 0) || strcmp(progname, "atrun") == 0) {
		scheduler();
		Perror("scheduler failed");
	}

	if (!at_allowed())
		Perror("execute permission denied");

	while ((ch = getopt(argc, argv, "csmrlq:")) != EOF) {
		switch (ch) {
		    case 'c': job->shell = _PATH_CSHELL; break;
		    case 's': job->shell = _PATH_BSHELL; break;
		    case 'm': job->notify++; break;
		    case 'r':
			argc -= optind; argv += optind;
			removejobs(&argc, &argv);
			Perror("removejobs failed");
		    case 'l':
			argc -= optind; argv += optind;
			listjobs(&argc, &argv);
			Perror("listjobs failed");
		    case 'q':
			Pwarn("q doesn't do anything");
			break;
		    default: usage(); break;
		}
	}
	argc -= optind; argv += optind;

	if (argc < 1)
		usage();

	if ((job->when = when(&argc, &argv)) < 0)
		usage();

	queuejob(&argc, &argv, envp);

	free(job);
	return (0);
}

/*
 * run external program and get exit status
 * return true if access allowed
 */

int
at_allowed()
{
	int ret;
	char buf[MAXPATHLEN];

	sprintf(buf, "%s %s", AT_ALLOWED, username(job->owner));
	ret = system(buf);
	Pmsg(("at_allowed returned %d\n", ret));
	if (ret == 127)
		Perror("exec %s", buf);
	return (ret == 0);
}

/*
 * List jobs in the queue directory
 */

void
listjobs(int *argc, char **argv[])
{
	int ac = *argc;
	char **av = *argv;
	JobQ p, head;

	Pmsg(("listing jobs:\n"));
	head = readqueue();
	sortqueue(head);

	/*
	 * if no args print all for root else all users jobs
	 * otherwise list the jobs requested in av
	 */
	printhdr();
	if (ac == 0 && job->owner == 0)
		for (p = head->next; p; p = p->next)
			printitem(p);
	else if (ac == 0)
		for (p = head->next; p; p = p->next)
			if (job->owner == p->owner)
				printitem(p);
	while (ac) {
		long jobid = atol(*av++); ac--;
		for (p = head->next; p; p = p->next) {
			if (p->id == jobid) {
				printitem(p);
				break;
			}
		}
	}
	freequeue(head);

	exit(0);
}

void
removeitem(JobQ p)
{
	char *outf;

	Pmsg(("removing: %s\n", p->fn));
	if (unlink(p->fn) < 0) {
		Pwarn("unlink(%s)", p->fn);
		errno = 0;
	}

	/* remove output file */
	outf = outputpath(p);
	Pmsg(("removing: %s\n", outf));
	if (unlink(outf) < 0 && errno != ENOENT)
		Pwarn("unlink(%s)", outf);
	errno = 0;

	p->fn[0] = '\0';			/* hide from removejobs() */
}

void
removejobs(int *argc, char **argv[])
{
	int ac = *argc;
	char **av = *argv;
	JobQ p, head;

	Pmsg(("removing jobs:\n"));
	head = readqueue();

	if (ac == 0)
		usage();
	while (ac) {
		long jobid = atol(*av++); ac--;
		for (p = head->next; p; p = p->next) {
			if (p->id == jobid) {
#ifdef	DEBUG
				printitem(p);
#endif
				if (job->owner == 0 || job->owner == p->owner)
					removeitem(p);
				else {
					errno = 0;
					Pwarn("you don't own %d", p->id);
				}
				goto nextarg;
			}
		}
		Pwarn("couldn't find job %d", jobid);
nextarg:
		/* anchor for label */;
	}
	freequeue(head);
	*argc = ac;
	*argv = av;
	exit(0);
}
