
static char rcsid[] ="@(#)Id: summarize.c,v 5.8 1993/02/08 18:38:12 syd Exp ";

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
 * $Log: summarize.c,v $
 * Revision 1.2  1994/01/14  00:51:41  cp
 * 2.4.23
 *
 * Revision 5.8  1993/02/08  18:38:12  syd
 * Fix to copy_file to ignore unescaped from if content_length not yet reached.
 * Fixes to NLS messages match number of newlines between default messages
 * and NLS messages. Also an extra ) was removed.
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.7  1993/01/27  19:40:01  syd
 * I implemented a change to filter's default verbose message format
 * including %x %X style date and time along with username
 * From: mark@drd.com (Mark Lawrence)
 *
 * Revision 5.6  1992/12/11  01:45:04  syd
 * remove sys/types.h include, it is now included by defs.h
 * and this routine includes defs.h or indirectly includes defs.h
 * From: Syd
 *
 * Revision 5.5  1992/12/07  05:14:19  syd
 * add sys/types.h for time_t
 * From: Syd
 *
 * Revision 5.4  1992/11/15  01:40:43  syd
 * Add regexp processing to filter.
 * Add execc operator
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.3  1992/11/07  20:48:55  syd
 * fix applied rule count message
 *
 * Revision 5.2  1992/10/24  14:20:24  syd
 * remove the 25 (MAXRULES) limitation.
 * Basically it mallocs rules in hunks of RULESINC (25) as it goes along.
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.1  1992/10/03  22:18:09  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** This routine is called from the filter program (or can be called
    directly with the correct arguments) and summarizes the users filterlog
    file.  To be honest, there are two sorts of summaries that are
    available - either the '.filterlog' file can be output (filter -S) 
    or a summary by rule and times acted upon can be output (filter -s).
    Either way, this program will delete the two associated files each
    time ($HOME/.filterlog and $HOME/.filtersum) *if* the -c option is
    used to the program (e.g. clear_logs is set to TRUE).

**/

#include <stdio.h>

#include "defs.h"
#include "filter.h"
#include "s_filter.h"

extern char *date_n_user();

show_summary()
{
	/* Summarize usage of the program... */

	FILE   *fd;				/* for output to temp file! */
	char filename[SLEN],			/* name of the temp file    */
	     buffer[SLEN];			/* input buffer space       */
	int  erroneous_rules = 0,
	     default_rules   = 0,
	     messages_filtered = 0,		/* how many have we touched? */
	     rule,
	     *applied;

	sprintf(filename, "%s/%s", home, filtersum);

	if ((fd = fopen(filename, "r")) == NULL) {
	  if (outfd != NULL)
	    fprintf(outfd,catgets(elm_msg_cat,
				  FilterSet,FilterCantOpenFiltersum,
			"filter (%s): Can't open filtersum file %s!\n"),
		    date_n_user(), filename);
	  if (outfd != NULL) fclose(outfd);
	  exit(1);
	}

	applied = (int *)malloc(sizeof(int)*total_rules);
	if (applied == NULL){
	  if (outfd != NULL)
	    fprintf(outfd,catgets(elm_msg_cat,
				  FilterSet,FilterOutOfMemory,
                        "filter (%s): Out of memory [malloc failed]\n"),
			 username);
	  if (outfd != NULL) fclose(outfd);
	  exit(1);
	}

	for (rule=0;rule < total_rules; rule++)
	  applied[rule] = 0;			/* initialize it all! */

	/** Next we need to read it all in, incrementing by which rule
	    was used.  The format is simple - each line represents a 
	    single application of a rule, or '-1' if the default action
	    was taken.  Simple stuff, eh?  But oftentimes the best.  
	**/

	while (fgets(buffer, SLEN, fd) != NULL) {
	  if ((rule = atoi(buffer)) > total_rules || rule < -1) {
	    if (outfd != NULL)
	      fprintf(outfd,catgets(elm_msg_cat,
				    FilterSet,FilterWarningInvalidForShort,
      "filter (%s): Warning - rule #%d is invalid data for short summary!!\n"),
	            date_n_user(), rule);
	    erroneous_rules++;
	  }
	  else if (rule == -1)
	    default_rules++;
	  else
	    applied[rule]++;
	  messages_filtered++;
	}
	
	fclose(fd);

	/** now let's summarize the data... **/

	if (outfd == NULL) return;		/* no reason to go further */

	fprintf(outfd,catgets(elm_msg_cat,FilterSet,FilterSumTitle,
		"\n\t\t\tA Summary of Filter Activity\n"));
	fprintf(outfd, 
		  "\t\t\t----------------------------\n\n");

	fprintf(outfd,
		messages_filtered>1 ?
		catgets(elm_msg_cat,
			FilterSet,FilterTotalMsgsFlrdPlural,
			"A total of %d messages were filtered:\n\n") :
		catgets(elm_msg_cat,
			FilterSet,FilterTotalMessagesFiltered,
			"A total of %d message was filtered:\n\n"),
		messages_filtered);

	if (erroneous_rules)
	  {
		fprintf(outfd,
			erroneous_rules>1 ?
			catgets(elm_msg_cat,
				FilterSet,FilterErroneousRulesPlural,
		"[Warning: %d erroneous rules were logged and ignored!]") :
			catgets(elm_msg_cat,
				FilterSet,FilterErroneousRules,
		"[Warning: %d erroneous rule was logged and ignored!]"),
			erroneous_rules);
		
	  }
	
	
	if (default_rules) {
	   fprintf(outfd,catgets(elm_msg_cat,FilterSet,FilterDefaultRuleMesg,
             "The default rule of putting mail into your mailbox"));
	   
	   fprintf(outfd,
		   default_rules>1 ?
		   catgets(elm_msg_cat,FilterSet,FilterAppliedTimesPlural,
			   "\n\tapplied %d times (%d%%)\n\n") :
		   catgets(elm_msg_cat,FilterSet,FilterAppliedTimes,
			   "\n\tapplied %d time (%d%%)\n\n"),
		   default_rules,
		   (default_rules*100+(messages_filtered>>1))/
		                   messages_filtered);
	}

	 /** and now for each rule we used... **/

	 for (rule = 0; rule < total_rules; rule++) {
	   if (applied[rule]) {
	      fprintf(outfd,catgets(elm_msg_cat,FilterSet,FilterRuleNum,
				    "Rule #%d: "), rule+1);
	      switch (rules[rule].action) {
		  case LEAVE:	    fprintf(outfd,
					    catgets(elm_msg_cat,
						    FilterSet,FilterLeaveMesg,
						  "(leave mail in mailbox)"));
				    break;
		  case DELETE_MSG:  fprintf(outfd,
					    catgets(elm_msg_cat,
						    FilterSet,FilterDeleteMesg,
						    "(delete message)"));
				    break;
		  case SAVE  :      fprintf(outfd,catgets(elm_msg_cat,
							  FilterSet,
							  FilterSaveMesg,
							  "(save in \"%s\")"),
					    rules[rule].argument2);
		                    break;
		  case SAVECC:      fprintf(outfd,catgets(elm_msg_cat,
							  FilterSet,
							  FilterSaveCcMesg,
				    "(left in mailbox and saved in \"%s\")"),
					    rules[rule].argument2);
		                    break;
		  case FORWARD:     fprintf(outfd,catgets(elm_msg_cat,
							  FilterSet,
							  FilterForwardMesg,
						  "(forwarded to \"%s\")"),
					    rules[rule].argument2);
		                    break;
		  case FORWARDC:    fprintf(outfd,catgets(elm_msg_cat,
							  FilterSet,
							  FilterForwardCMesg,
			  "(left in mailbox and forwarded to \"%s\")"),
					    rules[rule].argument2);
		                    break;
		  case EXEC :      fprintf(outfd,catgets(elm_msg_cat,
							  FilterSet,
							  FilterExecMesg,
					  "(given to command \"%s\")"),
					    rules[rule].argument2);
		                    break;
		  case EXECC :      fprintf(outfd,catgets(elm_msg_cat,
							  FilterSet,
							  FilterExecCMesg,
			   "(left in mailbox and given to command \"%s\")"),
					    rules[rule].argument2);
		                    break;
	      }
	      fprintf(outfd,
		      applied[rule]>1 ?
		      catgets(elm_msg_cat,FilterSet,FilterAppliedTimesPlural,
			      "\n\tapplied %d times (%d%%)\n\n") :
		      catgets(elm_msg_cat,FilterSet,FilterAppliedTimes,
			      "\n\tapplied %d time (%d%%)\n\n"),
		      applied[rule],
		      (applied[rule]*100+(messages_filtered>>1))/
		      messages_filtered);
	  }
	}

	if (long_summary) {

	  /* next, after a ^L, include the actual log file... */

	  sprintf(filename, "%s/%s", home, filterlog);

	  if ((fd = fopen(filename, "r")) == NULL) {
	    fprintf(outfd,catgets(elm_msg_cat,
				  FilterSet,FilterCantOpenFilterlog,
			"filter (%s): Can't open filterlog file %s!\n"),
		      date_n_user(), filename);
	  }
	  else {
	    fprintf(outfd,catgets(elm_msg_cat,FilterSet,FilterExplicitLog,
			"\n\n\n%c\n\nExplicit log of each action;\n\n"), 
		    (char) 12);
	    while (fgets(buffer, SLEN, fd) != NULL)
	      fprintf(outfd, "%s", buffer);
	    fprintf(outfd, "\n-----\n");
	    fclose(fd);
	  }
	}

	/* now remove the log files, please! */

	if (clear_logs) {
	  sprintf(filename, "%s/%s", home, filterlog);
	  unlink(filename);
	  sprintf(filename, "%s/%s", home, filtersum);
	  unlink(filename);
	}

	return;
}
