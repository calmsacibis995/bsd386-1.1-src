/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	RCSID $Id: dk.c,v 1.1.1.1 1992/09/28 20:08:45 trent Exp $
**
**	$Log: dk.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:45  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include "condevs.h"

#ifdef DATAKIT

#include <dk.h>

#define DKTRIES 2

/*
**	dkopn(flds)	make datakit connection
**
**	return codes:
**		>0 - file number - ok
**		FAIL - failed
*/

CallType
dkopn(flds)
char *flds[];
{
	int dkphone;
	register char *cp;
	register ret, i;

	if ( setjmp(AlarmJmp) )
		return CF_DIAL;

	signal(SIGALRM, Timeout);
	dkphone = 0;
	cp = flds[F_PHONE];
	while ( *cp )
		dkphone = 10 * dkphone + (*cp++ - '0');
	Debug((4, "dkphone (%d) ", dkphone));
	for ( i = 0 ; i < DKTRIES ; i++ )
	{
		getnextfd();
		ret = dkdial(D_SH, dkphone, 0);
		NextFd = -1;
		Debug((4, "dkdial (%d)", ret));
		if ( ret > -1 )
			break;
	}
	return ret;
}
#endif	/* DATAKIT */
