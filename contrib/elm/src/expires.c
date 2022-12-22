
static char rcsid[] = "@(#)Id: expires.c,v 5.4 1993/08/10 18:53:31 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.2 $   $State: Exp $
 *
 *			Copyright (c) 1988-1992 USENET Community Trust
 *			Copyright (c) 1986,1987 Dave Taylor
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 *******************************************************************************
 * $Log: expires.c,v $
 * Revision 1.2  1994/01/14  00:54:51  cp
 * 2.4.23
 *
 * Revision 5.4  1993/08/10  18:53:31  syd
 * I compiled elm 2.4.22 with Purify 2 and fixed some memory leaks and
 * some reads of unitialized memory.
 * From: vogt@isa.de
 *
 * Revision 5.3  1993/08/03  19:28:39  syd
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
 * Revision 5.2  1992/12/07  02:58:56  syd
 * Fix long -> time_t
 * From: Syd
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** This routine is written to deal with the Expires: header on the
    individual mail coming in.  What it does is to look at the date,
    compare it to todays date, then set the EXPIRED flag on the
    current message if it is true...
**/

#include "headers.h"

#ifdef I_TIME
#  include <time.h>
#endif
#ifdef I_SYSTIME
#  include <sys/time.h>
#endif

process_expiration_date(date, message_status)
char *date;
int  *message_status;
{
	struct tm *timestruct;
	time_t thetime;
	char word1[WLEN], word2[WLEN], word3[WLEN], word4[WLEN], word5[WLEN];
	int  month = 0, day = 0, year = 0, hour = 0, minute = 0, items;
#ifndef	_POSIX_SOURCE
	struct tm *localtime();
	time_t time();
#endif

	/** first step is to break down the date given into MM DD YY HH MM
	    format:  The possible formats for this field are, by example:
	
		(1) Mon, Jun 11, 87
		(2) Mon, 11 Jun 87
		(3) Jun 11, 87
		(4) 11 Jun 87
		(5) 11/06/87	<- ambiguous - will be ignored!!
		(6) 8711061248GMT
		(7) Mon, Jun 11, 87 12:48:35 GMT

	    The reason #5 is considered ambiguous will be made clear
	    if we consider a message to be expired on Jan 4, 88:
			01/04/88	in the United States
			04/01/88	in Europe
	    so is the first field the month or the day?  Standard prob.
	**/

	items = sscanf(date, "%s %s %s %s %s",
		       word1, word2, word3, word4, word5);

	if (items < 5)
	  word5[0] = '\0';
	if (items < 4)
	  word4[0] = '\0';
	if (items < 3)
	  word3[0] = '\0';
	if (items < 2)
	  word2[0] = '\0';
	if (items < 1)
	  word1[0] = '\0';

	if (strlen(word5) != 0) {	/* we have form #7 */
	  day   = atoi(word1);
	  month = month_number(word2);
	  year  = atoi(word3);
	  sscanf(word4, "%02d%*c%02d",
	       &hour, &minute);
	}
	else if (strlen(word2) == 0) {	/* we have form #6 or form #5 */
	  if (isdigit(word1[1]) && isdigit(word1[2]))	  /* form #6 */
	    sscanf(word1, "%02d%02d%02d%02d%02d%*c",
		 &year, &month, &day, &hour, &minute);
	}
	else if (strlen(word4) != 0) {		   /* form #1 or form #2 */
	  if(isdigit(word2[0])) {		   /* form #2 */
	      month = month_number(word3);
	      day   = atoi(word2);
	      year  = atoi(word4);
	  } else {				   /* form #1 */
	      month = month_number(word2);
	      day   = atoi(word3);
	      year  = atoi(word4);
	  }
	}
	else if (! isdigit(word1[0])) {		   /* form #3 */
	  month = month_number(word1);
	  day   = atoi(word2);
	  year  = atoi(word3);
	}
	else {					   /* form #4 */
	  day   = atoi(word1);
	  month = month_number(word2);
	  year  = atoi(word3);
	}

	if (day == 0 || year == 0)
	  return;			/* we didn't get a valid date */

	/** next let's get the current time and date, please **/

	thetime = time((time_t *) 0);

	timestruct = localtime(&thetime);

	/** and compare 'em **/

	if (year > timestruct->tm_year)
	  return;
	else if (year < timestruct->tm_year)
	  goto expire_message;

	if (month > timestruct->tm_mon)
	  return;
	else if (month < timestruct->tm_mon)
	  goto expire_message;

	if (day > timestruct->tm_mday)
	  return;
	else if (day < timestruct->tm_mday)
	  goto expire_message;

	if (hour > timestruct->tm_hour)
	  return;
	else if (hour < timestruct->tm_hour)
	  goto expire_message;

	if (minute > timestruct->tm_min)
	  return;

expire_message:

	/** it's EXPIRED!  Yow!! **/

	(*message_status) |= EXPIRED;
}
