/*	BSDI $Id: rstat_proc.c,v 1.3 1994/01/28 14:50:17 cp Exp $	*/

/* @(#)rstat_proc.c	2.2 88/08/01 4.0 RPCSRC */
#ifndef lint
static  char bsdid[] = "@(#)BSDI $Id: rstat_proc.c,v 1.3 1994/01/28 14:50:17 cp Exp $";
static  char sccsid[] = "@(#)rpc.rstatd.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

/*
 * rstat service:  built with rstat.x and derived from rpc.rstatd.c
 */
#include <sys/param.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/malloc.h>
#include <sys/signal.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/buf.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <paths.h>
#include <nlist.h>
#include <sys/kinfo.h>
#include <sys/sysinfo.h>
#include <sys/cpustats.h>
#include <sys/diskstats.h>
#include <sys/socket.h>
#include <sys/namei.h>
#include <sys/vmmeter.h>
#include <vm/vm.h>
#include <vm/vm_statistics.h>
#undef TRUE		/* XXX */
#undef FALSE		/* XXX */
#include <net/if.h>
#include <rpc/rpc.h>
#include "rstat.h"

#ifdef DEBUG
pid_t fork(void) { return 0; }	/* prevent RPC code from forking */
#endif

static struct nlist nl[] = {
#define	X_IFNET		0
	{ "_ifnet" },
#define X_LAST		1
	{ "" },
};

static struct sysinfo *sysinfo;	/* sysinfo structure */
static dk_cnt;			/* number of disk drives */

static int kmem;		/* /dev/kmem fd */
static int firstifnet, numintfs;/* chain of ethernet interfaces */
int stats_service();

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

int sincelastreq = 0;		/* number of alarms since last request */
#define CLOSEDOWN 20		/* how long to wait before exiting */
int closedown = CLOSEDOWN;

union {
    struct stats s1;
    struct statsswtch s2;
    struct statstime s3;
} stats_all;

void updatestat();
static stat_state = 0;
extern int errno;

void
stat_update()
{
    if (stat_state == 0) {
	    stat_state = 1;
	    setup();
	    signal(SIGALRM, updatestat);
    }
    sincelastreq = 0;
    if (stat_state == 1) 
	    updatestat();
}

statstime *
rstatproc_stats_3()
{
    stat_update();
    return(&stats_all.s3);
}

statsswtch *
rstatproc_stats_2()
{
    stat_update();
    return(&stats_all.s2);
}

stats *
rstatproc_stats_1()
{
    stat_update();
    return(&stats_all.s1);
}

u_int *
rstatproc_havedisk_3()
{
    static u_int have;

    stat_update();
    have = havedisk();
	return(&have);
}

u_int *
rstatproc_havedisk_2()
{
    return(rstatproc_havedisk_3());
}

u_int *
rstatproc_havedisk_1()
{
    return(rstatproc_havedisk_3());
}

/*
 * returns true if found any disks
 */
havedisk()
{
	return (dk_cnt != 0);
}

static void
setup_disks()
{
	int expected, bufsize;
	char *dk_names, *p;

	/*
	 * read disk names, init globals: dk_cnt
	 */
	expected = bufsize = getkerninfo(KINFO_DISK_NAMES, NULL, NULL, 0);
	if (bufsize < 0) {
		fprintf(stderr, "rstatd: Can't size KINFO_DISK_NAMES\n");
		exit(1);
	}
	dk_names = malloc(bufsize);
	if (getkerninfo(KINFO_DISK_NAMES, dk_names, &bufsize, 0) < 0) {
		fprintf(stderr, "rstatd: KINFO_DISK_NAMES: %s\n",
		    strerror(errno));
		exit(1);
	}
	if (expected != bufsize) {
		fprintf(stderr, "rstatd: KINFO_DISK_NAMES: %s\n",
		    strerror(errno));
		exit(1);
	}
	/* count number of drives */
	for (p = dk_names; p < dk_names + bufsize; p += strlen(p) + 1)
		dk_cnt++;
	free(dk_names);
}

static void
disk_stats(int *dk_xfer)
{
	int bufsize, i = 0;
	struct diskstats *dk;
	int expected = 0;
	struct diskstats *dk_stat = NULL;

	expected = bufsize = getkerninfo(KINFO_DISK_STATS, NULL, NULL, 0);
	if (bufsize < 0) {
		fprintf(stderr, "rstatd: Can't size KINFO_DISK_STATS\n");
		exit(1);
	}
	dk_stat = (struct diskstats *) malloc(bufsize);
	if (getkerninfo(KINFO_DISK_STATS, dk_stat, &bufsize, 0) < 0) {
		fprintf(stderr,"rstatd: KINFO_DISK_STATS: %s\n",
		    strerror(errno));
		exit(1);
	}
	if (expected != bufsize) {
		fprintf(stderr,"rstatd: KINFO_DISK_STATS: wrong size\n");
		exit(1);
	}

	dk = dk_stat;
	while (i < (bufsize / sizeof(*dk_stat)) && i < DK_NDRIVE) {
		dk_xfer[i] = dk->dk_xfers;
		i++, dk++;
	}
	free(dk_stat);
}

static void
setup_sysinfo()
{
	int bufsize, expected;

	/*
	 * System Info
	 */
	bufsize = getkerninfo(KINFO_SYSINFO, NULL, NULL, 0);
	if (bufsize < 0) {
		fprintf(stderr, "rstatd: can't size KINFO_SYSINFO\n");
		exit(1);
	}
	sysinfo = (struct sysinfo *) malloc(bufsize);
	if ((expected = getkerninfo(KINFO_SYSINFO, (char *)sysinfo,
	    &bufsize, 0)) < 0) {
		fprintf(stderr, "rstatd: KINFO_SYSINFO: %s\n",
		    strerror(errno));
		exit(1);
	}
	if (expected > bufsize || bufsize < sizeof(struct sysinfo)) {
		fprintf(stderr, "rstatd: KINFO_SYSINFO: wrong size\n");
		exit(1);
	}
}

static void
setup_networking()
{
	struct ifnet ifnet;
	int off;
	/*
	 * Networking setup
	 */
	nlist(_PATH_KERNEL, nl);
	if (nl[X_IFNET].n_value == 0) {
		fprintf(stderr, "rstatd: _ifnet missing from namelist\n");
		exit (1);
	}
	if ((kmem = open(_PATH_KMEM, 0)) < 0) {
		fprintf(stderr, "rstatd: can't open kmem\n");
		exit(1);
	}

	off = nl[X_IFNET].n_value;
	if (lseek(kmem, (long)off, 0) == -1) {
		fprintf(stderr, "rstatd: can't seek in kmem\n");
		exit(1);
	}
	if (read(kmem, (char *)&firstifnet, sizeof(int)) != sizeof (int)) {
		fprintf(stderr, "rstatd: can't read firstifnet from kmem\n");
		exit(1);
	}
	numintfs = 0;
	for (off = firstifnet; off;) {
		if (lseek(kmem, (long)off, 0) == -1) {
			fprintf(stderr, "rstatd: can't seek in kmem\n");
			exit(1);
		}
		if (read(kmem, (char *)&ifnet, sizeof ifnet) != sizeof ifnet) {
			fprintf(stderr, "rstatd: can't read ifnet from kmem\n");
			exit(1);
		}
		numintfs++;
		off = (int) ifnet.if_next;
	}
}

/*
 * read cpu stats
 */
static void
cpu_stats(int *cp)
{
	struct cpustats cpustats;
	int bufsize = sizeof(cpustats);

	if (getkerninfo(KINFO_CPU, &cpustats, &bufsize, 0) < 0) {
		fprintf(stderr, "rstatd: KINFO_CPU: %s", strerror(errno));
		exit(1);
	}
	if (bufsize != sizeof(cpustats)) {
		fprintf(stderr, "rstatd: KINFO_CPU: wrong size");
		exit(1);
	}
	bcopy((char *)cpustats.cp_time, (char *)cp, sizeof(cpustats.cp_time));
}

static void
vm_stats(struct vmmeter *sum)
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
		if (size < 0) {
			fprintf(stderr, "rstatd: Can't size KINFO_VM\n");
			exit(1);
		}
		ki_vm = malloc(size);
	}

	/*
	 * get vm stats and translate into ``native'' structures
	 */
	start = buf = ki_vm;
	bufsize = size;
        if (getkerninfo(KINFO_VM, buf, &bufsize, 0) <= 0) {
                fprintf(stderr, "rstatd: KINFO_VM: %s\n", strerror(errno));
                exit(1);
	}
	if (size != bufsize) {
                fprintf(stderr, "rstatd: KINFO_VM: wrong size\n");
                exit(1);
	}

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
	bcopy((char *)&cnt, (char *)sum, sizeof(cnt));
	sum->v_pgpgin = vm_stat.pageins;
	sum->v_pgpgout = vm_stat.pageouts;
}

setup()
{
	setup_disks();
	setup_sysinfo();
	setup_networking();
}

void
updatestat()
{
	int off, i, hz;
	struct vmmeter sum;
	struct ifnet ifnet;
	double avrun[3];
	struct timeval tm, btm;

#ifdef DEBUG
	fprintf(stderr, "entering updatestat\n");
#endif
	sincelastreq++;
	if (sincelastreq > closedown) {
		stat_state = 1;
		return;
	}
	stat_state = 2;
	hz = sysinfo->sys_phz ? sysinfo->sys_phz : sysinfo->sys_hz;
	btm = sysinfo->sys_boottime;
	stats_all.s2.boottime.tv_sec = btm.tv_sec;
	stats_all.s2.boottime.tv_usec = btm.tv_usec;

	cpu_stats(stats_all.s1.cp_time);

	(void)getloadavg(avrun, sizeof(avrun) / sizeof(avrun[0]));
	stats_all.s2.avenrun[0] = avrun[0] * RSTAT_FSCALE;
	stats_all.s2.avenrun[1] = avrun[1] * RSTAT_FSCALE;
	stats_all.s2.avenrun[2] = avrun[2] * RSTAT_FSCALE;

	vm_stats(&sum);
	stats_all.s1.v_pgpgin = sum.v_pgpgin;
	stats_all.s1.v_pgpgout = sum.v_pgpgout;
	stats_all.s1.v_pswpin = sum.v_pswpin;
	stats_all.s1.v_pswpout = sum.v_pswpout;
	stats_all.s1.v_intr = sum.v_intr;
	gettimeofday(&tm, (struct timezone *) 0);
	stats_all.s1.v_intr -= hz*(tm.tv_sec - btm.tv_sec) +
	    hz*(tm.tv_usec - btm.tv_usec)/1000000;
	stats_all.s2.v_swtch = sum.v_swtch;

	disk_stats(stats_all.s1.dk_xfer);

	stats_all.s1.if_ipackets = 0;
	stats_all.s1.if_opackets = 0;
	stats_all.s1.if_ierrors = 0;
	stats_all.s1.if_oerrors = 0;
	stats_all.s1.if_collisions = 0;
	for (off = firstifnet, i = 0; off && i < numintfs; i++) {
		if (lseek(kmem, (long)off, 0) == -1) {
			fprintf(stderr, "rstatd: can't seek in kmem\n");
			exit(1);
		}
		if (read(kmem, (char *)&ifnet, sizeof ifnet) != sizeof ifnet) {
			fprintf(stderr, "rstatd: can't read ifnet from kmem\n");
			exit(1);
		}
		stats_all.s1.if_ipackets += ifnet.if_ipackets;
		stats_all.s1.if_opackets += ifnet.if_opackets;
		stats_all.s1.if_ierrors += ifnet.if_ierrors;
		stats_all.s1.if_oerrors += ifnet.if_oerrors;
		stats_all.s1.if_collisions += ifnet.if_collisions;
		off = (int) ifnet.if_next;
	}
	gettimeofday((struct timeval *)&stats_all.s3.curtime,
		(struct timezone *) 0);
	alarm(1);
}
