
static char rcsid[] = "@(#)Id: find_alias.c,v 5.1 1992/10/03 22:58:40 syd Exp ";

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
 * $Log: find_alias.c,v $
 * Revision 1.2  1994/01/14  00:54:58  cp
 * 2.4.23
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** 
	Search the list of aliases for a specific address.  Search
	is limited to either SYSTEM or USER alias types....
**/

#include "headers.h"

extern int num_duplicates;

int
find_alias(word, alias_type)
char *word;
int alias_type;
{
	/** find word and return loc, or -1 **/
	register int loc = -1;

	/** cannot be an alias if its longer than NLEN chars **/
	if (strlen(word) > NLEN)
	    return(-1);

	while (++loc < (message_count+num_duplicates)) {
	    if ( aliases[loc]->type & alias_type ) {
	        if (istrcmp(word, aliases[loc]->alias) == 0)
	            return(loc);
	    }
	}

	return(-1);				/* Not found */
}
