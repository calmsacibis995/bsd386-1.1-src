/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Setup current time + usec.
**
**	Returns true if new time.
**
**	RCSID $Id: SetTimes.c,v 1.1.1.1 1992/09/28 20:08:38 trent Exp $
**
**	$Log: SetTimes.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:38  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	SYS_TIME

#include	<global.h>

TimeBuf		TimeNow;	/* Current time (=> Time/Timeusec) */


bool
SetTimes()
{
	bool		ret;
	struct timeval	tb;

	while ( gettimeofday(&tb, (struct timezone *)0) == SYSERROR )
		SysError("gettimeofday");

	if ( Timeusec != (Time_t)tb.tv_usec )
		ret = true;
	else
	if ( Time != (Time_t)tb.tv_sec )
		ret = true;
	else
		ret = false;

	Time = (Time_t)tb.tv_sec;
	Timeusec = (Time_t)tb.tv_usec;

	Trace((2, "SetTimes => %lu.%lu%s", Time, Timeusec, ret?" NEW":EmptyStr));

	return ret;
}
