static char rcsid[] = "@(#)Id: header_cmp.c,v 5.3 1993/08/03 19:28:39 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.2 $   $State: Exp $
 *
 *			Copyright (c) 1988-1992 USENET Community Trust
 *			Copyright (c) 1986,1987 Dave Taylor
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 *******************************************************************************
 * $Log: header_cmp.c,v $
 * Revision 1.2  1994/01/14  00:53:14  cp
 * 2.4.23
 *
 * Revision 5.3  1993/08/03  19:28:39  syd
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
 * Revision 5.2  1992/11/07  20:59:49  syd
 * fix typo
 *
 * Revision 5.1  1992/11/07  20:16:29  syd
 * Initial Checkin
 *
 *
 ******************************************************************************/

/** 
	compare a header, ignoring case and allowing linear white space
	around the :.  Header must be anchored to the start of the line.

	returns NULL if no match, or first character after trailing linear
	white space of the :.

**/

#include "headers.h"


char *
header_cmp(header, prefix, suffix)
register char *header, *prefix, *suffix;
{
	int len;

	len = strlen(prefix);
	if (strincmp(header, prefix, len))
		return(NULL);

	/* skip over while space if any */
	header += len;

	if (*header != ':')	/* headers must end in a : */
		return(NULL);

	/* skip over while space if any */
	header++;

	while (*header) {
		if (!whitespace(*header))
			break;
		header++;
	}

	if (suffix != NULL) {
		len = strlen(suffix);
		if (len > 0)
			if (strincmp(header, suffix, len))
				return(NULL);
	}

	return(header);
}
