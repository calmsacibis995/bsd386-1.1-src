
static char rcsid[] = "@(#)Id: getword.c,v 5.2 1993/08/03 19:28:39 syd Exp ";

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
 * $Log: getword.c,v $
 * Revision 1.1  1994/01/14  01:34:58  cp
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

#include <stdio.h>
#include "defs.h"

int get_word(buffer, start, word, wordlen)
char *buffer;
int start;
char *word;
int wordlen;
{
    /*
     * Extracts the next white-space delimited word from the "buffer"
     * starting at "start" characters into the buffer and skipping any
     * leading white-space there.  Handles backslash-quoted characters
     * and double-quote bracked strings as an atomic unit.  The resulting
     * word, up to "wordlen" bytes long, is saved in "word".  Returns the
     * buffer index where extraction terminated, e.g. the next word can be
     * extracted by starting at start+<return-val>.  If no words are found
     * in the buffer then -1 is returned.
     */

    register int len;
    register char *p;

    for (p = buffer+start ; isspace(*p) ; ++p)
	;

    if (*p == '\0')
	return (-1);		/* nothing IN buffer! */

    while (*p != '\0') {
	len = len_next_part(p);
	if (len == 1 && isspace(*p))
	    break;

	while (--len >= 0) {
	    if (--wordlen > 0)
		*word++ = *p;
	    ++p;
	}
    }

    *word = '\0';
    return (p - buffer);
}


#ifdef _TEST
main()
{
	char buf[1024], word[1024], *bufp;
	int start, len;

	while (gets(buf) != NULL) {

		puts("parsing with front of buffer anchored");
		start = 0;
		while ((len = get_word(buf, start, word, sizeof(word))) > 0) {
			printf("start=%d len=%d word=%s\n", start, len, word);
			start = len;
		}
		putchar('\n');

		puts("parsing with front of buffer updated");
		bufp = buf;
		while ((len = get_word(bufp, 0, word, sizeof(word))) > 0) {
			printf("start=%d len=%d word=%s\n", 0, len, word);
			bufp += len;
		}
		putchar('\n');

	}

	exit(0);
}
#endif

