
static char rcsid[] = "@(#)Id: mkhdrs.c,v 5.3 1993/05/08 20:25:33 syd Exp ";

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
 * $Log: mkhdrs.c,v $
 * Revision 1.2  1994/01/14  00:55:19  cp
 * 2.4.23
 *
 * Revision 5.3  1993/05/08  20:25:33  syd
 * Add sleepmsg to control transient message delays
 * From: Syd
 *
 * Revision 5.2  1993/02/03  17:12:53  syd
 * move more declarations to defs.h, including sleep
 * From: Syd
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** This contains all the header generating routines for the ELM
    program.

**/

#include "headers.h"
#include "s_elm.h"

extern char in_reply_to[SLEN];

char *elm_date_str();

generate_reply_to(msg)
int msg;
{
	/** Generate an 'in-reply-to' message... **/
	char buffer[SLEN], date_buf[SLEN];


	if (msg == -1)		/* not a reply! */
	  in_reply_to[0] = '\0';
	else {
	  if (chloc(headers[msg]->from, '!') != -1)
	    tail_of(headers[msg]->from, buffer, 0);
	  else
	    strcpy(buffer, headers[msg]->from);
	  sprintf(in_reply_to, "%s from \"%s\" at %s",
                  headers[msg]->messageid[0] == '\0'? "<no.id>":
		  headers[msg]->messageid,
		  buffer,
		  elm_date_str(date_buf, headers[msg]->time_sent + headers[msg]->tz_offset));
	}
}

add_mailheaders(filedesc)
FILE *filedesc;
{
	/** Add the users .mailheaders file if available.  Allow backquoting 
	    in the file, too, for fortunes, etc...*shudder*
	**/

	FILE *fd;
	char filename[SLEN], buffer[SLEN];

	sprintf(filename, "%s/%s", home, mailheaders);

	if ((fd = fopen(filename, "r")) != NULL) {
	  while (fgets(buffer, SLEN, fd) != NULL)
	    if (strlen(buffer) < 2) {
	      dprint(2, (debugfile,
	         "Strlen of line from .elmheaders is < 2 (write_header_info)"));
	      error1(catgets(elm_msg_cat, ElmSet, ElmWarningBlankIgnored,
		"Warning: blank line in %s ignored!"), filename);
	      if (sleepmsg > 0)
		    sleep(sleepmsg);
	    }
	    else if (occurances_of(BACKQUOTE, buffer) >= 2) 
	      expand_backquote(buffer, filedesc);
	    else 
	      fprintf(filedesc, "%s", buffer);

	    fclose(fd);
	}
}

expand_backquote(buffer, filedesc)
char *buffer;
FILE *filedesc;
{
	/** This routine is called with a line of the form:
		Fieldname: `command`
	    and is expanded accordingly..
	**/

	FILE *fd;
	char command[SLEN], command_buffer[SLEN], fname[SLEN],
	     prefix[SLEN];
	register int i, j = 0;

	for (i=0; buffer[i] != BACKQUOTE; i++)
	  prefix[j++] = buffer[i];
	prefix[j] = '\0';

	j = 0;

	for (i=chloc(buffer, BACKQUOTE)+1; buffer[i] != BACKQUOTE;i++)
	  command[j++] = buffer[i];
	command[j] = '\0';

	sprintf(fname,"%s%s%d", temp_dir, temp_print, getpid());

	sprintf(command_buffer, "%s > %s", command, fname);

	(void) system_call(command_buffer, 0);

	if ((fd = fopen(fname, "r")) == NULL) {
	  error1(catgets(elm_msg_cat, ElmSet, ElmBackquoteCmdFailed,
		"Backquoted command \"%s\" in elmheaders failed."), command);
	  return;	
	}

	/* If we get a line that is less than 80 - length of prefix then we
	   can toss it on the same line, otherwise, simply prepend each line
	   *starting with this line* with a leading tab and cruise along */

	if (fgets(command_buffer, SLEN, fd) == NULL) 
	  fprintf(filedesc, prefix);
	else {
	  if (strlen(command_buffer) + strlen(prefix) < 80) 
	    fprintf(filedesc, "%s%s", prefix, command_buffer);
	  else
	    fprintf(filedesc, "%s\n\t%s", prefix, command_buffer);
	  
	  while (fgets(command_buffer, SLEN, fd) != NULL) 
	    fprintf(filedesc, "\t%s", command_buffer);
	
	  fclose(fd);
	}

	unlink(fname);	/* don't leave the temp file laying around! */
}
