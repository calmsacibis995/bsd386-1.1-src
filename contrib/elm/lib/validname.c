
static char rcsid[] = "@(#)Id: validname.c,v 5.3 1992/12/11 01:45:04 syd Exp ";

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
 * $Log: validname.c,v $
 * Revision 1.2  1994/01/14  00:53:52  cp
 * 2.4.23
 *
 * Revision 5.3  1992/12/11  01:45:04  syd
 * remove sys/types.h include, it is now included by defs.h
 * and this routine includes defs.h or indirectly includes defs.h
 * From: Syd
 *
 * Revision 5.2  1992/12/07  04:55:16  syd
 * add sys/types include for time_t
 * From: Syd
 *
 * Revision 5.1  1992/10/03  22:41:36  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

#include <stdio.h>
#include "defs.h"

#ifndef NOCHECK_VALIDNAME		 /* Force a return of valid */
# ifdef PWDINSYS
#  include <sys/pwd.h>
# else
#  include <pwd.h>
# endif
#endif

int
valid_name(name)
char *name;
{
	/** Determine whether "name" is a valid logname on this system.
	    It is valid if there is a password entry, or if there is
	    a mail file in the mail spool directory for "name".
	 **/

#ifdef NOCHECK_VALIDNAME		 /* Force a return of valid */

	return(TRUE);

#else

	char filebuf[SLEN];
	struct passwd *getpwnam();

	if(getpwnam(name) != NULL)
	  return(TRUE);

	sprintf(filebuf,"%s/%s", mailhome, name);
	if (access(filebuf, ACCESS_EXISTS) == 0)
	  return(TRUE);

	return(FALSE);

#endif
}
