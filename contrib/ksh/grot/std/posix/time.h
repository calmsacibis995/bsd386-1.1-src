/*
 * Replacement for BSD <sys/time.h>
 * because Ultrix screws it up.
 */
/* $Id: time.h,v 1.2 1993/12/21 02:20:41 polk Exp $ */

struct timeval {
	long tv_sec;		/* time_t */
	long tv_usec;		/* microsex */
};

struct timezone {
	int tz_minuteswest;	/* of Greenwinch */
	int tz_dsttime;		/* type of dst correction to apply */
};
