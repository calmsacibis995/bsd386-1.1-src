
static char rcsid[] = "@(#)Id: in_utils.c,v 5.13 1993/08/03 19:28:39 syd Exp ";

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
 * $Log: in_utils.c,v $
 * Revision 1.2  1994/01/14  00:55:05  cp
 * 2.4.23
 *
 * Revision 5.13  1993/08/03  19:28:39  syd
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
 * Revision 5.12  1993/08/03  19:10:50  syd
 * Patch for Elm 2.4 PL22 to correct handling of SIGWINCH signals on
 * DecStations with Ultrix 4.2.
 * The problem was that elm running in an xterm exits silently when the
 * window is resize. This was caused by incorrect signal handling for BSD.
 * From: vogt@isa.de
 *
 * Revision 5.11  1993/05/16  20:56:18  syd
 * fix want-to patch collision
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.10  1993/05/08  20:25:33  syd
 * Add sleepmsg to control transient message delays
 * From: Syd
 *
 * Revision 5.9  1993/04/12  03:38:19  syd
 * In GetPrompt(), when timeout = 0 make sure we check the return of
 * ReadCh() for EINTR on non-POSIX (BSDish) systems.
 * From: tom@osf.org
 *
 * Revision 5.8  1993/04/12  03:09:17  syd
 * want_to() wrote "No." when user pressed return even if the default answer
 * wasn't == *def_ans_no.
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.7  1993/04/12  02:34:36  syd
 * I have now added a parameter which controls whether want_to clears the
 * line and centers the question or behaves like it did before. I also
 * added a 0 at the end of the parameter list to all the other calls to
 * want_to where a centered question on a clean line is not desirable.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.6  1993/01/30  15:57:19  syd
 * fix where it clears the prompt from
 *
 * Revision 5.5  1993/01/29  03:43:46  syd
 * back out change to clear question line,
 * its worse that the original problem.
 * From: Syd
 *
 * Revision 5.4  1993/01/27  20:48:01  syd
 * Change where confirm prompt are written.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.3  1992/12/24  21:42:01  syd
 * Fix messages and nls messages to match.  Plus use want_to
 * where appropriate.
 * From: Syd, via prompting from Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.2  1992/11/15  01:15:28  syd
 * The alias message_count isn't set to zero if the last alias has
 * been deleted from the alias table. As no aliases are reread from
 * the aliases database the message_count is left as it was before.
 *
 * Fixed that the function do_newalias() sometimes returns without freeing
 * the buffer allocated before. The patch adds these free calls.
 *
 * When you erroneously type a number in your folder elm asks you for
 * a new current message number. But now if you erase this one number
 * and leave the string empty elm will set the new current message to
 * the second message on our sun4! The patch adds a check for an empty
 * string and returns the current number if no number was entered.
 * From: vogt@isa.de (Gerald Vogt)
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** Mindless I/O routines for ELM 
	
**/

#include "headers.h"
#include "s_elm.h"
#include <errno.h>

extern int errno;		/* system error number */

unsigned alarm();

#define isstopchar(c)		(c == ' ' || c == '\t' || c == '/')
#define isslash(c)		(c == '/')
#define erase_a_char()		{ Writechar(BACKSPACE); Writechar(' '); \
			          Writechar(BACKSPACE); fflush(stdout); }

int
want_to(question, dflt, where, clear_and_center)
char *question, dflt;
int where, clear_and_center;
{
	/** Ask 'question' on 'where' left enough to just leave room for an
	    answer, returning the answer in lower case.
	    Echo answer as full "Yes" or "No".  'dflt' is the 
	    default answer if <return> is pressed. (Note: 'dflt' is also what 
	    will be returned if <return> is pressed!)
	**/
	register int ch, cols;

	cols = COLUMNS - (strlen(question) + 5 );	/* 5 for "Yes." + 1 */
	if (cols < 0) {
	    cols = 0;
	}

	MoveCursor(where, (clear_and_center || (cols < 2)) ? 0 : cols-2);
	CleartoEOLN();

	PutLine3(where, clear_and_center ? cols/2 : cols,
		 "%s%c%c", question, dflt, BACKSPACE);
	fflush(stdout);
	fflush(stdin);

	ch = ReadCh();
	ch = tolower(ch);

	while (!( ch == *def_ans_yes || ch == *def_ans_no || ch == '\n' || ch == '\r')) {
	  ch = ReadCh();
	  ch = tolower(ch);
	}
	if(ch == '\n' || ch == '\r')
	  ch = dflt;

	if(ch == *def_ans_yes)
	  Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmYesWord, "Yes."), 0);
	else if (ch == *def_ans_no)
	  Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmNoWord, "No."), 0);
	else
	  return(ch); /* Don't write anything, just return */

	if (sleepmsg > 0)
		sleep((sleepmsg + 1) / 2);
	MoveCursor(where, (clear_and_center || (cols < 2)) ? 0 : cols-2);
	CleartoEOLN();

	return(ch);
}

int
read_number(ch, item)
char ch, *item;
{
	/** Read a number, where 'ch' is the leading digit! **/
	
	char buff[NLEN];
	int  num;

	buff[0] = ch;
	buff[1] = '\0';

	PutLine1(LINES-3, COLUMNS-40,catgets(elm_msg_cat, ElmSet, ElmSetCurrentTo,
		"Set current %s to :"), item);
	if (optionally_enter(buff, LINES-3, COLUMNS-15, TRUE, FALSE) == -1)
	  return(current);

	if (buff[0] == '\0')
	  return(current);

	sscanf(buff,"%d", &num);
	return(num);
}

int
optionally_enter(string, x, y, append_current, passwd)
char *string;
int  x,y, append_current, passwd;
{
	/** This will display the string on the screen and allow the user to
	    either accept it (by pressing RETURN) or alter it according to
	    what the user types.   The various flags are:
	         string    is the buffer to use (with optional initial value)
	 	 x,y	   is the location we're at on the screen (-1,-1 means
			   that we can't use this info and need to find out
			   the current location)
		 append_current  means that we have an initial string and that
			   the cursor should be placed at the END of the line,
			   not the beginning (the default).
		 passwd	   accept non-printing characters and do not echo
			   entered characters.
	      
	    If we hit an interrupt or EOF we'll return non-zero.
	**/

	int ch;
	register int ch_count = 0, iindex = 0, escaped = OFF;
	register int len = strlen(string), maxbuf = SLEN;

	clearerr(stdin);

	if(!passwd) {
	  if(!(x >=0 && y >= 0))
	    GetXYLocation(&x, &y);
	  PutLine1(x, y, "%s", string);	
	}

	CleartoEOLN();

	if ( len > maxbuf ) {
	  maxbuf = len;
	}

	if (! append_current) {
	  MoveCursor(x,y);
	}
	else
	  iindex = strlen(string);

	if (cursor_control)
	  transmit_functions(OFF);

	/** now we have the screen as we want it and the cursor in the 
	    right place, we can loop around on the input and return the
	    string as soon as the user presses <RETURN>
	**/

	do {
	  ch = getchar();

	  if (ch == ctrl('D') || ch == EOF) {		/* we've hit EOF */
	    if (cursor_control)
	      transmit_functions(ON);
	    return(1);
	  }

	  if (ch_count++ == 0) {
	    if (ch == '\n' || ch == '\r') {
	      if (cursor_control)
	        transmit_functions(ON);
	      return(0);
	    }
	    else if (! append_current) {
	      CleartoEOLN();
	      iindex = (append_current? strlen(string) : 0);
	    }
	  }

	  /* the following is converted from a case statement to
	     allow the variable characters (backspace, kill_line
	     and break) to be processed.  Case statements in
	     C require constants as labels, so it failed ...
	  */

	  if (ch == backspace &&
	    (!escaped || (!isprint(ch) && !passwd))) {
	    /* This is tricky. Here we are dealing with all situations
	     * under which a backspace (really whatever erase char is
	     * set to, not necessarily \b) erases the previous character.
	     * It will erase unless escaped, because if it's escaped
	     * it is taken literally. There is one exception to that --
	     * if backspace would be rejected (we don't accept non-printing
	     * characters in non-passwd mode), we accept it here as an
	     * erasing character, for it if got rejected there would
	     * be no way of erasing a preceding backslash. */
	    escaped = OFF;
	    if (iindex > 0) {
	      if(!passwd)
		Writechar(BACKSPACE);
  	      iindex--;
	    }
	    if(!passwd) {
	      Writechar(' ');
	      Writechar(BACKSPACE);
	      fflush(stdout);
	    }
	  }
	  else if (ch == EOF || ch == '\n' || ch == '\r') {
	    escaped = OFF;
	    string[iindex] = '\0';
	    if (cursor_control)
	      transmit_functions(ON);
	    return(0);
	  }
	  else if (!passwd && ch == ctrl('W')) {	/* back up a word! */
	    escaped = OFF;
	    if (iindex == 0)
	      continue;		/* no point staying here.. */
	    iindex--;
	    if (isslash(string[iindex])) {
	      erase_a_char();
	    }
	    else {
	      while (iindex >= 0 && isspace(string[iindex])) {
	        iindex--;
	        erase_a_char();
	      }

	      while (iindex >= 0 && ! isstopchar(string[iindex])) {
	        iindex--;
	        erase_a_char();
	      }
	      iindex++;	/* and make sure we point at the first AVAILABLE slot */
	    }
	  }
	  else if (!passwd && ch == ctrl('R')) {
	    escaped = OFF;
	    string[iindex] = '\0';
	    PutLine1(x,y, "%s", string);	
	    CleartoEOLN();
	  }
	  else if (!escaped && ch == kill_line) {
	    /* needed to test if escaped since kill_line character could
	     * be a desired valid printing character */
	    escaped = OFF;
	    if(!passwd) {
	      MoveCursor(x,y);
	      CleartoEOLN();
	    }
	    iindex = 0;
	  }
	  else if (ch == '\0') {
	    escaped = OFF;
	    if (cursor_control)
	      transmit_functions(ON);
	    fflush(stdin); 	/* remove extraneous chars, if any */
	    string[0] = '\0'; /* clean up string, and... */
	    return(-1);
	  }
	  else if (!passwd && !isprint(ch)) {
	    /* non-printing character - warn with bell*/
	    /* don't turn off escaping backslash since current character
	     * doesn't "use it up".
	     */
	    Writechar('\007');
	  }
	  else {  /* default case */
	      if(escaped && (ch == backspace || ch == kill_line)) {
		/* if last character was a backslash,
		 * and if this character is escapable
		 * simply write this character over it even if
		 * this character is a backslash.
		 */
		if(!passwd)
		  Writechar(BACKSPACE);
		iindex--;
		string[iindex++] = ch;
		if(!passwd)
		  Writechar(ch);
	        escaped = OFF;
	      } else {
		string[iindex++] = ch;
		if(!passwd)
		  Writechar(ch);
		escaped = ( ch == '\\' ? ON : OFF);
	      }
	  }
	} while (iindex < maxbuf);

	string[iindex] = '\0';

	if (cursor_control)
	  transmit_functions(ON);

	return(0);
}

int
pattern_enter(string, alt_string, x, y, alternate_prompt)
char *string, *alt_string, *alternate_prompt;
int  x,y;
{
	/** This function is functionally similar to the routine
	    optionally-enter, but if the first character pressed
	    is a '/' character, then the alternate prompt and string
	    are used rather than the normal one.  This routine 
	    returns 1 if alternate was used, 0 if not
	**/

	int ch;
	register int iindex = 0, escaped = OFF;

	PutLine1(x, y, "%s", string);	
	CleartoEOLN();
	MoveCursor(x,y);

	if (cursor_control)
	  transmit_functions(OFF);

	ch = getchar();

	if (ch == '\n' || ch == '\r') {
	  if (cursor_control)
	    transmit_functions(ON);
	  return(0);	/* we're done.  No change needed */
	}
	
	if (ch == '/') {
	  PutLine1(x, 0, "%s", alternate_prompt);
	  CleartoEOLN();
	  (void) optionally_enter(alt_string, x, strlen(alternate_prompt)+1,
		 FALSE, FALSE);
	  return(1);
	}

	CleartoEOLN();

	iindex = 0;

	if (ch == kill_line) {
	  MoveCursor(x,y);
          CleartoEOLN();
	  iindex = 0;
	}
	else if (ch != backspace) {
	  if(ch == '\\') escaped = ON;
	  Writechar(ch);
	  string[iindex++] = ch;
	}
	else if (iindex > 0) {
	  iindex--;
	  erase_a_char();
	}
	else {
	  Writechar(' ');
	  Writechar(BACKSPACE);
	}

	do {
	  fflush(stdout);
	  ch = getchar();

	  /* the following is converted from a case statement to
	     allow the variable characters (backspace, kill_line
	     and break) to be processed.  Case statements in
	     C require constants as labels, so it failed ...
	  */

	    if (ch == backspace &&
	      (!escaped || !isprint(ch)) ) {
	      /* This is tricky. Here we are dealing with all situations
	       * under which a backspace (really whatever erase char is
	       * set to, not necessarily \b) erases the previous character.
	       * It will erase unless escaped, because if it's escaped
	       * it is taken literally. There is one exception to that --
	       * if backspace would be rejected (we don't accept non-printing
	       * characters in non-passwd mode), we accept it here as an
	       * erasing character, for it if got rejected there would
	       * be no way of erasing a preceding backslash. */
	      escaped = OFF;
              if (iindex > 0) {
		iindex--;
		erase_a_char();
	      }
	      else {
		Writechar(' ');
		Writechar(BACKSPACE);
	      }
	    }
	    else if (ch == '\n' || ch == '\r') {
	      escaped = OFF;
	      string[iindex] = '\0';
	      if (cursor_control)
	        transmit_functions(ON);
	      return(0);
	    }
	    else if (ch == ctrl('W')) {
	      escaped = OFF;
	      if (iindex == 0)
	        continue;		/* no point staying here.. */
	      iindex--;
	      if (isslash(string[iindex])) {
	        erase_a_char();
	      }
	      else {
	        while (iindex >= 0 && isspace(string[iindex])) {
	          iindex--;
	          erase_a_char();
	        }

	        while (iindex >= 0 && ! isstopchar(string[iindex])) {
	          iindex--;
	          erase_a_char();
	        }
	        iindex++;/* and make sure we point at the first AVAILABLE slot */
	      }
	    }
	    else if (ch == ctrl('R')) {
	      escaped = OFF;
	      string[iindex] = '\0';
	      PutLine1(x,y, "%s", string);	
	      CleartoEOLN();
	    }
	    else if (!escaped && ch == kill_line) {
	      /* needed to test if escaped since kill_line character could
	       * be a desired valid printing character */
	      escaped = OFF;
	      MoveCursor(x,y);
              CleartoEOLN();
	      iindex = 0;
	    }
	    else if (ch == '\0') {
	      escaped = OFF;
	      if (cursor_control)
	        transmit_functions(ON);
	      fflush(stdin); 	/* remove extraneous chars, if any */
	      string[0] = '\0'; /* clean up string, and... */
	      return(-1);
	    }
	    else if (!isprint(ch)) {
	      /* non-printing character - warn with bell*/
	      /* don't turn off escaping backslash since current character
	       * doesn't "use it up".
	       */
	      Writechar('\007');
	    }
	    else {  /* default case */
		if(escaped && (ch == backspace || ch == kill_line)) {
		  /* if last character was a backslash,
		   * and if this character is escapable
		   * simply write this character over it even if
		   * this character is a backslash.
		   */
		  Writechar(BACKSPACE);
		  iindex--;
		  string[iindex++] = ch;
		  Writechar(ch);
		  escaped = OFF;
		} else {
		  string[iindex++] = ch;
		  Writechar(ch);
		  escaped = ( ch == '\\' ? ON : OFF);
		}
	    }
	} while (iindex < SLEN);

	string[iindex] = '\0';

	if (cursor_control)
	  transmit_functions(ON);
	return(0);
}

int
GetPrompt()
{
	/** This routine does a read/timeout for a single character.
	    The way that this is determined is that the routine to
	    read a character is called, then the "errno" is checked
	    against EINTR (interrupted call).  If they match, this
	    returns NO_OP_COMMAND otherwise it returns the normal
	    command.  On BSD systems, the EINTR will never be returned
	    so we instead longjmp from the signal handler.
	**/

	int ch;

	if (timeout > 0) {
	  alarm((unsigned) timeout);
#ifdef	BSD
	  if (setjmp(GetPromptBuf)) {
	    InGetPrompt = 0;
	    ch = NO_OP_COMMAND;
	    alarm((unsigned) 0);
	  }
	  else {
	    InGetPrompt = 1;
	    ch = ReadCh();
	    InGetPrompt = 0;
	    alarm((unsigned) 0);
	  }
	}
	else {
	    ch = ReadCh();
	    if (errno == EINTR) ch = NO_OP_COMMAND;
	}
#else
	    errno = 0;	/* we actually have to do this.  *sigh*  */
	    ch = ReadCh();
	    if (errno == EINTR) ch = NO_OP_COMMAND;
	    alarm((unsigned) 0);
	}
	else {
	  errno = 0;
	  ch = ReadCh();
	  if (errno == EINTR) ch = NO_OP_COMMAND;
	}
#endif

	return(ch);
}

