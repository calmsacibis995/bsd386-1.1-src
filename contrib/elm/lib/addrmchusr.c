
static char rcsid[] = "@(#)Id: addrmchusr.c,v 5.1 1992/10/03 22:41:36 syd Exp ";

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
 * $Log: addrmchusr.c,v $
 * Revision 1.2  1994/01/14  00:52:59  cp
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
addr_matches_user(addr,user)
register char *addr, *user;
{
	int len = strlen(user);
	static char c_before[] = "!:%";	/* these can appear before a username */
	static char c_after[] = ":%@";	/* these can appear after a username  */

	do {
	  if ( strncmp(addr,user,len) == 0 ) {
	    if ( addr[len] == '\0' || index(c_after,addr[len]) != NULL )
	      return TRUE;
	  }
	} while ( (addr=qstrpbrk(addr,c_before)) != NULL && *++addr != '\0' ) ;
	return FALSE;
}
