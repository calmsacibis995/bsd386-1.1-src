
static char rcsid[] = "@(#)Id: limit.c,v 5.3 1993/05/31 19:17:02 syd Exp ";

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
 * $Log: limit.c,v $
 * Revision 1.2  1994/01/14  00:55:10  cp
 * 2.4.23
 *
 * Revision 5.3  1993/05/31  19:17:02  syd
 * While looking into the feasibility of adding `limit sender' as requested
 * on Usenet, I noticed that the limit code was replicated for each of
 * the supported conditions.  The following patch simplifies limit_selection()
 * by sharing the common code between all conditions.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
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

/** This stuff is inspired by MH and dmail and is used to 'select'
    a subset of the existing mail in the folder based on one of a
    number of criteria.  The basic tricks are pretty easy - we have
    as status of VISIBLE associated with each header stored in the
    (er) mind of the computer (!) and simply modify the commands to
    check that flag...the global variable `selected' is set to the
    number of messages currently selected, or ZERO if no select.
**/

#include "headers.h"
#include "s_elm.h"
#include "s_aliases.h"

#define TO		1
#define FROM		2

char *shift_lower();

int
limit()
{
	/** returns non-zero if we changed selection criteria = need redraw **/
	
	char criteria[STRING], first[STRING], rest[STRING], msg[STRING];
	static char *prompt = NULL;
	int  last_selected, all;

	last_selected = selected;
	all = 0;
	if (prompt == NULL) {
	  prompt = catgets(elm_msg_cat, ElmSet, ElmLimitEnterCriteria,
		"Enter criteria or '?' for help: ");
	}

	if (selected) {
	  MCsprintf(msg, catgets(elm_msg_cat, ElmSet, ElmLimitAlreadyHave,
		"Already have selection criteria - add more? (%c/%c) %c%c"),
		*def_ans_yes, *def_ans_no, *def_ans_no, BACKSPACE);
	  PutLine0(LINES-2, 0, msg);
	  criteria[0] = ReadCh();
	  if (tolower(criteria[0]) == *def_ans_yes) {
	    Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmYesWord, "Yes."), 0);
	    PutLine0(LINES-3, COLUMNS-30, catgets(elm_msg_cat, ElmSet, ElmLimitAdding,
		"Adding criteria..."));
	  } else {
	    Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmNoWord, "No."), 0);
	    selected = 0;
	    PutLine0(LINES-3, COLUMNS-30, catgets(elm_msg_cat, ElmSet, ElmLimitChanging,
		"Change criteria..."));
	  }
	}

	while(1) {
	  PutLine1(LINES-2, 0, prompt);
	  CleartoEOLN();

	  criteria[0] = '\0';
	  optionally_enter(criteria, LINES-2, strlen(prompt), FALSE, FALSE);
	  error("");
	  
	  if (strlen(criteria) == 0) {
	    /* no change */
	    selected = last_selected;
	    return(FALSE);	
	  }

	  split_word(criteria, first, rest);

	  if (inalias) {
	    if (equal(first, "?")) {
	      if (last_selected)
	        error(catgets(elm_msg_cat, AliasesSet, AliasesEnterLastSelected,
	          "Enter:{\"name\",\"alias\"} [pattern] OR {\"person\",\"group\",\"user\",\"system\"} OR \"all\""));
	      else
	        error(catgets(elm_msg_cat, AliasesSet, AliasesEnterSelected,
	          "Enter: {\"name\",\"alias\"} [pattern] OR {\"person\",\"group\",\"user\",\"system\"}"));
	      continue;
	    }
	    else if (equal(first, "all")) {
	      all++;
	      selected = 0;
	    }
	    else if (equal(first, "name"))
	      selected = limit_alias_selection(BY_NAME, rest, selected);
	    else if (equal(first, "alias"))
	      selected = limit_alias_selection(BY_ALIAS, rest, selected);
	    else if (equal(first, "person"))
	      selected = limit_alias_selection(PERSON, rest, selected);
	    else if (equal(first, "group"))
	      selected = limit_alias_selection(GROUP, rest, selected);
	    else if (equal(first, "user"))
	      selected = limit_alias_selection(USER, rest, selected);
	    else if (equal(first, "system"))
	      selected = limit_alias_selection(SYSTEM, rest, selected);
	    else {
	      error1(catgets(elm_msg_cat, ElmSet, ElmLimitNotValidCriterion,
		"\"%s\" not a valid criterion."), first);
	      continue;
	    }
	    break;
	  }
	  else {
	    if (equal(first, "?")) {
	      if (last_selected)
	        error(catgets(elm_msg_cat, ElmSet, ElmEnterLastSelected,
	          "Enter: {\"subject\",\"to\",\"from\"} [pattern] OR \"all\""));
	      else
	        error(catgets(elm_msg_cat, ElmSet, ElmEnterSelected,
		  "Enter: {\"subject\",\"to\",\"from\"} [pattern]"));
	      continue;
	    }
	    else if (equal(first, "all")) {
	      all++;
	      selected = 0;
	    }
	    else if (equal(first, "subj") || equal(first, "subject"))
	      selected = limit_selection(SUBJECT, rest, selected);
	    else if (equal(first, "to"))
	      selected = limit_selection(TO, rest, selected);
	    else if (equal(first, "from"))
	      selected = limit_selection(FROM, rest, selected);
	    else {
	      error1(catgets(elm_msg_cat, ElmSet, ElmLimitNotValidCriterion,
		"\"%s\" not a valid criterion."), first);
	      continue;
	    }
	    break;
	  }
	}

	if (all && last_selected)
	  strcpy(msg, catgets(elm_msg_cat, ElmSet, ElmLimitReturnToUnlimited,
		"Returned to unlimited display."));
	else {
	  if (inalias) {
	    if (selected > 1)
	      sprintf(msg, catgets(elm_msg_cat, AliasesSet,
		AliasesLimitMessagesSelected, "%d aliases selected."),
		selected);
	    else if (selected == 1)
	      strcpy(msg, catgets(elm_msg_cat, AliasesSet,
		AliasesLimitMessageSelected, "1 alias selected."));
	    else
	      strcpy(msg, catgets(elm_msg_cat, AliasesSet,
		AliasesLimitNoMessagesSelected, "No aliases selected."));
	  }
	  else {
	    if (selected > 1)
	      sprintf(msg, catgets(elm_msg_cat, ElmSet,
		ElmLimitMessagesSelected, "%d messages selected."), selected);
	    else if (selected == 1)
	      strcpy(msg, catgets(elm_msg_cat, ElmSet, ElmLimitMessageSelected,
		"1 message selected."));
	    else
	      strcpy(msg, catgets(elm_msg_cat, ElmSet,
		ElmLimitNoMessagesSelected, "No messages selected."));
	  }
	}
	set_error(msg);

	/* we need a redraw if there had been a selection or there is now. */
	if (last_selected || selected) {
	  /* if current message won't be on new display, go to first message */
	  if (selected && !(ifmain(headers[current-1]->status,
	                           aliases[current-1]->status) & VISIBLE))
	    current = visible_to_index(1)+1;
	  return(TRUE);
	} else {
	  return(FALSE);
	}
}

int
limit_selection(based_on, pattern, additional_criteria)
int based_on, additional_criteria;
char *pattern;
{
	/** Given the type of criteria, and the pattern, mark all
	    non-matching headers as ! VISIBLE.  If additional_criteria,
	    don't mark as visible something that isn't currently!
	**/

	register int iindex;
	register char *hdr_value;
	int count = 0;

	dprint(2, (debugfile, "\n\n\n**limit on %d - '%s' - (%s) **\n\n",
		   based_on, pattern, additional_criteria?"add'tl":"base"));

	for (iindex = 0 ; iindex < message_count ; iindex++) {

	    switch (based_on) {
	    case FROM:
		hdr_value = headers[iindex]->from;
		break;
	    case TO:
		hdr_value = headers[iindex]->to;
		break;
	    case SUBJECT:
	    default:
		hdr_value = headers[iindex]->subject;
		break;
	    }

	    if (!in_string(shift_lower(hdr_value), pattern))
		headers[iindex]->status &= ~VISIBLE;
	    else if (additional_criteria && !(headers[iindex]->status&VISIBLE))
		; /* leave this marked not visible */
	    else {
		headers[iindex]->status |= VISIBLE;
		count++;
		dprint(5, (debugfile,
		    "  Message %d (%s from %s) marked as visible\n",
		    iindex, headers[iindex]->subject, headers[iindex]->from));
	    }

	}

	dprint(4, (debugfile, "\n** returning %d selected **\n\n\n", count));
	return(count);
}

int
limit_alias_selection(based_on, pattern, additional_criteria)
int based_on, additional_criteria;
char *pattern;
{
	/** Given the type of criteria, and the pattern, mark all
	    non-matching aliases as ! VISIBLE.  If additional_criteria,
	    don't mark as visible something that isn't currently!
	**/

	register int iindex, count = 0;

	dprint(2, (debugfile, "\n\n\n**limit on %d - '%s' - (%s) **\n\n",
		   based_on, pattern, additional_criteria?"add'tl":"base"));

	if (based_on == BY_NAME) {
	  for (iindex = 0; iindex < message_count; iindex++)
	    if (! in_string(shift_lower(aliases[iindex]->name), pattern))
	      clearit(aliases[iindex]->status, VISIBLE);
	    else if (additional_criteria && 	
		     !(aliases[iindex]->status & VISIBLE))
	      clearit(aliases[iindex]->status, VISIBLE);	/* shut down! */
	    else { /* mark it as readable */
	      setit(aliases[iindex]->status, VISIBLE);
	      count++;
	      dprint(5, (debugfile,
		     "  Alias %d (%s, %s) marked as visible\n",
			iindex, aliases[iindex]->alias,
			aliases[iindex]->name));
	    }
	}
	else if (based_on == BY_ALIAS) {
	  for (iindex = 0; iindex < message_count; iindex++)
	    if (! in_string(shift_lower(aliases[iindex]->alias), pattern))
	      clearit(aliases[iindex]->status, VISIBLE);
	    else if (additional_criteria && 	
		     !(aliases[iindex]->status & VISIBLE))
	      clearit(aliases[iindex]->status, VISIBLE);	/* shut down! */
	    else { /* mark it as readable */
	      setit(aliases[iindex]->status, VISIBLE);
	      count++;
	      dprint(5, (debugfile, 
			"  Alias %d (%s, %s) marked as visible\n",
			iindex, aliases[iindex]->alias,
			aliases[iindex]->name));
	    }
	}
	else {
	  for (iindex = 0; iindex < message_count; iindex++)
	    if (! (based_on & aliases[iindex]->type))
	      clearit(aliases[iindex]->status, VISIBLE);
	    else if (additional_criteria && 	
		     !(aliases[iindex]->status & VISIBLE))
	      clearit(aliases[iindex]->status, VISIBLE);	/* shut down! */
	    else { /* mark it as readable */
	      setit(aliases[iindex]->status, VISIBLE);
	      count++;
	      dprint(5, (debugfile,
			"  Alias %d (%s, %s) marked as visible\n",
			iindex, aliases[iindex]->alias,
			aliases[iindex]->name));
	    }
	}

	dprint(4, (debugfile, "\n** returning %d selected **\n\n\n", count));

	return(count);
}

int
next_message(iindex, skipdel)
register int iindex, skipdel;
{
	/** Given 'iindex', this routine will return the actual iindex into the
	    array of the NEXT message, or '-1' iindex is the last.
	    If skipdel, return the iindex for the NEXT undeleted message.
	    If selected, return the iindex for the NEXT message marked VISIBLE.
	**/

	register int remember_for_debug, stat;

	if (iindex < 0) return(-1);	/* invalid argument value! */

	remember_for_debug = iindex;

	for(iindex++;iindex < message_count; iindex++) {
	  stat = ifmain(headers[iindex]->status, aliases[iindex]->status);
	  if (((stat & VISIBLE) || (!selected))
	    && (!(stat & DELETED) || (!skipdel))) {
	      dprint(9, (debugfile, "[Next%s%s: given %d returning %d]\n", 
		  (skipdel ? " undeleted" : ""),
		  (selected ? " visible" : ""),
		  remember_for_debug+1, iindex+1));
	      return(iindex);
	  }
	}
	return(-1);
}

int
prev_message(iindex, skipdel)
register int iindex, skipdel;
{
	/** Like next_message, but the PREVIOUS message. **/

	register int remember_for_debug, stat;

	if (iindex >= message_count) return(-1);	/* invalid argument value! */

	remember_for_debug = iindex;
	for(iindex--; iindex >= 0; iindex--) {
	  stat = ifmain(headers[iindex]->status, aliases[iindex]->status);
	  if (((stat & VISIBLE) || (!selected))
	    && (!(stat & DELETED) || (!skipdel))) {
	      dprint(9, (debugfile, "[Previous%s%s: given %d returning %d]\n", 
		  (skipdel ? " undeleted" : ""),
		  (selected ? " visible" : ""),
		  remember_for_debug+1, iindex+1));
	      return(iindex);
	  }
	}
	return(-1);
}


int
compute_visible(message)
int message;
{
	/** return the 'virtual' iindex of the specified message in the
	    set of messages - that is, if we have the 25th message as
	    the current one, but it's #2 based on our limit criteria,
	    this routine, given 25, will return 2.
	**/

	register int iindex, count = 0;

	if (! selected) return(message);

	if (message < 1) message = 1;	/* normalize */

	for (iindex = 0; iindex < message; iindex++)
	   if (ifmain(headers[iindex]->status,
	              aliases[iindex]->status) & VISIBLE)
	     count++;

	dprint(4, (debugfile,
		"[compute-visible: displayed message %d is actually %d]\n",
		count, message));

	return(count);
}

int
visible_to_index(message)
int message;
{
	/** Given a 'virtual' iindex, return a real one.  This is the
	    flip-side of the routine above, and returns (message_count+1)
	    if it cannot map the virtual iindex requested (too big) 
	**/

	register int iindex = 0, count = 0;

	for (iindex = 0; iindex < message_count; iindex++) {
	   if (ifmain(headers[iindex]->status,
	              aliases[iindex]->status) & VISIBLE)
	     count++;
	   if (count == message) {
	     dprint(4, (debugfile,
		     "visible-to-index: (up) index %d is displayed as %d\n",
		     message, iindex));
	     return(iindex);
	   }
	}

	dprint(4, (debugfile, "index %d is NOT displayed!\n", message));

	return(message_count+1);
}
