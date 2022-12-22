/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

/*      BSDI $Id: errlib.h,v 1.1 1993/01/07 18:56:23 sanders Exp $ */

void    Psetname(const char *name); /* used by Perror and Pwarn */
void    Perror(char *fmt, ...);     /* perror and exit(1) */
void    Pwarn(char *fmt, ...);      /* perror and continue */
void    Perrmsg(char *fmt, ...);    /* print on stderr */
void    __Pmsg(char *fmt, ...);     /* debug printf, enable with -DPDEBUG */

#ifdef PDEBUG
#define Pmsg(x) ( __Pmsg x )
#else
#define Pmsg(x) /*empty*/
#endif
