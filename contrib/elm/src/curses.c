
static char rcsid[] = "@(#)Id: curses.c,v 5.13 1993/08/23 02:56:35 syd Exp ";

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
 * $Log: curses.c,v $
 * Revision 1.2  1994/01/14  00:54:37  cp
 * 2.4.23
 *
 * Revision 5.13  1993/08/23  02:56:35  syd
 * have Writechar() backspace over the left edge of the screen to the end
 * of the previous line if the current line is not the first line on the
 * screen.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.12  1993/08/03  19:28:39  syd
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
 * Revision 5.11  1993/07/20  02:13:20  syd
 * Make tabspacing check for <= 0 so we dont get divide by
 * zero errors when the termcap has tabspacing 0
 * From: Syd via request from G A Smant
 *
 * Revision 5.10  1993/04/12  03:57:45  syd
 * Give up and add an Ultrix specific patch. There is a bug in Ispell under
 * ultrix.  The problem is that when ispell returns, the terminal is no
 * longer in raw mode. (Ispell isn't restoring the terminal parameters)
 * From: Scott Ames <scott@cwis.unomaha.edu>
 *
 * Revision 5.9  1993/02/03  19:06:31  syd
 * Remove extra strchr/strcat/strcpy et al declarations
 * From: Syd
 *
 * Revision 5.8  1992/12/11  01:45:04  syd
 * remove sys/types.h include, it is now included by defs.h
 * and this routine includes defs.h or indirectly includes defs.h
 * From: Syd
 *
 * Revision 5.7  1992/11/07  20:45:39  syd
 * add no tite flag on options that should not use ti/te
 * Hack by Syd
 *
 * Revision 5.6  1992/10/27  15:46:35  syd
 * Suns dont like ioctl on top of termios
 * From: syd
 *
 * Revision 5.5  1992/10/27  01:52:16  syd
 * Always include <sys/ioctl.h> in curses.c When calling ioctl()
 *
 * Remove declaration of getegid() from leavembox.c & lock.c
 * They aren't even used there.
 * From: tom@osf.org
 *
 * Revision 5.4  1992/10/24  13:35:39  syd
 * changes found by using codecenter on Elm 2.4.3
 * From: Graham Hudspith <gwh@inmos.co.uk>
 *
 * Revision 5.3  1992/10/17  22:58:57  syd
 * patch to make elm use (or in my case, not use) termcap/terminfo ti/te.
 * From: Graham Hudspith <gwh@inmos.co.uk>
 *
 * Revision 5.2  1992/10/11  01:02:05  syd
 * Add AIX to those who dont define window size in termios.h
 * From: Syd via note from Tom Kovar
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/**  This library gives programs the ability to easily access the
     termcap information and write screen oriented and raw input
     programs.  The routines can be called as needed, except that
     to use the cursor / screen routines there must be a call to
     InitScreen() first.  The 'Raw' input routine can be used
     independently, however.

**/

/** NOTE THE ADDITION OF: the #ifndef ELM stuff around routines that
    we don't use.  This is for code size and compile time speed...
**/

#include "headers.h"

#ifdef TERMIOS
# include <termios.h>
# ifndef sun
#  include <sys/ioctl.h>	/* for TIOCGWINSZ */
# endif
#else
# ifdef TERMIO
#  include <termio.h>
# else
#  include <sgtty.h>
# endif
#endif

#ifdef PTEM
#  include <sys/stream.h>
#  include <sys/ptem.h>
#endif

#define TTYIN	0

#ifdef SHORTNAMES
# define _clearinverse	_clrinv
# define _cleartoeoln	_clrtoeoln
# define _cleartoeos	_clr2eos
# define _transmit_off	xmit_off
# define _transmit_on	xmit_on
#endif

#ifdef TERMIOS
struct termios _raw_tty,
	       _original_tty;
#define	ttgetattr(fd,where)	tcgetattr((fd),(where))
#define	ttsetattr(fd,where)	tcsetattr((fd),TCSADRAIN,(where))
#else	/*TERMIOS*/
# ifdef TERMIO
struct termio _raw_tty, 
              _original_tty;
#define	ttgetattr(fd,where)	ioctl((fd),TCGETA,(where))
#define	ttsetattr(fd,where)	ioctl((fd),TCSETAW,(where))
# else
struct sgttyb _raw_tty,
	      _original_tty;
#define	ttgetattr(fd,where)	ioctl((fd),TIOCGETP,(where))
#define	ttsetattr(fd,where)	ioctl((fd),TIOCSETP,(where))
# endif	/*TERMIO*/
#endif	/*TERMIOS*/

static int _inraw = 0;                  /* are we IN rawmode?    */

#define DEFAULT_LINES_ON_TERMINAL	24
#define DEFAULT_COLUMNS_ON_TERMINAL	80

static int _memory_locked = 0;		/* are we IN memlock??   */
static int _line  = -1,			/* initialize to "trash" */
           _col   = -1;

static int _intransmit;			/* are we transmitting keys? */

static
char *_clearscreen, *_moveto, *_up, *_down, *_right, *_left,
     *_setbold, *_clearbold, *_setunderline, *_clearunderline, 
     *_sethalfbright, *_clearhalfbright, *_setinverse, *_clearinverse,
     *_cleartoeoln, *_cleartoeos, *_transmit_on, *_transmit_off,
     *_set_memlock, *_clear_memlock, *_start_termcap, *_end_termcap;

static int _lines, _columns, _automargin, _eatnewlineglitch;
int tabspacing;

static char _terminal[1024];              /* Storage for terminal entry */
static char _capabilities[1024];           /* String for cursor motion */

static char *ptr = _capabilities;	/* for buffering         */

int    outchar();			/* char output for tputs */
char  *tgetstr(),     		       /* Get termcap capability */
      *tgoto();				/* and the goto stuff    */

InitScreen()
{
	/* Set up all this fun stuff: returns zero if all okay, or;
        -1 indicating no terminal name associated with this shell,
        -2..-n  No termcap for this terminal type known
   */

	int  tgetent(),      /* get termcap entry */
	     err;
	char termname[40];
	char *termenv;
	
	if ((termenv = getenv("TERM")) == NULL) return(-1);

	if (strcpy(termname, termenv) == NULL)
		return(-1);

	if ((err = tgetent(_terminal, termname)) != 1)
		return(err-2);

	_line  =  0;		/* where are we right now?? */
	_col   =  0;		/* assume zero, zero...     */

	/* load in all those pesky values */
	_clearscreen       = tgetstr("cl", &ptr);
	_moveto            = tgetstr("cm", &ptr);
	_up                = tgetstr("up", &ptr);
	_down              = tgetstr("do", &ptr);
	_right             = tgetstr("nd", &ptr);
	_left              = tgetstr("bc", &ptr);
	_setbold           = tgetstr("so", &ptr);
	_clearbold         = tgetstr("se", &ptr);
	_setunderline      = tgetstr("us", &ptr);
	_clearunderline    = tgetstr("ue", &ptr);
	_setinverse        = tgetstr("so", &ptr);
	_clearinverse      = tgetstr("se", &ptr);
	_sethalfbright     = tgetstr("hs", &ptr);
	_clearhalfbright   = tgetstr("he", &ptr);
	_cleartoeoln       = tgetstr("ce", &ptr);
	_cleartoeos        = tgetstr("cd", &ptr);
	_lines	      	   = tgetnum("li");
	_columns	   = tgetnum("co");
	tabspacing	   = ((tabspacing=tgetnum("it"))<= 0 ? 8 : tabspacing);
	_automargin	   = tgetflag("am");
	_eatnewlineglitch   = tgetflag("xn");
	_transmit_on	   = tgetstr("ks", &ptr);
	_transmit_off      = tgetstr("ke", &ptr);
	_set_memlock	   = tgetstr("ml", &ptr);
	_clear_memlock	   = tgetstr("mu", &ptr);
	_start_termcap	   = tgetstr("ti", &ptr);
	_end_termcap	   = tgetstr("te", &ptr);


	if (!_left) {
		_left = "\b";
	}

	return(0);
}

char *return_value_of(termcap_label)
char *termcap_label;
{
	/** This will return the string kept by termcap for the 
	    specified capability. Modified to ensure that if 
	    tgetstr returns a pointer to a transient address	
	    that we won't bomb out with a later segmentation
	    fault (thanks to Dave@Infopro for this one!)

	    Tweaked to remove padding sequences.
	 **/

	static char escape_sequence[20];
	register int i=0,j=0;
	char buffer[20];
	char *myptr, *tgetstr();     		/* Get termcap capability */

	if (strlen(termcap_label) < 2)
	  return(NULL);

	if (termcap_label[0] == 's' && termcap_label[1] == 'o')
	  {
	  if (_setinverse)
	    strcpy(escape_sequence, _setinverse);
	  else
	    return( (char *) NULL );
	  }
	else if (termcap_label[0] == 's' && termcap_label[1] == 'e')
	  {
	  if (_clearinverse)
	    strcpy(escape_sequence, _clearinverse);
	  else
	    return( (char *) NULL );
	  }
	else if ((myptr = tgetstr(termcap_label, &ptr)) == NULL)
	  return( (char *) NULL );
	else
	  strcpy(escape_sequence, myptr);

	if (chloc(escape_sequence, '$') != -1) {
	  while (escape_sequence[i] != '\0') {
	    while (escape_sequence[i] != '$' && escape_sequence[i] != '\0')
	      buffer[j++] = escape_sequence[i++];
	    if (escape_sequence[i] == '$') {
	      while (escape_sequence[i] != '>') i++;
	      i++;
	    }
	  }
	  buffer[j] = '\0';
	  strcpy(escape_sequence, buffer);
	}

	return( (char *) escape_sequence);
}

transmit_functions(newstate)
int newstate;
{
	/** turn function key transmission to ON | OFF **/

	if (newstate != _intransmit) {
		_intransmit = ! _intransmit;
		if (newstate == ON)
		  tputs(_transmit_on, 1, outchar);
		else 
		  tputs(_transmit_off, 1, outchar);
		fflush(stdout);      /* clear the output buffer */
	}
}

/****** now into the 'meat' of the routines...the cursor stuff ******/

ScreenSize(lines, columns)
int *lines, *columns;
{
	/** returns the number of lines and columns on the display. **/

#ifdef TIOCGWINSZ
	struct winsize w;

	if (ioctl(1,TIOCGWINSZ,&w) != -1) {
		if (w.ws_row > 0)
			_lines = w.ws_row;
		if (w.ws_col > 0)
			_columns = w.ws_col;
	}
#endif

	if (_lines == 0) _lines = DEFAULT_LINES_ON_TERMINAL;
	if (_columns == 0) _columns = DEFAULT_COLUMNS_ON_TERMINAL;

	*lines = _lines - 1;		/* assume index from zero */
	*columns = _columns;
}

SetXYLocation(x,y)
int x,y;
{
	/* declare where the cursor is on the screen - useful after using
	 * a function that moves cursor in predictable fasion but doesn't
	 * set the static x and y variables used in this source file -
	 * e.g. getpass().
	 */

	_line = x;
	_col = y;
}

GetXYLocation(x,y)
int *x,*y;
{
	/* return the current cursor location on the screen */

	*x = _line;
	*y = _col;
}

ClearScreen()
{
	/* clear the screen: returns -1 if not capable */

	_line = 0;	/* clear leaves us at top... */
	_col  = 0;

	if (!_clearscreen)
		return(-1);

	tputs(_clearscreen, 1, outchar);
	fflush(stdout);      /* clear the output buffer */
	return(0);
}

static
CursorUp(n)
int n;
{
	/** move the cursor up 'n' lines **/
	/** Calling function must check that _up is not null before calling **/

	_line = (_line-n > 0? _line - n: 0);	/* up 'n' lines... */

	while (n-- > 0)
		tputs(_up, 1, outchar);

	fflush(stdout);
	return(0);
}


static
CursorDown(n)
int n;
{
	/** move the cursor down 'n' lines **/
	/** Caller must check that _down is not null before calling **/

	_line = (_line+n <= LINES? _line + n: LINES);    /* down 'n' lines... */

	while (n-- > 0)
		tputs(_down, 1, outchar);

	fflush(stdout);
	return(0);
}


static
CursorLeft(n)
int n;
{
	/** move the cursor 'n' characters to the left **/
	/** Caller must check that _left is not null before calling **/

	_col = (_col - n> 0? _col - n: 0);	/* left 'n' chars... */

	while (n-- > 0)
		tputs(_left, 1, outchar);

	fflush(stdout);
	return(0);
}


static
CursorRight(n)
int n;
{
	/** move the cursor 'n' characters to the right (nondestructive) **/
	/** Caller must check that _right is not null before calling **/

	_col = (_col+n < COLUMNS? _col + n: COLUMNS);	/* right 'n' chars... */

	while (n-- > 0)
		tputs(_right, 1, outchar);

	fflush(stdout);
	return(0);
}

static
moveabsolute(col, row)
{

	char *stuff, *tgoto();

	stuff = tgoto(_moveto, col, row);
	tputs(stuff, 1, outchar);
	fflush(stdout);
}

MoveCursor(row, col)
int row, col;
{
	/** move cursor to the specified row column on the screen.
            0,0 is the top left! **/

	int scrollafter = 0;

	/* we don't want to change "rows" or we'll mangle scrolling... */

	if (col < 0)
	  col = 0;
	if (col >= COLUMNS)
	  col = COLUMNS - 1;
	if (row < 0)
	  row = 0;
	if (row > LINES) {
	  if (col == 0)
	    scrollafter = row - LINES;
	  row = LINES;
	}

	if (!_moveto)
		return(-1);

	if (row == _line) {
	  if (col == _col)
	    return(0);				/* already there! */

	  else if (abs(col - _col) < 5) {	/* within 5 spaces... */
	    if (col > _col && _right)
	      CursorRight(col - _col);
	    else if (col < _col &&  _left)
	      CursorLeft(_col - col);
	    else
	      moveabsolute(col, row);
          }
	  else 		/* move along to the new x,y loc */
	    moveabsolute(col, row);
	}
	else if (_line == row-1 && col == 0) {
	  if (_col != 0)
	    putchar('\r');
	  putchar('\n');
	  fflush(stdout);
	}
	else if (col == _col && abs(row - _line) < 5) {
	  if (row < _line && _up)
	    CursorUp(_line - row);
	  else if (row > _line && _down)
	    CursorDown(row - _line);
	  else
	    moveabsolute(col, row);
	}
	else 
	  moveabsolute(col, row);

	_line = row;	/* to ensure we're really there... */
	_col  = col;

	if (scrollafter) {
	  putchar('\r');
	  while (scrollafter--)
	    putchar('\n');
	}

	return(0);
}

CarriageReturn()
{
	/** move the cursor to the beginning of the current line **/
	Writechar('\r');
}

NewLine()
{
	/** move the cursor to the beginning of the next line **/

	Writechar('\r');
	Writechar('\n');
}

StartBold()
{
	/** start boldface/standout mode **/

	if (!_setbold)
		return(-1);

	tputs(_setbold, 1, outchar);
	fflush(stdout);
	return(0);
}


EndBold()
{
	/** compliment of startbold **/

	if (!_clearbold)
		return(-1);

	tputs(_clearbold, 1, outchar);
	fflush(stdout);
	return(0);
}

#ifndef ELM

StartUnderline()
{
	/** start underline mode **/

	if (!_setunderline)
		return(-1);

	tputs(_setunderline, 1, outchar);
	fflush(stdout);
	return(0);
}


EndUnderline()
{
	/** the compliment of start underline mode **/

	if (!_clearunderline)
		return(-1);

	tputs(_clearunderline, 1, outchar);
	fflush(stdout);
	return(0);
}


StartHalfbright()
{
	/** start half intensity mode **/

	if (!_sethalfbright)
		return(-1);

	tputs(_sethalfbright, 1, outchar);
	fflush(stdout);
	return(0);
}

EndHalfbright()
{
	/** compliment of starthalfbright **/

	if (!_clearhalfbright)
		return(-1);

	tputs(_clearhalfbright, 1, outchar);
	fflush(stdout);
	return(0);
}

StartInverse()
{
	/** set inverse video mode **/

	if (!_setinverse)
		return(-1);

	tputs(_setinverse, 1, outchar);
	fflush(stdout);
	return(0);
}


EndInverse()
{
	/** compliment of startinverse **/

	if (!_clearinverse)
		return(-1);

	tputs(_clearinverse, 1, outchar);
	fflush(stdout);
	return(0);
}

int
HasMemlock()
{
	/** returns TRUE iff memory locking is available (a terminal
	    feature that allows a specified portion of the screen to
	    be "locked" & not cleared/scrolled... **/

	return ( _set_memlock && _clear_memlock );
}

static int _old_LINES;

int
StartMemlock()
{
	/** mark the current line as the "last" line of the portion to 
	    be memory locked (always relative to the top line of the
	    screen) Note that this will alter LINES so that it knows
	    the top is locked.  This means that (plus) the program 
	    will scroll nicely but (minus) End memlock MUST be called
	    whenever we leave the locked-memory part of the program! **/

	if (! _set_memlock)
	  return(-1);

	if (! _memory_locked) {

	  _old_LINES = LINES;
	  LINES -= _line;		/* we can't use this for scrolling */

	  tputs(_set_memlock, 1, outchar);
	  fflush(stdout);
	  _memory_locked = TRUE;
	}

	return(0);
}

int
EndMemlock()
{
	/** Clear the locked memory condition...  **/

	if (! _set_memlock)
	  return(-1);

	if (_memory_locked) {
	  LINES = _old_LINES;		/* back to old setting */
  
	  tputs(_clear_memlock, 1, outchar);
	  fflush(stdout);
	  _memory_locked = FALSE;
	}
	return(0);
}

#endif /* ndef ELM */

Writechar(ch)
register int ch;
{
	/** write a character to the current screen location. **/

	static int wrappedlastchar = 0;
	int justwrapped, nt;

	ch &= 0xFF;
	justwrapped = 0;

	/* if return, just go to left column. */
	if(ch == '\r') {
	  if (wrappedlastchar)
	    justwrapped = 1;                /* preserve wrap flag */
	  else {
	    putchar('\r');
	    _col = 0;
	  }
	}

	/* if newline and terminal just did a newline without our asking,
	 * do nothing, else output a newline and increment the line count */
	else if (ch == '\n') {
	  if (!wrappedlastchar) {
	    putchar('\n');
	    if (_line < LINES)
	      ++_line;
	  }
	}

	/* if backspace, move back  one space  if not already in column 0 */
	else if (ch == BACKSPACE) {
	    if (_col != 0) {
		tputs(_left, 1, outchar);
		_col--;
	    }
	    else if (_line > 0) {
		_col = COLUMNS - 1;
		_line--;
		moveabsolute (_col, _line);
	    }
	    /* else BACKSPACE does nothing */
	}

	/* if bell, ring the bell but don't advance the column */
	else if (ch == '\007') {
	  putchar(ch);
	}

	/* if a tab, output it */
	else if (ch == '\t') {
	  putchar(ch);
	  if((nt=next_tab(_col+1)) > prev_tab(COLUMNS))
	    _col = COLUMNS-1;
	  else
	    _col = nt-1;
	}

	else {
	  /* if some kind of non-printable character change to a '?' */
#ifdef ASCII_CTYPE
	  if(!isascii(ch) || !isprint(ch))
#else
	  if(!isprint(ch))
#endif
	    ch = '?';

	  /* if we only have one column left, simulate automargins if
	   * the terminal doesn't have them */
	  if (_col == COLUMNS - 1) {
	    putchar(ch);
	    if (!_automargin || _eatnewlineglitch) {
	      putchar('\r');
	      putchar('\n');
	    }
	    if (_line < LINES)
	      ++_line;
	    _col = 0;
	    justwrapped = 1;
	  }

	  /* if we are here this means we have no interference from the
	   * right margin - just output the character and increment the
	   * column position. */
	  else {
	    putchar(ch);
	    _col++;
	  }
	}

	wrappedlastchar = justwrapped;

	return(0);
}

/*VARARGS2*/

Write_to_screen(line, argcount, arg1, arg2, arg3)
char *line;
int   argcount; 
char *arg1, *arg2, *arg3;
{
	/** This routine writes to the screen at the current location.
  	    when done, it increments lines & columns accordingly by
	    looking for "\n" sequences... **/

	switch (argcount) {
	case 0 :
		PutLine0(_line, _col, line);
		break;
	case 1 :
		PutLine1(_line, _col, line, arg1);
		break;
	case 2 :
		PutLine2(_line, _col, line, arg1, arg2);
		break;
	case 3 :
		PutLine3(_line, _col, line, arg1, arg2, arg3);
		break;
	}
}

PutLine0(x, y, line)
int x,y;
register char *line;
{
	/** Write a zero argument line at location x,y **/

	MoveCursor(x,y);
	while(*line)
	  Writechar(*line++);
	fflush(stdout);
}

/*VARARGS2*/
PutLine1(x,y, line, arg1)
int x,y;
char *line;
char *arg1;
{
	/** write line at location x,y - one argument... **/

	char buffer[VERY_LONG_STRING];

	sprintf(buffer, line, arg1);

	PutLine0(x, y, buffer);
        fflush(stdout);
}

/*VARARGS2*/
PutLine2(x,y, line, arg1, arg2)
int x,y;
char *line;
char *arg1, *arg2;
{
	/** write line at location x,y - one argument... **/

	char buffer[VERY_LONG_STRING];

	MCsprintf(buffer, line, arg1, arg2);

	PutLine0(x, y, buffer);
        fflush(stdout);
}

/*VARARGS2*/
PutLine3(x,y, line, arg1, arg2, arg3)
int x,y;
char *line;
char *arg1, *arg2, *arg3;
{
	/** write line at location x,y - one argument... **/

	char buffer[VERY_LONG_STRING];

	MCsprintf(buffer, line, arg1, arg2, arg3);

	PutLine0(x, y, buffer);
        fflush(stdout);
}

CleartoEOLN()
{
	/** clear to end of line **/

	if (!_cleartoeoln)
		return(-1);

	tputs(_cleartoeoln, 1, outchar);
	fflush(stdout);  /* clear the output buffer */
	return(0);
}

CleartoEOS()
{
	/** clear to end of screen **/

	if (!_cleartoeos)
		return(-1);

	tputs(_cleartoeos, 1, outchar);
	fflush(stdout);  /* clear the output buffer */
	return(0);
}


RawState()
{
	/** returns either 1 or 0, for ON or OFF **/

	return( _inraw );
}

#ifdef ultrix
force_raw()
{
	_inraw = 0;
	Raw(ON);
}
#endif

Raw(state)
int state;
{
	int do_tite = (state & NO_TITE) == 0;

	state = state & ~NO_TITE;

	/** state is either ON or OFF, as indicated by call **/

	if (state == OFF && _inraw) {
	  if (use_tite && _end_termcap && do_tite) {
	    tputs(_end_termcap, 1, outchar);
	    fflush(stdout);
	  }
	  (void) ttsetattr(TTYIN,&_original_tty);
	  _inraw = 0;
	}
	else if (state == ON && ! _inraw) {

	  (void) ttgetattr(TTYIN, &_original_tty);
	  (void) ttgetattr(TTYIN, &_raw_tty);    /** again! **/

#if !defined(TERMIO) && !defined(TERMIOS)
	  _raw_tty.sg_flags &= ~(ECHO);	/* echo off */
	  _raw_tty.sg_flags |= CBREAK;	/* raw on    */
#else
	  _raw_tty.c_lflag &= ~(ICANON | ECHO);	/* noecho raw mode        */

	  _raw_tty.c_cc[VMIN] = '\01';	/* minimum # of chars to queue    */
	  _raw_tty.c_cc[VTIME] = '\0';	/* minimum time to wait for input */

#endif
	  (void) ttsetattr(TTYIN, &_raw_tty);
	  if (use_tite && _start_termcap && do_tite)
	    tputs(_start_termcap, 1, outchar);
	  _inraw = 1;
	}
}

int
ReadCh()
{
	/** read a character with Raw mode set! **/

	register int result;
	char ch;
	result = read(0, &ch, 1);
        return((result <= 0 ) ? EOF : ch);
}

outchar(c)
char c;
{
	/** output the given character.  From tputs... **/
	/** Note: this CANNOT be a macro!              **/

	putc(c, stdout);
}

