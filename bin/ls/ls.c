/*	BSDI $Id: ls.c,v 1.3 1993/03/29 03:50:07 polk Exp $	*/

/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Michael Fischbein.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1989 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)ls.c	5.64 (Berkeley) 4/2/92";
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <unistd.h>
#include <fts.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "ls.h"
#include "extern.h"

char	*getbsize __P((char *, int *, long *));
char	*group_from_gid __P((u_int, int));
char	*user_from_uid __P((u_int, int));

static void	 display __P((FTSENT *, FTSENT *));
static char	*flags_from_fid __P((u_long));
static int	 mastercmp __P((const FTSENT **, const FTSENT **));
static void	 traverse __P((int, char **, int));

static void (*printfcn) __P((DISPLAY *));
static int (*sortfcn) __P((const FTSENT *, const FTSENT *));

long blocksize;			/* block size units */
int termwidth = 80;		/* default terminal width */

/* flags */
int f_accesstime;		/* use time of last access */
int f_column;			/* columnated format */
int f_flags;			/* show flags associated with a file */
int f_inode;			/* print inode */
int f_listdir;			/* list actual directory, not contents */
int f_listdot;			/* list files beginning with . */
int f_longform;			/* long listing format */
int f_newline;			/* if precede with newline */
int f_nonprint;			/* show unprintables as ? */
int f_nosort;			/* don't sort output */
int f_recursive;		/* ls subdirectories also */
int f_reversesort;		/* reverse whatever sort is used */
int f_sectime;			/* print the real time for all files */
int f_singlecol;		/* use single column output */
int f_size;			/* list size in short listing */
int f_statustime;		/* use time of last mode change */
int f_dirname;			/* if precede with directory name */
int f_timesort;			/* sort by time vice name */
int f_type;			/* add type character for non-regular files */

int
main(argc, argv)
	int argc;
	char *argv[];
{
	static char dot[] = ".", *dotav[] = { dot, NULL };
	struct winsize win;
	int ch, fts_options, notused;
	char *p;

	/* Terminal defaults to -Cq, non-terminal defaults to -1. */
	if (isatty(STDOUT_FILENO)) {
		if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &win) == -1 ||
		    !win.ws_col) {
			if (p = getenv("COLUMNS"))
				termwidth = atoi(p);
		}
		else
			termwidth = win.ws_col;
		f_column = f_nonprint = 1;
	} else
		f_singlecol = 1;

	/* Root is -A automatically. */
	if (!getuid())
		f_listdot = 1;

	fts_options = FTS_PHYSICAL;
	while ((ch = getopt(argc, argv, "1ACFLRTacdfgikloqrstu")) != EOF) {
		switch (ch) {
		/*
		 * The -1, -C and -l options all override each other so shell
		 * aliasing works right.
		 */
		case '1':
			f_singlecol = 1;
			f_column = f_longform = 0;
			break;
		case 'C':
			f_column = 1;
			f_longform = f_singlecol = 0;
			break;
		case 'l':
			f_longform = 1;
			f_column = f_singlecol = 0;
			break;
		/* The -c and -u options override each other. */
		case 'c':
			f_statustime = 1;
			f_accesstime = 0;
			break;
		case 'u':
			f_accesstime = 1;
			f_statustime = 0;
			break;
		case 'F':
			f_type = 1;
			break;
		case 'L':
			fts_options &= ~FTS_PHYSICAL;
			fts_options |= FTS_LOGICAL;
			break;
		case 'R':
			f_recursive = 1;
			break;
		case 'a':
			fts_options |= FTS_SEEDOT;
			/* FALLTHROUGH */
		case 'A':
			f_listdot = 1;
			break;
		/* The -d option turns off the -R option. */
		case 'd':
			f_listdir = 1;
			f_recursive = 0;
			break;
		case 'f':
			f_nosort = 1;
			break;
		case 'g':		/* Compatibility with 4.3BSD. */
			break;
		case 'i':
			f_inode = 1;
			break;
		case 'k':		/* Delete before 4.4BSD. */
			(void)fprintf(stderr, "ls: -k no longer supported\n");
			break;
		case 'o':
			f_flags = 1;
			break;
		case 'q':
			f_nonprint = 1;
			break;
		case 'r':
			f_reversesort = 1;
			break;
		case 's':
			f_size = 1;
			break;
		case 'T':
			f_sectime = 1;
			break;
		case 't':
			f_timesort = 1;
			break;
		default:
		case '?':
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	/*
	 * If not -F, -i, -l, -s or -t options, don't require stat
	 * information.
	 */
	if (!f_inode && !f_longform && !f_size && !f_timesort && !f_type)
		fts_options |= FTS_NOSTAT;

	/*
	 * If not -F, -d or -l options, follow any symbolic links listed on
	 * the command line.
	 */
	if (!f_longform && !f_listdir && !f_type)
		fts_options |= FTS_COMFOLLOW;

	/* If -l or -s, figure out block size. */
	if (f_longform || f_size) {
		(void)getbsize("ls", &notused, &blocksize);
		blocksize /= 512;
	}

	/* Select a sort function. */
	if (f_reversesort) {
		if (!f_timesort)
			sortfcn = revnamecmp;
		else if (f_accesstime)
			sortfcn = revacccmp;
		else if (f_statustime)
			sortfcn = revstatcmp;
		else /* Use modification time. */
			sortfcn = revmodcmp;
	} else {
		if (!f_timesort)
			sortfcn = namecmp;
		else if (f_accesstime)
			sortfcn = acccmp;
		else if (f_statustime)
			sortfcn = statcmp;
		else /* Use modification time. */
			sortfcn = modcmp;
	}

	/* Select a print function. */
	if (f_singlecol)
		printfcn = printscol;
	else if (f_longform)
		printfcn = printlong;
	else
		printfcn = printcol;

	if (argc)
		traverse(argc, argv, fts_options);
	else
		traverse(1, dotav, fts_options);
	exit(0);
}

static int output;			/* If anything output. */

/*
 * Traverse() walks the logical directory structure specified by the argv list
 * in the order specified by the mastercmp() comparison function.  During the
 * traversal it passes linked lists of structures to display() which represent
 * a superset (may be exact set) of the files to be displayed.
 */
static void
traverse(argc, argv, options)
	int argc, options;
	char *argv[];
{
	register FTS *ftsp;
	register FTSENT *p, *chp;
	int ch_options;

	if ((ftsp =
	    fts_open(argv, options, f_nosort ? NULL : mastercmp)) == NULL)
		err(1, "%s", strerror(errno));

	display(NULL, fts_children(ftsp, 0));
	if (f_listdir)
		return;

	/*
	 * If not recursing down this tree and don't need stat info, just get
	 * the names.
	 */
	ch_options = !f_recursive && options & FTS_NOSTAT ? FTS_NAMEONLY : 0;

	while (p = fts_read(ftsp))
		switch(p->fts_info) {
		case FTS_DC:
			err(0, "%s: directory causes a cycle", p->fts_name);
			break;
		case FTS_DNR:
		case FTS_ERR:
			err(0, "%s: %s",
			    p->fts_name, strerror(p->fts_errno));
			break;
		case FTS_D:
			if (p->fts_level != FTS_ROOTLEVEL &&
			    p->fts_name[0] == '.' && !f_listdot)
				break;

			/*
			 * If already output something, put out a newline as
			 * a separator.  If multiple arguments, precede each
			 * directory with its name.
			 */
			if (output)
				(void)printf("\n%s:\n", p->fts_path);
			else if (argc > 1) {
				(void)printf("%s:\n", p->fts_path);
				output = 1;
			}

			chp = fts_children(ftsp, ch_options);
			display(p, chp);

			if (!f_recursive && chp != NULL)
				(void)fts_set(ftsp, p, FTS_SKIP);
			break;
		}
	(void)fts_close(ftsp);
}

/*
 * Display() takes a linked list of FTSENT structures passes the list along
 * with any other necessary information to the print function (printfcn()).
 * P points to the parent directory of the display list.
 */
static void
display(p, list)
	register FTSENT *p;
	FTSENT *list;
{
	register FTSENT *cur;
	struct stat *sp;
	DISPLAY d;
	NAMES *np;
	u_long btotal, flen, glen, ulen;
	u_long maxblock, maxgroup, maxinode, maxlen, maxnlink, maxsize;
	u_long maxuser, maxflags;
	int entries, needstats;
	char *user, *group, *flags, buf[20];	/* 32 bits == 10 digits */

	/*
	 * If list is NULL there are two possibilities: that the parent
	 * directory p has no children, or that fts_children() returned an
	 * error.  We ignore the error case since it will be replicated
	 * on the next call to fts_read() on the post-order visit to the
	 * directory p, and will be signalled in traverse().
	 */
	if (list == NULL)
		return;

	needstats = f_inode || f_longform || f_size;
	flen = 0;
	btotal = maxblock = maxinode = maxlen = maxnlink = maxsize = 0;
	maxuser = maxgroup = maxflags = 0;
	for (cur = list, entries = 0; cur; cur = cur->fts_link) {
		if (cur->fts_info == FTS_ERR || cur->fts_info == FTS_NS) {
			err(0, "%s: %s",
			    cur->fts_name, strerror(cur->fts_errno));
			cur->fts_number = NO_PRINT;
			continue;
		}

		/*
		 * P is NULL if list is the argv list, to which different rules
		 * apply.
		 */
		if (p == NULL) {
			/* Directories will be displayed later. */
			if (cur->fts_info == FTS_D && !f_listdir) {
				cur->fts_number = NO_PRINT;
				continue;
			}
		} else {
			/* Only display dot file if -a/-A set. */
			if (cur->fts_name[0] == '.' && !f_listdot) {
				cur->fts_number = NO_PRINT;
				continue;
			}
		}
		if (f_nonprint)
			prcopy(cur->fts_name, cur->fts_name, cur->fts_namelen);
		if (cur->fts_namelen > maxlen)
			maxlen = cur->fts_namelen;
		if (needstats) {
			sp = cur->fts_statp;
			if (sp->st_blocks > maxblock)
				maxblock = sp->st_blocks;
			if (sp->st_ino > maxinode)
				maxinode = sp->st_ino;
			if (sp->st_nlink > maxnlink)
				maxnlink = sp->st_nlink;
			if (sp->st_size > maxsize)
				maxsize = sp->st_size;

			btotal += sp->st_blocks;
			if (f_longform) {
				user = user_from_uid(sp->st_uid, 0);
				if ((ulen = strlen(user)) > maxuser)
					maxuser = ulen;
				group = group_from_gid(sp->st_gid, 0);
				if ((glen = strlen(group)) > maxgroup)
					maxgroup = glen;
				if (f_flags) {
					flags = flags_from_fid(sp->st_flags);
					if ((flen = strlen(flags)) > maxflags)
						maxflags = flen;
				} else
					flen = 0;

				if ((np = malloc(sizeof(NAMES) +
				    ulen + glen + flen + 3)) == NULL)
					err(1, "%s", strerror(errno));

				np->user = &np->data[0];
				(void)strcpy(np->user, user);
				np->group = &np->data[ulen + 1];
				(void)strcpy(np->group, group);

				if (f_flags) {
					np->flags = &np->data[ulen + glen + 2];
				  	(void)strcpy(np->flags, flags);
				}
				cur->fts_pointer = np;
			}
		}
		++entries;
	}

	if (!entries)
		return;

	d.list = list;
	d.entries = entries;
	d.maxlen = maxlen;
	if (needstats) {
		d.btotal = btotal;
		d.s_block = snprintf(buf, sizeof(buf), "%d", maxblock);
		d.s_flags = maxflags;
		d.s_group = maxgroup;
		d.s_inode = snprintf(buf, sizeof(buf), "%d", maxinode);
		d.s_nlink = snprintf(buf, sizeof(buf), "%d", maxnlink);
		d.s_size = snprintf(buf, sizeof(buf), "%d", maxsize);
		d.s_user = maxuser;
	}

	printfcn(&d);
	output = 1;

	if (f_longform)
		for (cur = list; cur; cur = cur->fts_link)
			free(cur->fts_pointer);
}

/*
 * Ordering for mastercmp:
 * If ordering the argv (fts_level = FTS_ROOTLEVEL) return non-directories
 * as larger than directories.  Within either group, use the sort function.
 * All other levels use the sort function.  Error entries remain unsorted.
 */
static int
mastercmp(a, b)
	const FTSENT **a, **b;
{
	register int a_info, b_info;

	a_info = (*a)->fts_info;
	if (a_info == FTS_ERR)
		return (0);
	b_info = (*b)->fts_info;
	if (b_info == FTS_ERR)
		return (0);

	if (a_info == FTS_NS || b_info == FTS_NS)
		return (namecmp(*a, *b));

	if (a_info == b_info)
		return (sortfcn(*a, *b));

	if ((*a)->fts_level == FTS_ROOTLEVEL)
		if (a_info == FTS_D)
			return (1);
		else if (b_info == FTS_D)
			return (-1);
		else
			return (sortfcn(*a, *b));
	else
		return (sortfcn(*a, *b));
}

static char *
flags_from_fid(flags)
	u_long flags;
{
	static char buf[20];
	register int comma = 0;
	register char *p;

	p = buf;
#ifdef notyet
	if (flags & ARCHIVED) {
		(void)strcpy(p, "arch");
		p += sizeof("arch") - 1;
		comma = 1;
	} else
		comma = 0;
	if (flags & NODUMP) {
		if (comma++)
			*p++ = ',';
		(void)strcpy(p, "nodump");
		p += sizeof("nodump") - 1;
	}
	if (flags & IMMUTABLE) {
		if (comma++)
			*p++ = ',';
		(void)strcpy(p, "nochg");
		p += sizeof("nochg") - 1;
	}
#endif
	if (!comma)
		(void)strcpy(p, "-");
	return (buf);
}
