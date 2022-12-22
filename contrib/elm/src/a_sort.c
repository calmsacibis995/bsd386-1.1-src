
static char rcsid[] = "@(#)Id: a_sort.c,v 5.5 1993/06/12 05:27:44 syd Exp ";

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
 * $Log: a_sort.c,v $
 * Revision 1.2  1994/01/14  00:54:22  cp
 * 2.4.23
 *
 * Revision 5.5  1993/06/12  05:27:44  syd
 * Fix calling sequence error
 * From: Syd
 *
 * Revision 5.4  1993/04/12  01:10:15  syd
 * fix @aliasname sort problem
 * From: "Robert L. Howard" <robert.howard@matd.gatech.edu>
 *
 * Revision 5.3  1993/01/19  04:52:19  syd
 * 	add c)hange alias command to alias helpfile
 * 	if a deleted alias is changed, undelete it.  Also added the 'N'
 * flag to changed aliases to help remind the user.  Documented it.
 * Note:  if they mark the alias for deletion AFTER making the change it
 * WILL be deleted. (and marked accordingly)
 * 	modified alias mode title string to indicate when a resync was
 * needed.
 * 	allow editing alias file when none exist.
 * 	Now aliases are check for illegal characters (and WS) and
 * addresses are check for illegal WS when they are being entered.  If
 * anything illegal is found and message is printed and they keep entering
 * the item until they get it right.
 * 	I fixed a couple of places where int should be long to match
 * the declared type of alias_rec.length
 * From: "Robert L. Howard" <robert.howard@matd.gatech.edu>
 *
 * Revision 5.2  1992/11/22  00:05:07  syd
 * Fix bug where alias records were not sorting by both last and
 * first names.
 * From: "Robert L. Howard" <robert.howard@matd.gatech.edu>
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** Sort alias table by the field specified in the global
    variable "alias_sortby"...

**/

#include "headers.h"
#include "s_aliases.h"

char *alias_sort_name();

sort_aliases(entries, visible, are_in_aliases)
int entries, visible, are_in_aliases;
{
	/** Sort the header_table definitions... If 'visible', then
	    put the status lines etc **/
	
	long last_index = -1;
	int compare_aliases();	/* for sorting */

	dprint(2, (debugfile, "\n** sorting aliases by %s **\n\n", 
		alias_sort_name(FULL)));

	/* Don't get last_index if no entries or no current. */
	/* There would be no current if we are sorting for the first time. */
	if (entries > 0 && current > 0)
	  last_index = aliases[current-1]->length;

	if ((entries > 30) && visible && are_in_aliases) {
	    error1(catgets(elm_msg_cat, AliasesSet, AliasesSort,
		    "Sorting aliases by %s..."), alias_sort_name(FULL));
	}
	
	if (entries > 1)
	  qsort((char *) aliases, (unsigned) entries,
	        sizeof (struct alias_rec *), compare_aliases);

	if (last_index > -1)
	  alias_old_current(last_index);

	if (are_in_aliases) {
	    clear_error();
	}
}

int
compare_aliases(p1, p2)
struct alias_rec **p1, **p2;
{
	/** compare two aliases according to the sortby value.

	    Both are simple strcmp()'s on the alias or last_name
	    components of the alias.
	 **/

	register struct alias_rec *first, *second;
	register int ret;
	register long diff;

	first = *p1;
	second = *p2;

	/* If (only) one of the compares is a duplicate we want it
	 * to go to the end of the list regardless of the sorting
	 * method.
	 */
	if ((first->type ^ second->type) & DUPLICATE) {
	    if (first->type & DUPLICATE)
	        ret = 1;
	    else			/* It must be second... */
	        ret = -1;
	    return ret;
	}

	switch (abs(alias_sortby)) {
	case ALIAS_SORT:
		ret = strcmp(first->alias, second->alias);
		break;

	case NAME_SORT:
		ret = strcmp(first->last_name, second->last_name);
	     /*
	      * If equal on last name then compare on first name
	      * which is the first part of 'name'.
	      */
		if (ret == 0) {
		    ret = strcmp(first->name, second->name);
		}
		break;

	case TEXT_SORT:
		diff = (first->length - second->length);
 		if ( diff < 0 )	ret = -1;
 		else if ( diff > 0 ) ret = 1;
 		else ret = 0;
		break;

	default:
		/* never get this! */
		ret = 0;
		break;
	}

	if (alias_sortby < 0)
	  ret = -ret;

	return ret;
}

char *alias_sort_name(type)
int type;
{
	/** return the name of the current sort option...
	    type can be "FULL", "SHORT" or "PAD"
	**/
	int pad, abr;
	extern char *a_rev_alias_pad, *a_rev_alias_abr, *a_rev_alias_name,
			*a_rev_full_pad, *a_full_abr, *a_rev_full_name,
			*a_rev_text_pad, *a_text_abr, *a_rev_text_file,
			*a_alias_pad, *a_alias_abr, *a_alias_name,
			*a_full_pad, *a_full_abr, *a_full_name,
			*a_text_pad, *a_text_abr, *a_text_file;
	
	pad = (type == PAD);
	abr = (type == SHORT);

	if (alias_sortby < 0) {
	  switch (- alias_sortby) {
	    case ALIAS_SORT   : return(
		              pad?     "Reverse Alias Name      " :
			      abr?     "Reverse-Alias" :
				       "Reverse Alias Name");
	    case NAME_SORT    : return(
		              pad?     "Reverse Full (Real) Name" :
			      abr?     "Reverse-Name" :
				       "Reverse Full (Real) Name");
	    case TEXT_SORT    : return(
		              pad?     "Reverse Text File       " :
			      abr?     "Reverse-Text" :
				       "Reverse Text File");
	  }
	}
	else {
	  switch (alias_sortby) {
	    case ALIAS_SORT   : return(
		              pad?     "Alias Name              " :
			      abr?     "Alias" :
				       "Alias Name");
	    case NAME_SORT    : return(
		              pad?     "Full (Real) Name        " :
			      abr?     "Name" :
				       "Full (Real) Name");
	    case TEXT_SORT    : return(
		              pad?     "Text File               " :
			      abr?     "Text" :
				       "Text File");
	  }
	}

	return("*UNKNOWN-SORT-PARAMETER*");
}

alias_old_current(iindex)
long iindex;
{
	/** Set current to the message that has "index" as it's 
	    index number.  This is to track the current message
	    when we resync... **/

	register int i;

	dprint(4, (debugfile, "alias-old-current(%d)\n", iindex));

	for (i = 0; i < message_count; i++)
	  if (aliases[i]->length == iindex) {
	    current = i+1;
	    dprint(4, (debugfile, "\tset current to %d!\n", current));
	    return;
	  }

	dprint(4, (debugfile,
		"\tcouldn't find current index.  Current left as %d\n",
		current));
	return;		/* can't be found.  Leave it alone, then */
}
