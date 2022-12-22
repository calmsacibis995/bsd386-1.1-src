
static char rcsid[] = "@(#)Id: mail_gets.c,v 5.4 1993/08/03 19:28:39 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.2 $   $State: Exp $
 *
 * 			Copyright (c) 1992 USENET Community Trust
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 *******************************************************************************
 * $Log: mail_gets.c,v $
 * Revision 1.2  1994/01/14  00:53:23  cp
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
 * Revision 5.3  1993/08/03  19:05:33  syd
 * When STDC is used on Convex the feof() function is defined as
 * a true library routine in the header files and moreover the
 * library routine also leaks royally. It returns always 1!!
 * So we have to use a macro. Convex naturally does not provide
 * you with one though if you are using a STDC compiler. So we
 * have to include one.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.2  1993/04/12  01:13:30  syd
 * In some cases, with certain editors, the user can create an
 * aliases.text file in which the last line is terminated with an EOF but
 * doesn't have a '\n'.  Currently, elm with complain that the line is
 * too long.
 * From: "William F. Pemberton" <wfp5p@holmes.acc.virginia.edu>
 *
 * Revision 5.1  1992/10/03  22:41:36  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** get a line from the mail file, but be tolerant of nulls

  The length of the line is returned

**/

#include <stdio.h>
#include "defs.h"

int
mail_gets(buffer, size, mailfile)
char *buffer;
int size;
FILE *mailfile;
{
	register int line_bytes = 0, ch;
	register char *c = buffer;

	size--; /* allow room for zero terminator on end, just in case */

	while (!feof(mailfile) && !ferror(mailfile) && line_bytes < size) {
	  ch = getc(mailfile); /* Macro, faster than  fgetc() ! */

	  if (ch == EOF)
	  {
	    if (line_bytes > 0 && *c != '\n')
	    {
	        ++line_bytes;
	    	*c++ = '\n';
	    }
	    break;
	  }

	  *c++ = ch;
	  ++line_bytes;

	  if (ch == '\n')
	    break;
	}
	*c = 0;	/* Actually this should NOT be needed.. */
	return line_bytes;
}
