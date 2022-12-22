
static char rcsid[] = "@(#)Id: parsarpwho.c,v 5.4 1993/07/20 02:06:13 syd Exp ";

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
 * $Log: parsarpwho.c,v $
 * Revision 1.2  1994/01/14  00:53:37  cp
 * 2.4.23
 *
 * Revision 5.4  1993/07/20  02:06:13  syd
 * Changes for vms problem
 * From: M.  Anio
 *
 * Revision 5.3  1993/01/19  05:07:05  syd
 * Trim erroreous extra log entry
 * From: Syd
 *
 * Revision 5.2  1993/01/19  04:47:12  syd
 * Significant changes to provide consistent Date and From_ header
 * cracking.  Overhauled date utilities and moved into library.  Moved
 * real_from() into library.  Modified frm, newmail, and readmsg utilities
 * to use library version of real_from().  Moved get_word() from Elm
 * source into library.  Added new library routines atonum() and strfcpy().
 * Fixed trailing backslash bug in len_next().
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.1  1992/10/03  22:41:36  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** 

**/

#include "headers.h"

parse_arpa_who(buffer, newfrom, is_really_a_to)
char *buffer, *newfrom;
int is_really_a_to;
{
	/** try to parse the 'From:' line given... It can be in one of
	    two formats:
		From: Dave Taylor <hplabs!dat>
	    or  From: hplabs!dat (Dave Taylor)

	    Added: removes quotes if name is quoted (12/12)
	     HOWEVER:  From: "NAME \"VMS USER\"" <USER@VMS>
	           must be handled delicately !
	    Added: only copies STRING characters...
	    Added: if no comment part, copy address instead! 
	    Added: if is_really_a_to, this is really a 'to' line
		   and treat as if we allow embedded addresses
	**/

	int use_embedded_addresses;
	char temp_buffer[LONG_STRING], *temp;
	register int i, j = 0, in_parens;

	temp = (char *) temp_buffer;
	temp[0] = '\0';

	no_ret(buffer);		/* blow away '\n' char! */

	if (lastch(buffer) == '>') {
	  for (i=strlen("From: "); buffer[i] != '\0' && buffer[i] != '<' &&
	       buffer[i] != '('; i++)
	    temp[j++] = buffer[i];
	  temp[j] = '\0';
	}
	else if (lastch(buffer) == ')') {
	  in_parens = 1;
	  for (i=strlen(buffer)-2; buffer[i] != '\0' && buffer[i] != '<'; i--) {
	    switch(buffer[i]) {
	    case ')':	in_parens++;
			break;
	    case '(':	in_parens--;
			break;
	    }
	    if(!in_parens) break;
	    temp[j++] = buffer[i];
	  }
	  temp[j] = '\0';
	  reverse(temp);
	}

#ifdef USE_EMBEDDED_ADDRESSES
	use_embedded_addresses = TRUE;
#else
	use_embedded_addresses = FALSE;
#endif

	if(use_embedded_addresses || is_really_a_to) {
	  /** if we have a null string at this point, we must just have a 
	      From: line that contains an address only.  At this point we
	      can have one of a few possibilities...

		  From: address
		  From: <address>
		  From: address ()
	  **/
	    
	  if (strlen(temp) == 0) {
	    if (lastch(buffer) != '>') {       
	      for (i=strlen("From:");buffer[i] != '\0' && buffer[i] != '('; i++)
		temp[j++] = buffer[i];
	      temp[j] = '\0';
	    }
	    else {	/* get outta '<>' pair, please! */
	      for (i=strlen(buffer)-2;buffer[i] != '<' && buffer[i] != ':';i--)
		temp[j++] = buffer[i];
	      temp[j] = '\0';
	      reverse(temp);
	    }
	  }
	}
	  
	if (strlen(temp) > 0) {		/* mess with buffer... */

	  /* remove leading spaces and ONE quote... */

	  while (whitespace(temp[0]))
	    temp = (char *) (temp + 1);		/* increment address! */
	  if (temp[0] == '"')
	    temp = (char *) (temp + 1);		/* increment address! */
	  while (whitespace(temp[0]))
	    temp = (char *) (temp + 1);		/* increment address! */

	  /* remove trailing spaces and ONE quote... */

	  i = strlen(temp) - 1;

	  while (i >= 0 && (whitespace(temp[i])))
	   temp[i--] = '\0';
	  /* Can delete ONE trailing quote, NOT THEM ALL!
	     Assuming the incoming quotes were all right.. [mea@utu.fi] */
	  if (i >= 0 && temp[i] == '"')
	   temp[i--] = '\0';
	  while (i >= 0 && (whitespace(temp[i])))
	   temp[i--] = '\0';

	  /* if anything is left, let's change 'from' value! */

	  if (strlen(temp) > 0)
	    strfcpy(newfrom, temp, STRING);
	}
}
