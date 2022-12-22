
static char rcsid[] = "@(#)Id: bouncebk.c,v 5.2 1993/02/03 19:06:31 syd Exp ";

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
 * $Log: bouncebk.c,v $
 * Revision 1.2  1994/01/14  00:54:33  cp
 * 2.4.23
 *
 * Revision 5.2  1993/02/03  19:06:31  syd
 * Remove extra strchr/strcat/strcpy et al declarations
 * From: Syd
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** This set of routines implement the bounceback feature of the mailer.
    This feature allows mail greater than 'n' hops away (n specified by
    the user) to have a 'cc' to the user through the remote machine.  

    Due to the vagaries of the Internet addressing (uucp -> internet -> uucp)
    this will NOT generate bounceback copies with mail to an internet host!

**/

#include "headers.h"

char *bounce_off_remote();		/* forward declaration */

int
uucp_hops(to)
register char *to;
{	
	/** Given the entire "To:" list, return the number of hops in the
	    first address (a hop = a '!') or ZERO iff the address is to a
  	    non uucp address.
	**/

	register int hopcount = 0, len;

	while (*to) {
	  len = len_next_part(to);
	  if (len == 1) {
	    if (whitespace(*to))
	      break;
	    
	    if (*to == '!')
	      hopcount++;
	    else if (*to == '@' || *to == '%' || *to == ':')
	      return(0);	/* don't continue! */
	  }
	  to += len;
	}

	return(hopcount);
}
	
char *bounce_off_remote(to)
register char *to;
{
	/** Return an address suitable for framing (no, that's not it...)
	    Er, suitable for including in a 'cc' line so that it ends up
	    with the bounceback address.  The method is to take the first 
	    address in the To: entry and break it into machines, then 
	    build a message up from that.  For example, consider the
	    following address:
			a!b!c!d!e!joe
	    the bounceback address would be;
			a!b!c!d!e!d!c!b!a!ourmachine!ourname
	    simple, eh?
	**/

	static char address[LONG_STRING];	/* BEEG address buffer! */

	char   host[MAX_HOPS][NLEN];	/* for breaking up addr */
	register int hostcount = 0, hindex = 0, iindex, len;

	while (*to) {
	  len = len_next_part(to);
	  if (len == 1) {
	    if (whitespace(*to))
	      break;
	    
	    if (*to == '!') {
	      host[hostcount][hindex] = '\0';
	      hostcount++;
	      hindex = 0;
	    } else 
	      host[hostcount][hindex++] = *to++;
	  } else {
	    while (--len >= 0)
	      host[hostcount][hindex++] = *to++;
	  }
	}

	/* we have hostcount hosts... */

	strcpy(address, host[0]);	/* initialize it! */

	for (iindex=1; iindex < hostcount; iindex++) {
	  strcat(address, "!");
	  strcat(address, host[iindex]);
	}
	
	/* and now the same thing backwards... */

	for (iindex = hostcount -2; iindex > -1; iindex--) {
	  strcat(address, "!");
	  strcat(address, host[iindex]);
	}

	/* and finally, let's tack on our machine and login name */

	strcat(address, "!");
	strcat(address, hostname);
	strcat(address, "!");
	strcat(address, username);

	/* and we're done!! */

	return( (char *) address );
}
