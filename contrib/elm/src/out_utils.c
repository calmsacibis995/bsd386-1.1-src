
static char rcsid[] = "@(#)Id: out_utils.c,v 5.2 1993/02/03 19:06:31 syd Exp ";

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
 * $Log: out_utils.c,v $
 * Revision 1.2  1994/01/14  00:55:25  cp
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

/** This file contains routines used for output in the ELM program.

**/

#include "headers.h"


static char err_buffer[SLEN];		/* store last error message */

static char central_message_buffer[SLEN];

show_last_error()
{
	/** rewrite last error message! **/

	error(err_buffer);
}

clear_error()
{
	MoveCursor(LINES,0);
	CleartoEOLN();
	err_buffer[0] = '\0';
}

set_error(s)
char *s;
{
	strcpy(err_buffer, s);
}

error(s)
char *s;
{
	/** outputs error 's' to screen at line 22, centered! **/

	if(batch_only)
	  printf("%s\n\r", s);
	else {
	  MoveCursor(LINES,0);
	  CleartoEOLN();
	  PutLine0(LINES,(COLUMNS-strlen(s))/2,s);
	  fflush(stdout);
	}
	strcpy(err_buffer, s);	/* save it too! */
}

/*VARARGS1*/

error1(s, a)
char *s, *a;
{
	/** same as error, but with a 'printf' argument **/
	char buffer[SLEN];

	sprintf(buffer,s,a);
	error(buffer);
}

/*VARARGS1*/

error2(s, a1, a2)
char *s, *a1, *a2;
{
	/** same as error, but with two 'printf' arguments **/
	char buffer[SLEN];

	MCsprintf(buffer,s, a1, a2);
	error(buffer);
}

/*VARARGS1*/

error3(s, a1, a2, a3)
char *s, *a1, *a2, *a3;
{
	/** same as error, but with three 'printf' arguments **/
	char buffer[SLEN];

	MCsprintf(buffer,s, a1, a2, a3);
	error(buffer);
}

lower_prompt(s)
char *s;
{
	/** prompt user for input on LINES-1 line, left justified **/

	PutLine0(LINES-1,0,s);
	CleartoEOLN();
}

prompt(s)
char *s;
{
	/** prompt user for input on LINES-3 line, left justified **/

	PutLine0(LINES-3,0,s);
	CleartoEOLN();
}


set_central_message(string, arg)
char *string, *arg;
{
	/** set up the given message to be displayed in the center of
	    the current window **/ 

	sprintf(central_message_buffer, string, arg);
}

display_central_message()
{
	/** display the message if set... **/

	if (central_message_buffer[0] != '\0') {
	  ClearLine(LINES-15);
	  Centerline(LINES-15, central_message_buffer);
	  fflush(stdout);
	}
}

clear_central_message()
{
	/** clear the central message buffer **/

	central_message_buffer[0] = '\0';
}
