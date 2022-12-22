/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

/*-
 * Copyright (c) 1986, 1991 The Regents of the University of California.
 * All rights reserved.
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
"@(#) Copyright (c) 1986, 1991 The Regents of the University of California. \
 All rights reserved.\n\
@(#) Copyright (c) 1992, Berkeley Software Design, Inc. \
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)iostat.c	5.9 (Berkeley,BSDI) 6/27/91";
#endif /* not lint */

#include <sys/param.h>
#include <sys/buf.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <paths.h>
#include <sys/kinfo.h>
#include <sys/sysinfo.h>
#include <sys/cpustats.h>
#include <sys/diskstats.h>
#include <sys/ttystats.h>

/*
 * internal stat structure
 */
struct _disk {
	long	cp_time[CPUSTATES];
	long	tk_nin;
	long	tk_nout;
	long	*dk_time;
	long	*dk_sectors;
	long	*dk_seek;
	long	*dk_xfer;
} cur, last;

long	*dk_bpms;
long	*dk_secsize;
int	dk_ndrive;
int	*dr_select;
char	**dr_name;

int	ndrives;

int	hz;
double	etime;

static void cpustats __P((void));
static void dkstats __P((void));
static void phdr __P((int));
static void usage __P((void));
static void err __P((const char *, ...));

/*
 * read cpu stats
 */
void
getcpustats(struct _disk *dp)
{
	struct cpustats cpustats;
	int bufsize = sizeof(cpustats);

	if (getkerninfo(KINFO_CPU, &cpustats, &bufsize, 0) < 0)
		err("getkerninfo(KINFO_CPU): %s", strerror(errno));
	if (bufsize != sizeof(cpustats))
		err("getkerninfo(KINFO_CPU): wrong size");
	bcopy((char *) cpustats.cp_time, dp->cp_time, sizeof(dp->cp_time));
}

/*
 * read tty stats
 */
void
getttystats(struct _disk *dp)
{
	struct ttytotals ttytotals;
	int bufsize = sizeof(ttytotals);

	if (getkerninfo(KINFO_TTY_TOTALS, &ttytotals, &bufsize, 0) < 0)
		err("getkerninfo(KINFO_TTY_TOTALS): %s", strerror(errno));
	if (bufsize != sizeof(ttytotals))
		err("getkerninfo(KINFO_TTY_TOTALS): wrong size");
	dp->tk_nin = ttytotals.tty_nin;
	dp->tk_nout = ttytotals.tty_nout;
}

main(argc, argv)
	int argc;
	char **argv;
{
	register int i;
	long tmp;
	int ch, hdrcnt, reps, interval, ndrives;
	char **cp, *memfile, *namelist, buf[80];
	struct sysinfo *sysinfo;
	char *names_data, *p;
	struct diskstats *disk_data, *dk;
	int size, bufsize, expected;

	interval = reps = 0;
	namelist = memfile = NULL;
	while ((ch = getopt(argc, argv, "c:M:N:w:")) != EOF)
		switch(ch) {
		case 'c':
			reps = atoi(optarg);
			break;
		case 'M':
			memfile = optarg;
			break;
		case 'N':
			namelist = optarg;
			break;
		case 'w':
			interval = atoi(optarg);
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	/*
	 * read sysinfo structure
	 */
	bufsize = getkerninfo(KINFO_SYSINFO, NULL, NULL, 0);
	if (bufsize < 0)
		err("getkerninfo(KINFO_SYSINFO size): %s", strerror(errno));
	sysinfo = (struct sysinfo *) malloc(bufsize);
	if ((expected = getkerninfo(KINFO_SYSINFO, (char *)sysinfo,
	    &bufsize, 0)) < 0)
		err("getkerninfo(KINFO_SYSINFO): %s", strerror(errno));
	if (expected > bufsize || bufsize < sizeof(struct sysinfo))
		err("getkerninfo(KINFO_SYSINFO): wrong size");
#ifdef unneeded
#define	ADJ_STRING(info, field)	{ \
	(info)->field = (int) (info)->field + (char *) (info); }
	ADJ_STRING(sysinfo,sys_machine);
	ADJ_STRING(sysinfo,sys_model);
	ADJ_STRING(sysinfo,sys_ostype);
	ADJ_STRING(sysinfo,sys_osrelease);
	ADJ_STRING(sysinfo,sys_version);
	ADJ_STRING(sysinfo,sys_hostname);
#endif
	hz = sysinfo->sys_phz ? sysinfo->sys_phz : sysinfo->sys_hz;


	/*
	 * read disk names
	 */
	expected = bufsize = getkerninfo(KINFO_DISK_NAMES, NULL, NULL, 0);
	if (bufsize < 0)
		err("getkerninfo(KINFO_DISK_NAMES): %s", strerror(errno));
	names_data = malloc(bufsize);
	if (getkerninfo(KINFO_DISK_NAMES, names_data, &bufsize, 0) < 0)
		err("getkerninfo(KINFO_DISK_NAMES): %s", strerror(errno));
	if (expected != bufsize)
		err("getkerninfo(KINFO_DISK_NAMES): wrong size");
	/*
	 * count number of drives
	 */
	for (p = names_data; p < names_data + bufsize; p += strlen(p) + 1)
		dk_ndrive++;

	cur.dk_time = calloc(dk_ndrive, sizeof(long));
	cur.dk_sectors = calloc(dk_ndrive, sizeof(long));
	cur.dk_seek = calloc(dk_ndrive, sizeof(long));
	cur.dk_xfer = calloc(dk_ndrive, sizeof(long));
	last.dk_time = calloc(dk_ndrive, sizeof(long));
	last.dk_sectors = calloc(dk_ndrive, sizeof(long));
	last.dk_seek = calloc(dk_ndrive, sizeof(long));
	last.dk_xfer = calloc(dk_ndrive, sizeof(long));

	dk_bpms = calloc(dk_ndrive, sizeof(long));
	dk_secsize = calloc(dk_ndrive, sizeof(long));
	dr_select = calloc(dk_ndrive, sizeof(int));
	dr_name = calloc(dk_ndrive, sizeof(char *));

	/*
	 * assign drive names to calloc()'ed array
	 */
	for (p = names_data, i = 0; *p; p += strlen(p) + 1)
		dr_name[i++] = p;

	/*
	 * read diskinformation and save bpms and secsize
	 */
	expected = bufsize = getkerninfo(KINFO_DISK_STATS, NULL, NULL, 0);
	size = bufsize;
	if (bufsize < 0)
		err("getkerninfo(KINFO_DISK_STATS): %s", strerror(errno));
	disk_data = (struct diskstats *) malloc(bufsize);
	if (getkerninfo(KINFO_DISK_STATS, disk_data, &bufsize, 0) < 0)
		err("getkerninfo(KINFO_DISK_STATS): %s", strerror(errno));
	if (expected != bufsize)
		err("getkerninfo(KINFO_DISK_STATS): wrong size");

	for (i = 0; i < (size / sizeof(*disk_data)); i++) {
		dk_bpms[i] = disk_data[i].dk_bpms;
		dk_secsize[i] = disk_data[i].dk_secsize;
	}

	/*
	 * Choose drives to be displayed.  Priority goes to (in order) drives
	 * supplied as arguments and default drives.  If everything isn't
	 * filled in and there are drives not taken care of, display the first
	 * few that fit.
	 *
	 * The backward compatibility #ifdefs permit the syntax:
	 *	iostat [ drives ] [ interval [ count ] ]
	 */
	for (ndrives = 0; *argv; ++argv) {
#ifdef	BACKWARD_COMPATIBILITY
		if (isdigit(**argv))
			break;
#endif
		for (i = 0; i < dk_ndrive; i++) {
			if (strcmp(dr_name[i], *argv))
				continue;
			dr_select[i] = 1;
			++ndrives;
		}
	}
#ifdef	BACKWARD_COMPATIBILITY
	if (*argv) {
		interval = atoi(*argv);
		if (*++argv)
			reps = atoi(*argv);
	}
#endif

	if (interval) {
		if (!reps)
			reps = -1;
	} else
		if (reps)
			interval = 1;

	for (i = 0; i < dk_ndrive && ndrives < 4; i++) {
		char *defdrives[] = { "dk0", "dk1", "dk2", "dk3", 0 };
		if (dr_select[i] || dk_bpms[i] == 0)
			continue;
		for (cp = defdrives; *cp; cp++)
			if (strcmp(dr_name[i], *cp) == 0) {
				dr_select[i] = 1;
				++ndrives;
				break;
			}
	}
	for (i = 0; i < dk_ndrive && ndrives < 4; i++) {
		if (dr_select[i])
			continue;
		dr_select[i] = 1;
		++ndrives;
	}

	(void)signal(SIGCONT, phdr);

	for (hdrcnt = 1;;) {
		if (!--hdrcnt) {
			phdr(0);
			hdrcnt = 20;
		}

		bufsize = size;
		if (getkerninfo(KINFO_DISK_STATS, (char *) disk_data,
		    &bufsize, 0) < 0)
			err("getkerninfo(KINFO_DISK_STATS): %s", strerror(errno));
		if (expected != bufsize)
			err("getkerninfo(KINFO_DISK_STATS): wrong size");
		dk = disk_data;
		for (i = 0; i < (size / sizeof(*disk_data)); i++, dk++) {
			cur.dk_time[i] = dk->dk_time;
			cur.dk_seek[i] = dk->dk_seek;
			cur.dk_xfer[i] = dk->dk_xfers;
			cur.dk_sectors[i] = dk->dk_sectors;
		}
		getcpustats(&cur);
		getttystats(&cur);

		for (i = 0; i < dk_ndrive; i++) {
			if (!dr_select[i])
				continue;
#define X(fld) \
	tmp = cur.fld[i]; cur.fld[i] -= last.fld[i]; last.fld[i] = tmp
			X(dk_xfer);
			X(dk_seek);
			X(dk_sectors);
			X(dk_time);
		}
		tmp = cur.tk_nin;
		cur.tk_nin -= last.tk_nin;
		last.tk_nin = tmp;
		tmp = cur.tk_nout;
		cur.tk_nout -= last.tk_nout;
		last.tk_nout = tmp;
		etime = 0;
		for (i = 0; i < CPUSTATES; i++) {
			X(cp_time);
			etime += cur.cp_time[i];
		}
		if (etime == 0.0)
			etime = 1.0;
		etime /= (float)hz;
		(void)printf("%4.0f%5.0f",
		    cur.tk_nin / etime, cur.tk_nout / etime);
		dkstats();
		cpustats();
		(void)printf("\n");
		(void)fflush(stdout);

		if (reps >= 0 && --reps <= 0)
			break;
		(void)sleep(interval);
	}
	exit(0);
}
#undef X

/* ARGUSED */
void
phdr(notused)
	int notused;
{
	register int i;

	(void)printf("      tty");
	for (i = 0; i < dk_ndrive; i++)
		if (dr_select[i])
			(void)printf("          %3.3s ", dr_name[i]);
	(void)printf("         cpu\n tin tout");
	for (i = 0; i < dk_ndrive; i++)
		if (dr_select[i])
			(void)printf(" sps tps msps ");
	(void)printf(" us ni sy id\n");
}

void
dkstats()
{
	register int dn;
	double atime, itime, msps, secs, bytes, xtime;

	for (dn = 0; dn < dk_ndrive; ++dn) {
		if (!dr_select[dn])
			continue;
		secs = cur.dk_sectors[dn];	/* sectors xfer'd */
		(void)printf("%4.0f", secs / etime);
		bytes = secs * dk_secsize[dn];

		(void)printf("%4.0f", cur.dk_xfer[dn] / etime);

		if (dk_bpms[dn] && cur.dk_xfer[dn]) {
			atime = cur.dk_time[dn];	/* ticks disk busy */
			atime /= (float)hz;		/* ticks to seconds */
			xtime = bytes / dk_bpms[dn];	/* transfer time */
			itime = atime - xtime;		/* time not xfer'ing */
			if (itime < 0)
				msps = 0;
			else 
				msps = itime * 1000 / cur.dk_xfer[dn];
		} else
			msps = 0;
		(void)printf("%5.1f ", msps);
	}
}

void
cpustats()
{
	register int state;
	double time;

	time = 0;
	for (state = 0; state < CPUSTATES; ++state)
		time += cur.cp_time[state];
	for (state = 0; state < CPUSTATES; ++state)
		(void)printf("%3.0f",
		    100. * cur.cp_time[state] / (time ? time : 1));
}

void
usage()
{
	(void)fprintf(stderr,
"usage: iostat [-c count] [-M core] [-N system] [-w wait] [drives]\n");
	exit(1);
}

#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

void
#if __STDC__
err(const char *fmt, ...)
#else
err(fmt, va_alist)
	char *fmt;
        va_dcl
#endif
{
	va_list ap;
#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	(void)fprintf(stderr, "iostat: ");
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
	(void)fprintf(stderr, "\n");
	exit(1);
	/* NOTREACHED */
}
