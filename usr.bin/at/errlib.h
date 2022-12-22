/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

#ifndef PROGNAME
#define PROGNAME "at"
#else
extern char *PROGNAME;
#endif

void	Perror(char *fmt, ...);
void	Pwarn(char *fmt, ...);
void	Perrmsg(char *fmt, ...);
void	__Pmsg(char *fmt, ...);

#ifdef DEBUG
#define Pmsg(x)	( __Pmsg x )
#else
#define Pmsg(x)	0
#endif
