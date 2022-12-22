
static char rcsid[] = "@(#)Id: strtokq.c,v 5.3 1993/07/20 02:05:17 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.1 $   $State: Exp $
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
 * $Log: strtokq.c,v $
 * Revision 1.1  1994/01/14  01:35:08  cp
 * 2.4.23
 *
 * Revision 5.3  1993/07/20  02:05:17  syd
 * A long-standing bug of handling replies to VMS systems.
 * Original "From: " -line is of format:
 * 	From: "NAME \"Real Name\"" <USERNAME@vms-system>
 * (PMDF mailer)
 * 	Anyway,  parse_arpa_who()  strips quotes too cleanly
 * resulting data:
 * 	NAME \"Real Name\
 * which, when put into parenthesis, becomes:
 * 	(NAME \"Real Name\)
 * which in its turn lacks closing `)'
 * Patch of  lib/parsarpwho.c  fixes that.
 * strtokq() started one position too late to search for next double-quote (") char.
 * Another one-off (chops off trailing comment character, quote or not..)  in   src/reply.c
 * From:	Matti Aarnio <mea@utu.fi>
 *
 * Revision 5.2  1993/02/03  16:20:30  syd
 * add include file
 *
 * Revision 5.1  1993/02/03  16:19:31  syd
 * Initial checkin, taken from filter/parse.c
 *
 *
 ******************************************************************************/


/* Like strtok, but returns quoted strings as one token (quotes removed)
 * if flag is non-null. Quotes and backslahes can be escaped with backslash.
 */

#include "headers.h"

char *strtokq(source, keys, flag)
char *source, *keys;
int flag;
{
	register int  last_ch;
	static   char *sourceptr = NULL;
		 char *return_value;

	if (source != NULL)
	  sourceptr = source;
	
	if (sourceptr == NULL || *sourceptr == '\0') 
	  return(NULL);		/* we hit end-of-string last time!? */

	sourceptr += strspn(sourceptr, keys);	/* skip leading crap */
	
	if (*sourceptr == '\0') 
	  return(NULL);		/* we've hit end-of-string */

	if (flag)
	  if (*sourceptr == '"' || *sourceptr == '\'') {   /* quoted string */
	    register char *sp;
	    char quote = *sourceptr++;

	    for (sp = sourceptr; *sp != '\0' && *sp != quote; sp++)
	      if (*sp == '\\') sp++;	/* skip escaped characters */
					/* expand_macros will fix them later */

	    return_value = sourceptr;
	    sourceptr = sp;
	    if (*sourceptr != '\0') sourceptr++;
	    *sp = '\0';				/* zero at end */

	    return return_value;
	  }

	last_ch = strcspn(sourceptr, keys);	/* end of good stuff */

	return_value = sourceptr;		/* and get the ret   */

	sourceptr += last_ch;			/* ...value 	     */

	if (*sourceptr != '\0')	/* don't forget if we're at END! */
	  sourceptr++;		   /* and skipping for next time */

	return_value[last_ch] = '\0';		/* ..ending right    */
	
	return((char *) return_value);		/* and we're outta here! */
}

