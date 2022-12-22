
static char rcsid[] = "@(#)Id: newalias.c,v 5.4 1993/05/08 20:24:55 syd Exp ";

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
 * $Log: newalias.c,v $
 * Revision 1.2  1994/01/14  00:56:17  cp
 * 2.4.23
 *
 * Revision 5.4  1993/05/08  20:24:55  syd
 * add sleepmsg for not in elm mode
 *
 * Revision 5.3  1992/10/11  01:10:31  syd
 * Add missing setlocale and catopen (just forgotten)
 * From: Syd
 *
 * Revision 5.2  1992/10/11  00:59:39  syd
 * Fix some compiler warnings that I receive compiling Elm on my SVR4
 * machine.
 * From: Tom Moore <tmoore@fievel.DaytonOH.NCR.COM>
 *
 * Revision 5.1  1992/10/04  00:46:45  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** Install a new set of aliases for the 'Elm' mailer. 

	If invoked without the -g argument, it assumes that
  it is working with an individual users alias tables, and
  generates the .alias.pag and .alias.dir files in their
  .elm directory.
	If, however, it is invoked with the -g flag,
  it assumes that the user is updating the system alias
  file and uses the defaults for everything.

  The format for the input file is;
    alias1, alias2, ... = username = address
or  alias1, alias2, ... = groupname= member, member, member, ...
                                     member, member, member, ...

**/

#include "elmutil.h"
#include "s_newalias.h"
#include "sysdefs.h"		/* ELM system definitions */

void error();

#ifdef DEBUG
FILE *debugfile = stderr;
int  debug = 0;
#endif

int  is_system=0;		/* system file updating?     */
int  sleepmsg=0;		/* not in elm, dont wait for messages */

main(argc, argv)
int argc;
char *argv[];
{
	char inputname[SLEN], dataname[SLEN];
	char home_dir[SLEN];		/* the users home directory  */
	int  a;

#ifdef I_LOCALE
	setlocale(LC_ALL, "");
#endif

	elm_msg_cat = catopen("elm2.4", 0);

	for (a = 1; a < argc; ++a) {
	  if (strcmp(argv[a], "-g") == 0)
	    is_system = 1;
#ifdef DEBUG
	  else if (strcmp(argv[a], "-d") == 0)
	    debug = 10;
#endif
	  else {
	      fprintf(stderr, catgets(elm_msg_cat,
	            NewaliasSet, NewaliasUsage, "Usage: %s [-g]\n"), argv[0]);
	      exit(1);
	  }
	}

	if (is_system) {   /* update system aliases */
	    printf(catgets(elm_msg_cat, NewaliasSet, NewaliasUpdateSystem,
	            "Updating the system alias file..."));

	    strcpy(inputname, system_text_file);
	    strcpy(dataname,  system_data_file);
	}
	else {
	    printf(catgets(elm_msg_cat, NewaliasSet, NewaliasUpdatePersonal,
		"Updating your personal alias file..."));
	
	    if (strcpy(home_dir, getenv("HOME")) == NULL) {
	        error(catgets(elm_msg_cat, NewaliasSet, NewaliasNoHOME,
		        "I'm confused - no HOME variable in environment!"));
	        exit(1);
	    }

	    MCsprintf(inputname, "%s/%s", home_dir, ALIAS_TEXT);
	    MCsprintf(dataname,  "%s/%s", home_dir, ALIAS_DATA); 
	}

	if ((a = do_newalias(inputname, dataname, FALSE, TRUE)) < 0) {
	    exit(1);
	}
	else {
	    printf(catgets(elm_msg_cat, NewaliasSet, NewaliasProcessed,
	            "processed %d aliases.\n"), a);
	    exit(0);
	}

	/*NOTREACHED*/
}

void
error(err_message)
char *err_message;
{
	fflush(stdout);
	fprintf(stderr, "\n%s\n", err_message);
	return;
}
