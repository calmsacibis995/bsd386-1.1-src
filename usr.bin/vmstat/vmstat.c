/*	BSDI	$Id: vmstat.c,v 1.7 1994/01/04 04:16:14 polk Exp $	*/

/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

/*-
 * Copyright (c) 1980, 1986, 1991 The Regents of the University of California.
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
"@(#) Copyright (c) 1980, 1986, 1991 The Regents of the University \
of California.\nAll rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)vmstat.c	5.31 (Berkeley) 7/2/91";
#endif /* not lint */

/*
 * COMPILE TIME OPTIONS:
 *
 *     -DBACKWARD_COMPATIBILTIY
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/signal.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <paths.h>

#include <sys/kinfo.h>
#include <sys/sysinfo.h>
#include <sys/diskstats.h>
#include <sys/cpustats.h>
#include <sys/namei.h>
#include <sys/vmmeter.h>
#include <vm/vm.h>
#include <vm/vm_statistics.h>

void
usage()
{
	(void)fprintf(stderr, "usage: vmstat %s %s [disks]\n",
	    "[-ms] [-c count] [-w wait]",
	    "[-N system] [-M core]");
	exit(1);
}


/*
 * statistics gathering data structures
 */
struct _disk {
	long	time[CPUSTATES];
	long	*xfer;
} cur, last;

/*
 * Space for KINFO_VM statistics
 */
struct	vmtotal total;
struct	vmmeter cnt, ocnt;
struct	vm_statistics vm_stat, ostat;
int	pagesize;
struct  nchstats nchstats;
int	num_kmemstats;
struct	kmemstats kmemstats[M_LAST];
int	num_buckets;
struct	kmembuckets buckets[MINBUCKET + 16];

/*
 * Space for KINFO_DISK statistics
 */
char	**dr_name;
int	*dr_select, dk_ndrive, ndrives;

/*
 * Space for KINFO_SYSINFO statistics
 */
int	hz;
time_t	boottime;

int	hdrcnt;
int	winlines = 20;
char	*nlistfile = _PATH_KERNEL;		/* XXX */

#define	VMSTAT	0x01
#define	SUMSTAT	0x02
#define	MEMSTAT	0x04

void	cpustats(), dkstats(), dovmstat();
void	domem(), dosum();
void	printhdr();
void	getcpustats();
void	getdiskstats();
void	getvmstats();

#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

void
#ifdef __STDC__
Perror(const char *fmt, ...)
#else
Perror(fmt, va_alist)
	char *fmt;
        va_dcl
#endif
{
	int error = errno;
	va_list ap;
#ifdef __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	(void)fprintf(stderr, "vmstat: ");
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
	if (error)
		(void)fprintf(stderr, "%s", strerror(error));
	(void)fprintf(stderr, "\n");
	exit(1);
}

void
#ifdef __STDC__
error(const char *fmt, ...)
#else
Perror(fmt, va_alist)
	char *fmt;
        va_dcl
#endif
{
	va_list ap;
#ifdef __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	(void)fprintf(stderr, "vmstat: ");
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
	(void)fprintf(stderr, "\n");
	exit(1);
}

int
main(argc, argv)
	register int argc;
	register char **argv;
{
	extern int optind;
	extern char *optarg;
	register int c, todo;
	u_int interval;
	int reps;
	char *kmem;
	struct sysinfo *sysinfo;
	int bufsize, expected;

	kmem = NULL;
	interval = reps = todo = 0;
	while ((c = getopt(argc, argv, "c:w:M:N:ms")) != EOF) {
		switch (c) {
		case 'm':
			todo |= MEMSTAT;
			break;
		case 's':
			todo |= SUMSTAT;
			break;
		case 'c':
			reps = atoi(optarg);
			break;
		case 'w':
			interval = atoi(optarg);
			break;
		case 'M':
			kmem = optarg;
			break;
		case 'N':
			nlistfile = optarg;
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (todo == 0)
		todo = VMSTAT;

	/*
	 * read sysinfo structure
	 */
	bufsize = getkerninfo(KINFO_SYSINFO, NULL, NULL, 0);
	if (bufsize < 0)
		Perror("getkerninfo(KINFO_SYSINFO size)");
	sysinfo = (struct sysinfo *) malloc(bufsize);
	if ((expected = getkerninfo(KINFO_SYSINFO, (char *)sysinfo,
	    &bufsize, 0)) < 0)
		Perror("getkerninfo(KINFO_SYSINFO)");
	if (expected > bufsize || bufsize < sizeof(struct sysinfo))
		error("getkerninfo(KINFO_SYSINFO): wrong size");
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
	boottime = sysinfo->sys_boottime.tv_sec;
	pagesize = sysinfo->sys_pagesize;
	free(sysinfo);

	if (todo & VMSTAT) {
		char **getdrivedata();
		struct winsize winsize;

		argv = getdrivedata(argv);
		winsize.ws_row = 0;
		(void) ioctl(STDOUT_FILENO, TIOCGWINSZ, (char *)&winsize);
		if (winsize.ws_row > 0)
			winlines = winsize.ws_row;

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
	} else if (reps)
		interval = 1;

	if (todo & MEMSTAT)
		domem();
	if (todo & SUMSTAT)
		dosum();
	if (todo & VMSTAT)
		dovmstat(interval, reps);
	return (0);
}

char **
getdrivedata(argv)
	char **argv;
{
	int bufsize, expected;
	register int i;
	register char **cp;
	char buf[30];
	char *disk_names, *p;

        /*
         * read disk names
         */
        expected = bufsize = getkerninfo(KINFO_DISK_NAMES, NULL, NULL, 0);
        if (bufsize < 0)
                Perror("getkerninfo(KINFO_DISK_NAMES)");
        disk_names = (char *) malloc(bufsize);
        if (getkerninfo(KINFO_DISK_NAMES, disk_names, &bufsize, 0) < 0)
                Perror("getkerninfo(KINFO_DISK_NAMES)");
	if (expected != bufsize)
                error("getkerninfo(KINFO_DISK_NAMES): wrong size");
	/*
	 * count number of drives
	 */
	for (p = disk_names; p < disk_names + bufsize; p += strlen(p) + 1)
		dk_ndrive++;

	dr_select = calloc((size_t)dk_ndrive, sizeof(int));
	dr_name = calloc((size_t)dk_ndrive, sizeof(char *));
	for (i = 0; i < dk_ndrive; i++)
		dr_name[i] = NULL;
	cur.xfer = calloc((size_t)dk_ndrive, sizeof(long));
	last.xfer = calloc((size_t)dk_ndrive, sizeof(long));


        /*
         * assign drive names to dr_name[]
         */
	for (p = disk_names, i = 0; *p; p += strlen(p) + 1)
		dr_name[i++] = p;

	for (i = 0; i < dk_ndrive; i++) {
		if (dr_name[i] == NULL) {
			(void)sprintf(buf, "dk%d", i);
			dr_name[i] = strdup(buf);
		}
	}

	/*
	 * Choose drives to be displayed.  Priority goes to (in order) drives
	 * supplied as arguments, default drives.  If everything isn't filled
	 * in and there are drives not taken care of, display the first few
	 * that fit.
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
			break;
		}
	}
	for (i = 0; i < dk_ndrive && ndrives < 4; i++) {
		char *defdrives[] =
			{ "wd0", "sd0", "wd1", "sd1", "wd2", "sd2", 0 };
		if (dr_select[i])
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
	return(argv);
}

long
getuptime()
{
	time_t uptime, now;

	(void)time(&now);
	uptime = now - boottime;
	if (uptime <= 0 || uptime > 60*60*24*365*10)	/* 10 years */
		error("unexpected uptime value: %d = %d - %d",
			uptime, now, boottime);
	return(uptime);
}

void
dovmstat(interval, reps)
	u_int interval;
	int reps;
{
	time_t uptime, halfuptime;
	void needhdr();

	uptime = getuptime();
	halfuptime = uptime / 2;
	(void)signal(SIGCONT, needhdr);

	for (hdrcnt = 1;;) {
		if (!--hdrcnt)
			printhdr();
		getcpustats(&cur);
		getdiskstats(&cur);
		getvmstats();

		(void)printf("%2d %1d %1d ",
		    total.t_rq, total.t_dw + total.t_pw, total.t_sw);
#define pgtok(a) ((a) * pagesize >> 10)
#define	rate(x)	(((x) + halfuptime) / uptime)	/* round */
		(void)printf("%5ld %5ld ",
		    pgtok(total.t_avm), pgtok(total.t_free));

		(void)printf("%4lu ", rate(vm_stat.faults - ostat.faults));
		(void)printf("%3lu ",
		    rate(vm_stat.reactivations - ostat.reactivations));
		(void)printf("%3lu ", rate(vm_stat.pageins - ostat.pageins));
		(void)printf("%3lu %3lu ",
		    rate(vm_stat.pageouts - ostat.pageouts), 0);

		(void)printf("%3lu ", rate(cnt.v_scan - ocnt.v_scan));
		dkstats();
		(void)printf("%4lu %4lu %3lu ",
		    rate(cnt.v_intr - ocnt.v_intr),
		    rate(cnt.v_syscall - ocnt.v_syscall),
		    rate(cnt.v_swtch - ocnt.v_swtch));
		cpustats();
		(void)printf("\n");
		(void)fflush(stdout);
		if (reps >= 0 && --reps <= 0)
			break;
		ocnt = cnt;
		ostat = vm_stat;
		uptime = interval;
		/*
		 * We round upward to avoid losing low-frequency events
		 * (i.e., >= 1 per interval but < 1 per second).
		 */
		halfuptime = (uptime + 1) / 2;
		(void)sleep(interval);
	}
}

void
printhdr()
{
	register int i;

	(void)printf(" procs   memory     page%*s", 20, "");
	if (ndrives > 1)
		(void)printf("disks %*s  faults      cpu\n",
		   ndrives * 4 - 6, "");
	else
		(void)printf("%*s  faults      cpu\n", ndrives * 4, "");

	(void)printf(" r b w   avm   fre  flt  re  pi  po  fr  sr ");

	for (i = 0; i < dk_ndrive; i++)
		if (dr_select[i])
			(void)printf("%c%c%c ", dr_name[i][0], dr_name[i][1],
			    dr_name[i][strlen(dr_name[i]) - 1]);
	(void)printf("  in   sy  cs us sy id\n");
	hdrcnt = winlines - 2;
}

/*
 * Force a header to be prepended to the next output.
 */
void
needhdr()
{

	hdrcnt = 1;
}

int
pct(top, bot)
	long top, bot;
{
	if (bot == 0)
		return(0);
	return((top * 100.0) / bot);
}

#define	PCT(top, bot) pct((long)(top), (long)(bot))

void
dosum()
{
	long nchtotal;

	getvmstats();

	(void)printf("%9u cpu context switches\n", cnt.v_swtch);
	(void)printf("%9u device interrupts\n", cnt.v_intr);
	(void)printf("%9u software interrupts\n", cnt.v_soft);
	(void)printf("%9u traps\n", cnt.v_trap);
	(void)printf("%9u system calls\n", cnt.v_syscall);

	(void)printf("%9u bytes per page\n", vm_stat.pagesize);
	(void)printf("%9u pages free\n", vm_stat.free_count);
	(void)printf("%9u pages active\n", vm_stat.active_count);
	(void)printf("%9u pages inactive\n", vm_stat.inactive_count);
	(void)printf("%9u pages wired down\n", vm_stat.wire_count);
	(void)printf("%9u zero-fill pages\n", vm_stat.zero_fill_count);
	(void)printf("%9u pages reactivated\n", vm_stat.reactivations);
	(void)printf("%9u pageins\n", vm_stat.pageins);
	(void)printf("%9u pageouts\n", vm_stat.pageouts);
	(void)printf("%9u VM faults\n", vm_stat.faults);
	(void)printf("%9u copy-on-write faults\n", vm_stat.cow_faults);
	(void)printf("%9u VM object cache lookups\n", vm_stat.lookups);
	(void)printf("%9u VM object hits\n", vm_stat.hits);

	nchtotal = nchstats.ncs_goodhits + nchstats.ncs_neghits +
	    nchstats.ncs_badhits + nchstats.ncs_falsehits +
	    nchstats.ncs_miss + nchstats.ncs_long;
	(void)printf("%9ld total name lookups\n", nchtotal);
	(void)printf(
	    "%9s cache hits (%d%% pos + %d%% neg) system %d%% per-process\n",
	    "", PCT(nchstats.ncs_goodhits, nchtotal),
	    PCT(nchstats.ncs_neghits, nchtotal),
	    PCT(nchstats.ncs_pass2, nchtotal));
	(void)printf("%9s deletions %d%%, falsehits %d%%, toolong %d%%\n", "",
	    PCT(nchstats.ncs_badhits, nchtotal),
	    PCT(nchstats.ncs_falsehits, nchtotal),
	    PCT(nchstats.ncs_long, nchtotal));
}

void
dkstats()
{
	register int dn, state;
	double etime;
	long tmp;

	for (dn = 0; dn < dk_ndrive; ++dn) {
		tmp = cur.xfer[dn];
		cur.xfer[dn] -= last.xfer[dn];
		last.xfer[dn] = tmp;
	}
	etime = 0;
	for (state = 0; state < CPUSTATES; ++state) {
		tmp = cur.time[state];
		cur.time[state] -= last.time[state];
		last.time[state] = tmp;
		etime += cur.time[state];
	}
	etime /= hz;
	if (etime == 0)
		etime = 1;
	for (dn = 0; dn < dk_ndrive; ++dn) {
		if (!dr_select[dn])
			continue;
		(void)printf("%3.0f ", cur.xfer[dn] / etime);
	}
}

void
cpustats()
{
	register int state;
	double pct, total;

	total = 0;
	for (state = 0; state < CPUSTATES; ++state)
		total += cur.time[state];
	if (total)
		pct = 100 / total;
	else
		pct = 0;
	(void)printf("%2.0f ",				/* user + nice */
	    (cur.time[CP_USER] + cur.time[CP_NICE]) * pct);
	(void)printf("%2.0f ", cur.time[CP_SYS] * pct);	/* system */
	(void)printf("%2.0f", cur.time[CP_IDLE] * pct);	/* idle */
}

/*
 * These names are defined in <sys/malloc.h>.
 */
char *kmemnames[] = INITKMEMNAMES;

void
domem()
{
	register struct kmembuckets *kp;
	register struct kmemstats *ks;
	register int i;
	int size;
	long totuse = 0, totfree = 0, totreq = 0;

	getvmstats();

	(void)printf("Memory statistics by bucket size\n");
	(void)printf(
	    "    Size   In Use   Free   Requests  HighWater  Couldfree\n");
	for (i = MINBUCKET, kp = &buckets[i]; i < MINBUCKET + 16; i++, kp++) {
		if (kp->kb_calls == 0)
			continue;
		size = 1 << i;
		(void)printf("%8d %8ld %6ld %10ld %7ld %10ld\n", size, 
			kp->kb_total - kp->kb_totalfree,
			kp->kb_totalfree, kp->kb_calls,
			kp->kb_highwat, kp->kb_couldfree);
		totfree += size * kp->kb_totalfree;
	}

	(void)printf("\nMemory statistics by type\n");
	(void)printf(
"      Type  In Use  MemUse   HighUse  Limit Requests  TypeLimit KernLimit\n");
	for (i = 0, ks = &kmemstats[0]; i < M_LAST; i++, ks++) {
		if (ks->ks_calls == 0)
			continue;
		(void)printf("%10s %6ld %7ldK %8ldK %5ldK %8ld %6u %9u\n",
		    kmemnames[i] ? kmemnames[i] : "undefined",
		    ks->ks_inuse, (ks->ks_memuse + 1023) / 1024,
		    (ks->ks_maxused + 1023) / 1024,
		    (ks->ks_limit + 1023) / 1024, ks->ks_calls,
		    ks->ks_limblocks, ks->ks_mapblocks);
		totuse += ks->ks_memuse;
		totreq += ks->ks_calls;
	}
	(void)printf("\nMemory Totals:  In Use    Free    Requests\n");
	(void)printf("              %7ldK %6ldK    %8ld\n",
	     (totuse + 1023) / 1024, (totfree + 1023) / 1024, totreq);
}

/*
 * read cpu structure
 */
void
getcpustats(struct _disk *dp)
{
        struct cpustats cpustats;
        int bufsize = sizeof(cpustats);

        if (getkerninfo(KINFO_CPU, &cpustats, &bufsize, 0) < 0)
                Perror("getkerninfo(KINFO_CPU)");
	if (bufsize != sizeof(cpustats))
                error("getkerninfo(KINFO_CPU): wrong size");
        bcopy(cpustats.cp_time, dp->time, sizeof(long) * CPUSTATES);
}

/*
 * get disk stats
 */
void
getdiskstats(struct _disk *dp)
{
	static int size = 0;
	static struct diskstats *diskstats;
        int i, bufsize;

	/*
	 * Only allocate buffer once
	 */
	if (size == 0) {
		size = getkerninfo(KINFO_DISK_STATS, NULL, NULL, 0);
		if (size < 0)
			Perror("getkerninfo(KINFO_DISK_STATS)");
		diskstats = (struct diskstats *) malloc(size);
	}

	/*
	 * get disk stats and translate into struct _disk
	 */
	bufsize = size;
        if (getkerninfo(KINFO_DISK_STATS, diskstats, &bufsize, 0) < 0)
                Perror("getkerninfo(KINFO_DISK_STATS)");
	if (bufsize != size)
                error("getkerninfo(KINFO_DISK_STATS): wrong size");
	for (i = 0; i < bufsize / sizeof(*diskstats); i++)
		dp->xfer[i] = diskstats[i].dk_xfers;
}

/*
 * get vm stats
 */
void
getvmstats()
{
	static int size = 0;
	static char *ki_vm;
	char *buf, *start;
        int bufsize;

	/*
	 * Only allocate buffer once
	 */
	if (size == 0) {
		size = getkerninfo(KINFO_VM, NULL, NULL, 0);
		if (size < 0)
			Perror("getkerninfo(KINFO_VM)");
		ki_vm = malloc(size);
	}

	/*
	 * get vm stats and translate into ``native'' structures
	 */
	start = buf = ki_vm;
	bufsize = size;
        if (getkerninfo(KINFO_VM, buf, &bufsize, 0) <= 0)
                Perror("getkerninfo(KINFO_VM)");
	if (size != bufsize)
                error("getkerninfo(KINFO_VM): wrong size");

#define CPSTRUCT(src,dst)					\
	do {							\
		bcopy(src,&(dst),sizeof(dst));			\
		src += sizeof(dst);				\
	} while(0)

	CPSTRUCT(buf,total);
	CPSTRUCT(buf,cnt);
	CPSTRUCT(buf,vm_stat);
	CPSTRUCT(buf,nchstats);
	CPSTRUCT(buf,num_kmemstats);
	CPSTRUCT(buf,kmemstats);
	CPSTRUCT(buf,num_buckets);
	CPSTRUCT(buf,buckets);

#undef	CPSTRUCT
        return;
}
