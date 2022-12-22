
static char rcsid[] = "@(#)Id: expand.c,v 5.4 1993/09/19 23:38:55 syd Exp ";

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
 * $Log: expand.c,v $
 * Revision 1.2  1994/01/14  00:53:06  cp
 * 2.4.23
 *
 * Revision 5.4  1993/09/19  23:38:55  syd
 * expand() didn't read the global rc file if the user elmrc didn't exist or
 * didn't have an entry for maildir.
 * From: Jan.Djarv@sa.erisoft.se (Jan Djarv)
 *
 * Revision 5.3  1992/12/11  01:45:04  syd
 * remove sys/types.h include, it is now included by defs.h
 * and this routine includes defs.h or indirectly includes defs.h
 * From: Syd
 *
 * Revision 5.2  1992/12/07  04:48:15  syd
 * add include of types.h to get time_t define
 * From: Syd
 *
 * Revision 5.1  1992/10/03  22:41:36  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** This is a library routine for the various utilities that allows
    users to have the standard 'Elm' folder directory nomenclature
    for all filenames (e.g. '+', '=' or '%').  It should be compiled
    and then linked in as needed.

**/

#include <stdio.h>
#include "defs.h"
#include "s_elmrc.h"

extern nl_catd elm_msg_cat;	/* message catalog	    */

char *expand_define();

static char*
expand_maildir(rcfile, filename, buffer)
FILE *rcfile;
char *filename;
char *buffer;
{
	char *home = NULL, *bufptr;
	int  foundit = 0;

	bufptr = (char *) buffer;		/* same address */
	
	while (! foundit && mail_gets(buffer, SLEN, rcfile) != 0) {
	  if (strncmp(buffer, "maildir", 7) == 0 ||
	      strncmp(buffer, "folders", 7) == 0) {
	    while (*bufptr != '=' && *bufptr) 
	      bufptr++;
	    bufptr++;			/* skip the equals sign */
	    while (whitespace(*bufptr) && *bufptr)
	      bufptr++; 
	    home = bufptr;		/* remember this address */

	    while (! whitespace(*bufptr) && *bufptr != '\n')
	      bufptr++;

	    *bufptr = '\0';		/* remove trailing space */
	    foundit++;
	  }
	}

	return home;
}

int
expand(filename)
char *filename;
{
	/** Expand the filename since the first character is a meta-
	    character that should expand to the "maildir" variable
	    in the users ".elmrc" file or in the global rc file...

	    Note: this is a brute force way of getting the entry out 
	    of the .elmrc file, and isn't recommended for the faint 
	    of heart!
	**/

	FILE *rcfile;
	char  buffer[SLEN], *home, *expanded_dir;

	if ((home = getenv("HOME")) == NULL) {
	  printf(catgets(elm_msg_cat, ElmrcSet, ElmrcExpandHome,
	     "Can't expand environment variable $HOME to find .elmrc file!\n"));
	  return(NO);
	}

	sprintf(buffer, "%s/%s", home, elmrcfile);

	home = NULL;
	if ((rcfile = fopen(buffer, "r")) != NULL) {
	  home = expand_maildir(rcfile, filename, buffer);
	  fclose(rcfile);
	}

	if (home == NULL) { /* elmrc didn't exist or maildir wasn't in it */
	  if ((rcfile = fopen(system_rc_file, "r")) != NULL) {
	    home = expand_maildir(rcfile, filename, buffer);
	    fclose(rcfile);
	  }
	}

	if (home == NULL) {
	  /* Didn't find it, use default */
	  sprintf(buffer, "~/%s", default_folders);
	  home = buffer;
	}

	/** Home now points to the string containing your maildir, with
	    no leading or trailing white space...
	**/

	if ((expanded_dir = expand_define(home)) == NULL)
		return(NO);

	sprintf(buffer, "%s%s%s", expanded_dir, 
		(expanded_dir[strlen(expanded_dir)-1] == '/' ||
		filename[0] == '/') ? "" : "/", (char *) filename+1);

	strcpy(filename, buffer);
	return(YES);
}

char *expand_define(maildir)
char *maildir;
{
	/** This routine expands any occurances of "~" or "$var" in
	    the users definition of their maildir directory out of
	    their .elmrc file.

	    Again, another routine not for the weak of heart or staunch
	    of will!
	**/

	static char buffer[SLEN];	/* static buffer AIEE!! */
	char   name[SLEN],		/* dynamic buffer!! (?) */
	       *nameptr,	       /*  pointer to name??     */
	       *value;		      /* char pointer for munging */

	if (*maildir == '~') 
	  sprintf(buffer, "%s%s", getenv("HOME"), ++maildir);
	else if (*maildir == '$') { 	/* shell variable */

	  /** break it into a single word - the variable name **/

	  strcpy(name, (char *) maildir + 1);	/* hurl the '$' */
	  nameptr = (char *) name;
	  while (*nameptr != '/' && *nameptr) nameptr++;
	  *nameptr = '\0';	/* null terminate */
	  
	  /** got word "name" for expansion **/

	  if ((value = getenv(name)) == NULL) {
	    printf(catgets(elm_msg_cat, ElmrcSet, ElmrcExpandShell,
		    "Couldn't expand shell variable $%s in .elmrc!\n"),
		    name);
	    return(NULL);
	  }
	  sprintf(buffer, "%s%s", value, maildir + strlen(name) + 1);
	}
	else strcpy(buffer, maildir);

	return( ( char *) buffer);
}
