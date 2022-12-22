
static char rcsid[] = "@(#)Id: wordwrap.c,v 5.6 1993/08/03 19:28:39 syd Exp ";

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
 * $Log: wordwrap.c,v $
 * Revision 1.2  1994/01/14  00:56:01  cp
 * 2.4.23
 *
 * Revision 5.6  1993/08/03  19:28:39  syd
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
 * Revision 5.5  1993/06/10  03:12:10  syd
 * Add missing rcs id lines
 * From: Syd
 *
 * Revision 5.4  1993/04/12  03:02:40  syd
 * Check for EINTR if getchar() returns EOF. Happens after a resume from an
 * interactive stop.
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.3  1993/04/12  02:43:53  syd
 * The builtin editor couldn't back up to a line that had a character
 * at the wrapcolumn position.
 * Added tab handling to the builtin editor.
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.2  1993/02/03  16:45:26  syd
 * A Raw(OFF) was missing so when in mail only mode and one
 * does f)orget, the "Message saved" ends up on wrong screen.
 * Also added \r\n to end of messages to make output look nicer.
 *
 * When composing mail in the builtin editor, it wrapped on /.
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/***  Routines to wrap lines when using the "builtin" editor

***/

#include "headers.h"
#include <errno.h>

extern int errno;		/* system error number */

unsigned alarm();

#define isstopchar(c)		(c == ' ' || c == '\t')
#define isslash(c)		(c == '/')
#define erase_a_char()		{ Writechar(BACKSPACE); Writechar(' '); \
			          Writechar(BACKSPACE); fflush(stdout); }

	/* WARNING: this macro destroys nr */
#define erase_tab(nr)		do Writechar(BACKSPACE); while (--(nr) > 0)

int
wrapped_enter(string, tail, x, y, edit_fd, append_current)
char *string, *tail;
int  x,y, *append_current;
FILE *edit_fd;

{
	/** This will display the string on the screen and allow the user to
	    either accept it (by pressing RETURN) or alter it according to
	    what the user types.   The various flags are:
	         string    is the buffer to use (with optional initial value)
		 tail	   contains the portion of input to be wrapped to the
			   next line
	 	 x,y	   is the location we're at on the screen (-1,-1 means
			   that we can't use this info and need to find out
			   the current location)
		 append_current  means that we have an initial string and that
			   the cursor should be placed at the END of the line,
			   not the beginning (the default).
	      
	    If we hit an interrupt or EOF we'll return non-zero.
	**/

	int ch, wrapcolumn = 70, iindex = 0;
	int addon = 0;	/* Space added by tabs. iindex+addon == column */
	int tindex = 0;	/* Index to the tabs array. */
	int tabs[10];	/* Spaces each tab adds. size <= wrapcolumn/8+1 */
	register int ch_count = 0, escaped = OFF;
	long newpos, pos;
	char line[SLEN];

	clearerr(stdin);

	if(!(x >=0 && y >= 0))
	  GetXYLocation(&x, &y);
	PutLine1(x, y, "%s", string);	

	CleartoEOLN();

	if (! *append_current) {
	  MoveCursor(x,y);
	}
	else
	  iindex = strlen(string);

	if (cursor_control)
	  transmit_functions(OFF);

	/** now we have the screen as we want it and the cursor in the 
	    right place, we can loop around on the input and return the
	    string as soon as the user presses <RETURN> or the line wraps.
	**/

	do {
	  ch = getchar();

	  if (ch == EOF && ferror(stdin) && errno == EINTR) {
	    clearerr(stdin);
	    continue;
	  }

	  if (ch == ctrl('D') || ch == EOF) {		/* we've hit EOF */
	    if (cursor_control)
	      transmit_functions(ON);
	    *append_current = 0;
	    return(1);
	  }

	  if (ch_count++ == 0) {
	    if (ch == '\n' || ch == '\r') {
	      if (cursor_control)
	        transmit_functions(ON);
	      *append_current = 0;
	      return(0);
	    }
	    else if (! *append_current) {
	      CleartoEOLN();
	      iindex = (*append_current? strlen(string) : 0);
	    }
	  }

	  /* the following is converted from a case statement to
	     allow the variable characters (backspace, kill_line
	     and break) to be processed.  Case statements in
	     C require constants as labels, so it failed ...
	  */

	  if (ch == backspace) {
	    escaped = OFF;
	    if (iindex > 0) {
  	      iindex--;
	      if (string[iindex] == '\t') {
		addon -= tabs[--tindex] - 1;
		erase_tab(tabs[tindex]);
	      } else erase_a_char();

#ifdef FTRUNCATE

	    } else { /** backspace to end of previous line **/

	      fflush(edit_fd);
	      if ((pos = ftell(edit_fd)) <= 0L) { /** no previous line **/
		Writechar('\007');

	      } else {

		/** get the last 256 bytes written **/
		if ((newpos = pos - 256L) <= 0L) newpos = 0;
		(void) fseek(edit_fd, newpos, 0L);
		(void) fread(line, sizeof(*line), (int) (pos-newpos),
		             edit_fd);
		pos--;

		/** the last char in line should be '\n'
			change it to null **/
		if (line[(int) (pos-newpos)] == '\n')
		  line[(int) (pos-newpos)] = '\0';

		/** find the end of the previous line ('\n') **/
		for (pos--; pos > newpos && line[(int) (pos-newpos)] != '\n';
			pos--);
		/** check to see if this was the first line in the file **/
		if (line[(int) (pos-newpos)] == '\n') /** no - it wasn't **/
		  pos++;
		(void) strcpy(string, &line[(int) (pos-newpos)]);
		line[(int) (pos-newpos)] = '\0';

		/** truncate the file to the current position
			THIS WILL NOT WORK ON SYS-V **/
		(void) fseek(edit_fd, newpos, 0L);
		(void) fputs(line, edit_fd);
		fflush(edit_fd);
		(void) ftruncate(fileno(edit_fd), (int) ftell(edit_fd));
		(void) fseek(edit_fd, ftell(edit_fd), 0L);

		/** rewrite line on screen and continue working **/
		GetXYLocation(&x, &y);
		if (x > 0) x--;
		PutLine1(x, y, "%s", string);
		CleartoEOLN();
		iindex = strlen(string);

		/* Reload tab positions */
		addon = tindex = 0;
		for (pos = 0; pos < iindex; pos++)
		  if (string[pos] == '\t')
		    addon += (tabs[tindex++] = 8 - ((pos+addon) & 07)) - 1;
	      }

#endif

	    }
	    fflush(stdout);
	  }
	  else if (ch == EOF || ch == '\n' || ch == '\r') {
	    escaped = OFF;
	    string[iindex] = '\0';
	    if (cursor_control)
	      transmit_functions(ON);
	    *append_current = 0;
	    return(0);
	  }
	  else if (ch == ctrl('W')) {	/* back up a word! */
	    escaped = OFF;
	    if (iindex == 0)
	      continue;		/* no point staying here.. */
	    iindex--;
	    if (isslash(string[iindex])) {
	      erase_a_char();
	    }
	    else {
	      while (iindex >= 0 && isspace(string[iindex])) {
		if (string[iindex] == '\t') {
		  addon -= tabs[--tindex] - 1;
		  erase_tab(tabs[tindex]);
		} else erase_a_char();
	        iindex--;
	      }

	      while (iindex >= 0 && ! isstopchar(string[iindex])) {
	        iindex--;
	        erase_a_char();
	      }
	      iindex++;	/* and make sure we point at the first AVAILABLE slot */
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
	    *append_current = 0;
	    return(-1);
	  }
	  else if (!isprint(ch) && ch != '\t') {
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
		if (ch == '\t')
		  addon += (tabs[tindex++] = 8 - ((addon+iindex) & 07)) - 1;

		string[iindex++] = ch;
		Writechar(ch);
		escaped = ( ch == '\\' ? ON : OFF);
	      }
	  }
	} while (iindex+addon < wrapcolumn);

	string[iindex] = '\0';
	*append_current = line_wrap(string,tail,&iindex,&tabs[tindex-1]);

	if (cursor_control)
	  transmit_functions(ON);
	return(0);
}

int
line_wrap(string,tail,count,tabs)
char *string;	/* The string to be wrapped */
char *tail;	/* The part of the string which is wrapped */
int *count;	/* Offset of string terminator */
int *tabs;	/* List of how many spaces each tab adds */
{
	/** This will check for line wrap.  If the line was wrapped,
	    it will back up to white space (if possible), write the
	    shortened line, and put the remainder at the beginning
	    of the string.  Returns 1 if wrapped, 0 if not.
	**/

	int n = *count;
	int i, j;

	/* Look for a space */
	while (n && !isstopchar(string[n]))
	  --n;

	/* If break found */
	if (n) {

	  /* Copy part to be wrapped */
	  for (i=0,j=n+1;j<=*count;tail[i++]=string[j++]);

	  /* Skip the break character and any whitespace */
	  while (n && isstopchar(string[n]))
	    --n;

	  if (n) n++; /* Move back into the whitespace */
	}

	/* If no break found */
	if (!n) {
	  (*count)--;
	  strcpy(tail, &string[*count]);
	  erase_a_char();
	} else /* Erase the stuff that will wrap */
	  while (*count > n) {
	    --(*count);
	  if (string[*count] == '\t') erase_tab(*tabs--);
	  else erase_a_char();
	  }

	string[*count] = '\0';
	return(1);
}
