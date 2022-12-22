/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Return date/time in form "mm/dd-hh:mm".
**
**	RCSID $Id: UUCP_time.c,v 1.1.1.1 1992/09/28 20:08:42 trent Exp $
**
**	$Log: UUCP_time.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:42  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	TIME

#include	"global.h"



char *
UUCPDateTimeStr(ts, string)
	Time_t			ts;
	char *			string;	/* Pointer to buffer of length UUCPDATETIMESTRLEN */
{
	register struct tm *	tmp;

	tmp = localtime((long *)&ts);

	(void)sprintf
	(
		string,
		"%02d/%02d-%02d:%02d",
		tmp->tm_mon+1, tmp->tm_mday,
		tmp->tm_hour, tmp->tm_min
	);

	return string;
}
