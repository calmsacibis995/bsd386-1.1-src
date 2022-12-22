
static char rcsid[] = "@(#)Id: sort.c,v 5.2 1993/04/12 03:42:56 syd Exp ";

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
 * $Log: sort.c,v $
 * Revision 1.2  1994/01/14  00:55:52  cp
 * 2.4.23
 *
 * Revision 5.2  1993/04/12  03:42:56  syd
 * I noticed when I was sorting a mailbox by subject, that 2 messages with
 * the following subjects
 *
 *     Subject: Re: Reading news
 *     Subject: Reading news
 *
 * they were sorted as shown above even though the "Re:" message was
 * "Sent" after the original.  It turns out that the routine skip_re has a
 * bug.  If the actual subject (the part after the "Re: ") starts with the
 * characters "re" skip_re will erroneously not strip the "Re:" part at
 * all.  The following patch fixes that behaviour.
 * From: Larry Philps <larryp@sco.COM>
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** Sort folder header table by the field specified in the global
    variable "sortby"...if we're sorting by something other than
    the default SENT_DATE, also put some sort of indicator on the
    screen.

**/

#include "headers.h"
#include "s_elm.h"

char *sort_name(), *skip_re();

void
find_old_current(iindex)
int iindex;
{
	/** Set current to the message that has "index" as it's 
	    index number.  This is to track the current message
	    when we resync... **/

	register int i;

	dprint(4, (debugfile, "find-old-current(%d)\n", iindex));

	for (i = 0; i < message_count; i++)
	  if (headers[i]->index_number == iindex) {
	    current = i+1;
	    dprint(4, (debugfile, "\tset current to %d!\n", current));
	    return;
	  }

	dprint(4, (debugfile, 
		"\tcouldn't find current index.  Current left as %d\n",
		current));
	return;		/* can't be found.  Leave it alone, then */
}

sort_mailbox(entries, visible)
int entries, visible;
{
	/** Sort the header_table definitions... If 'visible', then
	    put the status lines etc **/
	
	int last_index = -1;
	int compare_headers();	/* for sorting */

	dprint(2, (debugfile, "\n** sorting folder by %s **\n\n", 
		sort_name(FULL)));

	/* Don't get last_index if no entries or no current. */
	/* There would be no current if we are sorting a new mail file. */
	if (entries > 0 && current > 0)
	  last_index = headers[current-1]->index_number;

	if (entries > 30 && visible)  
	  error1(catgets(elm_msg_cat, ElmSet, ElmSortingMessagesBy, 
		"Sorting messages by %s..."), sort_name(FULL));
	
	if (entries > 1)
	  qsort((char *) headers, (unsigned) entries,
	      sizeof (struct header_rec *) , compare_headers);

	if (last_index > -1)
	  find_old_current(last_index);

	clear_error();
}

int
compare_headers(p1, p2)
struct header_rec **p1, **p2;
{
	/** compare two headers according to the sortby value.

	    Sent Date uses a routine to compare two dates,
	    Received date is keyed on the file offsets (think about it)
	    Sender uses the truncated from line, same as "build headers",
	    and size and subject are trivially obvious!!
	    (actually, subject has been modified to ignore any leading
	    patterns [rR][eE]*:[ \t] so that replies to messages are
	    sorted with the message (though a reply will always sort to
	    be 'greater' than the basenote)
	 **/

	char from1[SLEN], from2[SLEN];	/* sorting buffers... */
	struct header_rec *first, *second;
	int ret;
	long diff;
	
	first = *p1;
	second = *p2;

	switch (abs(sortby)) {
	case SENT_DATE:
 		diff = first->time_sent - second->time_sent;
 		if ( diff < 0 )	ret = -1;
 		else if ( diff > 0 ) ret = 1;
 		else ret = 0;
  		break;

	case RECEIVED_DATE:
 		diff = first->received_time - second->received_time;
 		if ( diff < 0 )	ret = -1;
 		else if ( diff > 0 ) ret = 1;
 		else ret = 0;
  		break;

	case SENDER:
		tail_of(first->from, from1, first->to);
		tail_of(second->from, from2, second->to);
		ret = strcmp(from1, from2);
		break;

	case SIZE:
		ret = (first->lines - second->lines);
		break;

	case MAILBOX_ORDER:
		ret = (first->index_number - second->index_number);
		break;

	case SUBJECT:
		/* need some extra work 'cause of STATIC buffers */
		strcpy(from1, skip_re(shift_lower(first->subject)));
		ret = strcmp(from1, skip_re(shift_lower(second->subject)));
		break;

	case STATUS:
		ret = (first->status - second->status);
		break;

	default:
		/* never get this! */
		ret = 0;
		break;
	}

	/* on equal status, use sent date as second sort param. */
	if (ret == 0) {
		diff = first->time_sent - second->time_sent;
		if ( diff < 0 )	ret = -1;
		else if ( diff > 0 ) ret = 1;
		else ret = 0;
	}

	if (sortby < 0)
	  ret = -ret;

	return ret;
}

char *sort_name(type)
int type;
{
	/** return the name of the current sort option...
	    type can be "FULL", "SHORT" or "PAD"
	**/
	int pad, abr;
	
	pad = (type == PAD);
	abr = (type == SHORT);

	if (sortby < 0) {
	  switch (- sortby) {
	    case SENT_DATE    : return( 
		              pad?     catgets(elm_msg_cat, ElmSet, ElmPadRevDateMailSent, "Reverse Date Mail Sent  ") : 
			      abr?     catgets(elm_msg_cat, ElmSet, ElmAbrtRevDateMailSent, "Reverse-Sent") :
				       catgets(elm_msg_cat, ElmSet, ElmLongRevDateMailSent, "Reverse Date Mail Sent"));
	    case RECEIVED_DATE: return(
			      pad?     catgets(elm_msg_cat, ElmSet, ElmPadRevRecv, "Reverse Date Mail Rec'vd") :
			      abr?     catgets(elm_msg_cat, ElmSet, ElmAbrRevRecv, "Reverse-Received"):
				       catgets(elm_msg_cat, ElmSet, ElmLongRevRecv, "Reverse Date Mail Rec'vd"));

	    case MAILBOX_ORDER: return(
			      pad?     catgets(elm_msg_cat, ElmSet, ElmPadRevMailbox, "Reverse Mailbox Order   ") :
			      abr?     catgets(elm_msg_cat, ElmSet, ElmAbrRevMailbox, "Reverse-Mailbox"):
			               catgets(elm_msg_cat, ElmSet, ElmLongRevMailbox, "Reverse Mailbox Order"));

	    case SENDER       : return(
			      pad?     catgets(elm_msg_cat, ElmSet, ElmPadRevSender, "Reverse Message Sender  ") : 
			      abr?     catgets(elm_msg_cat, ElmSet, ElmAbrRevSender, "Reverse-From"):
				       catgets(elm_msg_cat, ElmSet, ElmLongRevSender, "Reverse Message Sender"));
	    case SIZE         : return(
			      pad?     catgets(elm_msg_cat, ElmSet, ElmPadRevLines, "Reverse Lines in Message") :
			      abr?     catgets(elm_msg_cat, ElmSet, ElmAbrRevLines, "Reverse-Lines") : 
				       catgets(elm_msg_cat, ElmSet, ElmLongRevLines, "Reverse Lines in Message"));
	    case SUBJECT      : return(
			      pad?     catgets(elm_msg_cat, ElmSet, ElmPadRevSubject, "Reverse Message Subject ") : 
			      abr?     catgets(elm_msg_cat, ElmSet, ElmAbrRevSubject, "Reverse-Subject") : 
				       catgets(elm_msg_cat, ElmSet, ElmLongRevSubject, "Reverse Message Subject"));
	    case STATUS	      : return(
			      pad?     catgets(elm_msg_cat, ElmSet, ElmPadRevStatus, "Reverse Message Status  ") :
			      abr?     catgets(elm_msg_cat, ElmSet, ElmAbrRevStatus, "Reverse-Status"):
			               catgets(elm_msg_cat, ElmSet, ElmLongRevStatus, "Reverse Message Status"));
	  }
	}
	else {
	  switch (sortby) {
	    case SENT_DATE    : return( 
		                pad?   catgets(elm_msg_cat, ElmSet, ElmPadMailSent, "Date Mail Sent          ") : 
		                abr?   catgets(elm_msg_cat, ElmSet, ElmAbrMailSent, "Sent") : 
				       catgets(elm_msg_cat, ElmSet, ElmLongMailSent, "Date Mail Sent"));
	    case RECEIVED_DATE: return(
	                        pad?   catgets(elm_msg_cat, ElmSet, ElmPadMailRecv, "Date Mail Rec'vd        ") :
	                        abr?   catgets(elm_msg_cat, ElmSet, ElmAbrMailRecv, "Received") :
				       catgets(elm_msg_cat, ElmSet, ElmLongMailRecv, "Date Mail Rec'vd"));
	    case MAILBOX_ORDER: return(
	                        pad?   catgets(elm_msg_cat, ElmSet, ElmPadMailbox, "Mailbox Order           ") :
	                        abr?   catgets(elm_msg_cat, ElmSet, ElmAbrMailbox, "Mailbox") :
	                               catgets(elm_msg_cat, ElmSet, ElmLongMailbox, "Mailbox Order"));
	    case SENDER       : return(
			        pad?   catgets(elm_msg_cat, ElmSet, ElmPadSender, "Message Sender          ") : 
			        abr?   catgets(elm_msg_cat, ElmSet, ElmAbrSender, "From") : 
				       catgets(elm_msg_cat, ElmSet, ElmLongSender, "Message Sender"));
	    case SIZE         : return(
	    			pad?   catgets(elm_msg_cat, ElmSet, ElmPadLines, "Lines in Message        ") :
	    			abr?   catgets(elm_msg_cat, ElmSet, ElmAbrLines, "Lines") :
	    			       catgets(elm_msg_cat, ElmSet, ElmLongLines, "Lines in Message"));
	    case SUBJECT      : return(
			        pad?   catgets(elm_msg_cat, ElmSet, ElmPadSubject, "Message Subject         ") : 
			        abr?   catgets(elm_msg_cat, ElmSet, ElmAbrSubject, "Subject") : 
				       catgets(elm_msg_cat, ElmSet, ElmLongSubject, "Message Subject"));
	    case STATUS	      : return(
			        pad?   catgets(elm_msg_cat, ElmSet, ElmPadStatus, "Message Status          ") :
			        abr?   catgets(elm_msg_cat, ElmSet, ElmAbrStatus, "Status") :
			               catgets(elm_msg_cat, ElmSet, ElmLongStatus, "Message Status"));
	  }
	}

	return(catgets(elm_msg_cat, ElmSet, ElmSortUnknown, "*UNKNOWN-SORT-PARAMETER*"));
}

char *skip_re(string)
char *string;
{
	/** this routine returns the given string minus any sort of
	    "re:" prefix.  specifically, it looks for, and will
	    remove, any of the pattern:

		( [Rr][Ee][^:]:[ ] ) *

	    If it doesn't find a ':' in the line it will return it
	    intact, just in case!
	**/

	static char buffer[SLEN];
	register int i=0;

	while (whitespace(string[i])) i++;

	/* Initialize buffer */
	strcpy(buffer, (char *) string);

	do {
	  if (string[i] == '\0') return( (char *) buffer);	/* forget it */

	  if (string[i] != 'r' || string[i+1] != 'e') 
	    return( (char *) buffer);				/*   ditto   */

	  i += 2;	/* skip the "re" */

	  while (string[i] != ':') 
	    if (string[i] == '\0')
	      return( (char *) buffer);		      /* no colon in string! */
	    else
	      i++;

	  /* now we've gotten to the colon, skip to the next non-whitespace  */

	  i++;	/* past the colon */

	  while (whitespace(string[i])) i++;

	  /* Now save the resulting subject for return purposes */
	  strcpy(buffer, (char *) string + i);

	} while (string[i] == 'r' && string[i+1] == 'e');
 
	return( (char *) buffer);
}
