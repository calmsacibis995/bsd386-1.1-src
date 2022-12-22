#ifndef lint
static char *rcsid =
   "$Header: /bsdi/MASTER/BSDI_OS/usr.bin/lisp/franz/lamp.c,v 1.2 1992/01/04 18:55:42 kolstad Exp $";
#endif

/*					-[Tue Mar 22 15:17:09 1983 by jkf]-
 * 	lamp.c				$Locker:  $
 * interface with unix profiling
 *
 * (c) copyright 1982, Regents of the University of California
 */

#include "global.h"

#ifdef PROF

#define PBUFSZ 500000
short pbuf[PBUFSZ];

/* data space for fasl to put counters */
int mcnts[NMCOUNT];
int mcntp = (int) mcnts;
int doprof = TRUE;

lispval
Lmonitor()
{
	extern etext, countbase;

	if (np==lbot) { monitor((int(*)())0); countbase = 0; }
	else if (TYPE(lbot->val)==INT) 
	 { monitor((int (*)())2, (int (*)())lbot->val->i, pbuf,
	 				PBUFSZ*(sizeof(short)), 7000); 
	   countbase = ((int)pbuf) +12; 
	}
	else {
	   monitor((int (*)())2, (int (*)())sbrk(0), pbuf,
	   				PBUFSZ*(sizeof(short)), 7000); 
	   countbase = ((int)pbuf) + 12; }
	return(tatom);
}


#else

/* if prof is not defined, create a dummy Lmonitor */

short	pbuf[8];

/* data space for fasl to put counters */
int mcnts[1];
int mcntp = (int) mcnts;
int doprof = FALSE;

Lmonitor()
{
	error("Profiling not enabled",FALSE);
}


#endif
