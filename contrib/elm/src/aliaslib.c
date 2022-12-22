
static char rcsid[] = "@(#)Id: aliaslib.c,v 5.10 1993/08/03 19:28:39 syd Exp ";

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
 * $Log: aliaslib.c,v $
 * Revision 1.2  1994/01/14  00:54:29  cp
 * 2.4.23
 *
 * Revision 5.10  1993/08/03  19:28:39  syd
 * Elm tries to replace the system toupper() and tolower() on current
 * BSD systems, which is unnecessary.  Even worse, the replacements
 * collide during linking with routines in isctype.o.  This patch adds
 * a Configure test to determine whether replacements are really needed
 * (BROKE_CTYPE definition).  The <ctype.h> header file is now included
 * globally through hdrs/defs.h and the BROKE_CTYPE patchup is handled
 * there.  Inclusion of <ctype.h> was removed from *all* the individual
 * files, and the toupper() and tolower() routines in lib/opt_utils.c
 * were dropped.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.9  1993/05/31  19:39:43  syd
 * Elm either failed to expand a group alias or crashed in strlen
 * (called from do_expand_group()).
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.8  1993/05/14  03:53:46  syd
 * Fix wrong message being displayed and then overwritten
 * for long aliases.
 * From: "Robert L. Howard" <robert.howard@matd.gatech.edu>
 *
 * Revision 5.7  1993/05/08  17:03:16  syd
 * If there are local user names (account names) in the alias, they don't
 * get fully expanded with a GCOS field like they do when you type an
 * account name on the To line.
 * From: "Robert L. Howard" <robert.howard@matd.gatech.edu>
 *
 * Revision 5.6  1993/04/12  01:10:15  syd
 * fix @aliasname sort problem
 * From: "Robert L. Howard" <robert.howard@matd.gatech.edu>
 *
 * Revision 5.5  1993/02/07  15:13:13  syd
 * remove extra declaration of lseek, now in hdrs/defs.h
 * From: Syd
 *
 * Revision 5.4  1993/01/20  03:02:19  syd
 * Move string declarations to defs.h
 * From: Syd
 *
 * Revision 5.3  1992/12/12  01:28:24  syd
 * in do_get_alias().  abuf[] was under dimensioned.
 * From: "Robert L. Howard" <robert.howard@matd.gatech.edu>
 *
 * Revision 5.2  1992/10/11  01:21:17  syd
 * 1. If firstname && lastname is null then copy aliasname into the
 * personal name field (inside the ()'s) when creating an alias
 * from the menu using the 'n' command.
 *
 * 2. Now if for some reason and alias has a null personal name field
 * (the person hand edited aliases.text) the blank () is not printed
 * as part of the address.  This actually cured another problem, where
 * the To: field on the screen (when you hit 'm' on the alias menu)
 * used to be blank, now the address shows up....
 * From: "Robert L. Howard" <robert.howard@matd.gatech.edu>
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** Library of functions dealing with the alias system...

 **/

#include "headers.h"
#include "s_elm.h"

char *get_alias_address(), *qstrpbrk();
extern int current_mail_message;

/*
 * Expand "name" as an alias and return a pointer to static data containing
 * the expansion.  If "name" is not an alias, then NULL is returned.
 */
char *get_alias_address(name, mailing, too_longp)
char *name;	/* name to expand as an alias				*/
int mailing;	/* TRUE to fully expand group names & recursive aliases	*/
int *too_longp;	/* error code if expansion overflows buffer		*/
{
	static char buffer[VERY_LONG_STRING];
	char *bufptr;
	int bufsize, are_in_aliases = TRUE;

	if (!inalias) {
	    main_state();
	    are_in_aliases = FALSE;
	}
/*
 *	Reopens files iff changed since last read
 */
	open_alias_files(are_in_aliases);
/*
 *	If name is an alias then return its expansion
 */
	bufptr = buffer;
	bufsize = sizeof(buffer);
	if (do_get_alias(name,&bufptr,&bufsize,mailing,FALSE,0,too_longp)) {
	/*
	 *  Skip comma/space from add_name_to_list()
	 */
	    bufptr = buffer+2;
	}
	else {
	/*
	 *  Nope...not an alias (or it was too long to expand)
	 */
	    dprint(2, (debugfile,
		"Could not expand alias in get_alias_address()%s\n",
		*too_longp ? "\t...alias buffer overflowed." : ""));
	    bufptr = NULL;
	}

	if (! are_in_aliases)
	    main_state();

	return(bufptr);
}


/*
 * Determine if "name" is an alias, and if so expand it and store the result in
 * "*bufptr".  TRUE returned if any expansion occurs, else FALSE is returned.
 */
int do_get_alias(name, bufptr, bufsizep, mailing, sysalias, depth, too_longp)
char *name;	/* name to expand as an alias				*/
char **bufptr;	/* place to store result of expansion			*/
int *bufsizep;	/* available space in the buffer			*/
int mailing;	/* TRUE to fully expand group names & recursive aliases	*/
int sysalias;	/* TRUE to suppress checks of the user's aliases	*/
int depth;	/* recursion depth - initially call at depth=0		*/
int *too_longp;	/* error code if expansion overflows buffer		*/
{
	struct alias_rec *match;
	char abuf[VERY_LONG_STRING];
	int loc;

	/* update the recursion depth counter */
	++depth;

	dprint(6, (debugfile, "%*s->attempting alias expansion on \"%s\"\n",
		(depth*2), "", name));

	/* strip out (comments) and leading/trailing whitespace */
	remove_possible_trailing_spaces( name = strip_parens(name) );
	for ( ; isspace(*name)  ; ++name ) ;

	/* throw back empty addresses */
	if ( *name == '\0' )
	  return FALSE;

/* The next two blocks could be merged somewhat */
	/* check for a user alias, unless in the midst of sys alias expansion */
	if ( !sysalias ) {
	  if ( (loc = find_alias(name, USER)) >= 0 ) {
	    match = aliases[loc];
	    strcpy(abuf, match->address);
	    if ( match->type & PERSON ) {
	      if (strlen(match->name) > 0) {
                sprintf(abuf+strlen(abuf), " (%s)", match->name);
	      }
	    }
	    goto do_expand;
	  }
	}

	/* check for a system alias */
	  if ( (loc = find_alias(name, SYSTEM)) >= 0 ) {
	    match = aliases[loc];
	    strcpy(abuf, match->address);
	    if ( match->type & PERSON ) {
	      if (strlen(match->name) > 0) {
                sprintf(abuf+strlen(abuf), " (%s)", match->name);
	      }
	    }
	    sysalias = TRUE;
	    goto do_expand;
	  }

	/* nope...this name wasn't an alias */
	return FALSE;

do_expand:

	/* at this point, alias is expanded into "abuf" - now what to do... */

	dprint(7, (debugfile, "%*s  ->expanded alias to \"%s\"\n",
	    (depth*2), "", abuf));

	/* check for an exact match */
	loc = strlen(name);
	if ( strncmp(name, abuf, loc) == 0 &&
	     (isspace(abuf[loc]) || abuf[loc] == '\0') ) {
	  if (add_name_to_list(abuf, bufptr, bufsizep)) {
	      return TRUE;
	  }
	  else {
	      *too_longp = TRUE;
	      return FALSE;
	  }
	}

	/* see if we are stuck in a loop */
	if ( depth > 12 ) {
	  dprint(2, (debugfile,
	      "alias expansion loop detected at \"%s\" - bailing out\n", name));
	    error1(catgets(elm_msg_cat, ElmSet, ElmErrorExpanding,
		"Error expanding \"%s\" - probable alias definition loop."),
		name);
	    return FALSE;
	}

	/* see if the alias equivalence is a group name */
	if ( mailing && match->type & GROUP )
	  return do_expand_group(abuf,bufptr,bufsizep,sysalias,depth,too_longp);

	/* see if the alias equivalence is an email address */
	if ( qstrpbrk(abuf,"!@:") != NULL ) {
	    if (add_name_to_list(abuf, bufptr, bufsizep)) {
	        return TRUE;
	    }
	    else {
	        *too_longp = TRUE;
	        return FALSE;
	    }
	}

	/* see if the alias equivalence is itself an alias */
	if ( mailing &&
	     do_get_alias(abuf,bufptr,bufsizep,TRUE,sysalias,depth,too_longp) )
	  return TRUE;

	/* the alias equivalence must just be a local address */
	if (add_name_to_list(abuf, bufptr, bufsizep)) {
	    return TRUE;
	}
	else {
	    *too_longp = TRUE;
	    return FALSE;
	}
}


/*
 * Expand the comma-delimited group of names in "group", storing the result
 * in "*bufptr".  Returns TRUE if expansion occurs OK, else FALSE in the
 * event of errors.
 */
int do_expand_group(group, bufptr, bufsizep, sysalias, depth, too_longp)
char *group;	/* group list to expand					*/
char **bufptr;	/* place to store result of expansion			*/
int *bufsizep;	/* available space in the buffer			*/
int sysalias;	/* TRUE to suppress checks of the user's aliases	*/
int depth;	/* nesting depth					*/
int *too_longp;	/* error code if expansion overflows buffer		*/
{
	char *name, *gecos;
	char expanded_address[LONG_STRING];
	extern char *get_full_name();

	/* go through each comma-delimited name in the group */
	while ( group != NULL ) {

	  /* extract the next name from the list */
	  for ( name = group ; isspace(*name) ; ++name ) ;
	  if ( (group = index(name,',')) != NULL )
	      *group++ = '\0';
	  remove_possible_trailing_spaces(name);
	  if ( *name == '\0' )
	    continue;

	  /* see if this name is really an alias */
	  if (do_get_alias(name,bufptr,bufsizep,TRUE,sysalias,depth,too_longp))
	    continue;

	  if ( *too_longp )
	      return FALSE;

	  /* verify it is a valid address */
	  if ( valid_name(name) ) {
	    gecos = get_full_name(name);

	    if (gecos && (strlen(gecos) > 0)) {
	        sprintf(expanded_address, "%s (%s)", name, gecos);
	        name = expanded_address;
	    }
	  }

	  /* add it to the list */
	  if ( !add_name_to_list(name, bufptr, bufsizep) ) {
	    *too_longp = TRUE;
	    return FALSE;
	  }

	}

	return TRUE;
}


/*
 * Append "<comma><space>name" to the list, checking to ensure the buffer
 * does not overflow.  Upon return, *bufptr and *bufsizep will be updated to
 * reflect the stuff added to the buffer.  If a buffer overflow would occur,
 * an error message is printed and FALSE is returned, else TRUE is returned.
 */
int add_name_to_list(name,bufptr,bufsizep)
register char *name;	/* name to append to buffer			*/
register char **bufptr;	/* pointer to pointer to end of buffer		*/
register int *bufsizep;	/* pointer to space remaining in buffer		*/
{
	if ( *bufsizep < 0 )
	    return FALSE;

	*bufsizep -= strlen(name)+2;
	if ( *bufsizep <= 0 ) {
	    *bufsizep = -1;
	    dprint(2, (debugfile,
		"Alias expansion is too long in add_name_to_list()\n"));
	    error(catgets(elm_msg_cat, ElmSet, ElmAliasExpTooLong,
		"Alias expansion is too long."));
	    return FALSE;
	}

	*(*bufptr)++ = ',';
	*(*bufptr)++ = ' ';
	while ( *name != '\0' )
	  *(*bufptr)++ = *name++ ;
	**bufptr = '\0';

	return TRUE;
}
