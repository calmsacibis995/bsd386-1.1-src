/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Produce time string in format suitable for RFC822 mail headers:-
*/
static char	buf[]	= "ddd, dd mmm yy hh:mm:ss +hhmm\n";
/*			   012  56 890 23 56 89 12 45678 9 0
**			   0         1         2           3  [30]
**
**	RCSID $Id: rfc822time.c,v 1.1.1.1 1992/09/28 20:08:50 trent Exp $
**
**	$Log: rfc822time.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:50  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include	<time.h>



char *
rfc822time(
	long *			timep
)
{
	register char *		s;
	register struct tm *	tp;
	register int		day;
	char			sign;

	static char		zone[8];
	struct tm		gmt;

	extern char *		ctime();
	extern struct tm *	gmtime();
	extern struct tm *	localtime();
	extern char *		strcpy();
	extern char *		strncpy();

	s = ctime(timep);

	buf[0] = s[0];
	buf[1] = s[1];
	buf[2] = s[2];

	if ( (buf[5] = s[8]) == ' ' )
		buf[5] = '0';
	buf[6] = s[9];

	buf[8] = s[4];
	buf[9] = s[5];
	buf[10] = s[6];

	buf[12] = s[22];
	buf[13] = s[23];

	(void)strncpy(&buf[15], &s[11], 8);

	if ( zone[0] == '\0' )
	{
		gmt = *gmtime(timep);
		tp = localtime(timep);

		if ( gmt.tm_year == tp->tm_year )
		{
			if ( gmt.tm_yday == tp->tm_yday )
				day = 0;
			else
			if ( gmt.tm_yday < tp->tm_yday )
				day = 1;
			else
				day = -1;
		}
		else
		if ( gmt.tm_year < tp->tm_year )
			day = 1;
		else
			day = -1;

		day = (tp->tm_hour + day * 24) * 60 + tp->tm_min;
		day -= gmt.tm_hour * 60 + gmt.tm_min;

		if ( day < 0 )
		{
			day = -day;
			sign = '-';
		}
		else
			sign = '+';

		(void)sprintf(zone, "%c%2.2d%2.2d\n", sign, day/60, day%60);
	}

	(void)strcpy(&buf[24], zone);

	return buf;
}
