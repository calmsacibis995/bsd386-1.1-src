
static char rcsid[] = "@(#)Id: wildcards.c,v 5.5 1992/12/11 01:45:04 syd Exp ";

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
 * $Log: wildcards.c,v $
 * Revision 1.2  1994/01/14  00:55:59  cp
 * 2.4.23
 *
 * Revision 5.5  1992/12/11  01:45:04  syd
 * remove sys/types.h include, it is now included by defs.h
 * and this routine includes defs.h or indirectly includes defs.h
 * From: Syd
 *
 * Revision 5.4  1992/12/07  05:00:39  syd
 * Add include of sys/types.h for time_t
 * From: Syd
 *
 * Revision 5.3  1992/10/31  18:59:24  syd
 * Prevent index underflow when wildchar is in first three chars of string
 * From: Syd via note from gwh@dogmatix.inmos.co.uk
 *
 * Revision 5.2  1992/10/11  01:18:23  syd
 * Fix where a SHELL=sh would cause a segv due to no / in
 * the shell name.
 * From: Syd
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/*
 * Wildcard handling module for elm.
 */

#include <stdio.h>
#include "defs.h"

/*
 * Common wildcards.  Note that we count space as a wildcard, for
 * concatenation of individual unwildcarded file names.
 */

static char	sh_wildcards[] =   "[]*? " ;
static char	csh_wildcards[] =  "[]*?{} " ;

/* List of known shells and their wildcards.  Expand as needed. */

static struct	wildcard_list
{
	char	*shell_name ;
	char	*wildcard_chars ;
} known_shells[] = {
	{  "csh", csh_wildcards	},
	{  "sh", sh_wildcards },
	{  "ksh", sh_wildcards },
	{  "tcsh", csh_wildcards },
	{  "bash", sh_wildcards },
	{  "zsh", sh_wildcards },
	{  NULL, sh_wildcards }
} ;
 

/*
 * Function: has_wildcards.
 *
 * Scans the incoming file name for wildcard characters, returns
 * true if they are there and false otherwise.  Watch for escapes
 * (ie, preceeding backslash).
 *
 * One tricky part here -- different shells have different wildcarding
 * characters (yuck).  We need to examine the shell used, and set up
 * the right list.  If the shell isn't one we're familiar with, assume
 * Bourne shell.
 */


int
has_wildcards( name )
char *name ;
{
	static char		*user_wildcards = NULL ;
	char			*wildchar ;
	struct wildcard_list 	*wptr ;
	char			*user_shell ;
	char			*sh_name ;

	if ( NULL == user_wildcards )	/* Determine wildcards only once */
	{
		if ( NULL == ( user_shell = getenv( "SHELL" ) ) )
			user_wildcards = sh_wildcards ;
		else
		{
			if ( NULL != ( sh_name = rindex( user_shell, '/' ) ) )
				sh_name++ ;
			else
				sh_name = user_shell;

			for ( wptr = known_shells ; ( wptr->shell_name != NULL ) ; wptr++ )
				if ( 0 == strcmp( wptr->shell_name, sh_name ) )
					break ;
			user_wildcards = wptr->wildcard_chars ;
		}
	}

	/* If unescaped wildcard chars found, return true */

	for ( wildchar = name ; NULL != ( wildchar = strpbrk( wildchar, user_wildcards ) ) ; wildchar++ )
	{
		if ( *( wildchar - 1 ) != '\\' ) {
			if (wildchar - name < 3)
				return TRUE;
			/* elm predoubles the backslashes */
			if ( *( wildchar - 3 ) != '\\' )
				return TRUE ;
		}
	}
	return FALSE ;
}
