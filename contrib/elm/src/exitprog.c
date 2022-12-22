
static char rcsid[] = "@(#)Id: exitprog.c,v 5.5 1993/04/12 02:34:36 syd Exp ";

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
 * $Log: exitprog.c,v $
 * Revision 1.2  1994/01/14  00:54:49  cp
 * 2.4.23
 *
 * Revision 5.5  1993/04/12  02:34:36  syd
 * I have now added a parameter which controls whether want_to clears the
 * line and centers the question or behaves like it did before. I also
 * added a 0 at the end of the parameter list to all the other calls to
 * want_to where a centered question on a clean line is not desirable.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.4  1993/01/19  03:55:39  syd
 * exitprog.c makes a reference to a null character pointer, savecopy.c
 * tries to reference an uninitialized variable, and the previous patch to
 * src/lock.c to get rid of an uninitialized variable compiler message
 * needed to be put in filter/lock.c as well.
 * From: wdh@grouper.mkt.csd.harris.com (W. David Higgins)
 *
 * Revision 5.3  1992/12/25  00:26:49  syd
 * Fix routine names
 * From: Syd
 *
 * Revision 5.2  1992/12/24  21:42:01  syd
 * Fix messages and nls messages to match.  Plus use want_to
 * where appropriate.
 * From: Syd, via prompting from Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

#include "headers.h"
#include "s_elm.h"

int
exit_prog()
{
	/** Exit, abandoning all changes to the mailbox (if there were
	    any, and if the user say's it's ok)
	**/

	char msg[SLEN], answer;
	register int i, changes;

	dprint(1, (debugfile, "\n\n-- exiting --\n\n"));

	/* Determine if any messages are scheduled for deletion, or if
	 * any message has changed status
	 */
	for (changes = 0, i = 0; i < message_count; i++)
	  if (ison(headers[i]->status, DELETED) || headers[i]->status_chgd)
	    changes++;
	
	if (changes) {
	  /* YES or NO on softkeys */
	  if (hp_softkeys) {
	    define_softkeys(YESNO);
	    softkeys_on();
	  }
	  if (changes == 1)
	    MCsprintf(msg, catgets(elm_msg_cat, ElmSet, ElmAbandonChange,
		"Abandon change to mailbox? (%c/%c) "), *def_ans_yes, *def_ans_no);
	  else
	    MCsprintf(msg, catgets(elm_msg_cat, ElmSet, ElmAbandonChangePlural,
		"Abandon changes to mailbox? (%c/%c) "), *def_ans_yes, *def_ans_no);
	  answer = want_to(msg, *def_ans_no, LINES-3, 0);

	  if(answer != *def_ans_yes) return -1;
	}

	fflush(stdout);
	return leave(0);
}
