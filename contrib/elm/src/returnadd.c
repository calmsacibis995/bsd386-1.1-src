
static char rcsid[] = "@(#)Id: returnadd.c,v 5.8 1993/04/12 03:11:23 syd Exp ";

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
 * $Log: returnadd.c,v $
 * Revision 1.2  1994/01/14  00:55:37  cp
 * 2.4.23
 *
 * Revision 5.8  1993/04/12  03:11:23  syd
 * Fix to don't use address from reply-to field if it is empty.
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * This can not happen according to RFC822. It requires at least one address if the
 * reply-to is present, so this bug (EB48) isn't really a bug. But one can
 * always try to be nice :-).
 *
 * Revision 5.7  1993/02/03  19:06:31  syd
 * Remove extra strchr/strcat/strcpy et al declarations
 * From: Syd
 *
 * Revision 5.6  1992/12/20  05:25:13  syd
 * Always pass three parameters to header_cmp().
 * From: chip@tct.com (Chip Salzenberg)
 *
 * Revision 5.5  1992/12/11  01:45:04  syd
 * remove sys/types.h include, it is now included by defs.h
 * and this routine includes defs.h or indirectly includes defs.h
 * From: Syd
 *
 * Revision 5.4  1992/11/26  00:46:50  syd
 * Fix how errno is used so err is inited and used instead
 * as errno gets overwritten by print system call
 * From: Syd
 *
 * Revision 5.3  1992/11/07  20:05:52  syd
 * change to use header_cmp to allow for linear white space around the colon
 * From: Syd
 *
 * Revision 5.2  1992/11/07  14:59:05  syd
 * fix format variable for long
 * From: Syd
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 * 
 ******************************************************************************/

/** This set of routines is used to generate real return addresses
    and also return addresses suitable for inclusion in a users
    alias files (ie optimized based on the pathalias database).

**/

#include "headers.h"
#include "s_elm.h"

#include <errno.h>
#include <sys/stat.h>

char *shift_lower();

extern int errno;

char *error_description();

void
get_existing_address(buffer, msgnum)
char *buffer;
int msgnum;
{
	/** This routine is called when the message being responded to has
	    "To:xyz" as the return address, signifying that this message is
	    an automatically saved copy of a message previously sent.  The
	    correct to address can be obtained fairly simply by reading the
	    To: header from the message itself and (blindly) copying it to
	    the given buffer.  Note that this header can be either a normal
	    "To:" line (Elm) or "Originally-To:" (previous versions e.g.Msg)
	**/

	char mybuf[LONG_STRING];
	register char ok = 1, in_to = 0;
	int  err;

	buffer[0] = '\0';

	/** first off, let's get to the beginning of the message... **/

	if(msgnum < 0 || msgnum >= message_count || message_count < 1) {
	  dprint(1, (debugfile,
		"Error: %d not a valid message number message_count = %d (%s)",
		msgnum, message_count, "get_existing_address"));
	  error1(catgets(elm_msg_cat, ElmSet, ElmNotAValidMessageNum,
		"%d not a valid message number!"), msgnum);
	  return;
	}
        if (fseek(mailfile, headers[msgnum]->offset, 0) == -1) {
	    err = errno;
	    dprint(1, (debugfile, 
		    "Error: seek %ld bytes into file hit errno %s (%s)", 
		    headers[msgnum]->offset, error_description(err), 
		    "get_existing_address"));
	    error2(catgets(elm_msg_cat, ElmSet, ElmCouldntSeekBytesIntoFlle,
		   "Couldn't seek %ld bytes into file (%s)."),
	           headers[msgnum]->offset, error_description(err));
	    return;
        }
 
        /** okay!  Now we're there!  **/

        while (ok) {
          ok = (int) (mail_gets(mybuf, LONG_STRING, mailfile) != 0);
	  no_ret(mybuf);	/* remove return character */

          if (header_cmp(mybuf, "To", NULL) ||
	      header_cmp(mybuf, "Original-To", NULL)) {
	    in_to = TRUE;
	    strcpy(buffer, index(mybuf, ':') + 1);
          }
	  else if (in_to && whitespace(mybuf[0])) {
	    strcat(buffer, " ");		/* tag a space in   */
	    strcat(buffer, (char *) mybuf + 1);	/* skip 1 whitespace */
	  }
	  else if (strlen(mybuf) < 2)
	    return;				/* we're done for!  */
	  else
	    in_to = 0;
      }
}

int
get_return(buffer, msgnum)
char *buffer;
int msgnum;
{
	/** reads msgnum message again, building up the full return 
	    address including all machines that might have forwarded 
	    the message.  Returns whether it is using the To line **/

	char buf[SLEN], name1[SLEN], name2[SLEN], lastname[SLEN];
	char hold_return[SLEN], alt_name2[SLEN], buf2[SLEN];
	int lines, len_buf, len_buf2, colon_offset, decnet_found;
	int using_to = FALSE, err;

	/* now initialize all the char buffers [thanks Keith!] */

	buf[0] = name1[0] = name2[0] = lastname[0] = '\0';
	hold_return[0] = alt_name2[0] = buf2[0] = '\0';

	/** get to the first line of the message desired **/

	if(msgnum < 0 || msgnum >= message_count || message_count < 1) {
	  dprint(1, (debugfile,
		"Error: %d not a valid message number message_count = %d (%s)",
		msgnum, message_count, "get_return"));
	  error1(catgets(elm_msg_cat, ElmSet, ElmNotAValidMessageNum,
		"%d not a valid message number!"), msgnum);
	  return(using_to);
	}

	if (fseek(mailfile, headers[msgnum]->offset, 0) == -1) {
	  err = errno;
	  dprint(1, (debugfile,
		"Error: seek %ld bytes into file hit errno %s (%s)", 
		headers[msgnum]->offset, error_description(err), 
	        "get_return"));
	  error2(catgets(elm_msg_cat, ElmSet, ElmCouldntSeekBytesIntoFlle,
		"Couldn't seek %ld bytes into file (%s)."),
	       headers[msgnum]->offset, error_description(err));
	  return(using_to);
	}
 
	/** okay!  Now we're there!  **/

	lines = headers[msgnum]->lines;

	buffer[0] = '\0';

	if (len_buf2 = mail_gets(buf2, SLEN, mailfile)) {
	  if(buf2[len_buf2-1] == '\n')
	    lines--; /* got a full line */
	}

	while (len_buf2 && lines) {
	  buf[0] = '\0';
	  strncat(buf, buf2, SLEN);
	  len_buf = strlen(buf);
	  if (len_buf2 = mail_gets(buf2, SLEN, mailfile)) {
	    if(buf2[len_buf2-1] == '\n')
	      lines--; /* got a full line */
	  }
	  while (len_buf2 && lines && whitespace(buf2[0]) && len_buf >= 2) {
	    if (buf[len_buf-1] == '\n') {
	      len_buf--;
	      buf[len_buf] = '\0';
	    }
	    strncat(buf, buf2, (SLEN-len_buf-1));
	    len_buf = strlen(buf);
	    if (len_buf2 = mail_gets(buf2, SLEN, mailfile)) {
	      if(buf2[len_buf2-1] == '\n')
		lines--; /* got a full line */
	    }
	  }

/* At this point, "buf" contains the unfolded header line, while "buf2" contains
   the next single line of text from the mail file */

	  if (first_word(buf, "From ")) 
	    sscanf(buf, "%*s %s", hold_return);
	  else if (first_word_nc(buf, ">From")) {
	    sscanf(buf,"%*s %s %*s %*s %*s %*s %*s %*s %*s %s %s", 
	           name1, name2, alt_name2);
	    if (strcmp(name2, "from") == 0)		/* remote from xyz  */
	      strcpy(name2, alt_name2);
	    else if (strcmp(name2, "by") == 0)	/* forwarded by xyz */
	      strcpy(name2, alt_name2);
	    add_site(buffer, name2, lastname);
	  }

#ifdef USE_EMBEDDED_ADDRESSES

	  else if (header_cmp(buf, "From", NULL)) {
	    get_address_from(buf, hold_return);
	    buffer[0] = '\0';
          }
          else if (header_cmp(buf, "Reply-To", NULL)) {
	    get_address_from(buf, buffer);
	    if (strlen(buffer) > 0) return(using_to);
          }

#endif

	  else if (len_buf < 2)	/* done with header */
            lines = 0; /* let's get outta here!  We're done!!! */
	}

	if (buffer[0] == '\0')
	  strcpy(buffer, hold_return); /* default address! */
	else
	  add_site(buffer, name1, lastname);	/* get the user name too! */

	if (header_cmp(buffer, "To", NULL)) {	/* backward compatibility ho */
	  get_existing_address(buffer, msgnum);
	  using_to = TRUE;
	}
	else {
	  /*
	   * KLUDGE ALERT - DANGER WILL ROBINSON
	   * We can't just leave a bare login name as the return address,
	   * or it will be alias-expanded.
	   * So we qualify it with the current host name (and, maybe, domain).
	   * Sigh.
	   */

	  if ((colon_offset = qchloc(buffer, ':')) > 0)
		decnet_found = buffer[colon_offset + 1] == ':';
	  else
		decnet_found = NO;
		
	  if (qchloc(buffer, '@') < 0
	   && qchloc(buffer, '%') < 0
	   && qchloc(buffer, '!') < 0
	   && decnet_found == NO)
	  {
#ifdef INTERNET
	    sprintf(buffer + strlen(buffer), "@%s", hostfullname);
#else
	    strcpy(buf, buffer);
	    sprintf(buffer, "%s!%s", hostname, buf);
#endif
	  }

	  /*
	   * If we have a space character,
	   * or we DON'T have '!' or '@' chars,
	   * append the user-readable name.
	   */
	  if (qchloc(headers[msgnum]->from, ' ') >= 0 ||
	      (qchloc(headers[msgnum]->from, '!') < 0 &&
	       qchloc(headers[msgnum]->from, '@') < 0)) {
	       sprintf(buffer + strlen(buffer),
		       " (%s)", headers[msgnum]->from);
          }
	}

	return(using_to);
}
