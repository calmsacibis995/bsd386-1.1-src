
static char rcsid[] = "@(#)Id: getfullnam.c,v 5.1 1992/10/03 22:41:36 syd Exp ";

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
 * $Log: getfullnam.c,v $
 * Revision 1.2  1994/01/14  00:53:13  cp
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
#ifdef PWDINSYS
#  include <sys/pwd.h>
#else
#  include <pwd.h>
#endif

char *gcos_name();

char *
get_full_name(logname)
char *logname;
{
	/* return a pointer to the full user name for the passed logname
	 * or NULL if cannot be found
	 * If PASSNAMES get it from the gcos field, otherwise get it
	 * from ~/.fullname.
	 */

#ifndef PASSNAMES
	FILE *fp;
	char fullnamefile[SLEN];
#endif
	static char fullname[SLEN];
	struct passwd *getpwnam(), *pass;

	if((pass = getpwnam(logname)) == NULL)
	  return(NULL);
#ifdef PASSNAMES	/* get full_username from gcos field */
	strcpy(fullname, gcos_name(pass->pw_gecos, logname));
#else			/* get full_username from ~/.fullname file */
	sprintf(fullnamefile, "%s/.fullname", pass->pw_dir);

	if(can_access(fullnamefile, READ_ACCESS) != 0)
	  return(NULL);		/* fullname file not accessible to user */
	if((fp = fopen(fullnamefile, "r")) == NULL)
	  return(NULL);		/* fullname file cannot be opened! */
	if(fgets(fullname, SLEN, fp) == NULL) {
	  fclose(fp);
	  return(NULL);		/* fullname file empty! */
	}
	fclose(fp);
	no_ret(fullname);	/* remove trailing '\n' */
#endif
	return(fullname);
}
