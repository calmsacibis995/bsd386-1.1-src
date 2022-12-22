/*	SC	A Spreadsheet Calculator
 *		Command routines
 *
 *		original by James Gosling, September 1982
 *		modifications by Mark Weiser and Bruce Israel,
 *			University of Maryland
 *
 *              More mods Robert Bond, 12/86
 *
 *		$Revision: 1.1.1.1 $
 */

#include <sys/types.h>
#if defined(BSD42) || defined(BSD43)
#include <strings.h>
#else
#ifndef SYSIII
#include <string.h>
#endif
#endif

#include <curses.h>
#if defined(BSD42) || defined(BSD43) || defined(VMS)
#include <sys/file.h>
#else
#include <fcntl.h>
#endif
#include "sc.h"
#include <signal.h>
#include <errno.h>

#ifdef SYSV3
extern void exit();
#else
extern int exit();
#endif

void	openrow();
void	syncref();
void	unspecial();

extern	int	errno;

/* a linked list of free [struct ent]'s, uses .next as the pointer */
extern	struct ent *freeents;

/* a linked list of free [struct enodes]'s, uses .e.o.left as the pointer */
extern	struct enode *freeenodes;

#define DEFCOLDELIM ':'

/* copy the current row (currow) and place the cursor in the new row */
void
duprow()
{
    if (currow >= maxrows - 1 || maxrow >= maxrows - 1) {
	if (!growtbl(GROWROW, 0, 0))
		return;
    }
    modflg++;
    currow++;
    openrow (currow);
    for (curcol = 0; curcol <= maxcol; curcol++) {
	register struct ent *p = *ATBL(tbl, currow - 1, curcol);
	if (p) {
	    register struct ent *n;
	    n = lookat (currow, curcol);
	    (void)copyent ( n, p, 1, 0);
	}
    }
    for (curcol = 0; curcol <= maxcol; curcol++) {
	register struct ent *p = *ATBL(tbl, currow, curcol);
	if (p && (p -> flags & is_valid) && !p -> expr)
	    break;
    }
    if (curcol > maxcol)
	curcol = 0;
}

/* copy the current column (curcol) and place the cursor in the new column */
void
dupcol() 
{
    if (curcol >= maxcols - 1 || maxcol >= maxcols - 1) {
	if (!growtbl(GROWCOL, 0, 0))
		return;
    }
    modflg++;
    curcol++;
    opencol (curcol, 1);
    for (currow = 0; currow <= maxrow; currow++) {
	register struct ent *p = *ATBL(tbl, currow, curcol - 1);
	if (p) {
	    register struct ent *n;
	    n = lookat (currow, curcol);
	    copyent ( n, p, 0, 1);
	}
    }
    for (currow = 0; currow <= maxrow; currow++) {
	register struct ent *p = *ATBL(tbl, currow, curcol);
	if (p && (p -> flags & is_valid) && !p -> expr)
	    break;
    }
    if (currow > maxrow)
	currow = 0;
}

/* insert 'arg' rows before currow */
void
insertrow(arg)
register int arg;
{
    while (--arg>=0) openrow (currow);
}

/* delete 'arg' rows starting at currow (deletes from currow downward) */
void
deleterow(arg)
register int arg;
{
    if (any_locked_cells(currow, 0, currow + arg - 1, maxcol))
	error("Locked cells encountered. Nothing changed");
    else {
    	flush_saved();
    	erase_area(currow, 0, currow + arg - 1, maxcol);
    	currow += arg;
    	while (--arg>=0) closerow (--currow);
    	sync_refs();
    }
}

void
erase_area(sr, sc, er, ec)
int sr, sc, er, ec;
{
    register int r, c;
    register struct ent **pp;

    if (sr > er) {
	r = sr; sr = er; er= r;	
    }

    if (sc > ec) {
	c = sc; sc = ec; ec= c;	
    }

    if (sr < 0)
	sr = 0; 
    if (sc < 0)
	sc = 0;
    checkbounds(&er, &ec);

    for (r = sr; r <= er; r++) {
	for (c = sc; c <= ec; c++) {
	    pp = ATBL(tbl, r, c);
	    if (*pp && !((*pp)->flags&is_locked)) {
		free_ent(*pp);
		*pp = (struct ent *)0;
	    }
	}
    }
}

/*
 * deletes the expression associated w/ a cell and turns it into a constant
 * containing whatever was on the screen
 */
void
valueize_area(sr, sc, er, ec)
int sr, sc, er, ec;
{
    register int r, c;
    register struct ent *p;

    if (sr > er) {
	r = sr; sr = er; er= r;	
    }

    if (sc > ec) {
	c = sc; sc = ec; ec= c;	
    }

    if (sr < 0)
	sr = 0; 
    if (sc < 0)
	sc = 0;
    checkbounds(&er, &ec);

    for (r = sr; r <= er; r++) {
	for (c = sc; c <= ec; c++) {
	    p = *ATBL(tbl, r, c);
	    if (p && p->flags&is_locked) {
		error(" Cell %s%d is locked", coltoa(c), r);
		continue;
	    }
	    if (p && p->expr) {
		efree(p->expr);
		p->expr = (struct enode *)0;
		p->flags &= ~is_strexpr;
	    }
	}
    }

}

void
pullcells(to_insert)
int to_insert;
{
    register struct ent *p, *n;
    register int deltar, deltac;
    int minrow, mincol;
    int mxrow, mxcol;
    int numrows, numcols;

    if (! to_fix)
    {
	error ("No data to pull");
	return;
    }

    minrow = maxrows; 
    mincol = maxcols;
    mxrow = 0;
    mxcol = 0;

    for (p = to_fix; p; p = p->next) {
	if (p->row < minrow)
	    minrow = p->row;
	if (p->row > mxrow)
	    mxrow = p->row;
	if (p->col < mincol)
	    mincol = p->col;
	if (p->col > mxcol)
	    mxcol = p->col;
    }

    numrows = mxrow - minrow + 1;
    numcols = mxcol - mincol + 1;
    deltar = currow - minrow;
    deltac = curcol - mincol;

    if (to_insert == 'r') {
	insertrow(numrows);
	deltac = 0;
    } else if (to_insert == 'c') {
	opencol(curcol, numcols);
	deltar = 0;
    }

    FullUpdate++;
    modflg++;

    for (p = to_fix; p; p = p->next) {
	n = lookat (p->row + deltar, p->col + deltac);
	(void) clearent(n);
	copyent( n, p, deltar, deltac);
	n -> flags = p -> flags & ~is_deleted;
    }
}

void
colshow_op()
{
    register int i,j;
    for (i=0; i<maxcols; i++)
	if (col_hidden[i]) 
	    break;
    for(j=i; j<maxcols; j++)
	if (!col_hidden[j])
	    break;
    j--;
    if (i>=maxcols)
	error ("No hidden columns to show");
    else {
	(void) sprintf(line,"show %s:", coltoa(i));
	(void) sprintf(line + strlen(line),"%s",coltoa(j));
	linelim = strlen (line);
    }
}

void
rowshow_op()
{
    register int i,j;
    for (i=0; i<maxrows; i++)
	if (row_hidden[i]) 
	    break;
    for(j=i; j<maxrows; j++)
	if (!row_hidden[j]) {
	    break;
	}
    j--;

    if (i>=maxrows)
	error ("No hidden rows to show");
    else {
	(void) sprintf(line,"show %d:%d", i, j);
        linelim = strlen (line);
    }
}

/*
 * Given a row/column command letter, emit a small menu, then read a qualifier
 * character for a row/column command and convert it to 'r' (row), 'c'
 * (column), or 0 (unknown).  If ch is 'p', an extra qualifier 'm' is allowed.
 */

int
get_rcqual (ch)
    int ch;
{
    error ("%sow/column:  r: row  c: column%s",

	    (ch == 'i') ? "Insert r" :
	    (ch == 'a') ? "Append r" :
	    (ch == 'd') ? "Delete r" :
	    (ch == 'p') ? "Pull r" :
	    (ch == 'v') ? "Values r" :
	    (ch == 'z') ? "Zap r" :
	    (ch == 's') ? "Show r" : "R",

	    (ch == 'p') ? "  m: merge" : "");

    (void) refresh();

    switch (nmgetch())
    {
	case 'r':
	case 'l':
	case 'h':
	case ctl('f'):
	case ctl('b'):	return ('r');

	case 'c':
	case 'j':
	case 'k':
	case ctl('p'):
	case ctl('n'):	return ('c');

	case 'm':	return ((ch == 'p') ? 'm' : 0);

	case ESC:
	case ctl('g'):	return (ESC);

	default:	return (0);
    }
    /*NOTREACHED*/
}

void
openrow (rs)
int	rs;
{
    register	r, c;
    struct ent	**tmprow, **pp;

    if (rs > maxrow) maxrow = rs;
    if (maxrow >= maxrows - 1 || rs > maxrows - 1) {
	if (!growtbl(GROWROW, rs, 0))
		return;
    }
	/*
	 * save the last active row+1, shift the rows downward, put the last
	 * row in place of the first
	 */
    tmprow = tbl[++maxrow];
    for (r = maxrow; r > rs; r--) {
	row_hidden[r] = row_hidden[r-1];
	tbl[r] = tbl[r-1];
	pp = ATBL(tbl, r, 0);
	for (c = 0; c < maxcols; c++, pp++)
		if (*pp)
			(*pp)->row = r;
    }
    tbl[r] = tmprow;	/* the last row was never used.... */
    FullUpdate++;
    modflg++;
}

/* delete row r */
void
closerow (r)
register r;
{
    register struct ent **pp;
    register c;
    struct ent	**tmprow;

    if (r > maxrow) return;

    /* save the row and empty it out */
    tmprow = tbl[r];
    pp = ATBL(tbl, r, 0);
    for (c=maxcol+1; --c>=0; pp++) {
	if (*pp)
	{	free_ent(*pp);
		*pp = (struct ent *)0;
	}
    }

    /* move the rows, put the deleted, but now empty, row at the end */
    for (; r < maxrows - 1; r++) {
	row_hidden[r] = row_hidden[r+1];
	tbl[r] = tbl[r+1];
	pp = ATBL(tbl, r, 0);
	for (c = 0; c < maxcols; c++, pp++)
		if (*pp)
			(*pp)->row = r;
    }
    tbl[r] = tmprow;

    maxrow--;
    FullUpdate++;
    modflg++;
}

void
opencol (cs, numcol)
int	cs;
int	numcol;
{
    register r;
    register struct ent **pp;
    register c;
    register lim = maxcol-cs+1;
    int i;

    if (cs > maxcol)
	maxcol = cs;
    maxcol += numcol;

    if ((maxcol >= maxcols - 1) && !growtbl(GROWCOL, 0, maxcol))
		return;

    for (i = maxcol; i > cs; i--) {
	fwidth[i] = fwidth[i-numcol];
	precision[i] = precision[i-numcol];
	realfmt[i] = realfmt[i-numcol];
	col_hidden[i] = col_hidden[i-numcol];
    }
    for (c = cs; c - cs < numcol; c++)
    {	fwidth[c] = DEFWIDTH;
	precision[c] =  DEFPREC;
	realfmt[c] = DEFREFMT;
    }
	
    for (r=0; r<=maxrow; r++) {
	pp = ATBL(tbl, r, maxcol);
	for (c=lim; --c>=0; pp--)
	    if (pp[0] = pp[-numcol])
		pp[0]->col += numcol;

	pp = ATBL(tbl, r, cs);
	for (c = cs; c - cs < numcol; c++, pp++)
		*pp = (struct ent *)0;
    }
    FullUpdate++;
    modflg++;
}

/* delete group of columns (1 or more) */
void
closecol (cs, numcol)
int cs;
int	numcol;
{
    register r;
    register struct ent **pp;
    register struct ent *q;
    register c;
    register lim = maxcol-cs;
    int i;
    char buf[50];

    if (lim - numcol < -1)
    {	(void) sprintf(buf, "Can't delete %d column%s %d columns left", numcol,
			(numcol > 1 ? "s," : ","), lim+1);
	error(buf);
	return;
    }
    if (any_locked_cells(0, curcol, maxrow, curcol + numcol - 1)) {
	error("Locked cells encountered. Nothing changed");
	return;
    }
    flush_saved();
    erase_area(0, curcol, maxrow, curcol + numcol - 1);
    sync_refs();

    /* clear then copy the block left */
    lim = maxcols - numcol - 1;
    for (r=0; r<=maxrow; r++) {
	for (c = cs; c - cs < numcol; c++)
		if (q = *ATBL(tbl, r, c))
			free_ent(q);

	pp = ATBL(tbl, r, cs);
	for (c=cs; c <= lim; c++, pp++)
	{   if (c > lim)
		*pp = (struct ent *)0;
	    else
	    if (pp[0] = pp[numcol])
		pp[0]->col -= numcol;
	}

	c = numcol;
	for (; --c >= 0; pp++)		
		*pp = (struct ent *)0;
    }

    for (i = cs; i < maxcols - numcol - 1; i++) {
	fwidth[i] = fwidth[i+numcol];
	precision[i] = precision[i+numcol];
	realfmt[i] = realfmt[i+numcol];
	col_hidden[i] = col_hidden[i+numcol];
    }
    for (; i < maxcols - 1; i++) {
	fwidth[i] = DEFWIDTH;
	precision[i] = DEFPREC;
	realfmt[i] = DEFREFMT;
	col_hidden[i] = FALSE;
    }

    maxcol -= numcol;
    FullUpdate++;
    modflg++;
}

void
doend(rowinc, colinc)
int rowinc, colinc;
{
    register struct ent *p;
    int r, c;

    if (VALID_CELL(p, currow, curcol)) {
	r = currow + rowinc;
	c = curcol + colinc;
	if (r >= 0 && r < maxrows && 
	    c >= 0 && c < maxcols &&
	    !VALID_CELL(p, r, c)) {
		currow = r;
		curcol = c;
	}
    }

    if (!VALID_CELL(p, currow, curcol)) {
        switch (rowinc) {
        case -1:
	    while (!VALID_CELL(p, currow, curcol) && currow > 0)
		currow--;
	    break;
        case  1:
	    while (!VALID_CELL(p, currow, curcol) && currow < maxrows-1)
		currow++;
	    break;
        case  0:
            switch (colinc) {
 	    case -1:
	        while (!VALID_CELL(p, currow, curcol) && curcol > 0)
		    curcol--;
	        break;
 	    case  1:
	        while (!VALID_CELL(p, currow, curcol) && curcol < maxcols-1)
		    curcol++;
	        break;
	    }
            break;
        }

	error ("");	/* clear line */
	return;
    }

    switch (rowinc) {
    case -1:
	while (VALID_CELL(p, currow, curcol) && currow > 0)
	    currow--;
	break;
    case  1:
	while (VALID_CELL(p, currow, curcol) && currow < maxrows-1)
	    currow++;
	break;
    case  0:
	switch (colinc) {
	case -1:
	    while (VALID_CELL(p, currow, curcol) && curcol > 0)
		curcol--;
	    break;
	case  1:
	    while (VALID_CELL(p, currow, curcol) && curcol < maxcols-1)
		curcol++;
	    break;
	}
	break;
    }
    if (!VALID_CELL(p, currow, curcol)) {
        currow -= rowinc;
        curcol -= colinc;
    }
}

/* Modified 9/17/90 THA to handle more formats */
void
doformat(c1,c2,w,p,r)
int c1,c2,w,p,r;
{
    register int i;
    int crows = 0;
    int ccols = c2;

    if (c1 >= maxcols && !growtbl(GROWCOL, 0, c1)) c1 = maxcols-1 ;
    if (c2 >= maxcols && !growtbl(GROWCOL, 0, c2)) c2 = maxcols-1 ;

    if (w > COLS - RESCOL - 2) {
	error("Format too large - Maximum = %d", COLS - RESCOL - 2);
	w = COLS-RESCOL-2;
    }

    if (p > w) {
	error("Precision too large");
	p = w;
    }

    checkbounds(&crows, &ccols);
    if (ccols < c2) {
	error("Format statement failed to create implied column %d", c2);
	return;
    }

    for(i = c1; i<=c2; i++)
		fwidth[i] = w, precision[i] = p, realfmt[i] = r;

    FullUpdate++;
    modflg++;
}

void
print_options(f)
FILE *f;
{
    if(
       autocalc &&
       propagation == 10 &&
       calc_order == BYROWS &&
       !numeric &&
       prescale == 1.0 &&
       !extfunc &&
       showcell &&
       showtop &&
       tbl_style == 0 &&
       craction == 0 &&
       rowlimit == -1 &&
       collimit == -1
      )
		return;		/* No reason to do this */

    (void) fprintf(f, "set");
    if(!autocalc) 
	(void) fprintf(f," !autocalc");
    if(propagation != 10)
	(void) fprintf(f, " iterations = %d", propagation);
    if(calc_order != BYROWS )
	(void) fprintf(f, " bycols");
    if (numeric)
	(void) fprintf(f, " numeric");
    if (prescale != 1.0)
	(void) fprintf(f, " prescale");
    if (extfunc)
	(void) fprintf(f, " extfun");
    if (!showcell)
	(void) fprintf(f, " !cellcur");
    if (!showtop)
	(void) fprintf(f, " !toprow");
    if (tbl_style)
	(void) fprintf(f, " tblstyle = %s", tbl_style == TBL ? "tbl" :
					tbl_style == LATEX ? "latex" :
					tbl_style == SLATEX ? "slatex" :
					tbl_style == TEX ? "tex" :
					tbl_style == FRAME ? "frame" : "0" );
    if (craction)
	(void) fprintf(f, " craction = %d", craction);
    if (rowlimit >= 0)
	(void) fprintf(f, " rowlimit = %d", rowlimit);
    if (collimit >= 0)
	(void) fprintf(f, " collimit = %d", collimit);
    (void) fprintf(f, "\n");
}

void
printfile (fname, r0, c0, rn, cn)
char *fname;
int r0, c0, rn, cn;
{
    FILE *f;
    static char *pline = NULL;		/* only malloc once, malloc is slow */
    static unsigned fbufs_allocated = 0;
    int plinelim;
    int pid;
    int fieldlen, nextcol;
    register row, col;
    register struct ent **pp;
    char file[32];
    char path[256];
    char *tpp;

    /* printfile will be the [path/]file ---> [path/]file.out,
     * file is limited to 14 characters.
     */
    if (*fname == '\0') {
	strcpy(path, curfile);

	if ((tpp = strrchr(path, '/'))  == NULL)
		tpp = path;
	else
		tpp++;
	strcpy(file, tpp);
	file[10] = '\0';
	sprintf(tpp, "%s.asc", file);
	fname = path;
    }

    if ((strcmp(fname, curfile) == 0) &&
	!yn_ask("Confirm that you want to destroy the data base: (y,n)")) {
	return;
    }

    if (!pline && (pline = scxmalloc((unsigned)(FBUFLEN *
					++fbufs_allocated))) == (char *)NULL)
    {   error("Malloc failed in printfile()");
        return;
    }

    if ((f = openout(fname, &pid)) == (FILE *)0)
    {	error ("Can't create file \"%s\"", fname);
	return;
    }
    for (row=r0;row<=rn; row++) {
	register c = 0;

	if (row_hidden[row])
	    continue;

	pline[plinelim=0] = '\0';
	for (pp = ATBL(tbl, row, col=c0); col<=cn;
	        pp += nextcol-col, col = nextcol, c += fieldlen) {

	    nextcol = col+1;
	    if (col_hidden[col]) {
		fieldlen = 0;
		continue;
	    }

	    fieldlen = fwidth[col];
	    if (*pp) {
		char *s;

		/* 
		 * dynamically allocate pline, making sure we are not 
		 * attempting to write 'out of bounds'.
		 */
		while(c > (fbufs_allocated * FBUFLEN)) {
		  if((pline = scxrealloc
			       ((char *)pline, 
				(unsigned)(FBUFLEN * ++fbufs_allocated)))
		     == NULL) {
		    error ("Realloc failed in printfile()");
		    return;
		  }
		}		  
		while (plinelim<c) pline[plinelim++] = ' ';
		plinelim = c;
		if ((*pp)->flags&is_valid) {
		    while(plinelim + fwidth[col] > 
			  (fbufs_allocated * FBUFLEN)) {
		      if((pline = ((char *)scxrealloc
				   ((char *)pline, 
				    (unsigned)(FBUFLEN * ++fbufs_allocated))))
			 == NULL) {
			error ("Realloc failed in printfile()");
			return;
		      }
		    }
		    if ((*pp)->cellerror)
			(void) sprintf (pline+plinelim, "%*s",
				fwidth[col],
			((*pp)->cellerror == CELLERROR ? "ERROR" : "INVALID"));
		    else
		    {
		      if ((*pp)->format) {
	   	        char field[FBUFLEN];
			format ((*pp)->format, (*pp)->v, field,
				       sizeof(field));
			(void) sprintf (pline+plinelim, "%*s", fwidth[col],
					field);
		      } else {
	   	        char field[FBUFLEN];
			(void) engformat(realfmt[col], fwidth[col],
                                             precision[col], (*pp) -> v,
                                             field, sizeof(field));
			(void) sprintf (pline+plinelim, "%*s", fwidth[col],
				       field);
		      }
		    }
		    plinelim += strlen (pline+plinelim);
		}
		if (s = (*pp)->label) {
		    int slen;
		    char *start, *last;
		    register char *fp;
		    struct ent *nc;

		    /*
		     * Figure out if the label slops over to a blank field. A
		     * string started with backslah is defining repition char
		     */
		    slen = strlen(s);
		    if ( *s == '\\' && *(s+1)!= '\0' )
			slen = fwidth[col];
		    while (slen > fieldlen && nextcol <= cn &&
			    !((nc = lookat(row,nextcol))->flags & is_valid) &&
			    !(nc->label)) {
			
	                if (!col_hidden[nextcol])
		 	    fieldlen += fwidth[nextcol];

			nextcol++;
		    }
		    if (slen > fieldlen)
			slen = fieldlen;
		    
		    while(c + fieldlen + 2 > (fbufs_allocated * FBUFLEN)) {
		      if((pline = ((char *)scxrealloc
				   ((char *)pline, 
				    (unsigned)(FBUFLEN * ++fbufs_allocated))))
			 == NULL) {
			error ("scxrealloc failed in printfile()");
			return;
		      }
		    }		  

		    /* Now justify and print */
		    start = (*pp)->flags & is_leftflush ? pline + c
					: pline + c + fieldlen - slen;
		    if( (*pp)->flags & is_label )
			start = pline + (c + ((fwidth[col]>slen)?(fwidth[col]-slen)/2:0));
		    last = pline + c + fieldlen;
		    fp = plinelim < c ? pline + plinelim : pline + c;
		    while (fp < start)
			*fp++ = ' ';
		    if( *s == '\\' && *(s+1)!= '\0' ) {
			char *strt;
			strt = ++s;

			while(slen--) {
				*fp++ = *s++; if( *s == '\0' ) s = strt;
			}
		    }
		    else
		    while (slen--)
			*fp++ = *s++;

		    if (!((*pp)->flags & is_valid) || fieldlen != fwidth[col])
			while(fp < last)
			    *fp++ = ' ';
		    if (plinelim < fp - pline)
			plinelim = fp - pline;
		}
	    }
	}
	pline[plinelim++] = '\n';
	pline[plinelim] = '\0';
	(void) fputs (pline, f);
    }

    closeout(f, pid);
}

void
tblprintfile (fname, r0, c0, rn, cn)
char *fname;
int r0, c0, rn, cn;
{
    FILE *f;
    int pid;
    register row, col;
    register struct ent **pp;
    char coldelim = DEFCOLDELIM;
    char file[32];
    char path[256];
    char *tpp;

    /* tblprintfile will be the [path/]file ---> [path/]file.out,
     * file is limited to 14 characters.
     */
    if (*fname == '\0') {
    strcpy(path, curfile);
    if ((tpp = strrchr(path, '/'))  == NULL)
      tpp = path;
    else
      tpp++;
    strcpy(file, tpp);
    file[10] = '\0';
    if (tbl_style == 0)
      sprintf(tpp, "%s.cln", file);
    else if (tbl_style == TBL)
      sprintf(tpp, "%s.tbl", file);
    else if (tbl_style == LATEX)
      sprintf(tpp, "%s.lat", file);
    else if (tbl_style == TEX)
      sprintf(tpp, "%s.tex", file);
    fname = path;
    }

    if ((strcmp(fname, curfile) == 0) &&
	!yn_ask("Confirm that you want to destroy the data base: (y,n)"))
	    return;

    if ((f = openout(fname, &pid)) == (FILE *)0)
    {	error ("Can't create file \"%s\"", fname);
	return;
    }

    if ( tbl_style == TBL ) {
	fprintf(f,".\\\" ** %s spreadsheet output \n.TS\n",progname);
	fprintf(f,"tab(%c);\n",coldelim);
	for (col=c0;col<=cn; col++) fprintf(f," n");
	fprintf(f, ".\n");
    }
    else if ( tbl_style == LATEX ) {
	fprintf(f,"%% ** %s spreadsheet output\n\\begin{tabular}{",progname);
	for (col=c0;col<=cn; col++) fprintf(f,"c");
	fprintf(f, "}\n");
	coldelim = '&';
    }
    else if ( tbl_style == SLATEX ) {
	fprintf(f,"%% ** %s spreadsheet output\n!begin<tabular><",progname);
	for (col=c0;col<=cn; col++) fprintf(f,"c");
	fprintf(f, ">\n");
	coldelim = '&';
    }
    else if ( tbl_style == TEX ) {
	fprintf(f,"{\t%% ** %s spreadsheet output\n\\settabs %d \\columns\n",
		progname, cn-c0+1);
	coldelim = '&';
    }
    else if ( tbl_style == FRAME ) {
	fprintf(f,"<MIFFile 3.00> # generated by the sc spreadsheet calculator\n");
	fprintf(f,"<Tbls\n");
	fprintf(f," <Tbl \n");
	fprintf(f,"  <TblID 1> # This table's ID is 1\n");
	fprintf(f,"  <TblFormat \n");
	fprintf(f,"   <TblTag `Format A'> # Table Format Catalog\n");
	fprintf(f,"  > # end of TblFormat\n");
	fprintf(f,"  <TblNumColumns %d> # Has %d columns\n",cn-c0+1,cn-c0+1);
	fprintf(f,"  <TblTitleContent\n");
	fprintf(f,"   <Para\n");
	fprintf(f,"    <PgfTag `TableTitle'> # Forces lookup in Paragraph Format Catalog\n");
	fprintf(f,"    <ParaLine\n");
	fprintf(f,"     <String `%s'>\n",fname);
	fprintf(f,"    > # end of ParaLine\n");
	fprintf(f,"   > # end of Para\n");
	fprintf(f,"  > # end of TblTitleContent\n");
	fprintf(f,"  <TblH # The heading\n");
	fprintf(f,"   <Row # The heading row\n");
	for (col=c0; col <= cn; col++) {
	    fprintf(f,"    <Cell <CellContent <Para # Cell in column \n");
	    fprintf(f,"       <PgfTag `CellHeading'> # in Paragraph Format Catalog\n");
	    fprintf(f,"       <ParaLine <String `%c'>>\n",'A'+col);
	    fprintf(f,"    >>> # end of Cell\n");
	}
	fprintf(f,"   > # end of Row\n");
	fprintf(f,"  > # end of TblH\n");
	fprintf(f,"  <TblBody # The body\n");
    }

    for (row=r0; row<=rn; row++) {
	if ( tbl_style == TEX )
	    (void) fprintf (f, "\\+");
	else if ( tbl_style == FRAME ) {
	    fprintf(f,"   <Row # The next body row\n");
	}
	
	for (pp = ATBL(tbl, row, col=c0); col<=cn; col++, pp++) {
	    if ( tbl_style == FRAME ) {
		fprintf(f,"    <Cell <CellContent <Para\n");
		fprintf(f,"       <PgfTag `CellBody'> # in Paragraph Format Catalog\n");
		fprintf(f,"       <ParaLine <String `");
	    } 
	    if (*pp) {
		char *s;
		if ((*pp)->flags&is_valid) {
		    if ((*pp)->cellerror) {
			(void) fprintf (f, "%*s",
					fwidth[col],
			((*pp)->cellerror == CELLERROR ? "ERROR" : "INVALID"));
		    }
		    else
		    if ((*pp)->format) {
		        char field[FBUFLEN];
			
			(void) format ((*pp)->format, (*pp)->v, field,
				       sizeof(field));
			unspecial (f, field, coldelim);
		    } else {
		        char field[FBUFLEN];
                        (void) engformat(realfmt[col], fwidth[col],
                                             precision[col], (*pp) -> v,
                                             field, sizeof(field));
			unspecial (f, field, coldelim);
		    }
		}
		if (s = (*pp)->label) {
	            unspecial (f, s, coldelim);
		}
	    }
	    if (tbl_style == FRAME) {
		fprintf(f, "'>>\n");
		fprintf(f,"    >>> # end of Cell\n");
	    }
	    if ( col < cn )
		if (tbl_style != FRAME)
		    (void) fprintf(f,"%c", coldelim);
	}
	if ( tbl_style == LATEX ) {
		if ( row < rn ) (void) fprintf (f, "\\\\");
	}
	else if ( tbl_style == SLATEX ) {
		if ( row < rn ) (void) fprintf (f, "!!");
	}
	else if ( tbl_style == TEX ) {
		(void) fprintf (f, "\\cr");
	}
	else if ( tbl_style == FRAME ) {
	    fprintf(f,"   > # end of Row\n");
	}
	(void) fprintf (f,"\n");
    }

    if ( tbl_style == TBL )
    (void) fprintf (f,".TE\n.\\\" ** end of %s spreadsheet output\n", progname);
    else if ( tbl_style == LATEX )
    (void) fprintf (f,"\\end{tabular}\n%% ** end of %s spreadsheet output\n", progname);
    else if ( tbl_style == SLATEX )
    (void) fprintf (f,"!end<tabular>\n%% ** end of %s spreadsheet output\n", progname);
    else if ( tbl_style == TEX )
    (void) fprintf (f,"}\n%% ** end of %s spreadsheet output\n", progname);
    else if ( tbl_style == FRAME ) {
	fprintf(f,"  > # end of TblBody\n");
	fprintf(f," ># end of Tbl\n");
	fprintf(f,"> # end of Tbls\n");
	fprintf(f,"<TextFlow <Para \n");
	fprintf(f,"  <PgfTag Body> \n");
	fprintf(f,"  <ParaLine <ATbl 1>> # Reference to table ID 1\n");
	fprintf(f,">>\n");
    }

    closeout(f, pid);
}

/* unspecial (backquote) things that are special chars in a table */
void
unspecial(f, str, delim)
FILE	*f;
char	*str;
int	delim;
{
	if( *str == '\\' ) str++; /* delete wheeling string operator, OK? */
	while (*str)
	{	if (((tbl_style == LATEX) || (tbl_style == SLATEX) ||
		    (tbl_style == TEX)) &&
		    ((*str == delim) || (*str == '$') || (*str == '#') ||
		    (*str == '%') || (*str == '{') || (*str == '}') ||
		    (*str == '[') || (*str == ']') || (*str == '&')))
			putc('\\', f);
		putc(*str, f);
		str++;
	}
}

struct enode *
copye (e, Rdelta, Cdelta)
register struct enode *e;
int Rdelta, Cdelta;
{
    register struct enode *ret;

    if (e == (struct enode *)0) {
        ret = (struct enode *)0;
    } else if (e->op & REDUCE) {
	int newrow, newcol;
	if (freeenodes)
	{	ret = freeenodes;
		freeenodes = ret->e.o.left;
	}
	else
		ret = (struct enode *) scxmalloc ((unsigned) sizeof (struct enode));
	ret->op = e->op;
	newrow=e->e.r.left.vf & FIX_ROW ? e->e.r.left.vp->row :
					  e->e.r.left.vp->row+Rdelta;
	newcol=e->e.r.left.vf & FIX_COL ? e->e.r.left.vp->col :
					  e->e.r.left.vp->col+Cdelta;
	ret->e.r.left.vp = lookat (newrow, newcol);
	ret->e.r.left.vf = e->e.r.left.vf;
	newrow=e->e.r.right.vf & FIX_ROW ? e->e.r.right.vp->row :
					   e->e.r.right.vp->row+Rdelta;
	newcol=e->e.r.right.vf & FIX_COL ? e->e.r.right.vp->col :
					   e->e.r.right.vp->col+Cdelta;
	ret->e.r.right.vp = lookat (newrow, newcol);
	ret->e.r.right.vf = e->e.r.right.vf;
    } else {
	if (freeenodes)
	{	ret = freeenodes;
		freeenodes = ret->e.o.left;
	}
	else
		ret = (struct enode *) scxmalloc ((unsigned) sizeof (struct enode));
	ret->op = e->op;
	switch (ret->op) {
	case 'v':
		{
		    int newrow, newcol;
		    newrow=e->e.v.vf & FIX_ROW ? e->e.v.vp->row :
						 e->e.v.vp->row+Rdelta;
		    newcol=e->e.v.vf & FIX_COL ? e->e.v.vp->col :
						 e->e.v.vp->col+Cdelta;
		    ret->e.v.vp = lookat (newrow, newcol);
		    ret->e.v.vf = e->e.v.vf;
		    break;
		}
	case 'k':
		ret->e.k = e->e.k;
		break;
	case 'f':
		ret->e.o.right = copye (e->e.o.right,0,0);
		ret->e.o.left = (struct enode *)0;
 		break;
	case '$':
		ret->e.s = scxmalloc((unsigned) strlen(e->e.s)+1);
		(void) strcpy(ret->e.s, e->e.s);
		break;
	default:
		ret->e.o.right = copye (e->e.o.right,Rdelta,Cdelta);
		ret->e.o.left = copye (e->e.o.left,Rdelta,Cdelta);
		break;
	}
    }
    return ret;
}

/*
 * sync_refs and syncref are used to remove references to
 * deleted struct ents.  Note that the deleted structure must still
 * be hanging around before the call, but not referenced by an entry
 * in tbl.  Thus the free_ent calls in sc.c
 */
void
sync_refs ()
{
    register i,j;
    register struct ent *p;
    sync_ranges();
    for (i=0; i<=maxrow; i++)
	for (j=0; j<=maxcol; j++)
	    if ((p = *ATBL(tbl, i, j)) && p->expr)
		syncref(p->expr);
}

void
syncref(e)
register struct enode *e;
{
    if (e == (struct enode *)0)
	return;
    else if (e->op & REDUCE) {
 	e->e.r.right.vp = lookat(e->e.r.right.vp->row, e->e.r.right.vp->col);
 	e->e.r.left.vp = lookat(e->e.r.left.vp->row, e->e.r.left.vp->col);
    } else {
	switch (e->op) {
	case 'v':
		e->e.v.vp = lookat(e->e.v.vp->row, e->e.v.vp->col);
		break;
	case 'k':
		break;
	case '$':
		break;
	default:
		syncref(e->e.o.right);
		syncref(e->e.o.left);
		break;
	}
    }
}

/* mark a row as hidden */
void
hiderow(arg)
int arg;
{
    register int r1;
    register int r2;

    r1 = currow;
    r2 = r1 + arg - 1;
    if (r1 < 0 || r1 > r2) {
	error ("Invalid range");
	return;
    }
    if (r2 >= maxrows-1)
    {	if (!growtbl(GROWROW, arg+1, 0))
	{	error("You can't hide the last row");
		return;
	}
    }
    FullUpdate++;
    modflg++;
    while (r1 <= r2)
	row_hidden[r1++] = 1;
}

/* mark a column as hidden */
void
hidecol(arg)
int arg;
{
    register int c1;
    register int c2;

    c1 = curcol;
    c2 = c1 + arg - 1;
    if (c1 < 0 || c1 > c2) {
	error ("Invalid range");
	return;
    }
    if (c2 >= maxcols-1)
    {	if ((arg >= ABSMAXCOLS-1) || !growtbl(GROWCOL, 0, arg+1))
	{	error("You can't hide the last col");
		return;
	}
    }
    FullUpdate++;
    modflg++;
    while (c1 <= c2)
	col_hidden[c1++] = TRUE;
}

/* mark a row as not-hidden */
void
showrow(r1, r2)
int r1, r2;
{
    if (r1 < 0 || r1 > r2) {
	error ("Invalid range");
	return;
    }
    if (r2 > maxrows-1) {
	r2 = maxrows-1;
    }
    FullUpdate++;
    modflg++;
    while (r1 <= r2)
	row_hidden[r1++] = 0;
}

/* mark a column as not-hidden */
void
showcol(c1, c2)
int c1, c2;
{
    if (c1 < 0 || c1 > c2) {
	error ("Invalid range");
	return;
    }
    if (c2 > maxcols-1) {
	c2 = maxcols-1;
    }
    FullUpdate++;
    modflg++;
    while (c1 <= c2)
	col_hidden[c1++] = FALSE;
}

/* Open the output file, setting up a pipe if needed */
FILE *
openout(fname, rpid)
char *fname;
int *rpid;
{
    int pipefd[2];
    int pid;
    FILE *f;
    char *efname;

    while (*fname && (*fname == ' '))  /* Skip leading blanks */
	fname++;

    if (*fname != '|') {		/* Open file if not pipe */
	*rpid = 0;
	
	efname = findhome(fname);
#ifdef DOBACKUPS
	if (!backup_file(efname) &&
	    (yn_ask("Could not create backup copy, Save anyhow?: (y,n)") != 1))
		return(0);
#endif
	return(fopen(efname, "w"));
    }

#if defined(MSDOS)
    error("Piping not available under MS-DOS\n");
    return(0);
#else
    fname++;				/* Skip | */
    if ( pipe (pipefd) < 0) {
	error("Can't make pipe to child");
	*rpid = 0;
	return(0);
    }

    deraw();
#ifdef VMS
    fprintf(stderr, "No son tasks available yet under VMS--sorry\n");
#else /* VMS */

    if ((pid=fork()) == 0)			  /* if child  */
    {
	(void) close (0);			  /* close stdin */
	(void) close (pipefd[1]);
	(void) dup (pipefd[0]);		  /* connect to pipe input */
	(void) signal (SIGINT, SIG_DFL);	  /* reset */
	(void) execl ("/bin/sh", "sh", "-c", fname, 0);
	exit (-127);
    }
    else				  /* else parent */
    {
	*rpid = pid;
	if ((f = fdopen (pipefd[1], "w")) == (FILE *)0)
	{
	    (void) kill (pid, -9);
	    error ("Can't fdopen output");
	    (void) close (pipefd[1]);
	    *rpid = 0;
	    return(0);
	}
    }
#endif /* VMS */
    return(f);
#endif /* MSDOS */
}

/* close a file opened by openout(), if process wait for return */
void
closeout(f, pid)
FILE *f;
int pid;
{
    int temp;

    (void) fclose (f);
#if !defined(MSDOS)
    if (pid) {
         while (pid != wait(&temp)) /**/;
	 (void) printf("Press RETURN to continue ");
	 (void) fflush(stdout);
	 (void) nmgetch();
	 goraw();
    }
#endif /* MSDOS */
}

void
copyent(n,p,dr,dc)
	    register struct ent *n, *p;
	    int dr, dc;
{
    if(!n||!p){error("internal error");return;}
    n -> v = p -> v;
    n -> flags = p -> flags;
    n -> expr = copye (p -> expr, dr, dc);
    n -> label = (char *)0;
    if (p -> label) {
	n -> label = scxmalloc ((unsigned) (strlen (p -> label) + 1));
	(void) strcpy (n -> label, p -> label);
    }
    n -> format = 0;
    if (p -> format) {
        n -> format = scxmalloc ((unsigned) (strlen (p -> format) + 1));
	(void) strcpy (n -> format, p -> format);
    }
}

void
write_fd (f, r0, c0, rn, cn)
register FILE *f;
int r0, c0, rn, cn;
{
    register struct ent **pp;
    register r, c;
    extern char *v_name();

    (void) fprintf (f, "# This data file was generated by the Spreadsheet ");
    (void) fprintf (f, "Calculator.\n");
    (void) fprintf (f, "# You almost certainly shouldn't edit it.\n\n");
    print_options(f);
    for (c=0; c<maxcols; c++)
	if (fwidth[c] != DEFWIDTH || precision[c] != DEFPREC || realfmt[c] != DEFREFMT )
	    (void) fprintf (f, "format %s %d %d %d\n",coltoa(c),fwidth[c],precision[c],realfmt[c]);
    for (c=c0; c<cn; c++) {
        if (col_hidden[c]) {
            (void) fprintf(f, "hide %s\n", coltoa(c));
        }
    }
    for (r=r0; r<=rn; r++) {
	if (row_hidden[r]) {
	    (void) fprintf(f, "hide %d\n", r);
	}
    }

    write_range(f);

    if (mdir) 
	    (void) fprintf(f, "mdir \"%s\"\n", mdir);
    for (r=r0; r<=rn; r++) {
	pp = ATBL(tbl, r, c0);
	for (c=c0; c<=cn; c++, pp++)
	    if (*pp) {
		if ((*pp)->label) {
		    edits(r,c);
		    (void) fprintf(f, "%s\n",line);
		}
		if ((*pp)->flags&is_valid) {
		    editv (r, c);
		    (void) fprintf (f, "%s\n",line);
		}
		if ((*pp)->format) {
		    editfmt (r, c);
		    (void) fprintf (f, "%s\n",line);
		}
		if ((*pp)->flags&is_locked)
		    (void) fprintf(f, "lock %s%d\n", coltoa((*pp)->col),
						     (*pp)->row) ;
	    }
    }
    if (rndinfinity)
	fprintf(f, "set rndinfinity\n");
    fprintf(f, "goto %s\n", v_name( currow, curcol ) );
}

int
writefile (fname, r0, c0, rn, cn)
char *fname;
int r0, c0, rn, cn;
{
    register FILE *f;
    char save[PATHLEN];
    int pid;

#if !defined(VMS) && !defined(MSDOS) && defined(CRYPT_PATH)
    if (Crypt) {
	return (cwritefile(fname, r0, c0, rn, cn));
    }
#endif /* VMS */

    if (*fname == '\0') fname = curfile;

    (void) strcpy(save,fname);

    if ((f= openout(fname, &pid)) == (FILE *)0)
    {	error ("Can't create file \"%s\"", fname);
	return (-1);
    }

    write_fd(f, r0, c0, rn, cn);
    
    closeout(f, pid);

    if (!pid) {
        (void) strcpy(curfile, save);
        modflg = 0;
        error("File \"%s\" written.",curfile);
    }

    return (0);
}

void
readfile (fname,eraseflg)
char *fname;
int eraseflg;
{
    register FILE *f;
    char save[PATHLEN];
    int tempautolabel;

    tempautolabel = autolabel;		/* turn off auto label when */
    autolabel = 0;			/* when reading a file  */

    if (*fname == '*' && mdir) { 
       (void) strcpy(save, mdir);
       *fname = '/';
       (void) strcat(save, fname);
    } else {
        if (*fname == '\0')
	    fname = curfile;
        (void) strcpy(save,fname);
    }

#if !defined(VMS) && !defined(MSDOS) && defined(CRYPT_PATH)
    if (Crypt)  {
	creadfile(save, eraseflg);
	return;
    }
#endif /* VMS */

    if (eraseflg && strcmp(fname,curfile) && modcheck(" first")) return;

    if ((f = fopen(findhome(save), "r")) == (FILE *)0)
    {	error ("Can't read file \"%s\"", save);
	return;
    }

    if (eraseflg) erasedb ();

    loading++;
    while (fgets(line, sizeof(line), f)) {
	linelim = 0;
	if (line[0] != '#') (void) yyparse ();
    }
    --loading;
    (void) fclose (f);
    linelim = -1;
    modflg++;
    if (eraseflg) {
	(void) strcpy(curfile,save);
	modflg = 0;
    }
    autolabel = tempautolabel;
    EvalAll();
}

/* erase the database (tbl, etc.) */
void
erasedb ()
{
    register r, c;
    for (c = 0; c<=maxcol; c++) {
	fwidth[c] = DEFWIDTH;
	precision[c] = DEFPREC;
	realfmt[c] = DEFREFMT;
    }

    for (r = 0; r<=maxrow; r++) {
	register struct ent **pp = ATBL(tbl, r, 0);
	for (c=0; c++<=maxcol; pp++)
	    if (*pp) {
		if ((*pp)->expr)  efree ((*pp) -> expr);
		if ((*pp)->label) scxfree ((char *)((*pp) -> label));
		(*pp)->next = freeents;	/* save [struct ent] for reuse */
		freeents = *pp;
		*pp = (struct ent *)0;
	    }
    }
    maxrow = 0;
    maxcol = 0;
    clean_range();
    FullUpdate++;
}

/* moves curcol back one displayed column */
void
backcol(arg)
	int arg;
{
    while (--arg>=0) {
	if (curcol)
	    curcol--;
	else
	    {error ("At column A"); break;}
	while(col_hidden[curcol] && curcol)
	    curcol--;
    }
}

/* moves curcol forward one displayed column */
void
forwcol(arg)
	int arg;
{
    while (--arg>=0) {
	if (curcol < maxcols - 1)
	    curcol++;
	else
	if (!growtbl(GROWCOL, 0, arg))	/* get as much as needed */
		break;
	else
		curcol++;
	while(col_hidden[curcol]&&(curcol<maxcols-1))
	    curcol++;
    }
}

/* moves currow forward one displayed row */
void
forwrow(arg)
	int arg;
{
    while (--arg>=0) {
	if (currow < maxrows - 1)
	    currow++;
	else
	if (!growtbl(GROWROW, arg, 0))	/* get as much as needed */
		break;
	else
		currow++;
	while (row_hidden[currow]&&(currow<maxrows-1))
	    currow++;
    }
}

/* moves currow backward one displayed row */
void
backrow(arg)
	int arg;
{
    while (--arg>=0) {
	if (currow)
	    currow--;
	else
	    {error ("At row zero"); break;}
	while (row_hidden[currow] && currow)
	    currow--;
    }
}


/*
 * Show a cell's label string or expression value.  May overwrite value if
 * there is one already displayed in the cell.  Created from old code in
 * update(), copied with minimal changes.
 */

void
showstring (string, dirflush, hasvalue, row, col, nextcolp, mxcol, fieldlenp, r, c)
    char *string;	/* to display */
    int dirflush;	/* or rightflush or centered */
    int hasvalue;	/* is there a numeric value? */
    int row, col;	/* spreadsheet location */
    int *nextcolp;	/* value returned through it */
    int mxcol;		/* last column displayed? */
    int *fieldlenp;	/* value returned through it */
    int r, c;		/* screen row and column */
{
    register int nextcol  = *nextcolp;
    register int fieldlen = *fieldlenp;

    char field[FBUFLEN];
    int  slen;
    char *start, *last;
    register char *fp;
    struct ent *nc;

    /* This figures out if the label is allowed to
       slop over into the next blank field */

    slen = strlen (string);
    if( *string == '\\' && *(string+1)!= '\0' )
	slen = fwidth[col];
    while ((slen > fieldlen) && (nextcol <= mxcol) &&
	   !((nc = lookat (row, nextcol)) -> flags & is_valid) &&
	   !(nc->label)) {

	if (! col_hidden [nextcol])
	    fieldlen += fwidth [nextcol];

	nextcol++;
    }
    if (slen > fieldlen)
	slen = fieldlen;

    /* Now justify and print */
    start = (dirflush&is_leftflush) ? field : field + fieldlen - slen;
    if( dirflush & is_label )
	start = field + ((slen<fwidth[col])?(fieldlen-slen)/2:0);
    last = field+fieldlen;
    fp = field;
    while (fp < start)
	*fp++ = ' ';
    if( *string == '\\'  && *(string+1)!= '\0') {
	char *strt;
	strt = ++string;

	while(slen--) {
		*fp++ = *string++;
		if( *string == '\0' )
			string = strt;
	}
    }
    else
    while (slen--)
	*fp++ = *string++;

    if ((! hasvalue) || fieldlen != fwidth[col]) 
	while (fp < last)
	    *fp++ = ' ';
    *fp = '\0';
#ifdef VMS
    mvaddstr(r, c, field);	/* this is a macro */
#else
    (void) mvaddstr(r, c, field);
#endif

    *nextcolp  = nextcol;
    *fieldlenp = fieldlen;
}

int
etype(e)
register struct enode *e;
{
    if (e == (struct enode *)0)
	return NUM;
    switch (e->op) {
    case UPPER: case LOWER: case CAPITAL:
    case O_SCONST: case '#': case DATE: case FMT: case STINDEX:
    case EXT: case SVAL: case SUBSTR:
        return (STR);

    case '?':
    case IF:
        return(etype(e->e.o.right->e.o.left));

    case 'f':
        return(etype(e->e.o.right));

    case O_VAR: {
	register struct ent *p;
	p = e->e.v.vp;
	if (p->expr) 
	    return(p->flags & is_strexpr ? STR : NUM);
	else if (p->label)
	    return(STR);
	else
	    return(NUM);
	}

    default:
	return(NUM);
    }
}

/* return 1 if yes given, 0 otherwise */
int
yn_ask(msg)
char	*msg;
{	char ch;

	(void) move (0, 0);
	(void) clrtoeol ();
	(void) addstr (msg);
	(void) refresh();
	ch = nmgetch();
	if ( ch != 'y' && ch != 'Y' && ch != 'n' && ch != 'N' ) {
		if (ch == ctl('g') || ch == ESC)
			return(-1);
		error("y or n response required");
		return (-1);
	}
	if (ch == 'y' || ch == 'Y')
		return(1);
	else
		return(0);
}

/* expand a ~ in a path to your home directory */
#if !defined(MSDOS) && !defined(VMS)
#include <pwd.h>
#endif
char	*
findhome(path)
char	*path;
{
	static	char	*HomeDir = NULL;
	extern	char	*getenv();

	if (*path == '~')
	{	char	*pathptr;
		char	tmppath[PATHLEN];

		if (HomeDir == NULL)
		{	HomeDir = getenv("HOME");
			if (HomeDir == NULL)
				HomeDir = "/";
		}
		pathptr = path + 1;
		if ((*pathptr == '/') || (*pathptr == '\0'))
		{	strcpy(tmppath, HomeDir);
		}
#if !defined(MSDOS) && !defined(VMS)
		else
		{	struct	passwd *pwent;
			extern	struct	passwd *getpwnam();
			char	*namep;
			char	name[50];

			namep = name;
			while ((*pathptr != '\0') && (*pathptr != '/'))
				*(namep++) = *(pathptr++);
			*namep = '\0';
			if ((pwent = getpwnam(name)) == NULL)
			{	(void) sprintf(path, "Can't find user %s", name);
				return(NULL);
			}
			strcpy(tmppath, pwent->pw_dir);
		}
#endif
		strcat(tmppath, pathptr);
		strcpy(path, tmppath);
	}
	return(path);
}

#ifdef DOBACKUPS
#include <sys/stat.h>

/*
 * make a backup copy of a file, use the same mode and name in the format
 * [path/]#file~
 * return 1 if we were successful, 0 otherwise
 */
int
backup_file(path)
char	*path;
{
	struct	stat	statbuf;
	char	fname[PATHLEN];
	char	tpath[PATHLEN];
#ifdef sequent
	static	char	*buf = NULL;
	static	unsigned buflen = 0;
#else
	char	buf[BUFSIZ];
#endif
	char	*tpp;
	int	infd, outfd;
	int	count;

	/* tpath will be the [path/]file ---> [path/]#file~ */
	strcpy(tpath, path);
	if ((tpp = strrchr(tpath, '/')) == NULL)
		tpp = tpath;
	else
		tpp++;
	strcpy(fname, tpp);
	(void) sprintf(tpp, "#%s~", fname);

	if (stat(path, &statbuf) == 0)
	{
		/* if we know the optimum block size, use it */
#ifdef sequent
		if ((statbuf.st_blksize > buflen) || (buf == NULL))
		{	buflen = statbuf.st_blksize;
			if ((buf = scxrealloc(buf, buflen)) == (char *)0)
			{	buflen = 0;
				return(0);
			}
		}
#endif

		if ((infd = open(path, O_RDONLY, 0)) < 0)
			return(0);

		if ((outfd = open(tpath, O_TRUNC|O_WRONLY|O_CREAT,
					statbuf.st_mode)) < 0)
			return(0);

#ifdef sequent
		while((count = read(infd, buf, statbuf.st_blksize)) > 0)
#else
		while((count = read(infd, buf, sizeof(buf))) > 0)
#endif
		{	if (write(outfd, buf, count) != count)
			{	count = -1;
				break;
			}
		}
		close(infd);
		close(outfd);

		return((count < 0) ? 0 : 1);
	}
	else
	if (errno == ENOENT)
		return(1);
	return(0);
}
#endif

static int day_month_starts[12] = {0,31,59,90,120,151,181,212,243,273,304,334};

double convert_date(d, m, y)
int d;
int m;
int y;
{
  /* Convert to years since 1970. (or 2000, fix by 2070) */
  if (y > 1970) y -= 1970;	/* Full year given */
  else if (y >= 70) y -= 70;	/* Years since 1900 */
  else y += 30;			/* Years since 2000 */
  /* Use quarter days to compensate for leap years. */
  return (double)((y * (365 * 4 + 1) + day_month_starts[m-1] * 4 + d * 4 - 2) *
		  6 * 60 * 60);
}

