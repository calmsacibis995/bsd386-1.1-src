/*	SC	A Spreadsheet Calculator
 *		Curses based Screen driver
 *
 *		original by James Gosling, September 1982
 *		modifications by Mark Weiser and Bruce Israel,
 *			University of Maryland
 *
 *              More mods Robert Bond, 12/86
 *		More mods by Alan Silverstein, 3-4/88, see list of changes.
 *		Currently supported by sequent!sawmill!buhrt (Jeff Buhrt)
 *		$Revision: 1.1.1.1 $
 *
 */


#include <curses.h>
#include "sc.h"

#ifdef VMS
extern int VMS_read_raw;   /*sigh*/
    VMS_read_raw = 1;
#endif

#ifdef BROKENCURSES
		/* nl/nonl bug fix */
#undef nl
#undef nonl
#define nl()	 (_tty.sg_flags |= CRMOD,_pfast = _rawmode,stty(_tty_ch, &_tty))
#define nonl()	 (_tty.sg_flags &= ~CRMOD, _pfast = TRUE, stty(_tty_ch, &_tty))
#endif

void	repaint();

char	under_cursor = ' '; /* Data under the < cursor */
char	mode_ind = '.';
extern	char    revmsg[];

int	rows, lcols;
int	lastmx, lastmy;	/* Screen address of the cursor */
int	lastcol;	/* Spreadsheet Column the cursor was in last */
extern	int *fwidth;
extern	int showrange;	/* Causes ranges to be highlighted	*/
extern	int showneed;	/* Causes cells needing values to be highlighted */
extern	int showexpr;	/* Causes cell exprs to be displayed, highlighted */
#ifdef RIGHT_CBUG
extern	int wasforw;	/* Causes screen to be redisplay if on lastcol */
#endif

/*
 * update() does general screen update
 *
 * standout last time in update()?
 *	At this point we will let curses do work
 */
int	standlast	= FALSE;

void
update (anychanged)
int	anychanged;	/* did any cell really change in value? */
{
    register    row, col;
    register struct ent **pp;
    int	mxrow, mxcol;
    int	minsr = 0, minsc = 0, maxsr = 0, maxsc = 0;
    register r;
    register i;
    static	int	lastcurcol = -1, lastcurrow = -1;

    /*
     * place the cursor on the screen, set col, curcol, stcol, lastcol as
     * needed
     */
    if ((curcol != lastcurcol) || FullUpdate)
    {
	while (col_hidden[curcol])   /* You can't hide the last row or col */
		curcol++;

	/* First see if the last display still covers curcol */
	if (stcol <= curcol) { 
		for (i = stcol, lcols = 0, col = RESCOL;
			   (col + fwidth[i]) < COLS-1 && i < maxcols; i++) {
			lcols++;

			if (col_hidden[i])
				continue;
			col += fwidth[i];
		}
	}
	while (stcol + lcols - 1 < curcol || curcol < stcol) {
		FullUpdate++;
		if (stcol - 1 == curcol) {    /* How about back one? */
			stcol--;
		} else if (stcol + lcols == curcol) {   /* Forward one? */
			stcol++;
		} else {
			/* Try to put the cursor in the center of the screen */
			col = (COLS - RESCOL - fwidth[curcol]) / 2 + RESCOL; 
			stcol = curcol;
			for (i=curcol-1; i >= 0 && col-fwidth[i] > RESCOL; i--)
			{	stcol--;
				if (col_hidden[i])
					continue;
				col -= fwidth[i];
			}
		}
		/* Now pick up the counts again */
		for (i = stcol, lcols = 0,col = RESCOL;
			(col + fwidth[i]) < COLS-1 && i < maxcols; i++) {
			lcols++;
			if (col_hidden[i])
				continue;
			col += fwidth[i];
		}
	}
	lastcurcol = curcol;
    }

    /* Now - same process on the rows as the columns */
    if ((currow != lastcurrow) || FullUpdate)
    {
	while (row_hidden[currow])   /* You can't hide the last row or col */
		currow++;
	if (strow <= currow) { 
		for (i = strow, rows = 0, row=RESROW; row<LINES && i<maxrows; i++)
		{	rows++;
			if (row_hidden[i])
				continue;
			row++;
		}
	}

	while (strow + rows - 1 < currow || currow < strow) {
		FullUpdate++;
		if (strow - 1 == currow) {    /* How about up one? */
			strow--;
		} else if (strow + rows == currow) {   /* Down one? */
			strow++;
		} else {
			/* Try to put the cursor in the center of the screen */
			row = (LINES - RESROW) / 2 + RESROW; 
			strow = currow;
			for (i=currow-1; i >= 0 && row-1 > RESROW; i--) {
				strow--;
				if (row_hidden[i])
					continue;
				row--;
			}
		}
		/* Now pick up the counts again */
		for (i = strow, rows = 0, row=RESROW; row<LINES && i<maxrows; i++) {
			rows++;
			if (row_hidden[i])
				continue;
			row++;
		}
	}
	lastcurrow = currow;
    }
    mxcol = stcol + lcols - 1;
    mxrow = strow + rows - 1;

    /* Get rid of cursor standout on the cell at previous cursor position */
    if (!FullUpdate)
    {	if (showcell)
		repaint(lastmx, lastmy, fwidth[lastcol]);

	(void) move(lastmy, lastmx+fwidth[lastcol]);

	if ((inch() & A_CHARTEXT ) == '<')
		(void) addch(under_cursor);
    }

    /* where is the the cursor now? */
    lastmy =  RESROW;
    for (row = strow; row < currow; row++)
	if (!row_hidden[row])
		lastmy++;

    lastmx = RESCOL;
    for (col = stcol; col < curcol; col++)
	if (!col_hidden[col])
		lastmx += fwidth[col];
    lastcol = curcol;

    if (FullUpdate || standlast) {
	(void) move(2, 0);
	(void) clrtobot();
	(void) standout();

	for (row=RESROW, i=strow; i <= mxrow; i++) {
	    if (row_hidden[i]) 
		continue;
	    (void) move(row,0);
	    if (maxrows < 1000)
		(void) printw("%-*d", RESCOL-1, i);
	    else
		(void) printw("%-*d", RESCOL, i);
	    row++;
	}
#ifdef RIGHT_CBUG
	if(wasforw) {
		clearok(stdscr, TRUE);
		wasforw = 0;
	}
#endif
	(void) move(2,0);
	(void) printw("%*s", RESCOL, " ");

	for (col=RESCOL, i = stcol; i <= mxcol; i++) {
	    register int k;
	    if (col_hidden[i])
		continue;
	    (void) move(2, col);
	    k = fwidth[i]/2;
	    if (k == 0)
		(void) printw("%1s", coltoa(i));
	    else
	        (void) printw("%*s%-*s", k, " ", fwidth[i]-k, coltoa(i));
	    col += fwidth[i];
	}
	(void) standend();
    }

    if (showrange) {
	minsr = showsr < currow ? showsr : currow;
	minsc = showsc < curcol ? showsc : curcol;
	maxsr = showsr > currow ? showsr : currow;
	maxsc = showsc > curcol ? showsc : curcol;

	if (showtop) {
	    (void) move(1,0);
	    (void) clrtoeol();
	    (void) printw("Default range:  %s",
			    r_name(minsr, minsc, maxsr, maxsc));
	}
    }


    /* Repaint the visible screen */
    if (showrange || anychanged || FullUpdate || standlast)
    {
	/* may be reset in loop, if not next time we will do a FullUpdate */
      if (standlast)
      {	FullUpdate = TRUE;
	standlast = FALSE;
      }

      for (row = strow, r = RESROW; row <= mxrow; row++) {
	register c = RESCOL;
	int do_stand = 0;
	int fieldlen;
	int nextcol;

	if (row_hidden[row])
	    continue;
	for (pp = ATBL(tbl, row, col = stcol); col <= mxcol;
	         pp += nextcol - col,  col = nextcol, c += fieldlen) {

	    nextcol = col+1;
	    if (col_hidden[col]) {
		fieldlen = 0;
		continue;
	    }

	    fieldlen = fwidth[col];

	    /*
	     * Set standout if:
	     *
	     * - showing ranges, and not showing cells which need to be filled
	     *	 in, and not showing cell expressions, and in a range, OR
	     *
	     * - if showing cells which need to be filled in and this one is
	     *	 of that type (has a value and doesn't have an expression,
	     *	 or it is a string expression), OR
	     *
	     * - if showing cells which have expressions and this one does.
	     */
	    if ((showrange && (! showneed) && (! showexpr)
			   && (row >= minsr) && (row <= maxsr)
			   && (col >= minsc) && (col <= maxsc))
		    || (showneed && (*pp) && ((*pp) -> flags & is_valid) &&
			  (((*pp) -> flags & is_strexpr) || !((*pp) -> expr)))
		    || (showexpr && (*pp) && ((*pp) -> expr)))
	    {
		(void) move(r, c);
		(void) standout();
		standlast++;
		if (!*pp)	/* no cell, but standing out */
		{	(void) printw("%*s", fwidth[col], " ");
			(void) standend();
			continue;
		}
		else
			do_stand = 1;
	    }
	    else
		do_stand = 0;

	    if ((*pp) && (((*pp) -> flags & is_changed || FullUpdate) || do_stand)) {
		if (do_stand) {
		    (*pp) -> flags |= is_changed; 
		} else {
		    (void) move(r, c);
		    (*pp) -> flags &= ~is_changed;
		}

		/*
		 * Show expression; takes priority over other displays:
		 */

		if ((*pp)->cellerror)
			(void) printw("%*.*s", fwidth[col], fwidth[col],
			  (*pp)->cellerror == CELLERROR ? "ERROR" : "INVALID");
		else
		if (showexpr && ((*pp) -> expr)) {
		    linelim = 0;
		    editexp(row, col);		/* set line to expr */
		    linelim = -1;
		    showstring(line, /* leftflush = */ 1, /* hasvalue = */ 0,
				row, col, & nextcol, mxcol, & fieldlen, r, c);
		} else {
		    /*
		     * Show cell's numeric value:
                     */

		    if ((*pp) -> flags & is_valid) {
			char field[FBUFLEN];

			if ((*pp) -> format) {
				(void) format((*pp) -> format, (*pp) -> v,
					     field, sizeof(field));
			} else {
				(void) engformat(realfmt[col], fwidth[col], 
                                             precision[col], (*pp) -> v, 
                                             field, sizeof(field));
			}
			if (strlen(field) > fwidth[col]) {
				for(i = 0; i<fwidth[col]; i++)
					(void)addch('*');
			} else {
				for(i = 0; i < fwidth[col] - strlen(field);i++)
					(void)addch(' ');
				(void)addstr(field);
			}
		    }

		    /*
		     * Show cell's label string:
		     */

		    if ((*pp) -> label) {
			showstring((*pp) -> label,
				    (*pp) -> flags & (is_leftflush|is_label),
				    (*pp) -> flags & is_valid,
				    row, col, & nextcol, mxcol,
				    & fieldlen, r, c);
		    }
		    else	/* repaint a blank cell: */
		    if ((do_stand || !FullUpdate) &&
				((*pp)->flags & is_changed) &&
				!((*pp)->flags & is_valid) && !(*pp)->label) {
			(void) printw("%*s", fwidth[col], " ");
		    }
		} /* else */

		if (do_stand) {
		    (void) standend();
		    do_stand = 0;
		}
	    }
	}
	r++;
      }
    }

    /* place 'cursor marker' */
    if (showcell && (! showneed) && (! showexpr)) {
	(void) move(lastmy, lastmx);
        (void) standout();
        repaint(lastmx, lastmy, fwidth[lastcol]);
        (void) standend();
    }
    (void) move(lastmy, lastmx+fwidth[lastcol]);
    under_cursor = (inch() & A_CHARTEXT);
    (void) addch('<');

    (void) move(0, 0);
    (void) clrtoeol();
    if (linelim >= 0) {
	(void) addch(mode_ind);
	(void) addstr("> ");
	(void) addstr(line);
	(void) move((linelim + 3) / COLS, (linelim+3) % COLS);
    } else {
	if (showtop) {			/* show top line */
	    register struct ent *p1;

	    int printed = 0;		/* printed something? */

	    /* show the current cell's format */
	    (void) printw("%s%d ", coltoa(curcol), currow);
	    if ((p1 = *ATBL(tbl, currow, curcol)) && p1->format)
		printw("(%s) ", p1->format);
	    else
		printw("(%d %d %d) ", fwidth[curcol], precision[curcol],
				realfmt[curcol]);

	    if (p1) {
		if (p1 -> expr) {
		    /* has expr of some type */
		    linelim = 0;
		    editexp(currow, curcol);	/* set line to expr */
		    linelim = -1;
		}

		/*
		 * Display string part of cell:
		 */

		if ((p1 -> expr) && (p1 -> flags & is_strexpr)) {
 		    if( (p1-> flags & is_label) )
			(void) addstr("|{");
		    else
			(void) addstr((p1 -> flags & is_leftflush) ? "<{" : ">{");
		    (void) addstr(line);
		    (void) addstr("} ");	/* and this '}' is for vi % */
		    printed = 1;

		} else if (p1 -> label) {
		    /* has constant label only */
		    if( (p1-> flags & is_label) )
			(void) addstr("|\"");
		    else
			(void) addstr ((p1 -> flags & is_leftflush) ? "<\"" : ">\"");
		    (void) addstr (p1 -> label);
		    (void) addstr ("\" ");
		    printed = 1;
		}

		/*
		 * Display value part of cell:
		 */

		if (p1 -> flags & is_valid) {
		    /* has value or num expr */
		    if ((! (p1 -> expr)) || (p1 -> flags & is_strexpr))
			(void) sprintf (line, "%.15g", p1 -> v);

		    (void) addch ('[');
		    (void) addstr (line);
		    (void) addch (']');
		    *line = '\0'; /* this is the input buffer ! */
		    printed = 1;
		}
	    }
	    if (! printed)
		(void) addstr ("[]");
	    /* Display if cell is locked */
	    if (p1 && p1->flags&is_locked)
		(void) addstr(" locked");
	}
	(void) move(lastmy, lastmx+fwidth[lastcol]);
    }

    if (revmsg[0]) {
	(void) move(0, 0);
	(void) clrtoeol ();	/* get rid of topline display */
	(void) printw(revmsg);
	*revmsg = '\0';		/* don't show it again */
	(void) move (lastmy, lastmx + fwidth[lastcol]);
    }

    FullUpdate = FALSE;
}

/* redraw what is under the cursor from curses' idea of the screen */
void
repaint(x, y, len)
int x, y, len;
{
    int c;

    while(len-- > 0) {
	(void) move(y, x);
	c = inch() & A_CHARTEXT;
	(void) addch(c);
	x++;
    }
}

int seenerr;

/* error routine for yacc (gram.y) */
void
yyerror(err)
char *err; {
    if (seenerr) return;
    seenerr++;
    (void) move(1,0);
    (void) clrtoeol();
    (void) printw("%s: %.*s<=%s",err,linelim,line,line+linelim);
}

#ifdef XENIX2_3
struct termio tmio;
#endif

void
startdisp()
{
#if sun
    int	 fd;
    fd = dup(0);
#endif
#ifdef TIOCGSIZE
    {	struct ttysize size;
	if (ioctl(0, TIOCGSIZE, &size) == 0)
	{ 
		LINES = size.ts_lines;
		COLS = size.ts_cols;
	}
    }
#endif

#ifdef XENIX2_3
    (void) ioctl (fileno (stdin), TCGETA, & tmio);
#endif
    (void) initscr();
#if sun
    close(0);
    dup(fd);
    close(fd);
#endif
    (void) clear();
#ifdef VMS
    VMS_read_raw = 1;
#else
    nonl();
    noecho ();
    cbreak();
#endif
    initkbd();
    scrollok(stdscr, 1);

#if defined(SYSV3) && !defined(NOIDLOK)
# ifndef IDLOKBAD
    /*
     * turn hardware insert/delete on, if possible.
     * turn on scrolling for systems with SYSVr3.{1,2} (SYSVr3.0 has this set
     * as the default)
     */
     idlok(stdscr,TRUE);
# else	/*
	 * This seems to fix (with an empty spreadsheet):
	 *	a) Redrawing the bottom half of the screen when you
	 *		move between row 9 <-> 10
	 *	b) the highlighted row labels being trash when you
	 *		move between row 9 <-> 10
	 *	c) On an xterm on Esix Rev. D+ from eating lines
	 *	 -goto (or move) a few lines (or more) past the bottom
	 *	 of the screen, goto (or move) to the top line on the
	 *	 screen, move upward and the current line is deleted, the
	 *	 others move up even when they should not, check by
	 *	 noticing the rows become 2, 3, 40, 41, 42... (etc).
	 */
     idlok(stdscr,FALSE);
# endif
#endif

    FullUpdate++;
}

void
stopdisp()
{
    deraw();
    resetkbd();
    endwin();
#ifdef XENIX2_3
    (void) ioctl (fileno (stdin), TCSETAW, & tmio);
#endif
}

/* init curses */
#ifdef VMS

goraw()
{
    VMS_read_raw = 1;
    FullUpdate++;
}

deraw()
{
    (void) move (LINES - 1, 0);
    (void) clrtoeol();
    (void) refresh();
    VMS_read_raw = 0;
}

#else /* VMS */
void
goraw()
{
#if SYSV2 || SYSV3
    fixterm();
#else /* SYSV2 || SYSV3 */
    cbreak();
    nonl();
    noecho ();
#endif /* SYSV2 || SYSV3 */
    kbd_again();
    (void) clear();
    FullUpdate++;
}

/* clean up curses */
void
deraw()
{
    (void) move (LINES - 1, 0);
    (void) clrtoeol();
    (void) refresh();
#if SYSV2 || SYSV3
    resetterm();
#else
    nocbreak();
    nl();
    echo();
#endif
    resetkbd();
}

#endif /* VMS */
