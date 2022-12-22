/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Yet another date/time format.
**
**	RCSID $Id: DateTimeStr.c,v 1.1.1.1 1992/09/28 20:08:41 trent Exp $
**
**	$Log: DateTimeStr.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:41  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:48:05  piers
 * Initial revision
 *
*/

#include	"global.h"
#include	"debug.h"

#include	<time.h>



/*
**	Return date in form "yy/mm/dd hh:mm:ss".
*/

char *
DateTimeStr(date, string)
	Time_t			date;
	char *			string;	/* Pointer to buffer of length DATETIMESTRLEN */
{
	register struct tm *	tmp;

	tmp = localtime((long *)&date);

	(void)sprintf
	(
		string,
		"%02d/%02d/%02d %02d:%02d:%02d",
		tmp->tm_year, tmp->tm_mon+1, tmp->tm_mday,
		tmp->tm_hour, tmp->tm_min, tmp->tm_sec
	);

	return string;
}
