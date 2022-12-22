static char rcsid[] = "@(#)Id: okay_addr.c,v 5.1 1992/10/03 22:41:36 syd Exp ";

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
 * $Log: okay_addr.c,v $
 * Revision 1.2  1994/01/14  00:53:35  cp
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
okay_address(address, return_address)
char *address, *return_address;
{
	/** This routine checks to ensure that the address we just got
	    from the "To:" or "Cc:" line isn't us AND isn't the person	
	    who sent the message.  Returns true iff neither is the case **/

	char our_address[SLEN];
	struct addr_rec  *alternatives;

	if (in_list(address, return_address))
	  return(FALSE);

	if(in_list(address, username))
	  return(FALSE);

	sprintf(our_address, "%s!%s", hostname, username);
	if (in_list(address, our_address))
	  return(FALSE);

	sprintf(our_address, "%s!%s", hostfullname, username);
	if (in_list(address, our_address))
	  return(FALSE);

	sprintf(our_address, "%s@%s", username, hostname);
	if (in_list(address, our_address))
	  return(FALSE);

	sprintf(our_address, "%s@%s", username, hostfullname);
	if (in_list(address, our_address))
	  return(FALSE);

	alternatives = alternative_addresses;

	while (alternatives != NULL) {
	  if (in_list(address, alternatives->address))
	    return(FALSE);
	  alternatives = alternatives->next;
	}

	return(TRUE);
}
