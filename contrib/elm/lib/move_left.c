static char rcsid[] = "@(#)Id: move_left.c,v 5.1 1992/10/03 22:41:36 syd Exp ";

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
 * $Log: move_left.c,v $
 * Revision 1.2  1994/01/14  00:53:30  cp
 * 2.4.23
 *
 * Revision 5.1  1992/10/03  22:41:36  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** 

**/

#include "headers.h"


move_left(string, chars)
char *string;
int  chars;
{
	/** moves string chars characters to the left DESTRUCTIVELY **/

	register char *source, *destination;

	source = string + chars;
	destination = string;
	while (*source != '\0' && *source != '\n')
		*destination++ = *source++;

	*destination = '\0';
}
