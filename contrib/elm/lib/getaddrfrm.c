
static char rcsid[] = "@(#)Id: getaddrfrm.c,v 5.4 1993/08/03 19:28:39 syd Exp ";

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
 * $Log: getaddrfrm.c,v $
 * Revision 1.2  1994/01/14  00:53:10  cp
 * 2.4.23
 *
 * Revision 5.4  1993/08/03  19:28:39  syd
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
 * Revision 5.3  1993/05/16  20:55:52  syd
 * Fix bug where text following "<" within double-quote delimited comment
 * is taken as an address.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.2  1992/11/07  20:05:52  syd
 * change to use header_cmp to allow for linear white space around the colon
 * From: Syd
 *
 * Revision 5.1  1992/10/03  22:41:36  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** 

**/

#include "headers.h"

#ifdef USE_EMBEDDED_ADDRESSES

get_address_from(line, buffer)
char *line, *buffer;
{
	/** This routine extracts the address from either a 'From:' line
	    or a 'Reply-To:' line.  The strategy is as follows:  if the
	    line contains a '<', then the stuff enclosed is returned.
	    Otherwise we go through the line and strip out comments
	    and return that.  White space will be elided from the result.
	**/

    register char *s;
    register int l;

    /**  Skip start of line over prefix, e.g. "From:".  **/
    if ((s = index(line, ':')) != NULL)
	line = s + 1;

    /*
     * If there is a '<' then copy from it to '>' into the buffer.  We
     * used to search with an "index()", but we need to handle RFC-822
     * conventions (e.g. "<" within a double-quote comment) correctly.
     */
    for (s = line ; *s != '\0' && *s != '<' ; s += len_next_part(s))
	;
    if (*s == '<') {
	++s;
	while (*s != '\0' && *s != '>') {
	    if ((l = len_next_part(s)) == 1 && isspace(*s)) {
		++s; /* elide embedded whitespace */
	    } else {
		while (--l >= 0)
		    *buffer++ = *s++;
	    }
	}
	*buffer = '\0';
	return;
    }

    /**  Otherwise, strip comments and get address with whitespace elided.  **/
    s = strip_parens(line);
    while (*s != '\0') {
	l = len_next_part(s);
	if (l == 1) {
	  if ( !isspace(*s) )
	    *buffer++ = *s;
	  s++;
	} else {
	  while (--l >= 0)
	    *buffer++ = *s++;
	}
    }
    *buffer = '\0';

}

#endif
