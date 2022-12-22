/*
 * sys/times.h: POSIX times()
 */
/* $Id: times.h,v 1.2 1993/12/21 02:21:13 polk Exp $ */

#if ! _TIMES_H
#define	_TIMES_H 1

#include <time.h>		/* defines CLK_TCK */

#if __STDC__
#define	ARGS(args)	args
#else
#define	ARGS(args)	()
#endif

struct tms {
	clock_t	tms_utime, tms_stime;
	clock_t	tms_cutime, tms_cstime;
};

#if _V7
#define times times_
#endif

clock_t	times ARGS((struct tms *tmsp));

#endif

