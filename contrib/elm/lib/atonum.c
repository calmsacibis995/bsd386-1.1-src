
static char rcsid[] = "@(#)Id: atonum.c,v 5.2 1993/08/03 19:28:39 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.1 $   $State: Exp $
 *
 * 			Copyright (c) 1993 USENET Community Trust
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 *******************************************************************************
 * $Log: atonum.c,v $
 * Revision 1.1  1994/01/14  01:34:54  cp
 * 2.4.23
 *
 * Revision 5.2  1993/08/03  19:28:39  syd
 * Elm tries to replace the system toupper() and tolower() on current
 * BSD systems, which is unnecessary.  Even worse, the replacements
 * collide during linking with routines in isctype.o.  This patch adds
 * a Configure test to determine whether replacements are really needed
 * (BROKE_CTYPE definition).  The <ctype.h> header file is now included
 * globally through hdrs/defs.h and the BROKE_CTYPE patchup is handled
 * there.  Inclusion of <ctype.h> was removed from *all* the individual
 * files, and the toupper() and tolower() routines in lib/opt_utils.c
 * were dropped.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.1  1993/01/19  04:46:21  syd
 * Initial Checkin
 *
 *
 ******************************************************************************/

#include "defs.h"

/*
 * This is similar to atoi(), but it complains if the string
 * contains any non-numeric characters.  Returns the numeric
 * value on success, -1 on error.
 */
int atonum(str)
register char *str;
{
    register int value;

    if (*str == '\0')
	return -1;
    value = 0;
    while (isdigit(*str))
	value = (value*10) + (*str++ - '0');
    return (*str == '\0' ? value : -1);
}


#ifdef _TEST
#include <stdio.h>
main()
{
	char buf[1024];
	while (gets(buf) != NULL)
		printf("atonum(%s) = %d\n", buf, atonum(buf));
	putchar('\n');
	exit(0);
}
#endif

