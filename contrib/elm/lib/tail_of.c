static char rcsid[] = "@(#)Id: tail_of.c,v 5.1 1992/10/03 22:41:36 syd Exp ";

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
 * $Log: tail_of.c,v $
 * Revision 1.2  1994/01/14  00:53:49  cp
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
tail_of(from, buffer, to)
char *from, *buffer, *to;
{
	/** Return last two words of 'from'.  This is to allow
	    painless display of long return addresses as simply the
	    machine!username. 
	    Or if the first word of the 'from' address is username or
	    full_username and 'to' is not NULL, then use the 'to' line
	    instead of the 'from' line.
	    If the 'to' line is used, return 1, else return 0.

	    Also modified to know about X.400 addresses (sigh) and
	    that when we ask for the tail of an address similar to
	    a%b@c we want to get back a@b ...
	**/

	/** Note: '!' delimits Usenet nodes, '@' delimits ARPA nodes,
	          ':' delimits CSNet & Bitnet nodes, '%' delimits multi-
		  stage ARPA hops, and '/' delimits X.400 addresses...
	          (it is fortunate that the ASCII character set only has
	   	  so many metacharacters, as I think we're probably using
		  them all!!) **/

	register int loc, i = 0, cnt = 0, using_to = 0;
	
#ifndef INTERNET
	
	/** let's see if we have an address appropriate for hacking: 
	    what this actually does is remove the spuriously added
	    local bogus Internet header if we have one and the message
	    has some sort of UUCP component too...
	**/

	sprintf(buffer, "@%s", hostfullname); 
	if (chloc(from,'!') != -1 && in_string(from, buffer))
	   from[strlen(from)-strlen(buffer)] = '\0';

#endif

	/**
	    Produce a simplified version of the from into buffer.  If the
	    from is just "username" or "Full Username" it will be preserved.
	    If it is an address, the rightmost "stuff!stuff", "stuff@stuff",
	    or "stuff:stuff" will be used.
	**/
	for (loc = strlen(from)-1; loc >= 0 && cnt < 2; loc--) {
	  if (from[loc] == BANG || from[loc] == AT_SIGN ||
	      from[loc] == COLON) cnt++;
	  if (cnt < 2) buffer[i++] = from[loc];
	}
	buffer[i] = '\0';
	reverse(buffer);

#ifdef MMDF
	if (strlen(buffer) == 0) {
	  if(to && *to != '\0' && !addr_matches_user(to, username)) {
	    tail_of(to, buffer, (char *)0);
	    using_to = 1;
	  } else
	    strcpy(buffer, full_username);
        }
#endif /* MMDF */

	if ( strcmp(buffer,full_username) == 0 ||
	  addr_matches_user(buffer,username) ) {

	  /* This message is from the user, so use the "to" header instead
	   * if possible, to be more informative. Otherwise be nice and
	   * use full_username rather than the bare username even if
	   * we've only matched on the bare username.
	   */

	  if(to && *to != '\0' && !addr_matches_user(to, username)) {
	    tail_of(to, buffer, (char *)0);
	    using_to = 1;
	  } else
	    strcpy(buffer, full_username);

	} else {					/* user%host@host? */

	  /** The logic here is that we're going to use 'loc' as a handy
	      flag to indicate if we've hit a '%' or not.  If we have,
	      we'll rewrite it as an '@' sign and then when we hit the
	      REAL at sign (we must have one) we'll simply replace it
	      with a NULL character, thereby ending the string there.
	  **/

	  loc = 0;

	  for (i=0; buffer[i] != '\0'; i++)
	    if (buffer[i] == '%') {
	      buffer[i] = AT_SIGN;
	      loc++;
	    }
	    else if (buffer[i] == AT_SIGN && loc)
	      buffer[i] = '\0';
	}
	return(using_to);

}
