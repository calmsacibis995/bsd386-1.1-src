
static char rcsid[] = "@(#)Id: a_edit.c,v 5.2 1992/10/31 19:27:44 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.2 $   $State: Exp $
 *
 * 			Copyright (c) 1988-1992 USENET Community Trust
 * 			Copyright (c) 1986,1987 Dave Taylor
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 *******************************************************************************
 * $Log: a_edit.c,v $
 * Revision 1.2  1994/01/14  00:54:16  cp
 * 2.4.23
 *
 * Revision 5.2  1992/10/31  19:27:44  syd
 * Clear selection on resync of aliases
 * From: Syd
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** This routine is for allowing the user to edit their aliases.text
    as they wish. 

**/

#include "headers.h"

edit_aliases_text()
{
	/** Allow the user to edit their aliases text, always resynchronizing
	    afterwards.   This routine calls the function edit_a_file()
	    to do the actual editing.  Thus editing is done the same
	    way here as in mailbox editing.

	    We will ALWAYS resync on the aliases text file
	    even if nothing has changed since, not unreasonably, it's
	    hard to figure out what occurred in the edit session...
	**/

	char     at_file[SLEN];

	sprintf(at_file,"%s/%s", home, ALIAS_TEXT);
	if (edit_a_file(at_file) == 0) {
	    return (0);
	}
/*
 *	Redo the hash table...always!
 */
	install_aliases();

/*
 *	clear any limit in effect.
 */
	selected = 0;

	return(1);
}
