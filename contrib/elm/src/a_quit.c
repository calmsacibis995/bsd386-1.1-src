
static char rcsid[] = "@(#)Id: a_quit.c,v 5.4 1993/04/12 02:34:36 syd Exp ";

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
 * $Log: a_quit.c,v $
 * Revision 1.2  1994/01/14  00:54:18  cp
 * 2.4.23
 *
 * Revision 5.4  1993/04/12  02:34:36  syd
 * I have now added a parameter which controls whether want_to clears the
 * line and centers the question or behaves like it did before. I also
 * added a 0 at the end of the parameter list to all the other calls to
 * want_to where a centered question on a clean line is not desirable.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.3  1992/12/24  21:42:01  syd
 * Fix messages and nls messages to match.  Plus use want_to
 * where appropriate.
 * From: Syd, via prompting from Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.2  1992/12/11  02:09:06  syd
 * Fix where the user creates a first new alias, then deletes it, the
 * alias stays on screen, but the file really will be empty if it was the
 * last alias, so the retry to delete gives 'cannot open ...file' messages
 * From: "Robert L. Howard" <robert.howard@matd.gatech.edu>
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** a_quit: leave the aliases menu and return to the main menu.
  
**/

#include "headers.h"
#include "s_aliases.h"
#include <errno.h>


extern int errno;		/* system error number on failure */

int
delete_aliases(newaliases, prompt)
int newaliases, prompt;
{
/*
 *	Update aliases by processing deletions.  Prompting is
 *	done as directed by user input and elmrc options.
 *
 *	If "prompt" is false, then no prompting is done.
 *	Otherwise prompting is dependent upon the variable
 *	question_me, as set by an elmrc option.  This behavior
 *	makes the 'q' command prompt just like '$', while
 *	retaining the 'Q' command for a quick exit that never
 *	prompts.
 *
 *	Return	1		Aliases were deleted
 * 		newalias	Aliases were not deleted
 *
 *	The return allows the caller to determine if the "newalias"
 *	routine should be run.
 */

	char buffer[SLEN];
	char **list;

	register int to_delete = 0, to_keep = 0, i,
		     marked_deleted = 0,
		     ask_questions;
	char answer;

	dprint(1, (debugfile, "\n\n-- leaving aliases --\n\n"));

	if (message_count == 0)
	  return(newaliases);	/* nothing changed */

	ask_questions = ((!prompt) ? FALSE : question_me);

	/* YES or NO on softkeys */
	if (hp_softkeys && ask_questions) {
	  define_softkeys(YESNO);
	  softkeys_on();
	}

	/* Determine if deleted messages are really to be deleted */

	/* we need to know how many there are to delete */
	for (i=0; i<message_count; i++)
	  if (ison(aliases[i]->status, DELETED))
	    marked_deleted++;

	dprint(6, (debugfile, "Number of aliases marked deleted = %d\n",
	            marked_deleted));

        if(marked_deleted) {
	  answer = (always_del ? *def_ans_yes : *def_ans_no);	/* default answer */
	  if(ask_questions) {
	    if (marked_deleted == 1)
	      MCsprintf(buffer, catgets(elm_msg_cat, AliasesSet, AliasesDelete,
			"Delete 1 alias? (%c/%c) "), *def_ans_yes, *def_ans_no);
	    else
	      MCsprintf(buffer, catgets(elm_msg_cat, AliasesSet, AliasesDeletePlural,
			"Delete %d aliases? (%c/%c) "),
			marked_deleted, *def_ans_yes, *def_ans_no);
	                    
	    answer = want_to(buffer, answer, LINES-3, 0);
	  }

	  if(answer == *def_ans_yes) {
	    list = (char **) malloc(marked_deleted*sizeof(*list));
	    for (i = 0; i < message_count; i++) {
	      if (ison(aliases[i]->status, DELETED)) {
		list[to_delete] = aliases[i]->alias;
	        dprint(8,(debugfile, "Alias number %d, %s is deletion %d\n",
	               i, list[to_delete], to_delete));
	        to_delete++;
	      }
	      else {
	        to_keep++;
	      }
	    }
	   /*
	    * Formulate message as to number of keeps and deletes.
	    * This is only complex so that the message is good English.
	    */
	    if (to_keep > 0) {
	      current = 1;		/* Reset current alias */
	      if (to_keep == 1)
		sprintf(buffer, catgets(elm_msg_cat, AliasesSet, AliasesKeepDelete,
			  "[Keeping 1 alias and deleting %d.]"), to_delete);
	      else
		MCsprintf(buffer, catgets(elm_msg_cat, AliasesSet, AliasesKeepDeletePlural,
			  "[Keeping %d aliases and deleting %d.]"), to_keep, to_delete);
	    }
	    else {
	      current = 0;		/* No aliases left */
	      sprintf(buffer, catgets(elm_msg_cat, AliasesSet, AliasesDeleteAll,
			  "[Deleting all aliases.]"));
	    }

	    dprint(2, (debugfile, "Action: %s\n", buffer));
	    error(buffer);

	    delete_from_alias_text(list, to_delete);

	    free((char *) list);
	  }
	}
	dprint(3, (debugfile, "Aliases deleted: %d\n", to_delete));

	/* If all aliases are to be kept we don't need to do anything
	 * (like run newalias for the deleted messages).
	 */

	if(to_delete == 0) {
	  dprint(3, (debugfile, "Aliases kept as is!\n"));
	  return(newaliases);
	}

	return(1);
}

exit_alias()
{

	register int i;

	/* Clear the deletes from all aliases.  */

	for(i = 0; i < message_count; i++)
	  if (ison(aliases[i]->status, DELETED))
	    clearit(aliases[i]->status, DELETED);

	dprint(6, (debugfile, "\nexit_alias:  Done clearing deletes.\n"));

}
