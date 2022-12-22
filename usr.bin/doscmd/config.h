/*	Krystal $Id: config.h,v 1.1 1994/01/14 23:24:41 polk Exp $ */
/* various overall defines used in config.c and others */

#include <errno.h>

#define MAXPORT		0x400

#define N_PARALS_MAX	3
#define N_COMS_MAX	4	/* DOS restriction (sigh) */

extern char *xfont;

void setver(char *, short);
short getver(char *);
