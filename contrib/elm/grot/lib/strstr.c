
static char rcsid[] = "@(#)Id: strstr.c,v 5.1 1992/10/03 22:41:36 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.1 $   $State: Exp $
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
 * $Log: strstr.c,v $
 * Revision 1.1  1994/01/14  01:34:47  cp
 * 2.4.23
 *
 * Revision 5.1  1992/10/03  22:41:36  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** look for substring in string
**/

#include "headers.h"

/*
 * strstr() -- Locates a substring.
 *
 * This is a replacement for the POSIX function which does not
 * appear on all systems.
 *
 * Synopsis:
 *	#include <string.h>
 *	char *strstr(const char *s1, const char *s2);
 *
 * Arguments:
 *	s1	Pointer to the subject string.
 *	s2	Pointer to the substring to locate.
 *
 * Returns:
 *	A pointer to the located string or NULL
 *
 * Description:
 *	The strstr() function locates the first occurence in s1 of
 *	the string s2.  The terminating null characters are not
 *	compared.
 */

char *strstr(s1, s2)
char *s1, *s2;
{
	int len;
	char *ptr;
	char *tmpptr;

	ptr = NULL;
	len = strlen(s2);

	if ( len <= strlen(s1)) {
	    tmpptr = s1;
	    while ((ptr = index(tmpptr, (int)*s2)) != NULL) {
	        if (strncmp(ptr, s2, len) == 0) {
	            break;
	        }
	        tmpptr = ptr+1;
	    }
	}
	return (ptr);
}
