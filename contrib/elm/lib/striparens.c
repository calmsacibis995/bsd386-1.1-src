static char rcsid[] = "@(#)Id: striparens.c,v 5.2 1993/06/10 03:09:06 syd Exp ";

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
 * $Log: striparens.c,v $
 * Revision 1.2  1994/01/14  00:53:48  cp
 * 2.4.23
 *
 * Revision 5.2  1993/06/10  03:09:06  syd
 * Greatly simplified "lib/striparens.c" to use new rfc822_toklen() routine.
 * This cut more than 50% out of the object size.  Also added _TEST case.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.1  1992/10/03  22:41:36  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/* 
 * strip_parens() - Delete all (parenthesized) information from a string.
 * get_parens() - Extract all (parenthesized) information from a string.
 *
 * These routines handle RFC-822 comments.  Nested parens are understood.
 * get_parens() does not include the parens in the return result.  Both
 * routines are non-destructive.  They return a pointer to static data
 * that will be overwritten on the next call to either routine.
 */

#include "headers.h"

static char paren_buffer[VERY_LONG_STRING];

char *strip_parens(src)
register char *src;
{
	register int len;
	register char *dest = paren_buffer;

	while (*src != '\0') {
		len = rfc822_toklen(src);
		if (*src != '(') {	/*)*/
			strncpy(dest, src, len);
			dest += len;
		}
		src += len;
	}
	*dest = '\0';
	return paren_buffer;
}

char *get_parens(src)
register char *src;
{
	register int len;
	register char *dest = paren_buffer;

	while (*src != '\0') {
		len = rfc822_toklen(src);
		if (len > 2 && *src == '(') {	/*)*/
			strncpy(dest, src+1, len-2);
			dest += (len-2);
		}
		src += len;
	}
	*dest = '\0';
	return paren_buffer;
}

#ifdef _TEST
main()
{
	char buf[1024];
	while (fputs("\nstr> ", stdout), gets(buf) != NULL) {
		printf("strip_parens() |%s|\n", strip_parens(buf));
		printf("get_parens()   |%s|\n", get_parens(buf));
	}
	putchar('\n');
	exit(0);
}
#endif

