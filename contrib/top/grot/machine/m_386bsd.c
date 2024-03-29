/*
 * top - a top users display for Unix
 *
 * SYNOPSIS:  For a 386BSD system
 *	      Note memory statistisc and process sizes could be wrong,
 *	      by ps gets them wrong too...
 *
 * DESCRIPTION:
 * This is the machine-dependent module for BSD4.3  NET/2 386bsd
 * Works for:
 *	i386 PC
 *
 * LIBS: -lutil
 *
 * AUTHOR:  Steve Hocking (adapted from Christos Zoulas <christos@ee.cornell.edu>)
 *	    Updated by Andrew Herbert <andrew@werple.apana.org.au>
 */

#include <sys/types.h>
#include <sys/signal.h>
#include <sys/param.h>

#include <stdio.h>
#include <nlist.h>
#include <math.h>
#include <kvm.h>
#include <sys/errno.h>
#include <sys/kinfo.h>
#include <sys/kinfo_proc.h>
#ifdef notyet
#define time __time
#define hz __hz
#include <sys/kernel.h>
#undef time
#undef hz
#endif
#include <sys/dir.h>
#include <sys/dkstat.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/vmmeter.h>


/* #define PATCHED_KVM		/* YOU PROBABLY DON'T NEED THIS DEFINED! */
				/* define this if you have a kvm.c */
				/* that has been patched not to freshly */
				/* alloc an array of procs each time */
				/* kvm_getprocs is called, but to do a */
				/* realloc when the no. of processes inc. */

/* #define TOTAL_WORKING */	/* Uncomment when the total structure in */
				/* the kernel actually works */
#define DOSWAP

#include "top.h"
#include "machine.h"

#define VMUNIX	"/386bsd"
#define KMEM	"/dev/kmem"
#define MEM	"/dev/mem"
#ifdef DOSWAP
#define SWAP	"/dev/drum"
#endif

typedef struct _kinfo {
        struct proc ki_p;      /* proc structure */
        struct eproc ki_e;     /* extra stuff */
} KINFO;

/* get_process_info passes back a handle.  This is what it looks like: */

struct handle
{
    KINFO **next_proc;	/* points to next valid proc pointer */
    int remaining;		/* number of pointers remaining */
};

/* declarations for load_avg */
#include "loadavg.h"

#define PP(pp, field) ((pp)->ki_p . field)
#define EP(pp, field) ((pp)->ki_e . field)
#define VP(pp, field) ((pp)->ki_e.e_vm . field)

/* define what weighted cpu is.  */
#define weighted_cpu(pct, pp) (PP((pp), p_time) == 0 ? 0.0 : \
			 ((pct) / (1.0 - exp(PP((pp), p_time) * logcpu))))

/* what we consider to be process size: */
#define PROCSIZE(pp) (VP((pp), vm_tsize) + VP((pp), vm_dsize) + VP((pp), vm_ssize))

/* definitions for indices in the nlist array */
#define X_CCPU		0
#define X_CP_TIME	1
#define X_HZ		2
#define X_AVENRUN	3
#define X_TOTAL		4
#define X_PG_FREE	5
#define X_PG_ACTIVE	6
#define X_PG_INACTIVE	7
#define X_PG_WIRED	8

static struct nlist nlst[] = {
    { "_ccpu" },		/* 0 */
    { "_cp_time" },		/* 1 */
    { "_hz" },			/* 2 */
    { "_averunnable" },		/* 3 */
    { "_total"},		/* 4 */
    { "_vm_page_free_count" },	/* 5 */
    { "_vm_page_active_count" },	/* 6 */
    { "_vm_page_inactive_count" },/* 7 */
    { "_vm_page_wire_count" },	/* 8 */
    { 0 }
};

/*
 *  These definitions control the format of the per-process area
 */

static char header[] =
  "  PID X        PRI NICE   SIZE   RES STATE   TIME   WCPU    CPU COMMAND";
/* 0123456   -- field to fill in starts at header+6 */
#define UNAME_START 6

#define Proc_format \
	"%5d %-8.8s %3d %4d%6dK %4dK %-5s%4d:%02d %5.2f%% %5.2f%% %.14s"


/* process state names for the "STATE" column of the display */
/* the extra nulls in the string "run" are for adding a slash and
   the processor number when needed */

char *state_abbrev[] =
{
    "", "sleep", "WAIT", "run\0\0\0", "start", "zomb", "stop"
};


/* values that we stash away in _init and use in later routines */

static double logcpu;

/* these are retrieved from the kernel in _init */

static          long hz;
static load_avg  ccpu;
static          int  ncpu = 0;

/* these are offsets obtained via nlist and used in the get_ functions */

static unsigned long cp_time_offset;
static unsigned long avenrun_offset;

/* these are for calculating cpu state percentages */

static long cp_time[CPUSTATES];
static long cp_old[CPUSTATES];
static long cp_diff[CPUSTATES];

/* these are for detailing the process states */

int process_states[7];
char *procstatenames[] = {
    "", " sleeping, ", " ABANDONED, ", " running, ", " starting, ",
    " zombie, ", " stopped, ",
    NULL
};

/* these are for detailing the cpu states */

int cpu_states[4];
char *cpustatenames[] = {
    "user", "nice", "system", "idle", NULL
};

/* these are for detailing the memory statistics */

int memory_stats[8];
char *memorynames[] = {
#ifdef	TOTAL_WORKING
    "Real: ", "K/", "K ", "Virt: ", "K/",
    "K ", "Free: ", "K", NULL
#else
    " Free: ", "K ", " Active: ", "K ", " Inactive: ",
    "K ", " Wired: ", "K ", NULL
#endif
};

/* these are for keeping track of the proc array */

static int bytes;
static int nproc;
static int onproc = -1;
static int pref_len;
static KINFO *pbase;
static KINFO **pref;

/* these are for getting the memory statistics */

static int pageshift;		/* log base 2 of the pagesize */

/* define pagetok in terms of pageshift */

#define pagetok(size) ((size) << pageshift)

/* useful externals */
long percentages();

machine_init(statics)

struct statics *statics;

{
    register int i = 0;
    register int pagesize;
    char buf[1024];

#if 0		/* some funny stuff going on here */
    if (kvm_openfiles(NULL, NULL, NULL) == -1);
	return -1;
#else
    kvm_openfiles(VMUNIX, NULL, NULL);
#endif

    /* get the list of symbols we want to access in the kernel */
    (void) kvm_nlist(nlst);
    if (nlst[0].n_type == 0)
    {
	fprintf(stderr, "top: nlist failed\n");
	return(-1);
    }

    /* make sure they were all found */
    if (i > 0 && check_nlist(nlst) > 0)
    {
	return(-1);
    }

    /* get the symbol values out of kmem */
    (void) getkval(nlst[X_HZ].n_value,     (int *)(&hz),	sizeof(hz),
	    nlst[X_HZ].n_name);
    (void) getkval(nlst[X_CCPU].n_value,   (int *)(&ccpu),	sizeof(ccpu),
	    nlst[X_CCPU].n_name);

    /* stash away certain offsets for later use */
    cp_time_offset = nlst[X_CP_TIME].n_value;
    avenrun_offset = nlst[X_AVENRUN].n_value;

    /* this is used in calculating WCPU -- calculate it ahead of time */
    logcpu = log(loaddouble(ccpu));

    pbase = NULL;
    pref = NULL;
    nproc = 0;
    onproc = -1;
    /* get the page size with "getpagesize" and calculate pageshift from it */
    pagesize = getpagesize();
    pageshift = 0;
    while (pagesize > 1)
    {
	pageshift++;
	pagesize >>= 1;
    }

    /* we only need the amount of log(2)1024 for our conversion */
    pageshift -= LOG1024;

    /* fill in the statics information */
    statics->procstate_names = procstatenames;
    statics->cpustate_names = cpustatenames;
    statics->memory_names = memorynames;

    /* all done! */
    return(0);
}

char *format_header(uname_field)

register char *uname_field;

{
    register char *ptr;

    ptr = header + UNAME_START;
    while (*uname_field != '\0')
    {
	*ptr++ = *uname_field++;
    }

    return(header);
}

get_system_info(si)

struct system_info *si;

{
    long total;
    load_avg avenrun[3];

    /* get the cp_time array */
    (void) getkval(cp_time_offset, (int *)cp_time, sizeof(cp_time),
		   "_cp_time");
    (void) getkval(avenrun_offset, (int *)avenrun, sizeof(avenrun),
		   "_avenrun");

    /* convert load averages to doubles */
    {
	register int i;
	register double *infoloadp;
	load_avg *avenrunp;

#ifdef KINFO_LOADAVG
	struct loadavg sysload;
	int size;
	getkerninfo(KINFO_LOADAVG, &sysload, &size, 0);
#endif

	infoloadp = si->load_avg;
	avenrunp = avenrun;
	for (i = 0; i < 3; i++)
	{
#ifdef KINFO_LOADAVG
	    *infoloadp++ = ((double) sysload.ldavg[i]) / sysload.fscale;
#endif
	    *infoloadp++ = loaddouble(*avenrunp++);
	}
    }

    /* convert cp_time counts to percentages */
    total = percentages(CPUSTATES, cpu_states, cp_time, cp_old, cp_diff);

    /* sum memory statistics */
    {
#ifdef TOTAL_WORKING
	static struct vmtotal total;
	int size;

	/* get total -- systemwide main memory usage structure */
#ifdef KINFO_METER
	getkerninfo(KINFO_METER, &total, &size, 0);
#else
	(void) getkval(nlst[X_TOTAL].n_value, (int *)(&total), sizeof(total),
		nlst[X_TOTAL].n_name);
#endif
	/* convert memory stats to Kbytes */
	memory_stats[0] = -1;
	memory_stats[1] = pagetok(total.t_arm);
	memory_stats[2] = pagetok(total.t_rm);
	memory_stats[3] = -1;
	memory_stats[4] = pagetok(total.t_avm);
	memory_stats[5] = pagetok(total.t_vm);
	memory_stats[6] = -1;
	memory_stats[7] = pagetok(total.t_free);
#else
        static int free, active, inactive, wired;

	(void) getkval(nlst[X_PG_FREE].n_value, (int *)(&free), sizeof(free),
                nlst[X_PG_FREE].n_name);
	(void) getkval(nlst[X_PG_ACTIVE].n_value, (int *)(&active), sizeof(active),
                nlst[X_PG_ACTIVE].n_name);
	(void) getkval(nlst[X_PG_INACTIVE].n_value, (int *)(&inactive), sizeof(inactive),
                nlst[X_PG_INACTIVE].n_name);
	(void) getkval(nlst[X_PG_WIRED].n_value, (int *)(&wired), sizeof(wired),
                nlst[X_PG_WIRED].n_name);
	memory_stats[0] = -1;
	memory_stats[1] = pagetok(free);
	memory_stats[2] = -1;
	memory_stats[3] = pagetok(active);
	memory_stats[4] = -1;
	memory_stats[5] = pagetok(inactive);
	memory_stats[6] = -1;
	memory_stats[7] = pagetok(wired);
#endif
    }

    /* set arrays and strings */
    si->cpustates = cpu_states;
    si->memory = memory_stats;
    si->last_pid = -1;
}

static struct handle handle;

caddr_t get_process_info(si, sel, compare)

struct system_info *si;
struct process_select *sel;
int (*compare)();

{
    register int i;
    register int total_procs;
    register int active_procs;
    register KINFO **prefp;
    KINFO *pp;
    struct proc *pr;
    struct eproc *epr;

    /* these are copied out of sel for speed */
    int show_idle;
    int show_system;
    int show_uid;
    int show_command;

    
    nproc = kvm_getprocs(KINFO_PROC_ALL, 0);
    if (nproc > onproc)
    {
	pref = (KINFO **) realloc(pref, sizeof(KINFO *)
		* nproc);
        pbase = (KINFO *) realloc(pbase, sizeof(KINFO)
                * (nproc + 2));
        onproc = nproc;
    }
    if (pref == NULL || pbase == NULL) {
	(void) fprintf(stderr, "top: Out of memory.\n");
	quit(23);
    }
    /* get a pointer to the states summary array */
    si->procstates = process_states;

    /* set up flags which define what we are going to select */
    show_idle = sel->idle;
    show_system = sel->system;
    show_uid = sel->uid != -1;
    show_command = sel->command != NULL;

    /* count up process states and get pointers to interesting procs */
    total_procs = 0;
    active_procs = 0;
    memset((char *)process_states, 0, sizeof(process_states));
    prefp = pref;
    for (pp = pbase, i = 0; pr = kvm_nextproc(); pp++, i++)
    {
	/*
	 *  Place pointers to each valid proc structure in pref[].
	 *  Process slots that are actually in use have a non-zero
	 *  status field.  Processes with SSYS set are system
	 *  processes---these get ignored unless show_sysprocs is set.
	 */
        epr = kvm_geteproc(pr);
        pp->ki_p = *pr;
        pp->ki_e = *epr;
	if (PP(pp, p_stat) != 0 &&
	    (show_system || ((PP(pp, p_flag) & SSYS) == 0)))
	{
	    total_procs++;
	    process_states[PP(pp, p_stat)]++;
	    if ((PP(pp, p_stat) != SZOMB) &&
		(show_idle || (PP(pp, p_pctcpu) != 0) || 
		 (PP(pp, p_stat) == SRUN)) &&
		(!show_uid || EP(pp, e_pcred.p_ruid) == (uid_t)sel->uid))
	    {
		*prefp++ = pp;
		active_procs++;
	    }
	}
    }

    /* if requested, sort the "interesting" processes */
    if (compare != NULL)
    {
	qsort((char *)pref, active_procs, sizeof(KINFO *), compare);
    }

    /* remember active and total counts */
    si->p_total = total_procs;
    si->p_active = pref_len = active_procs;

    /* pass back a handle */
    handle.next_proc = pref;
    handle.remaining = active_procs;
#ifndef PATCHED_KVM
    kvm_freeprocs();
#endif
    return((caddr_t)&handle);
}

char fmt[128];		/* static area where result is built */

char *format_next_process(handle, get_userid)

caddr_t handle;
char *(*get_userid)();

{
    register KINFO *pp;
    register long cputime;
    register double pct;
    int where;
    struct handle *hp;

    /* find and remember the next proc structure */
    hp = (struct handle *)handle;
    pp = *(hp->next_proc++);
    hp->remaining--;
    

    /* get the process's user struct and set cputime */
    cputime = PP(pp, p_utime.tv_sec) + PP(pp, p_stime.tv_sec);

    /* calculate the base for cpu percentages */
    pct = pctdouble(PP(pp, p_pctcpu));

    /* format this entry */
    sprintf(fmt,
	    Proc_format,
	    PP(pp, p_pid),
	    (*get_userid)(EP(pp, e_pcred.p_ruid)),
	    PP(pp, p_pri) - PZERO,
	    PP(pp, p_nice) - NZERO,
	    pagetok(PROCSIZE(pp)),
#ifdef notyet
	    pagetok(VP(pp, vm_rssize)),
#else
            pagetok(pp->ki_e.e_vm.vm_pmap.pm_stats.resident_count),
#endif
	    state_abbrev[PP(pp, p_stat)],
	    cputime / 60l,
	    cputime % 60l,
	    100.0 * weighted_cpu(pct, pp),
	    100.0 * pct,
	    printable(PP(pp, p_comm)));

    /* return the result */
    return(fmt);
}


/*
 * check_nlist(nlst) - checks the nlist to see if any symbols were not
 *		found.  For every symbol that was not found, a one-line
 *		message is printed to stderr.  The routine returns the
 *		number of symbols NOT found.
 */

int check_nlist(nlst)

register struct nlist *nlst;

{
    register int i;

    /* check to see if we got ALL the symbols we requested */
    /* this will write one line to stderr for every symbol not found */

    i = 0;
    while (nlst->n_name != NULL)
    {
	if (nlst->n_type == 0)
	{
	    /* this one wasn't found */
	    fprintf(stderr, "kernel: no symbol named `%s'\n", nlst->n_name);
	    i = 1;
	}
	nlst++;
    }

    return(i);
}


/*
 *  getkval(offset, ptr, size, refstr) - get a value out of the kernel.
 *	"offset" is the byte offset into the kernel for the desired value,
 *  	"ptr" points to a buffer into which the value is retrieved,
 *  	"size" is the size of the buffer (and the object to retrieve),
 *  	"refstr" is a reference string used when printing error meessages,
 *	    if "refstr" starts with a '!', then a failure on read will not
 *  	    be fatal (this may seem like a silly way to do things, but I
 *  	    really didn't want the overhead of another argument).
 *  	
 */

getkval(offset, ptr, size, refstr)

unsigned long offset;
int *ptr;
int size;
char *refstr;

{
    if (kvm_read((void *) offset, (void *) ptr, size) != size)
    {
	if (*refstr == '!')
	{
	    return(0);
	}
	else
	{
	    fprintf(stderr, "top: kvm_read for %s: %s\n",
		refstr, strerror(errno));
	    quit(23);
	}
    }
    return(1);
}
    
/* comparison routine for qsort */

/*
 *  proc_compare - comparison function for "qsort"
 *	Compares the resource consumption of two processes using five
 *  	distinct keys.  The keys (in descending order of importance) are:
 *  	percent cpu, cpu ticks, state, resident set size, total virtual
 *  	memory usage.  The process states are ordered as follows (from least
 *  	to most important):  WAIT, zombie, sleep, stop, start, run.  The
 *  	array declaration below maps a process state index into a number
 *  	that reflects this ordering.
 */

static unsigned char sorted_state[] =
{
    0,	/* not used		*/
    3,	/* sleep		*/
    1,	/* ABANDONED (WAIT)	*/
    6,	/* run			*/
    5,	/* start		*/
    2,	/* zombie		*/
    4	/* stop			*/
};
 
proc_compare(pp1, pp2)

KINFO **pp1;
KINFO **pp2;

{
    register KINFO *p1;
    register KINFO *p2;
    register int result;
    register pctcpu lresult;

    /* remove one level of indirection */
    p1 = *pp1;
    p2 = *pp2;

    /* compare percent cpu (pctcpu) */
    if ((lresult = PP(p2, p_pctcpu) - PP(p1, p_pctcpu)) == 0)
    {
	/* use cpticks to break the tie */
	if ((result = PP(p2, p_cpticks) - PP(p1, p_cpticks)) == 0)
	{
	    /* use process state to break the tie */
	    if ((result = sorted_state[PP(p2, p_stat)] -
			  sorted_state[PP(p1, p_stat)])  == 0)
	    {
		/* use priority to break the tie */
		if ((result = PP(p2, p_pri) - PP(p1, p_pri)) == 0)
		{
		    /* use resident set size (rssize) to break the tie */
		    if ((result = VP(p2, vm_rssize) - VP(p1, vm_rssize)) == 0)
		    {
			/* use total memory to break the tie */
			result = PROCSIZE(p2) - PROCSIZE(p1);
		    }
		}
	    }
	}
    }
    else
    {
	result = lresult < 0 ? -1 : 1;
    }

    return(result);
}

/*
 * proc_owner(pid) - returns the uid that owns process "pid", or -1 if
 *		the process does not exist.
 *		It is EXTREMLY IMPORTANT that this function work correctly.
 *		If top runs setuid root (as in SVR4), then this function
 *		is the only thing that stands in the way of a serious
 *		security problem.  It validates requests for the "kill"
 *		and "renice" commands.
 */

int proc_owner(pid)

int pid;

{
    register int cnt;
    register KINFO **prefp;
    register KINFO *pp;

    prefp = pref;
    cnt = pref_len;
    while (--cnt >= 0)
    {
	pp = *prefp++;
	if (PP(pp, p_pid) == (pid_t)pid)
	{
	    return((int)EP(pp, e_pcred.p_ruid));
	}
    }
    return(-1);
}
