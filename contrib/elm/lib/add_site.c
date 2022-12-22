
static char rcsid[] = "@(#)Id: add_site.c,v 5.1 1992/10/03 22:41:36 syd Exp ";

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
 * $Log: add_site.c,v $
 * Revision 1.2  1994/01/14  00:52:57  cp
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

add_site(buffer, site, lastsite)
char *buffer, *site, *lastsite;
{
	/** add site to buffer, unless site is 'uucp' or site is
	    the same as lastsite.   If not, set lastsite to site.
	**/

	char local_buffer[SLEN], *stripped;
	char *strip_parens();

	stripped = strip_parens(site);

	if (istrcmp(stripped, "uucp") != 0)
	  if (strcmp(stripped, lastsite) != 0) {
	    if (buffer[0] == '\0')
	      strcpy(buffer, stripped);         /* first in list! */
	    else {
	      sprintf(local_buffer,"%s!%s", buffer, stripped);
	      strcpy(buffer, local_buffer);
	    }
	    strcpy(lastsite, stripped); /* don't want THIS twice! */
	  }
}
