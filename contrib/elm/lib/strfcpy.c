
static char rcsid[] = "@(#)Id: strfcpy.c,v 5.1 1993/01/19 04:46:21 syd Exp ";

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
 * $Log: strfcpy.c,v $
 * Revision 1.1  1994/01/14  01:35:05  cp
 * 2.4.23
 *
 * Revision 5.1  1993/01/19  04:46:21  syd
 * Initial Checkin
 *
 *
 ******************************************************************************/

/*
 * This is just like strncpy() except:
 *
 * - The result is guaranteed to be '\0' terminated.
 *
 * - strncpy is supposed to copy _exactly_ "len" characters.  We copy
 *   _at_most_ "len" characters.  (Actually "len-1" to save space for
 *   the trailing '\0'.  That is, strncpy() fills in the end with '\0'
 *   if strlen(src)<len.  We don't bother.
 */
char *strfcpy(dest, src, len)
register char *dest, *src;
register int len;
{
	char *dest0 = dest;
	while (--len > 0 && *src != '\0')
		*dest++ = *src++;
	*dest = '\0';
	return dest0;
}

#ifdef _TEST
#include <stdio.h>
main()
{
	char src[1024], dest[1024];
	int len;

	for (;;) {
		printf("string > ");
		fflush(stdout);
		if (gets(src) == NULL)
			break;
		printf("maxlen > ");
		fflush(stdout);
		if (gets(dest) == NULL)
			break;
		len = atoi(dest);
		(void) strfcpy(dest, src, len);
		printf("dest=\"%s\" maxlen=%d len=%d\n",
			dest, len, strlen(dest));
		putchar('\n');
	}
	putchar('\n');
	exit(0);
}
#endif

