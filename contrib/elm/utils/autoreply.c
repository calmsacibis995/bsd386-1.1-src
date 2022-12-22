
static char rcsid[] = "@(#)Id: autoreply.c,v 5.3 1993/01/20 03:37:16 syd Exp ";

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
 * $Log: autoreply.c,v $
 * Revision 1.2  1994/01/14  00:56:09  cp
 * 2.4.23
 *
 * Revision 5.3  1993/01/20  03:37:16  syd
 * Nits and typos in the NLS messages and corresponding default messages.
 * From: dwolfe@pffft.sps.mot.com (Dave Wolfe)
 *
 * Revision 5.2  1992/12/11  01:45:04  syd
 * remove sys/types.h include, it is now included by defs.h
 * and this routine includes defs.h or indirectly includes defs.h
 * From: Syd
 *
 * Revision 5.1  1992/10/04  00:46:45  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** This is the front-end for the autoreply system, and performs two 
    functions: it either adds the user to the list of people using the
    autoreply function (starting the daemon if no-one else) or removes
    a user from the list of people.

    Usage:  autoreply filename
	    autoreply "off"
	or  autoreply		[to find current status]
    
**/

#include "elmutil.h"
#include "s_autoreply.h"
#include <sys/stat.h>

#ifdef PWDINSYS
#  include <sys/pwd.h>
#else
#  include <pwd.h>
#endif

#define  tempdir	"/tmp/arep"		/* file prefix          */
#define  autoreply_file	"/etc/autoreply.data"   /* autoreply data file  */

extern   int errno;				/* system error code    */

main(argc, argv)
int    argc;
char *argv[];
{
	char filename[SLEN];
	int userid;
	struct passwd *pass;
#ifndef	_POSIX_SOURCE
	struct passwd *getpwuid();
#endif

#ifdef I_LOCALE
	setlocale(LC_ALL, "");
#endif

	elm_msg_cat = catopen("elm2.4", 0);

	if (argc > 2) {
	  printf(catgets(elm_msg_cat, AutoreplySet, AutoreplyArgsHelp1,
	      "Usage: %s <filename>\tto start autoreply,\n"), argv[0]);
	  printf(catgets(elm_msg_cat, AutoreplySet, AutoreplyArgsHelp2,
	      "       %s off\t\tto turn off autoreply\n"), argv[0]);
	  printf(catgets(elm_msg_cat, AutoreplySet, AutoreplyArgsHelp3,
	      "   or  %s    \t\tto check current status\n"), argv[0]);
	  exit(1);
	}

	userid  = getuid();

	/*
	 * Get username (logname) field from the password entry for this user id.
	 */

	if((pass = getpwuid(userid)) == NULL) {
	  printf(catgets(elm_msg_cat, AutoreplySet, AutoreplyNoPasswdEntry,
	      "You have no password entry!\n"));
	  exit(1);
	}
	strcpy(username, pass->pw_name);

	if (argc == 1 || strcmp(argv[1], "off") == 0) 
	  remove_user((argc == 1));
	else {
	  strcpy(filename, argv[1]);
	  if (access(filename,READ_ACCESS) != 0) {
	    printf(catgets(elm_msg_cat, AutoreplySet, AutoreplyErrRead,
		"Error: Can't read file '%s'\n"), filename);
	    exit(1);
	  }
	  
	  if (filename[0] != '/') /* prefix home directory */
	    sprintf(filename,"%s/%s", getenv("HOME"), argv[1]);

	  add_user(filename);
	}

	exit(0);
}

remove_user(stat_only)
int stat_only;
{
	/** Remove the user from the list of currently active autoreply 
	    people.  If 'stat_only' is set, then just list the name of
	    the file being used to autoreply with, if any. **/

	FILE *temp, *repfile;
	char  tempfile[SLEN], user[SLEN], filename[SLEN];
	int   c, copied = 0, found = 0;
	long  filesize, bytes();

	if (! stat_only) {
	  sprintf(tempfile, "%s.%06d", tempdir, getpid());

	  if ((temp = fopen(tempfile, "w")) == NULL) {
	    printf(catgets(elm_msg_cat, AutoreplySet, AutoreplyErrOpen,
		"Error: couldn't open temp file '%s'.  Not removed\n"),
		    tempfile);
	    exit(1);
	  }
	}

	if ((repfile = fopen(autoreply_file, "r")) == NULL) {
	  if (stat_only) {
	    printf(catgets(elm_msg_cat, AutoreplySet, AutoreplyNotAutoreply,
		"You're not currently autoreplying to mail.\n"));
	    exit(0);
	  }
	  printf(catgets(elm_msg_cat, AutoreplySet, AutoreplyNoOneAutoreply,
	      "No-one is autoreplying to their mail!\n"));
	  exit(0);
	}

	/** copy out of real replyfile... **/

	while (fscanf(repfile, "%s %s %ld", user, filename, &filesize) != EOF) 

	  if (strcmp(user, username) != 0) {
	    if (! stat_only) {
	      copied++;
	      fprintf(temp, "%s %s %ld\n", user, filename, filesize);
	    }
	  }
	  else {
	    if (stat_only) {
	      printf(catgets(elm_msg_cat, AutoreplySet, AutoreplyCurrArep,
	          "You're currently autoreplying to mail with the file %s\n"),
		      filename); 
	      exit(0);
	    }
	    found++;
	  }

	fclose(temp);
	fclose(repfile);

	if (! found) {
	  if (! stat_only) {
	    printf(catgets(elm_msg_cat, AutoreplySet, AutoreplyNoArepOff,
	      "You're not currently autoreplying to mail!\n"));
	    unlink(tempfile);
	  }
	  else
	    printf(catgets(elm_msg_cat, AutoreplySet, AutoreplyNoArep,
	        "You're not currently autoreplying to mail.\n"));
	  exit(! stat_only);
	}

	/** now copy tempfile back into replyfile **/

	if (copied == 0) {	/* removed the only person! */
	  unlink(autoreply_file);
	}
	else {			/* save everyone else   */
	  
	  if ((temp = fopen(tempfile,"r")) == NULL) {
	    printf(catgets(elm_msg_cat, AutoreplySet, AutoreplyErrReopenTemp,
	        "Error: couldn't reopen temp file '%s'.  Not removed.\n"),
		    tempfile);
	    unlink(tempfile);
	    exit(1);
	  }

	  if ((repfile = fopen(autoreply_file, "w")) == NULL) {
	    printf(catgets(elm_msg_cat, AutoreplySet, AutoreplyErrReopenArep,
        "Error: couldn't reopen autoreply file for writing!  Not removed.\n"));
	    unlink(tempfile);
	    exit(1);
	  }

	  while ((c = getc(temp)) != EOF)
	    putc(c, repfile);

	  fclose(temp);
	  fclose(repfile);
	
	}
	unlink(tempfile);

	if (found > 1)
	  printf(catgets(elm_msg_cat, AutoreplySet, AutoreplyRemovedMulti,
	      "Warning: your username appeared %d times!!   Removed all\n"), 
		  found);
	else
	  printf(catgets(elm_msg_cat, AutoreplySet, AutoreplyRemoved,
	      "You've been removed from the autoreply table.\n"));
}

add_user(filename)
char *filename;
{
	/** add the user to the autoreply file... **/

	FILE *repfile;
	char  mailfile[SLEN];
	long  bytes();

	if ((repfile = fopen(autoreply_file, "a")) == NULL) {
	  printf(catgets(elm_msg_cat, AutoreplySet, AutoreplyErrOpenArep,
	      "Error: couldn't open the autoreply file!  Not added\n"));
	  exit(1);
	}
	
	sprintf(mailfile,"%s/%s", mailhome, username);

	fprintf(repfile,"%s %s %ld\n", username, filename, bytes(mailfile));

	fclose(repfile);

	printf(catgets(elm_msg_cat, AutoreplySet, AutoreplyAddArep,
	  "You've been added to the autoreply system.\n"));
}


long
bytes(name)
char *name;
{
	/** return the number of bytes in the specified file.  This
	    is to check to see if new mail has arrived....  **/

	int ok = 1;
	extern int errno;	/* system error number! */
	struct stat buffer;

	if (stat(name, &buffer) != 0)
	  if (errno != 2)
	   exit(MCprintf(catgets(elm_msg_cat, AutoreplySet, AutoreplyErrFstat,
	       "Error %d attempting fstat on %s\n"), errno, name));
	  else
	    ok = 0;
	
	return(ok ? buffer.st_size : 0L);
}
