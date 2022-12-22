
static char rcsid[] = "@(#)Id: date.c,v 5.5 1993/08/03 19:28:39 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.2 $   $State: Exp $
 *
 * 			Copyright (c) 1988-1992 USENET Community Trust
 * 			Copyright (c) 1986,1987 Dave Taylor
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 *******************************************************************************
 * $Log: date.c,v $
 * Revision 1.2  1994/01/14  00:54:39  cp
 * 2.4.23
 *
 * Revision 5.5  1993/08/03  19:28:39  syd
 * Elm tries to replace the system toupper() and tolower() on current
 * BSD systems, which is unnecessary.  Even worse, the replacements
 * collide during linking with routines in isctype.o.  This patch adds
 * a Configure test to determine whether replacements are really needed
 * (BROKE_CTYPE definition).  The <ctype.h> header file is now included
 * globally through hdrs/defs.h and the BROKE_CTYPE patchup is handled
 * there.  Inclusion of <ctype.h> was removed from *all* the individual
 * files, and the toupper() and tolower() routines in lib/opt_utils.c
 * were dropped.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.4  1992/12/11  01:45:04  syd
 * remove sys/types.h include, it is now included by defs.h
 * and this routine includes defs.h or indirectly includes defs.h
 * From: Syd
 *
 * Revision 5.3  1992/12/07  02:57:09  syd
 * convert long to time_t where relevant
 * From: Syd via prompting from Jim Brown
 *
 * Revision 5.2  1992/11/15  02:10:11  syd
 * remove no longer used tzname
 * From: Syd
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** return the current date and time in a readable format! **/
/** also returns an ARPA RFC-822 format date...            **/


#include "headers.h"

#ifdef I_TIME
#  include <time.h>
#endif
#ifdef I_SYSTIME
#  include <sys/time.h>
#endif
#ifdef BSD
#  include <sys/timeb.h>
#endif

#ifndef	_POSIX_SOURCE
extern struct tm *localtime();
extern struct tm *gmtime();
extern time_t	  time();
#endif

#define MONTHS_IN_YEAR	11	/* 0-11 equals 12 months! */
#define FEB		 1	/* 0 = January 		  */
#define DAYS_IN_LEAP_FEB 29	/* leap year only 	  */

#define ampm(n)		(n > 12? n - 12 : n)
#define am_or_pm(n)	(n > 11? (n > 23? "am" : "pm") : "am")
#define leapyear(year)	((year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0)))

char *arpa_dayname[] = { "Sun", "Mon", "Tue", "Wed", "Thu",
		  "Fri", "Sat", "" };

char *arpa_monname[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
		  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", ""};

int  days_in_month[] = { 31,    28,    31,    30,    31,     30, 
		  31,     31,    30,   31,    30,     31,  -1};

days_ahead(days, buffer)
int days;
char *buffer;
{
	/** return in buffer the date (Day, Mon Day, Year) of the date
	    'days' days after today.  
	**/

	struct tm *the_time;		/* Time structure, see CTIME(3C) */
	time_t	   junk;		/* time in seconds....		 */

	junk = time((time_t *) 0);	/* this must be here for it to work! */
	the_time = localtime(&junk);

	/* increment the day of the week */

	the_time->tm_wday = (the_time->tm_wday + days) % 7;

	/* the day of the month... */
	the_time->tm_mday += days;
	
        while (the_time->tm_mday > days_in_month[the_time->tm_mon]) {
          if (the_time->tm_mon == FEB && leapyear(the_time->tm_year)) {
            if (the_time->tm_mday > DAYS_IN_LEAP_FEB) {
              the_time->tm_mday -= DAYS_IN_LEAP_FEB;
              the_time->tm_mon += 1;
            }
            else
              break;            /* Is Feb 29, so leave */
          }
          else {
            the_time->tm_mday -= days_in_month[the_time->tm_mon];
            the_time->tm_mon += 1;
          }

          /* check the month of the year */
          if (the_time->tm_mon > MONTHS_IN_YEAR) {
            the_time->tm_mon -= (MONTHS_IN_YEAR + 1);
            the_time->tm_year += 1;
          }
        }
  
        /* now, finally, build the actual date string */

	sprintf(buffer, "%s, %d %s %d",
	  arpa_dayname[the_time->tm_wday],
	  the_time->tm_mday % 32,
	  arpa_monname[the_time->tm_mon],
	  the_time->tm_year % 100);
}

int
month_number(name)
char *name;
{
	/** return the month number given the month name... **/

	char ch;

	switch (tolower(name[0])) {
	 case 'a' : if ((ch = tolower(name[1])) == 'p')	return(APRIL);
		    else if (ch == 'u') return(AUGUST);
		    else return(-1);	/* error! */
	
	 case 'd' : return(DECEMBER);
	 case 'f' : return(FEBRUARY);
	 case 'j' : if ((ch = tolower(name[1])) == 'a') return(JANUARY);
		    else if (ch == 'u') {
	              if ((ch = tolower(name[2])) == 'n') return(JUNE);
		      else if (ch == 'l') return(JULY);
		      else return(-1);		/* error! */
	            }
		    else return(-1);		/* error */
	 case 'm' : if ((ch = tolower(name[2])) == 'r') return(MARCH);
		    else if (ch == 'y') return(MAY);
		    else return(-1);		/* error! */
	 case 'n' : return(NOVEMBER);
	 case 'o' : return(OCTOBER);
	 case 's' : return(SEPTEMBER);
	 default  : return(-1);
	}
}

#ifdef SITE_HIDING

char *get_ctime_date()
{
	/** returns a ctime() format date, but a few minutes in the 
	    past...(more cunningness to implement hidden sites) **/

	static char buffer[SLEN];	/* static character buffer       */
	struct tm *the_time;		/* Time structure, see CTIME(3C) */

#ifdef BSD
	struct  timeval  time_val;		
	struct  timezone time_zone;
	long	   junk;		/* time in seconds....		 */
#else
	time_t	   junk;		/* time in seconds....		 */
#endif

#ifdef BSD
	gettimeofday(&time_val, &time_zone);
	junk = time_val.tv_sec;
#else
	junk = time((time_t *) 0);	/* this must be here for it to work! */
#endif
	the_time = localtime(&junk);

	sprintf(buffer, "%s %s %d %02d:%02d:%02d %d",
	  arpa_dayname[the_time->tm_wday],
	  arpa_monname[the_time->tm_mon],
	  the_time->tm_mday % 32,
	  min(the_time->tm_hour % 24, (rand() % 24)),
	  min(abs(the_time->tm_min  % 61 - (rand() % 60)), (rand() % 60)),
	  min(abs(the_time->tm_sec  % 61 - (rand() % 60)), (rand() % 60)),
	  the_time->tm_year % 100 + 1900);
	
	return( (char *) buffer);
}

#endif

char *
elm_date_str(buf, seconds)
char *buf;
time_t seconds;
{
	struct tm *tmbuf;

	tmbuf = gmtime(&seconds);

	sprintf(buf, "%s %d, %d %2.2d:%2.2d:%2.2d %s",
	    arpa_monname[tmbuf->tm_mon],
	    tmbuf->tm_mday,
	    tmbuf->tm_year % 100,
	    ampm(tmbuf->tm_hour),
	    tmbuf->tm_min,
	    tmbuf->tm_sec,
	    am_or_pm(tmbuf->tm_hour));

	return(buf);
}

make_menu_date(entry)
struct header_rec *entry;
{
	struct tm *tmbuf;
	time_t seconds;

	seconds = entry->time_sent + entry->tz_offset;
	tmbuf = gmtime(&seconds);
	sprintf(entry->time_menu, "%3.3s %-2d",
	    arpa_monname[tmbuf->tm_mon], tmbuf->tm_mday);
}
