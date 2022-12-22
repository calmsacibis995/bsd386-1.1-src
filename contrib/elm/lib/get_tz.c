
static char rcsid[] = "@(#)Id: get_tz.c,v 5.1 1993/08/10 18:56:53 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.1 $   $State: Exp $
 *
 * 			Copyright (c) 1992, 1993 USENET Community Trust
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 *******************************************************************************
 * $Log: get_tz.c,v $
 * Revision 1.1  1994/01/14  01:34:57  cp
 * 2.4.23
 *
 * Revision 5.1  1993/08/10  18:56:53  syd
 * Initial Checkin
 *
 *
 ******************************************************************************/

/*
 * get_tz - Site-specific timezone handling.
 *
 * get_tz_mins(tm) - Return timezone adjustment in minutes west of GMT.
 * get_tz_name(tm) - Return timezone name.
 *
 * These procedures return timezone infomation for the time specified by "tm".
 * If "tm" is NULL, then the local, current timezone info are returned.
 *
 * On some systems, regardless of the "tm" value, the local timezone
 * values are returned.
 *
 * On some systems, when passing a non-NULL "tm" value, a call to "tzset()"
 * must be performed prior to invoking these routines to obtain proper
 * timezone information.  Note that some systems will implicitly call
 * "tzset()" through other routines, such as "localtime()".  On such
 * systems an explicit "tzset()" is not required if the "tm" value was
 * obtained through a routine that does the implicit setup.
 *
 * The task of discovering timezone info is a horrid mess because so many
 * systems have different notions about how to do it.  The goal of these
 * routines is to encapsulate the system dependancies here.  Two definitions,
 * TZMINS_USE_xxxxxx and TZNAME_USE_xxxxxx must be enabled as appropriate
 * for this system.  Exactly _one_ definition from each group must be
 * specified.  The available choices are:
 *
 * TZMINS_USE_xxxxxx specifies how to get timezone offset.
 *
 *	TZMINS_USE_TM_TZADJ    use (struct tm*)->tm_tzadj
 *	TZMINS_USE_TM_GMTOFF   use (struct tm*)->tm_gmtoff
 *	TZMINS_USE_TZAZ_GLOBAL use "timezone, altzone" externals
 *	TZMINS_USE_TZ_GLOBAL   use "timezone" external
 *	TZMINS_USE_FTIME       use ftime() function
 *	TZMINS_USE_TIMEOFDAY   use gettimeofday() function
 *
 * TZNAME_USE_xxxxxx specifies how to get timezone name.
 *
 *	TZNAME_USE_TM_NAME     use (struct tm *)->tm_name
 *	TZNAME_USE_TM_ZONE     use (struct tm *)->tm_zone
 *	TZNAME_USE_TZNAME      use "tzname[]" external
 *	TZNAME_USE_TIMEZONE    use timezone() function
 *
 * The TZMINS_HANDLING and TZNAME_HANDLING definitions are just used
 * to verify the configurations were setup correctly.  They force
 * compiler warnings and/or errors in the event of a configuration problem.
 */

#include "defs.h"

#ifdef I_TIME
#  include <time.h>
#endif
#ifdef I_SYSTIME
#  include <sys/time.h>
#endif
#ifdef TZMINS_USE_FTIME
#  include <sys/timeb.h>
#endif

#ifndef	_POSIX_SOURCE
extern struct tm *localtime();
extern time_t	  time();
#endif

/****************************************************************************/

int get_tz_mins(tm)
struct tm *tm;
{

	if (tm == 0) {
		time_t t;
		(void) time(&t);
		tm = localtime(&t);
	}

#ifdef TZMINS_USE_TM_TZADJ
#define TZMINS_HANDLING 1
	/*
	 * This system maintains the timezone offset in the (struct tm)
	 * as a number of _seconds_ west of GMT.
	 */
	return (int)(tm->tm_tzadj / 60);
#endif

#ifdef TZMINS_USE_TM_GMTOFF
#define TZMINS_HANDLING 2
	/*
	 * This system maintains the timezone offset in the (struct tm)
	 * as a number of _seconds_ _east_ of GMT.  Since this is an
	 * easterly pointing offset, we need to flip the sign to go the
	 * other direction.
	 */
	return (int)(-tm->tm_gmtoff / 60);
#endif

#ifdef TZMINS_USE_TZAZ_GLOBAL
#define TZMINS_HANDLING 3
	/*
	 * This system maintains timezone offsets in global variables
	 * as a number of _seconds_ west of GMT.  There are two globals,
	 * one for when DST is in effect and one for when it is not,
	 * and we need to select the correct one.
	 */
	{
		extern long altzone, timezone;
		return (int)((tm->tm_isdst ? altzone : timezone) / 60);
	}
#endif

#ifdef TZMINS_USE_TZ_GLOBAL
#define TZMINS_HANDLING 4
	/*
	 * This system maintains the timezone offset in a global variable as
	 * a number of _seconds_ west of GMT.  We need to correct this value
	 * if DST is in effect.  Note that the global "daylight" indicates
	 * that DST applies to this site and NOT necessarily that the DST
	 * correction needs to be applied right now.  Be careful -- some
	 * systems have a "timezone()" procedure and this method will return
	 * the address of that procedure rather than a timezone offset!
	 */
	{
		extern long timezone;
		extern int daylight;
		return (int)(timezone/60) -
		    ((daylight && tm->tm_isdst) ? 60 : 0);
	}
#endif

#ifdef TZMINS_USE_TIMEOFDAY
#define TZMINS_HANDLING 5
	/*
	 * This system uses gettimeofday() to obtain the timezone
	 * information as minutes west of GMT.  The returned value will
	 * not be corrected for DST (unless you are unlucky enough to
	 * own a Unix written by some unmentionable vendor), so we will
	 * need to account for that.  Be careful -- some systems that
	 * have this procedure depreciate its use for timezone information
	 * and recommend it only for the high-resolution time information.
	 * On these systems the timezone info may be some kernel default
	 * or even garbage.
	 */
	{
		struct timeval tv;
		struct timezone tz;
		(void) gettimeofday(&tv, &tz);
#ifdef AIX
		return tz.tz_minuteswest;
#else
		return tz.tz_minuteswest -
		    (tm->tm_isdst && tz.tz_dsttime != DST_NONE ? 60 : 0);
#endif
	}
#endif

#ifdef TZMINS_USE_FTIME
#define TZMINS_HANDLING 6
	/*
	 * This system uses ftime() to obtain the timezone information
	 * as minutes west of GMT.  The returned value will not be
	 * corrected for DST, so we will need to account for that.  Be
	 * careful -- some systems that have this procedure depreciate
	 * its use for timezone information and recommend it only for
	 * the high-resolution time information.  On these systems the
	 * timezone info may be some kernel default or even garbage.
	 */
	{
		struct timeb tb;
		(void) ftime(&tb);
		return tb.timezone - (tm->tm_isdst ? 60 : 0);
	}
#endif

#ifndef TZMINS_HANDLING
	/* Force a compile error if the timezone config is wrong. */
	no_tzmins_handling_defined(TZMINS_HANDLING);
#endif
}

/****************************************************************************/

char *get_tz_name(tm)
struct tm *tm;
{

	if (tm == 0) {
		time_t t;
		(void) time(&t);
		tm = localtime(&t);
	}

#ifdef TZNAME_USE_TM_NAME
#define TZNAME_HANDLING 1
	/*
	 * This system maintains the timezone name in the (struct tm).
	 */
	return tm->tm_name;
#endif

#ifdef TZNAME_USE_TM_ZONE
#define TZNAME_HANDLING 2
	/*
	 * This system maintains the timezone name in the (struct tm).
	 */
	return tm->tm_zone;
#endif

#ifdef TZNAME_USE_TZNAME
#define TZNAME_HANDLING 3
	/*
	 * This system maintains a global array that contains two timezone
	 * names, one for when DST is in effect and one for when it is not.
	 * We simply need to pick the right one.
	 */
	{
		extern char *tzname[];
		return tzname[tm->tm_isdst];
	}
#endif

#ifdef TZNAME_USE_TIMEZONE
#define TZNAME_HANDLING 4
	/*
	 * This system provides a timezone() procedure to get a timezone
	 * name.  Be careful -- some systems have this procedure but
	 * depreciate its use, and in some cases it is outright broke.
	 */
	{
		extern char *timezone();
		return timezone(get_tz_mins(tm), tm->tm_isdst);
	}
#endif

#ifndef TZNAME_HANDLING
	/* Force a compile error if the timezone config is wrong. */
	no_tzname_handling_defined(TZNAME_HANDLING);
#endif
}

/****************************************************************************/

#ifdef _TEST

/*
 * It would be best to futz around with the TZ setting when running this
 * test.  In all cases, the "null" and the "localtime()" results should
 * be identical, and the "gmtime()" results should indicate "GMT 0"
 * regardless of TZ setting.  Here are a few possible TZ settings you
 * can try, and the result you should expect.
 *
 *	TZ=GMT			always GMT 0
 *	TZ=CST6CDT		CST 360 or CDT 300, depending upon time of year
 *	TZ=EST5EDT		EST 300 or EDT 240, depending upon time of year
 *	TZ=EST5EDT;0,364	always EDT 240
 *	TZ=EST5EDT;0,0		always EST 300
 *
 * Oh...this all assumes your system supports TZ. :-)
 */

main()
{
	time_t t;
	struct tm *tm;
	static char f[] = "using %s tm struct - name=\"%s\" mins_west=\"%d\"\n";

	(void) time(&t);
	tm = (struct tm *)0;
	printf(f, "null", get_tz_name(tm), get_tz_mins(tm));
	tm = localtime(&t);
	printf(f, "localtime()", get_tz_name(tm), get_tz_mins(tm));
	tm = gmtime(&t);
	printf(f, "gmtime()", get_tz_name(tm), get_tz_mins(tm));
	exit(0);
}

#endif /*_TEST*/

