
static char rcsid[] = "@(#)Id: delete.c,v 5.1 1992/10/03 22:58:40 syd Exp ";

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
 * $Log: delete.c,v $
 * Revision 1.2  1994/01/14  00:54:41  cp
 * 2.4.23
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/**  Delete or undelete files: just set flag in header record! 
     Also tags specified message(s)...

**/

#include "headers.h"
#include "s_elm.h"

char *show_status();

delete_msg(real_del, update_screen)
int real_del, update_screen;
{
	/** Delete current message.  If real-del is false, then we're
	    actually requested to toggle the state of the current
	    message... **/

	if (real_del) {
	  if (inalias) {
	    if (aliases[current-1]->type & SYSTEM)
	      error(catgets(elm_msg_cat, ElmSet, ElmNoDelSysAlias,
		"Can't delete a system alias!"));
	    else
	      setit(aliases[current-1]->status, DELETED);
	  }
	  else
	    setit(headers[current-1]->status, DELETED);
	}
	else {
	  if (inalias) {
	    if (aliases[current-1]->type & SYSTEM)
	      error(catgets(elm_msg_cat, ElmSet, ElmNoDelSysAlias,
		"Can't delete a system alias!"));
	    else if (ison(aliases[current-1]->status, DELETED))
	      clearit(aliases[current-1]->status, DELETED);
	    else
	      setit(aliases[current-1]->status, DELETED);
	  }
	  else if (ison(headers[current-1]->status, DELETED))
	    clearit(headers[current-1]->status, DELETED);
	  else
	    setit(headers[current-1]->status, DELETED);
	}

	if (update_screen)
	  show_msg_status(current-1);
}

undelete_msg(update_screen)
int update_screen;
{
	/** clear the deleted message flag **/

	if (inalias)
	  clearit(aliases[current-1]->status, DELETED);
	else
	  clearit(headers[current-1]->status, DELETED);

	if (update_screen)
	  show_msg_status(current-1);
}

show_msg_status(msg)
int msg;
{
	/** show the status of the current message only.  **/

	char tempbuf[3];

	strcpy(tempbuf, show_status(ifmain(headers[msg]->status,
	                                   aliases[msg]->status)));

	if (on_page(msg)) {
	  MoveCursor(((compute_visible(msg+1)-1) % headers_per_page) + 4, 2);
	  if (msg+1 == current && !arrow_cursor) {
	    StartBold();
	    Writechar( tempbuf[0] );
	    EndBold();
	  }
	  else
	    Writechar( tempbuf[0] );
	}
}

int
tag_message(update_screen)
int update_screen;
{
	/** Tag current message and return TRUE.
	    If already tagged, untag it and return FALSE. **/

	int istagged;

	if (ison(ifmain(headers[current-1]->status,
	                aliases[current-1]->status), TAGGED)) {
	  if (inalias)
	    clearit(aliases[current-1]->status, TAGGED);
	  else
	    clearit(headers[current-1]->status, TAGGED);
	  istagged = FALSE;
	} else {
	  if (inalias)
	    setit(aliases[current-1]->status, TAGGED);
	  else
	    setit(headers[current-1]->status, TAGGED);
	  istagged = TRUE;
	}

	if(update_screen)
	    show_msg_tag(current-1);
	return(istagged);
}

show_msg_tag(msg)
int msg;
{
	/** show the tag status of the current message only.  **/

	if (on_page(msg)) {
	  MoveCursor(((compute_visible(msg+1)-1) % headers_per_page) + 4, 4);
	  if (msg+1 == current && !arrow_cursor) {
	    StartBold();
	    Writechar( ison(ifmain(headers[msg]->status,
	                           aliases[msg]->status), TAGGED)? '+' : ' ');
	    EndBold();
	  }
	  else
	    Writechar( ison(ifmain(headers[msg]->status,
	                           aliases[msg]->status), TAGGED)? '+' : ' ');
	}	
}

show_new_status(msg)
int msg;
{
	/** If the specified message is on this screen, show
	    the new status (could be marked for deletion now,
	    and could have tag removed...)
	**/

	if (on_page(msg)) 
	  if (msg+1 == current && !arrow_cursor) {
	    StartBold();
	    PutLine2(((compute_visible(msg+1)-1) % headers_per_page) + 4,
		   2, "%s%c", show_status(ifmain(headers[msg]->status,
		                                 aliases[msg]->status)),
		   ison(ifmain(headers[msg]->status,
		               aliases[msg]->status), TAGGED )? '+' : ' ');
	    EndBold();
	  }
	  else
	    PutLine2(((compute_visible(msg+1)-1) % headers_per_page) + 4,
		   2, "%s%c", show_status(ifmain(headers[msg]->status,
		                                 aliases[msg]->status)),
		   ison(ifmain(headers[msg]->status,
		               aliases[msg]->status), TAGGED )? '+' : ' ');
}
