/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	RCSID $Id: df12.c,v 1.3 1994/01/31 01:26:01 donn Exp $
**
**	$Log: df12.c,v $
 * Revision 1.3  1994/01/31  01:26:01  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:58:01  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:42:24  vixie
 * Initial revision
 *
 * Revision 1.2  1993/02/28  15:30:48  pace
 * Add hasv dialer; change dialers to do non-blocking open when appropriate
 *
 * Revision 1.1.1.1  1992/09/28  20:08:45  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include "condevs.h"

#ifdef	DF112

/*
**	df12popn(telno, flds, dev) connect to df12 modem (pulse call)
**	df12topn(telno, flds, dev) connect to df12 modem (tone call)
**	char *flds[], *dev[];
**
**	return codes:
**		>0  -  file number  -  ok
**		CF_DIAL,CF_NODEV  -  failed
*/

extern CallType	df12cls(int);
extern CallType	df12opn(char *, char **, Device *, int);


CallType
df12popn(telno, flds, dev)
char   *telno,
       *flds[];
Device *dev;
{
    return df12opn (telno, flds, dev, 0);
}

CallType
df12topn(telno, flds, dev)
char   *telno,
       *flds[];
Device *dev;
{
    return df12opn (telno, flds, dev, 1);
}

CallType
df12opn(telno, flds, dev, toneflag)
char   *telno;
char   *flds[];
Device *dev;
int     toneflag;
{
    int     phindex, dh = -1;
    extern  errno;
    char    dcname[PATHNAMESIZE], newphone[64];

    sprintf (dcname, "/dev/%s", dev -> D_line);
    Debug((4, "dc - %s", dcname));
    if ( setjmp (AlarmJmp) )
    {
	Log("TIMEOUT %s", dcname);
	if ( dh >= 0 )
	    close_dev (dh);
	return CF_DIAL;
    }
    signal (SIGALRM, Timeout);
    getnextfd ();
    alarm (10);
    dh = open_dev(dcname);
    alarm (0);

 /* modem is open */

 /* First, adjust our phone number string.  These modems don't
  * like any characters but digits and "=" signs ( for delay )
  */
    for ( phindex = 0 ; *telno ; telno++ )
    {
	if ( *telno == '=' || (*telno >= '0' && *telno <= '9') )
	    newphone[phindex++] = *telno;
	if ( phindex == 64 )
	{
	    Log("Phone number too long %s", dcname);
	    close (dh);
	    return CF_DIAL;
	}
    }
    newphone[phindex] = '\0';
    NextFd = -1;
    if ( dh >= 0 )
    {
	fixline (dh, dev -> D_speed);
	if ( dochat (dev, flds, dh) )
	{
	    close (dh);
	    return CF_CHAT;
	}
	slow_write (dh, "\02");
	if ( expect ("Ready\r\n", dh) != 0 )
	{
	    Debug((4, "Didn't get 'Ready' response.", NULLSTR));
	    Log("Modem not responding %s", dcname);
	    close (dh);
	    return CF_DIAL;
	}
	Debug((4, "Got 'Ready' response", NULLSTR));
	Debug((7, "Writing control select flag %c", toneflag ? 'T' : 'P'));
	slow_write (dh, toneflag ? "T" : "P");
	Debug((4, "Writing telephone number %s", newphone));
	slow_write (dh, newphone);
	Debug((7, "Telephone number written", NULLSTR));
	slow_write (dh, "#");
	Debug((7, "Writing # sign", NULLSTR));

	if ( expect ("Attached\r\n", dh) != 0 )
	{
	    Log("No carrier %s", dcname);
	    strcpy (devSel, dev -> D_line);
	    df12cls (dh);
	    return CF_DIAL;
	}

    }
    if ( dh < 0 )
    {
	Log("CAN'T OPEN %s", dcname);
	return CF_NODEV;
    }
    else
    {
	Debug((4, "df12 ok"));
	return dh;
    }
}

CallType
df12cls(fd)
int     fd;
{
    char    dcname[PATHNAMESIZE];

    if ( fd > 0 )
    {
	sprintf (dcname, "/dev/%s", devSel);
	Debug((4, "Hanging up fd = %d", fd));
	drop_dtr (fd);
	sleep (2);
	close_dev (fd);
	sleep (2);
	rmlock (devSel);
    }
}
#endif	/* DF112 */
