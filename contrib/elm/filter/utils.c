
static char rcsid[] ="@(#)Id: utils.c,v 5.4 1993/08/03 19:28:39 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.2 $   $State: Exp $
 *
 * 			Copyright (c) 1988-1992 USENET Community Trust
 * 			Copyright (c) 1986,1987 Dave Taylor
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein - elm@DSI.COM
 *			dsinc!elm
 *
 *******************************************************************************
 * $Log: utils.c,v $
 * Revision 1.2  1994/01/14  00:51:43  cp
 * 2.4.23
 *
 * Revision 5.4  1993/08/03  19:28:39  syd
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
 * Revision 5.3  1993/01/27  19:40:01  syd
 * I implemented a change to filter's default verbose message format
 * including %x %X style date and time along with username
 * From: mark@drd.com (Mark Lawrence)
 *
 * Revision 5.2  1992/11/15  01:40:43  syd
 * Add regexp processing to filter.
 * Add execc operator
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.1  1992/10/03  22:18:09  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** Utility routines for the filter program...

**/

#include <stdio.h>
#include <pwd.h>
#include <fcntl.h>

#include "defs.h"
#include "filter.h"
#include "s_filter.h"

extern char *date_n_user();

leave(reason)
char *reason;
{
      if (outfd != NULL)
	{
	      fprintf(outfd,"filter (%s): LEAVE %s\n", date_n_user(), reason);
	      fclose(outfd);
	}
      
      exit(1);
}

log_msg(what)
int what;
{
	/** make an entry in the log files for the specified entry **/

	FILE *fd;
	char filename[SLEN];
	int subj_len;

	if (!logging)
		return;

	if (! show_only) {
	  sprintf(filename, "%s/%s", home, filtersum);	/* log action once! */
	  if ((fd = fopen(filename, "a")) == NULL) {
	    if (outfd != NULL)
	      fprintf(outfd,catgets(elm_msg_cat,
				    FilterSet,FilterCouldntOpenLogFile,
				  "filter (%s): Couldn't open log file %s\n"), 
		      date_n_user(), filename);
	    fd = stdout;
	  }
	  fprintf(fd, "%d\n", rule_choosen);
	  fclose(fd);
	}

	sprintf(filename, "%s/%s", home, filterlog);

	if (show_only)
	  fd = stdout;
	else if ((fd = fopen(filename, "a")) == NULL) {
	  if (outfd != NULL)
	    fprintf(outfd,catgets(elm_msg_cat,
				  FilterSet,FilterCouldntOpenLogFile,
				  "filter (%s): Couldn't open log file %s\n"), 
		    date_n_user(), filename);
	  fd = stdout;
	}
	
#ifdef _IOFBF
	setvbuf(fd, NULL, _IOFBF, BUFSIZ);
#endif

	subj_len = strlen(subject);
	fprintf(fd,"%s ",catgets(elm_msg_cat,FilterSet,FilterMailFrom,
			    "\nMail from"));
	       
	if (strlen(from) + subj_len > 60)
	  fprintf(fd, "%s\n\t", from);
	else
	  fprintf(fd, "%s ", from);

	if (subj_len > 0)
	  fprintf(fd,catgets(elm_msg_cat,FilterSet,FilterAbout,
			     "about %s"),subject);
	fprintf(fd, "\n");

	if (rule_choosen != -1)
	  if (rules[rule_choosen].condition->matchwhat == TO)
	    fprintf(fd,catgets(elm_msg_cat,FilterSet,FilterAddressedTo,
			       "\t(addressed to %s)\n"), to);

	switch (what) {
	  case DELETE_MSG : fprintf(fd,catgets(elm_msg_cat,
					       FilterSet,FilterDeletedMesg,
					       "\tDELETED"));
	                    break;
	  case FAILED_SAVE: fprintf(fd,catgets(elm_msg_cat,
					       FilterSet,FilterSaveFailedMesg,
				       "\tSAVE FAILED for file \"%s\""), 
				    rules[rule_choosen].argument2);
	                    break;
	  case SAVE       : fprintf(fd,catgets(elm_msg_cat,
					       FilterSet,FilterSavedMesg,
					       "\tSAVED in file \"%s\""), 
				    rules[rule_choosen].argument2);
	                    break;
	  case SAVECC     : fprintf(fd,catgets(elm_msg_cat,
					       FilterSet,FilterSavedAndPutMesg,
		            "\tSAVED in file \"%s\" AND PUT in mailbox"), 
				    rules[rule_choosen].argument2);
	                    break;
	  case FORWARD    : fprintf(fd,catgets(elm_msg_cat,
					       FilterSet,FilterForwardedMesg,
					       "\tFORWARDED to \"%s\""), 
				    rules[rule_choosen].argument2);
	                    break;
	  case FORWARDC    : fprintf(fd,catgets(elm_msg_cat,
						FilterSet,
						FilterForwardedAndPutMesg,
			    "\tFORWARDED to \"%s\" AND PUT in mailbox"), 
				     rules[rule_choosen].argument2);
	                     break;
	  case EXEC       : fprintf(fd,catgets(elm_msg_cat,
					       FilterSet,FilterExecutedMesg,
					       "\tEXECUTED \"%s\""),
				    rules[rule_choosen].argument2);
	                    break;
	  case EXECC      : fprintf(fd,catgets(elm_msg_cat,
					       FilterSet,FilterExecutedSMesg,
				"\tEXECUTED \"%s\" AND PUT in mailbox"),
				    rules[rule_choosen].argument2);
	                    break;
	  case LEAVE      : fprintf(fd,catgets(elm_msg_cat,
					       FilterSet,FilterPutMesg,
					       "\tPUT in mailbox"));
		            break;
	}

	if (rule_choosen != -1)
	  fprintf(fd,catgets(elm_msg_cat,FilterSet,FilterByRule,
			     " by rule #%d\n"), rule_choosen+1);
	else
	  fprintf(fd,catgets(elm_msg_cat,FilterSet,FilterTheDefaultAction,
			      ": the default action\n"));

	fflush(fd);
	fclose(fd);
}

int
contains(str, pat)
char *str, *pat;
{
	/** Returns TRUE iff pat occurs IN IT'S ENTIRETY in str.
	    The comparison is case insensitive.   **/

	register int i = 0, j = 0;

	while (str[i] != '\0') {
	  while (tolower(str[i]) == tolower(pat[j])) {
	    i++; j++;
	    if (pat[j] == '\0') 
	      return(TRUE);
	  }
	  i = i - j + 1;
	  j = 0;
	}
	return(FALSE);
}

char *itoa(i, two_digit)
int i, two_digit;
{	
	/** return 'i' as a null-terminated string.  If two-digit use that
	    size field explicitly!  **/

	static char value[10];
	
	if (two_digit)
	  sprintf(value, "%02d", i);
	else
	  sprintf(value, "%d", i);

	return( (char *) value);
}

lowercase(string)
char *string;
{
	/** translate string into all lower case **/

	register int i;

	for (i= strlen(string); --i >= 0; )
	  if (isupper(string[i]))
	    string[i] = tolower(string[i]);
}

/* the following code is borrowed from elm src/file.c
 * it has been stripped down to only handle ~ expansion
 */

expand_filename(filename)
char *filename;
{
	/** Expands	~/	to the current user's home directory

	    Side effect: strips off trailing blanks

	    Returns 	1	upon proper expansions
			0	upon failed expansions
	 **/

	char temp_filename[SLEN], *ptr;

	ptr = filename;
	while (*ptr == ' ') ptr++;	/* leading spaces GONE! */
	strcpy(temp_filename, ptr);

	/** New stuff - make sure no illegal char as last **/
	ptr = temp_filename + strlen(temp_filename) - 1;
	while (*ptr == '\n' || *ptr == '\r')
	  *ptr-- = '\0';
	  
	/** Strip off any trailing backslashes or blanks **/
	while (*ptr == '\\' || *ptr == ' ')
		*ptr-- = '\0';

	if ((temp_filename[0] == '~') &&
	    (temp_filename[1] == '/')) {
	    sprintf(filename, "%s%s%s",
		  home, (lastch(home) != '/' ? "/" : ""), &temp_filename[2]);

	/* any further expansion, such as = + < >
	 * would require parsing the elmrc file
	 */

	} else
	  strcpy(filename, temp_filename);
	  
	return(1);
}
