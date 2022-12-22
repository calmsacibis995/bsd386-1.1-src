static char rcsid[] = "@(#)Id: in_string.c,v 5.1 1992/10/03 22:41:36 syd Exp ";

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
 * $Log: in_string.c,v $
 * Revision 1.2  1994/01/14  00:53:17  cp
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


int 
in_string(buffer, pattern)
char *buffer, *pattern;
{
	/** Returns TRUE iff pattern occurs IN IT'S ENTIRETY in buffer. **/ 

	register int i = 0, j = 0;
	
	while (buffer[i] != '\0') {
	  while (buffer[i++] == pattern[j++]) 
	    if (pattern[j] == '\0') 
	      return(TRUE);
	  i = i - j + 1;
	  j = 0;
	}
	return(FALSE);
}
