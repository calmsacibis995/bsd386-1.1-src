static char rcsid[] = "@(#)Id: reverse.c,v 5.1 1992/10/03 22:41:36 syd Exp ";

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
 * $Log: reverse.c,v $
 * Revision 1.2  1994/01/14  00:53:44  cp
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


reverse(string)
char *string;
{
	/** reverse string... pretty trivial routine, actually! **/

	register char *head, *tail, c;

	for (head = string, tail = string + strlen(string) - 1; head < tail; head++, tail--)
		{
		c = *head;
		*head = *tail;
		*tail = c;
		}
}
