
static char rcsid[] ="@(#)Id: rules.c,v 5.8 1993/08/03 19:28:39 syd Exp ";

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
 * $Log: rules.c,v $
 * Revision 1.2  1994/01/14  00:51:40  cp
 * 2.4.23
 *
 * Revision 5.8  1993/08/03  19:28:39  syd
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
 * Revision 5.7  1993/01/27  19:45:15  syd
 * Filter turns spaces in quoted strings into _ and then back again. This destroys
 * any _ that where there in the first place. This patch removes that.
 * Also fixed a minor bug where 'filter -r' wrote out the wrong thing if the
 * relation in a rule was '~'.
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.6  1993/01/27  19:40:01  syd
 * I implemented a change to filter's default verbose message format
 * including %x %X style date and time along with username
 * From: mark@drd.com (Mark Lawrence)
 *
 * Revision 5.5  1992/12/11  01:45:04  syd
 * remove sys/types.h include, it is now included by defs.h
 * and this routine includes defs.h or indirectly includes defs.h
 * From: Syd
 *
 * Revision 5.4  1992/11/22  00:06:45  syd
 * Fix when expanding the macro '%S', the subject line is scanned for a
 * 'Re:', and if nothing is found, a '"Re: ' is added. But when a 'Re:'
 * *is* found, then nothing is added, not even the '"'.
 * From: Sigmund Austigard <austig@solan.unit.no>
 *
 * Revision 5.3  1992/11/17  04:10:01  syd
 * The changes to filter/regexp.c are to correct compiler warnings about
 * unreachable statements.
 *
 * The changes to filter/rules.c are to correct the fact that we are passing
 * a pointer to a condition_rec structore to a function expecting a pointer to
 * a character string.
 * From: Tom Moore <tmoore@fievel.DaytonOH.NCR.COM>
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

/** This file contains all the rule routines, including those that apply the
    specified rules and the routine to print the rules out.

**/

#include <stdio.h>
#include "defs.h"
#include <pwd.h>
#ifdef I_TIME
#  include <time.h>
#endif
#ifdef I_SYSTIME
#  include <sys/time.h>
#endif
#include <fcntl.h>

#include "filter.h"
#include "s_filter.h"

extern char *date_n_user();

static struct regexp *last_regexp = NULL;

static int
matches(str, relation, cond)
char *str;
int relation;
struct condition_rec *cond;
{
	if (relation == RE) {
	  if (cond->regex == NULL) cond->regex = regcomp(cond->argument1);
	  if (regexec(cond->regex, str)) {
	    last_regexp = cond->regex;
	    return TRUE;
	  } else {
	    return FALSE;
	  }
	} else {
	  return contains(str, cond->argument1);
	}
}

void
regerror(s)
char *s;
{
	if (outfd != NULL)
	  fprintf(outfd,
	    catgets(elm_msg_cat,
		    FilterSet,FilterCantCompileRegexp,
		    "filter (%s): Error: can't compile regexp: \"%s\"\n"),
		 username, s);
	if (outfd != NULL) fclose(outfd);
	exit(1); 		
}

int
action_from_ruleset()
{
	/** Given the set of rules we've read in and the current to, from, 
	    and subject, try to match one.  Return the ACTION of the match
            or LEAVE if none found that apply.
	**/

	register int iindex = 0, not, relation, try_next_rule, x;
	struct condition_rec *cond;

	while (iindex < total_rules) {
	  cond = rules[iindex].condition;
	  try_next_rule = 0;

	  while (cond != NULL && ! try_next_rule) {
	    
	    not = (cond->relation < 0);
	    relation = abs(cond->relation);
	
	    switch (cond->matchwhat) {

	      case TO     : x = matches(to, relation, cond);		break;
	      case FROM   : x = matches(from, relation, cond); 		break;
	      case SENDER : x = matches(sender, relation, cond);	break;
	      case SUBJECT: x = matches(subject, relation, cond);	break;
	      case LINES  : x = compare(lines, relation, cond);		break;
		       
	      case CONTAINS: if (outfd != NULL) fprintf(outfd,
				catgets(elm_msg_cat,
				    FilterSet,FilterContainsNotImplemented,
       "filter (%s): Error: rules based on 'contains' are not implemented!\n"),
			    date_n_user());
			    if (outfd != NULL) fclose(outfd);
			    exit(0); 		

	      case ALWAYS: not = FALSE; x = TRUE;			break;
	    }

	    if ((not && x) || ((! not) && (! x))) /* this test failed (LISP?) */
	      try_next_rule++;
	    else
	      cond = cond->next;		  /* next condition, if any?  */
	  }

	  if (! try_next_rule) {
	    rule_choosen = iindex;
 	    return(rules[rule_choosen].action);
	  }
	  iindex++;
	}

	rule_choosen = -1;
	return(LEAVE);
}

#define get_the_time()	if (!gotten_time) { 		  \
			   thetime = time( (time_t *) 0);   \
			   timerec = localtime(&thetime); \
			   gotten_time++; 		  \
			}

static struct {
	int	id;
	char	*str;
} regmessage[] = {
	FilterWholeRegsub,	"<match>",
	FilterRegsubOne,	"<submatch-1>",
	FilterRegsubTwo,	"<submatch-2>",
	FilterRegsubThree,	"<submatch-3>",
	FilterRegsubFour,	"<submatch-4>",
	FilterRegsubFive,	"<submatch-5>",
	FilterRegsubSix,	"<submatch-6>",
	FilterRegsubSeven,	"<submatch-7>",
	FilterRegsubEight,	"<submatch-8>",
	FilterRegsubNine,	"<submatch-9>",
};

expand_macros(word, buffer, line, display)
char *word, *buffer;
int  line, display;
{
	/** expand the allowable macros in the word;
		%d	= day of the month  
		%D	= day of the week  
	        %h	= hour (0-23)	 
		%m	= month of the year
		%n      = sender of message
		%r	= return address of sender
	   	%s	= subject of message
	   	%S	= "Re: subject of message"  (only add Re: if not there)
		%t	= hour:minute 	
		%y	= year		  
		%&	= the whole string that matched last regexp
		%1-%9	= matched subexpressions in last regexp
	    or simply copies word into buffer. If "display" is set then
	    instead it puts "<day-of-month>" etc. etc. in the output.
	**/

#ifndef	_POSIX_SOURCE
	struct tm *localtime();
	time_t    time();
#endif
	struct tm *timerec;
	time_t	thetime;
	register int i, j=0, gotten_time = 0, reading_a_percent_sign = 0,
                     len, backslashed=0, regsub;

	for (i = 0, len = strlen(word); i < len; i++) {
	  if (reading_a_percent_sign) {
	    reading_a_percent_sign = 0;
	    switch (word[i]) {

	      case 'n' : buffer[j] = '\0';
			 if (display)
			   strcat(buffer,catgets(elm_msg_cat,
						 FilterSet,FilterSender,
						 "<sender>"));
			 else {
			   strcat(buffer, "\"");
			   strcat(buffer, sender);
			   strcat(buffer, "\"");
			 }
			 j = strlen(buffer);
			 break;

	      case 'r' : buffer[j] = '\0';
			 if (display)
	 		   strcat(buffer,catgets(elm_msg_cat,
						 FilterSet,FilterReturnAddress,
						 "<return-address>"));
			 else
			   strcat(buffer, from);
	                 j = strlen(buffer);
			 break;

	      case 's' : buffer[j] = '\0';
			 if (display)
	 		   strcat(buffer,catgets(elm_msg_cat,
						FilterSet,FilterSubject,
						 "<subject>"));
			 else {
			   strcat(buffer, "\"");
			   strcat(buffer, subject);
			   strcat(buffer, "\"");
			 }
	                 j = strlen(buffer);
			 break;

	      case 'S' : buffer[j] = '\0';
			 if (display)
	 		   strcat(buffer,catgets(elm_msg_cat,
						 FilterSet,FilterReSubject,
						 "<Re: subject>"));
			 else {
			   strcat(buffer, "\"");
			   if (! the_same(subject, "Re:")) 
			     strcat(buffer, "Re: ");
			   strcat(buffer, subject);
			   strcat(buffer, "\"");
			 }
	                 j = strlen(buffer);
			 break;

	      case 'd' : get_the_time(); buffer[j] = '\0';
			 if (display)
			   strcat(buffer,catgets(elm_msg_cat,
						 FilterSet,FilterDayOfMonth,
						 "<day-of-month>"));
			 else
			   strcat(buffer, itoa(timerec->tm_mday,FALSE));
	                 j = strlen(buffer);
			 break;

	      case 'D' : get_the_time(); buffer[j] = '\0';
			 if (display)
			   strcat(buffer,catgets(elm_msg_cat,
						 FilterSet,FilterDayOfWeek,
						 "<day-of-week>"));
			 else
			   strcat(buffer, itoa(timerec->tm_wday,FALSE));
	                 j = strlen(buffer);
			 break;

	      case 'm' : get_the_time(); buffer[j] = '\0';
			 if (display)
			   strcat(buffer,catgets(elm_msg_cat,
						 FilterSet,FilterMonth,
						 "<month>"));
			 else
			   sprintf(&buffer[j],"%2.2d",timerec->tm_mon+1);
	                 j = strlen(buffer);
			 break;

	      case 'y' : get_the_time(); buffer[j] = '\0';
			 if (display)
			   strcat(buffer,catgets(elm_msg_cat,
						 FilterSet,FilterYear,
						 "<year>"));
			 else
			   strcat(buffer, itoa(timerec->tm_year,FALSE));
	                 j = strlen(buffer);
			 break;

	      case 'h' : get_the_time(); buffer[j] = '\0';
			 if (display)
			   strcat(buffer,catgets(elm_msg_cat,
						 FilterSet,FilterHour,
						 "<hour>"));
			 else
			   strcat(buffer, itoa(timerec->tm_hour,FALSE));
	                 j = strlen(buffer);
			 break;

	      case 't' : get_the_time(); buffer[j] = '\0';
			 if (display)
			   strcat(buffer,catgets(elm_msg_cat,
						 FilterSet,FilterTime,
						 "<time>"));
		         else {
			   strcat(buffer, itoa(timerec->tm_hour,FALSE));
			   strcat(buffer, ":");
			   strcat(buffer, itoa(timerec->tm_min,TRUE));
			 }
	                 j = strlen(buffer);
			 break;

	      case '&': case '1': case '2': case '3': case '4':
	      case '5': case '6': case '7': case '8': case '9':

			 if (word[i] == '&') regsub = 0;
			 else regsub = word[i] - '0';

			 if (display) {
			   strcat(buffer,catgets(elm_msg_cat,
						 FilterSet,
						 regmessage[regsub].id,
						 regmessage[regsub].str));
			   j = strlen(buffer);
			 } else {
			   if (last_regexp != NULL) {
			     char *sp = last_regexp->startp[regsub];
			     char *ep = last_regexp->endp[regsub];
			     if (sp != NULL && ep != NULL && ep > sp)
			       while (sp < ep) buffer[j++] = *sp++;
			   }
			 }
			 break;

	      default  : if (outfd != NULL) fprintf(outfd,
				catgets(elm_msg_cat,
					FilterSet,FilterErrorTranslatingMacro,
   "filter (%s): Error on line %d translating %%%c macro in word \"%s\"!\n"),
			         date_n_user(), line, word[i], word);
			 if (outfd != NULL) fclose(outfd);
			 exit(1);
	    }
	  }
	  else if (word[i] == '%') {
	    if (backslashed) {
                 buffer[j++] = '%';
                 backslashed = 0;
            } else {
	         reading_a_percent_sign++;
	    }
	  } else if (word[i] == '\\') {
	    if (backslashed) {
		 buffer[j++] = '\\';
		 backslashed = 0;
	    } else {
		 backslashed++;
	    }
	  } else {
	    buffer[j++] = word[i];
	    backslashed = 0;
	  }
	}
	buffer[j] = '\0';
}

print_rules()
{
	/** print the rules out.  A double check, of course! **/

	register int i = -1;
	char     *whatname(), *actionname();
	struct   condition_rec *cond;

	if (outfd == NULL) return;	/* why are we here, then? */

	while (++i < total_rules) {
	  if (rules[i].condition->matchwhat == ALWAYS) {
	    fprintf(outfd,catgets(elm_msg_cat,FilterSet,FilterAlways,
			"\nRule %d:  ** always ** \n\t%s %s\n"), i+1,
		 actionname(rules[i].action), rules[i].argument2);
	    continue;
	  }

	  fprintf(outfd,catgets(elm_msg_cat,FilterSet,FilterRuleIf,
				"\nRule %d:  if ("), i+1);

	  cond = rules[i].condition;

	  while (cond != NULL) {
	    if (cond->relation < 0)
	      fprintf(outfd,catgets(elm_msg_cat,FilterSet,FilterNot,
				    "not %s %s %s%s%s"), 
		      whatname(cond->matchwhat),
		      relationname(- (cond->relation)),
		      quoteit(cond->matchwhat, -cond->relation),
		      cond->argument1,
		      quoteit(cond->matchwhat, -cond->relation));
	    else
	      fprintf(outfd, "%s %s %s%s%s",
		      whatname(cond->matchwhat),
		      relationname(cond->relation),
		      quoteit(cond->matchwhat, cond->relation),
		      cond->argument1,
		      quoteit(cond->matchwhat, cond->relation));

	    cond = cond->next;

	    if (cond != NULL)
	      fprintf(outfd,catgets(elm_msg_cat,
				    FilterSet,FilterAnd,
				    " and "));
	  }
	    
	  fprintf(outfd,catgets(elm_msg_cat,FilterSet,FilterThen,
				") then\n\t  %s %s\n"), 
		 actionname(rules[i].action), rules[i].argument2);
	}
	fprintf(outfd, "\n");
}

char *whatname(n)
int n;
{
	static char buffer[10];

	switch(n) {
	  case FROM   : return("from");
	  case TO     : return("to");
	  case SENDER : return("sender");
	  case SUBJECT: return("subject");
	  case LINES  : return ("lines");
	  case CONTAINS: return("contains");
	  default     : sprintf(buffer, "?%d?", n); return((char *)buffer);
	}
}

char *actionname(n)
int n;
{
	switch(n) {
	  case DELETE_MSG : return(catgets(elm_msg_cat,
					   FilterSet,FilterDelete,
					   "Delete"));
	  case SAVE       : return(catgets(elm_msg_cat,
					   FilterSet,FilterSave,
					   "Save"));
	  case SAVECC     : return(catgets(elm_msg_cat,
					   FilterSet,FilterCopyAndSave,
					   "Copy and Save"));
	  case FORWARD    : return(catgets(elm_msg_cat,
					   FilterSet,FilterForward,
					   "Forward"));
	  case FORWARDC   : return(catgets(elm_msg_cat,
					   FilterSet,FilterCopyAndForward,
					   "Copy and Forward"));
	  case LEAVE      : return(catgets(elm_msg_cat,
					   FilterSet,FilterLeave,
					   "Leave")); 
	  case EXEC       : return(catgets(elm_msg_cat,
					   FilterSet,FilterExecute,
					   "Execute"));
	  case EXECC       : return(catgets(elm_msg_cat,
					   FilterSet,FilterExecuteAndSave,
					   "Execute and Save"));
	  default         : return(catgets(elm_msg_cat,
					   FilterSet,FilterAction,
					   "?action?"));
	}
}

int
compare(line, relop, cond)
int line, relop;
struct condition_rec *cond;
{
	/** Given the actual number of lines in the message, the relop
	    relation, and the number of lines in the rule, as a string (!),
   	    return TRUE or FALSE according to which is correct.
	**/

	int rule_lines;

	rule_lines = atoi(cond->argument1);

	switch (relop) {
	  case LE: return(line <= rule_lines);
	  case LT: return(line <  rule_lines);
	  case GE: return(line >= rule_lines);
	  case GT: return(line >  rule_lines);
	  case NE: return(line != rule_lines);
	  case EQ: return(line == rule_lines);
	}
	return(-1);
}
