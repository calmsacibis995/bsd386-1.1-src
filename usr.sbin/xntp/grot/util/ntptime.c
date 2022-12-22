/*
 * NTP test program
 *
 * This program tests to see if the NTP user interface routines
 * ntp_gettime() and ntp_adjtime() have been implemented in the kernel.
 * If so, each of these routines is called to display current timekeeping
 * data.
 *
 * For more information, see the README.kern file in the doc directory
 * of the xntp3 distribution.
 */
#include <stdio.h>
#include <ctype.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>

#include <sys/syscall.h>

#include "ntp_fp.h"
#include "ntp_unixtime.h"
#include "ntp_stdlib.h"

#ifndef	SYS_DECOSF1
#define BADCALL -1		/* this is supposed to be a bad syscall */
#endif
#include "ntp_timex.h"

#ifdef	KERNEL_PLL
#ifndef	SYS_ntp_adjtime
#define	SYS_ntp_adjtime NTP_SYSCALL_ADJ
#endif
#ifndef	SYS_ntp_gettime
#define	SYS_ntp_gettime NTP_SYSCALL_GET
#endif
#endif	/* KERNEL_PLL */

extern int sigvec	P((int, struct sigvec *, struct sigvec *));
void pll_trap		P((void));
extern int getopt_l	P((int, char **, char *));

static struct sigvec newsigsys;	/* new sigvec status */
static struct sigvec sigsys;	/* current sigvec status */
static int pll_control;		/* (0) daemon, (1) kernel loop */

static char* progname;
static char optargs[] = "ce:f:hm:o:rs:t:";

void
main(argc, argv)
	int argc;
	char *argv[];
{
	extern int optind;
	extern char *optarg;
	int status;
	struct ntptimeval ntv;
	struct timex ntx, _ntx;
	int	times[20];
	double ftemp;
	l_fp ts;
	int c;
	int errflg	= 0;
	int cost	= 0;
	int rawtime	= 0;

	ntx.mode = 0;
        progname = argv[0];
	while ((c = getopt_l(argc, argv, optargs)) != EOF) switch (c) {
		case 'c':
			cost++;
			break;
		case 'e':
			ntx.mode |= ADJ_ESTERROR;
			ntx.esterror = atoi(optarg);
			break;
		case 'f':
			ntx.mode |= ADJ_FREQUENCY;
			ntx.frequency = (int) (atof(optarg) * (1 << SHIFT_USEC));
			if (ntx.frequency < (-100 << SHIFT_USEC)
			||  ntx.frequency > ( 100 << SHIFT_USEC)) errflg++;
			break;
		case 'm':
			ntx.mode |= ADJ_MAXERROR;
			ntx.maxerror = atoi(optarg);
			break;
		case 'o':
			ntx.mode |= ADJ_OFFSET;
			ntx.offset = atoi(optarg);
			break;
		case 'r':
			rawtime++;
			break;
		case 's':
			ntx.mode |= ADJ_STATUS;
			ntx.status = atoi(optarg);
			if (ntx.status < 0 || ntx.status > 4) errflg++;
			break;
		case 't':
			ntx.mode |= ADJ_TIMECONST;
			ntx.time_constant = atoi(optarg);
			if (ntx.time_constant < 0 || ntx.time_constant > MAXTC)
				errflg++;
			break;
		default:
			errflg++;
	}
	if (errflg || (optind != argc)) {
		(void) fprintf(stderr,
			"usage: %s [-%s]\n\n\
	-c		display the time taken to call ntp_gettime (us)\n\
	-e esterror	estimate of the error (us)\n\
	-f frequency	Frequency error (-100 .. 100) (ppm)\n\
	-h		display this help info\n\
	-m maxerror	max possible error (us)\n\
	-o offset	current offset (ms)\n\
	-r		print the unix and NTP time raw\n\
	-s status	Set the status (0 .. 4)\n\
	-t timeconstant	log2 of PLL time constant (0 .. %d)\n",
		progname, optargs, MAXTC);
		exit(2);
	}


	/*
	 * Test to make sure the sigvec() works in case of invalid
	 * syscall codes.
	 */
	newsigsys.sv_handler = pll_trap;
	newsigsys.sv_mask = 0;
	newsigsys.sv_flags = 0;
	if (sigvec(SIGSYS, &newsigsys, &sigsys)) {
		perror("sigvec() fails to save SIGSYS trap");
		exit(1);
	}

#ifdef	BADCALL
	/*
	 * Make sure the trapcatcher works.
	 */
	pll_control = 1;
	(void)syscall(BADCALL, &ntv); /* dummy parameter f. ANSI compilers */
	if (pll_control)
		printf("sigvec() failed to catch an invalid syscall\n");
#endif

	if (cost) {
		for (c=0; c< sizeof times / sizeof times[0]; c++) {
			(void)ntp_gettime(&ntv);
			if (pll_control < 0) break;
			times[c] = ntv.time.tv_usec;
		}
		if (pll_control >= 0) {
			printf("[ usec %06d:", times[0]);
			for (c=1; c< sizeof times / sizeof times[0]; c++) printf(" %d", times[c] - times[c-1]);
			printf(" ]\n");
		}
	}
	(void)ntp_gettime(&ntv);
	ntx.mode = 0;	/* Ensure nothing is set */
	(void)ntp_adjtime(&_ntx);
	if (pll_control < 0) {
		printf("NTP user interface routines are not configured in this kernel.\n");
		goto lexit;
	}

	/*
	 * Fetch timekeeping data and display.
	 */
	if ((status = ntp_gettime(&ntv)) < 0)
		perror("ntp_gettime() call fails");
	else {
		printf("ntp_gettime() returns code %d\n", status);
		TVTOTS(&ntv.time, &ts);
		ts.l_uf += TS_ROUNDBIT;		/* guaranteed not to overflow */
		ts.l_ui += JAN_1970;
		ts.l_uf &= TS_MASK;
		printf("  time: %s, (.%06d)\n",
		    prettydate(&ts), ntv.time.tv_usec);
		printf("  confidence interval: %ld usec, estimated error: %ld usec\n",
		    ntv.maxerror, ntv.esterror);
		if (rawtime) printf("  ntptime=%x.%x unixtime=%x.%06d %s",
			ts.l_ui, ts.l_uf,
			ntv.time.tv_sec, ntv.time.tv_usec,
			ctime(&ntv.time.tv_sec));
	}
	if ((status = ntp_adjtime(&ntx)) < 0) perror((errno == EPERM) ? 
		">> Must be root to set kernel values\n>> ntp_adjtime() call fails" :
		">> ntp_adjtime() call fails");
	else {
		printf("ntp_adjtime() returns code %d\n", status);
		ftemp = ntx.frequency;
		ftemp /= (1 << SHIFT_USEC);
		printf("  mode: %02x, offset: %ld usec, frequency: %6.3f ppm,\n",
		    ntx.mode, ntx.offset, ftemp);
		printf("  confidence interval: %ld usec, estimated error: %ld usec,\n",
		    ntx.maxerror, ntx.esterror);
		printf("  status: %d, time constant: %ld, precision: %ld usec, tolerance: %ld usec\n",
		    ntx.status, ntx.time_constant, ntx.precision,
		    ntx.tolerance);
	}

	/*
	 * Put things back together the way we found them.
	 */
lexit:	if (sigvec(SIGSYS, &sigsys, (struct sigvec *)NULL)) {
		perror("sigvec() fails to restore SIGSYS trap");
		exit(1);
	}
	exit(0);
}

/*
 * pll1_trap - trap processor for undefined syscalls
 */
void
pll_trap()
{
	pll_control--;
}
