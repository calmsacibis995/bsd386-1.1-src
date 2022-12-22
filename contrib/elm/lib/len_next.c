
static char rcsid[] = "@(#)Id: len_next.c,v 5.5 1993/08/03 19:28:39 syd Exp ";

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
 * $Log: len_next.c,v $
 * Revision 1.2  1994/01/14  00:53:20  cp
 * 2.4.23
 *
 * Revision 5.5  1993/08/03  19:28:39  syd
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
 * Revision 5.4  1993/04/12  01:27:30  syd
 * len_next_part() was botching quote-delimited strings.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.3  1993/01/19  05:07:05  syd
 * Trim erroreous extra log entry
 * From: Syd
 *
 * Revision 5.2  1993/01/19  04:47:12  syd
 * Significant changes to provide consistent Date and From_ header
 * cracking.  Overhauled date utilities and moved into library.  Moved
 * real_from() into library.  Modified frm, newmail, and readmsg utilities
 * to use library version of real_from().  Moved get_word() from Elm
 * source into library.  Added new library routines atonum() and strfcpy().
 * Fixed trailing backslash bug in len_next().
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.1  1992/10/03  22:41:36  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** get the length of the next part of the address/data field

	This code returns the length of the next part of the
  string field containing address/data.  It takes into account
  quoting via " as well as \ escapes.
  Quoting via ' is not taken into account, as RFC-822 does not
  consider a ' character a valid 'quoting character'

  A 1 is returned for a single character unless:

  A 0 is returned at end of string.

  A 2 is returned for strings that start \

  The length of quoted sections is returned for quoted fields

**/


int
len_next_part(str)
register char *str;
{
	register char *s;

	switch (*str) {

	case '\0':
		return 0;

	case '\\':
		return (*++str != '\0' ? 2 : 1);

	case '"':
		for (s = str+1 ; *s != '\0' ; ++s) {
			if (*s == '\\') {
				if (*++s == '\0')
					break;
			} else if (*s == '"') {
				++s;
				break;
			}
		}
		return (s - str);

	default:
		return 1;

	}

	/*NOTREACHED*/
}

#ifdef _TEST
#include <stdio.h>
main()
{
	char buf[256], *s;
	int len;

	while (gets(buf) != NULL) {
		for (s = buf ; (len = len_next_part(s)) > 0 ; s += len)
			printf("%4d %-.*s\n", len, len, s);
	}
	exit(0);
}
#endif
