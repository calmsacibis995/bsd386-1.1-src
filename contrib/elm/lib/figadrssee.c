
static char rcsid[] = "@(#)Id: figadrssee.c,v 5.3 1993/06/12 05:33:32 syd Exp ";

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
 * $Log: figadrssee.c,v $
 * Revision 1.2  1994/01/14  00:53:08  cp
 * 2.4.23
 *
 * Revision 5.3  1993/06/12  05:33:32  syd
 * Remove useless auto
 * From: Syd
 *
 * Revision 5.2  1993/02/03  16:25:45  syd
 * Adresses with double quoted strings that contains comma was parsed
 * wrongly by break_down_tolist() and figure_out_addressee().
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.1  1992/10/03  22:41:36  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** 

**/

#include "headers.h"

figure_out_addressee(buffer, mail_to)
char *buffer;
char *mail_to;
{
	/** This routine steps through all the addresses in the "To:"
	    list, initially setting it to the first entry (if mail_to
	    is NULL) or, if the user is found (eg "alternatives") to
	    the current "username".
	**/

	char *address, *bufptr, mybuf[SLEN];
	
	if (equal(mail_to, username)) return;	/* can't be better! */

	bufptr = (char *) buffer;	       /* use the string directly   */

	while (whitespace(*bufptr)) bufptr++;
	while ((address = strtokq(bufptr, ",\t\n\r", 1)) != NULL) {

	  if (! okay_address(address, "don't match me!")) {
	    strcpy(mail_to, username);	/* it's to YOU! */
	    return;
	  }
	  else if (strlen(mail_to) == 0) {	/* it's SOMEthing! */
	
	    /** this next bit is kinda gory, but allows us to use the
		existing routines to parse the address - by pretending
		it's a From: line and going from there...
		Ah well - you get what you pay for, right?
	    **/

	      if (strlen(address) > (sizeof mybuf) - 7)	/* ensure it ain't */
		address[(sizeof mybuf)-7] = '\0';	/*  too long mon!  */

	      sprintf(mybuf, "From: %s", address);
	      parse_arpa_who(mybuf, mail_to, TRUE);
	  }

	  bufptr = (char *) NULL;	/* set to null */
	}

	return;
}
