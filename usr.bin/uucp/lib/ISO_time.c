/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Return date/time in form "yymmddhhmmss.pppp".
**
**	RCSID $Id: ISO_time.c,v 1.1.1.1 1992/09/28 20:08:38 trent Exp $
**
**	$Log: ISO_time.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:38  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	TIME

#include	"global.h"



char *
ISODateTimeStr(
	Time_t			secs,
	Time_t			usecs,
	char *			string	/* Pointer to buffer of length ISODATETIMESTRLEN */
)
{
	register struct tm *	tmp;

	tmp = localtime((long *)&secs);

	(void)sprintf
	(
		string,
		"%02d%02d%02d%02d%02d%02d.%04.0f",
		tmp->tm_year, tmp->tm_mon+1, tmp->tm_mday,
		tmp->tm_hour, tmp->tm_min, tmp->tm_sec,
		(float)usecs/1000.0
	);

	return string;
}
