/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
 
/*	BSDI readqueue.c,v 1.1 1992/08/24 21:18:11 sanders Exp	*/

#ifndef lint
static char sccsid[] = "@(#)readqueue.c	1.1 (BSDI) 08/24/92";
#endif /* lint */

/*-
 * readqueue.c
 *
 * Function:	Reads the queue files into memory for processing
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
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#include "pathnames.h"
#include "at.h"
#include "when.h"
#include "errlib.h"

static char *
findstr(char *what, char *where)
{
	char *s;
	char *p = strstr(where, what);
	if (p != NULL) {
		s = p += strlen(what);
		while (*s && *s != '\n')
			s++;
		*s = '\0';			/* stamp out newlines */
	}
	return p;
}

JobQ
readqueue()
{
	int	fd;
	DIR	*dirp;
	struct	dirent *dp;
	struct	stat st;
	JobQ	head, prev;
	char	*buf;
	char	*s;
	int	i;
	register JobQ p = NULL;

	chdir(AT_DIR);

	dirp = opendir(".");
	if (dirp == NULL)
		Perror("opendir(%s)", AT_DIR);

	prev = head = (JobQ)calloc(1, sizeof(*head));
	while ((dp = readdir(dirp)) != NULL) {
		if (!isdigit(*dp->d_name))
			continue;
		if (stat(dp->d_name, &st) < 0) {
			Pwarn("stat(%s)", dp->d_name);
			continue;
		}
		if (st.st_size == 0)
			continue;

		p = (JobQ)calloc(1, sizeof(*p));
		p->id = atol(dp->d_name);
		p->when = atol(index(dp->d_name, '.') + 1);
		p->fn = strdup(qfpath(p));

		p->owner = st.st_uid;
		p->gid = st.st_gid;

		/* we are commited now */
		prev = prev->next = p;

		/* unless something goes wrong */
		p->valid = 1;
		
		if ((fd = open(p->fn, O_RDONLY, 0)) < 0) {
			Pwarn("open(%s)", p->fn);
			p->valid = 0;
			continue;
		}

		if ((buf = malloc(st.st_size)) == NULL)
			Perror("Unable to allocate %d bytes", st.st_size);
		p->buf = buf;
		if (read(fd, buf, st.st_size) != st.st_size) {
			Pwarn("read failed on %s", p->fn);
			p->valid = 0;
			continue;
		}

		/* search for items backwards */
		s = findstr("groups: ", buf);
		if (s == NULL) {
			Pwarn("no group list in %s", p->fn);
			p->valid = 0;
			continue;
		}
		p->ngroups = 0;
		while (s && *s) {
			p->groups[p->ngroups++] = atoi(s);
			Pmsg(("group found: %d %d\n",
				p->ngroups-1, atoi(s)));
			s = index(s+1, ' ');
		}

		if ((s = findstr("mail: ", buf)) == NULL) {
			Pwarn("notify missing from %s", p->fn);
			p->valid = 0;
			continue;
		}
		p->notify = strcmp(s, "yes") == 0;
		Pmsg(("notify: %s\n", s));

		if ((p->jobname = findstr("jobname: ", buf)) == NULL) {
			Pwarn("jobname missing from %s", p->fn);
			p->valid = 0;
			continue;
		}
		Pmsg(("jobname: %s\n", p->jobname));
		
		/*
		 * file is still being written by at or was left
		 * in a bad state
		 */
		if ((s = findstr("# locked", buf)) != NULL) {
			if (job->owner == 0)
				Pwarn("job %d locked", p->id);
#ifdef	DEBUG
			else
				Pmsg(("job %d locked\n", p->id));
#endif
			p->valid = 0;
		}
	}

	(void)closedir(dirp);
	return head;
}

JobQ
freeitem(JobQ p)
{
	JobQ next = p->next;
	if (p->buf) free(p->buf);
	if (p->fn) free(p->fn);
	free(p);
	return next;
}

void
freequeue(JobQ p)
{
	while (p)
		p = freeitem(p);
}

bytime(const void *p, const void *q)
{
	const JobQ a = *(JobQ *)p;
	const JobQ b = *(JobQ *)q;
	return a->when - b->when;
}

/*
 * I don't think even the C-beautifier could make this beautiful
 *
 * Strategy:
 *	Count the number of elements so we can malloc an array of ptrs.
 *	Then fill the array with all but the head, it stays in place.
 *	qsort() the array of pointers.
 *	Now the tricky part, reorder the linked list according to the array
 *
 *	The overhead for the array of ptrs is fairly small compared to
 *	trying to sort the list in place which would probably require
 *	prev pointers in the structure to make it easier and that would
 *	take up the same amount of space as the array anyway.
 */
void
sortqueue(JobQ head)
{
	JobQ p, next;
	int count = 0;
	JobQ *array, *q;

	Pmsg(("sortqueue()\n"));
	Pmsg(("found: 0x%08x -> 0x%08x\n", head, head->next));
	for (p = head->next; p; p = p->next, count++)
		Pmsg(("found: 0x%08x -> 0x%08x\n", p, p->next));
	Pmsg(("counted %d\n", count));
	array = (JobQ *)malloc(count * sizeof(JobQ));
	/* pack the linked list pointers into the array */
	for (p = head->next, q = array; p; p = p->next)
		*q++ = p;
	qsort(array, count, sizeof(JobQ), bytime);
	/* unpack the array back onto the linked list */
	for (p = head, q = array; count--; p = p->next = *q++)
		Pmsg(("sorting 0x%08x -> 0x%08x\n", p, *q));
	p->next = NULL;
	Pmsg(("last 0x%08x -> 0x%08x\n", p, p->next));
	free(array);
}

void
printhdr()
{
#ifndef NOHDRS
        printf("%10s %10.10s %10s    %s\n",
                "job", "user", "jobname", "when");
#endif
}

void
printitem(register JobQ p)
{
	char *s = "(unknown)";

	if (p->jobname) {
		int len = strlen(p->jobname);
		s = ((len <= 10) ? p->jobname : 
			p->jobname + len - 10);
	}

	printf("%10d %10.10s %10s    ",
		p->id, username(p->owner), s);
	print_time(p->when);
	printf("\n");
}

void
printqueue(register JobQ p)
{
	for(p = p->next; p; p = p->next)
		printitem(p);
}
