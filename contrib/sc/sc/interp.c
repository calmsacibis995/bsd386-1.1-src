/*	SC	A Spreadsheet Calculator
 *		Expression interpreter and assorted support routines.
 *
 *		original by James Gosling, September 1982
 *		modified by Mark Weiser and Bruce Israel, 
 *			University of Maryland
 *
 *              More mods Robert Bond, 12/86
 *		More mods by Alan Silverstein, 3-4/88, see list of changes.
 *		$Revision: 1.1.1.1 $
 */

#define DEBUGDTS 1		/* REMOVE ME */
     
#include <sys/types.h>
#ifdef aiws
#undef _C_func			/* Fixes for undefined symbols on AIX */
#endif

#ifdef IEEE_MATH
#include <ieeefp.h>
#endif /* IEEE_MATH */

#include <math.h>
#include <signal.h>
#include <setjmp.h>
#include <stdio.h>
#include <ctype.h>

extern int errno;		/* set by math functions */
#ifdef BSD42
#include <strings.h>
#include <sys/time.h>
#ifndef strchr
#define strchr index
#endif
#else
#include <time.h>
#ifndef SYSIII
#include <string.h>
#endif
#endif

#include <curses.h>
#include "sc.h"

#if defined(RE_COMP)
char *re_comp();
#endif
#if defined(REGCMP)
char *regcmp();
char *regex();
#endif

#ifdef SIGVOID
    void doquit();
#else
    int doquit();
#endif

/* Use this structure to save the the last 'g' command */
struct go_save {
	int g_type;
	double g_n;
	char *g_s;
	int  g_row;
	int  g_col;
	int  errsearch;
} gs;

/* g_type can be: */
#define G_NONE 0			/* Starting value - must be 0*/
#define G_NUM 1
#define G_STR 2
#define G_CELL 3

#define ISVALID(r,c)	((r)>=0 && (r)<maxrows && (c)>=0 && (c)<maxcols)

extern FILE *popen();

jmp_buf fpe_save;
int	exprerr;	/* Set by eval() and seval() if expression errors */
double  prescale = 1.0;	/* Prescale for constants in let() */
int	extfunc  = 0;	/* Enable/disable external functions */
int     loading = 0;	/* Set when readfile() is active */
int	gmyrow, gmycol;	/* globals used to implement @myrow, @mycol cmds */

/* a linked list of free [struct enodes]'s, uses .e.o.left as the pointer */
struct enode *freeenodes = NULL;

char	*seval();
double	dolookup();
double	eval();
double	fn1_eval();
double	fn1_seval();
double	fn2_eval();
int	RealEvalAll();
int	constant();
void	RealEvalOne();
void	copyrtv();
void	decompile();
void	index_arg();
void	list_arg();
void	one_arg();
void	range_arg();
void	three_arg();
void	two_arg();
void	two_arg_index();

double	rint();
int	cellerror = CELLOK;	/* is there an error in this cell */

#ifndef PI
#define PI (double)3.14159265358979323846
#endif
#define dtr(x) ((x)*(PI/(double)180.0))
#define rtd(x) ((x)*(180.0/(double)PI))

double finfunc(fun,v1,v2,v3)
int fun;
double v1,v2,v3;
{
 	double answer,p;
 
 	p = fn2_eval(pow, 1 + v2, v3);
 
 	switch(fun)
 	{
 	case PV:
		if (v2)
			answer = v1 * (1 - 1/p) / v2;
		else
		{	cellerror = CELLERROR;
			answer = (double)0;
		}
 		break;
 	case FV:
		if (v2)
			answer = v1 * (p - 1) / v2;
		else
		{	cellerror = CELLERROR;
			answer = (double)0;
		}
 		break;
 	case PMT:
		/* CHECK IF ~= 1 - 1/1 */
		if (p && p != (double)1)
			answer = v1 * v2 / (1 - 1/p);
		else
		{	cellerror = CELLERROR;
			answer = (double)0;
		}
		
 		break;
	default:
		error("Unknown function in finfunc");
		cellerror = CELLERROR;
		return((double)0);
	}
	return(answer);
}

char *
dostindex( val, minr, minc, maxr, maxc)
double val;
int minr, minc, maxr, maxc;
{
    register r,c;
    register struct ent *p;
    char *pr;
    int x;

    x = (int) val;
    r = minr; c = minc;
    p = (struct ent *)0;
    if ( minr == maxr ) { /* look along the row */
	c = minc + x - 1;
	if (c <= maxc && c >=minc)
	    p = *ATBL(tbl, r, c);
    } else if ( minc == maxc ) { /* look down the column */
	r = minr + x - 1;
	if (r <= maxr && r >=minr)
	    p = *ATBL(tbl, r, c);
    } else {
	error ("range specified to @stindex");
	cellerror = CELLERROR;
	return((char *)0);
    }

    if (p && p->label) {
	pr = scxmalloc((unsigned)(strlen(p->label)+1));
	(void) strcpy(pr, p->label);
	if (p->cellerror)
		cellerror = CELLINVALID;
	return (pr);
     } else
	return((char *)0);
}

double
doindex( val, minr, minc, maxr, maxc)
double val;
int minr, minc, maxr, maxc;
{
    double v;
    register r,c;
    register struct ent *p;
    int x;

    x = (int) val;
    v = (double)0;
    r = minr; c = minc;
    if ( minr == maxr ) { /* look along the row */
	c = minc + x - 1;
	if (c <= maxc && c >=minc 
		&& (p = *ATBL(tbl, r, c)) && p->flags&is_valid )
	{	if (p->cellerror)
			cellerror = CELLINVALID;
		return p->v;
	}
    }
    else if ( minc == maxc ){ /* look down the column */
	r = minr + x - 1;
	if (r <= maxr && r >=minr 
		&& (p = *ATBL(tbl, r, c)) && p->flags&is_valid )
	{	if (p->cellerror)
			cellerror = CELLINVALID;
		return p->v;
	}
    }
    else {
	error(" range specified to @index");
	cellerror = CELLERROR;
    }
    return v;
}

double
dolookup( val, minr, minc, maxr, maxc, offr, offc)
struct enode * val;
int minr, minc, maxr, maxc, offr, offc;
{
    double v, ret = (double)0;
    register r,c;
    register struct ent *p = (struct ent *)0;
    int incr,incc,fndr,fndc;
    char *s;

    incr = (offc != 0); incc = (offr != 0);
    if (etype(val) == NUM) {
	cellerror = CELLOK;
	v = eval(val);
	for (r = minr, c = minc; r <= maxr && c <= maxc; r+=incr, c+=incc) {
	    if ( (p = *ATBL(tbl, r, c)) && p->flags&is_valid ) {
		if (p->v <= v) {
		    fndr = incc ? (minr + offr) : r;
		    fndc = incr ? (minc + offc) : c;
		    if (ISVALID(fndr,fndc))
			p = *ATBL(tbl, fndr, fndc);
		    else {
			error(" range specified to @[hv]lookup");
			cellerror = CELLERROR;
		    }
		    if ( p && p->flags&is_valid)
		    {	if (p->cellerror)
				cellerror = CELLINVALID;
			ret = p->v;
		    }
		} else break;
	    }
	}
    } else {
	cellerror = CELLOK;
	s = seval(val);
	for (r = minr, c = minc; r <= maxr && c <= maxc; r+=incr, c+=incc) {
	    if ( (p = *ATBL(tbl, r, c)) && p->label ) {
		if (strcmp(p->label,s) == 0) {
		    fndr = incc ? (minr + offr) : r;
		    fndc = incr ? (minc + offc) : c;
		    if (ISVALID(fndr,fndc))
		    {	p = *ATBL(tbl, fndr, fndc);
			if (p->cellerror)
				cellerror = CELLINVALID;
		    }
		    else {
			error(" range specified to @[hv]lookup");
			cellerror = CELLERROR;
		    }
		    break;
		}
	    }
	}
	if ( p && p->flags&is_valid)
	    ret = p->v;
	scxfree(s);
    }
    return ret;
}

double
docount(minr, minc, maxr, maxc)
int minr, minc, maxr, maxc;
{
    int v;
    register r,c;
    register struct ent *p;

    v = 0;
    for (r = minr; r<=maxr; r++)
	for (c = minc; c<=maxc; c++)
	    if ((p = *ATBL(tbl, r, c)) && p->flags&is_valid)
	    {	if (p->cellerror)
			cellerror = CELLINVALID;
		v++;
	    }
    return v;
}

double
dosum(minr, minc, maxr, maxc)
int minr, minc, maxr, maxc;
{
    double v;
    register r,c;
    register struct ent *p;

    v = (double)0;
    for (r = minr; r<=maxr; r++)
	for (c = minc; c<=maxc; c++)
	    if ((p = *ATBL(tbl, r, c)) && p->flags&is_valid)
	    {	if (p->cellerror)
			cellerror = CELLINVALID;
		v += p->v;
	    }
    return v;
}

double
doprod(minr, minc, maxr, maxc)
int minr, minc, maxr, maxc;
{
    double v;
    register r,c;
    register struct ent *p;

    v = 1;
    for (r = minr; r<=maxr; r++)
	for (c = minc; c<=maxc; c++)
	    if ((p = *ATBL(tbl, r, c)) && p->flags&is_valid)
	    {	if (p->cellerror)
			cellerror = CELLINVALID;
		v *= p->v;
	    }
    return v;
}

double
doavg(minr, minc, maxr, maxc)
int minr, minc, maxr, maxc;
{
    double v;
    register r,c,count;
    register struct ent *p;

    v = (double)0;
    count = 0;
    for (r = minr; r<=maxr; r++)
	for (c = minc; c<=maxc; c++)
	    if ((p = *ATBL(tbl, r, c)) && p->flags&is_valid) {
		if (p->cellerror)
			cellerror = CELLINVALID;

		v += p->v;
		count++;
	    }

    if (count == 0) 
	return ((double) 0);

    return (v / (double)count);
}

double
dostddev(minr, minc, maxr, maxc)
int minr, minc, maxr, maxc;
{
    double lp, rp, v, nd;
    register r,c,n;
    register struct ent *p;

    n = 0;
    lp = 0;
    rp = 0;
    for (r = minr; r<=maxr; r++)
	for (c = minc; c<=maxc; c++)
	    if ((p = *ATBL(tbl, r, c)) && p->flags&is_valid) {
		if (p->cellerror)
			cellerror = CELLINVALID;

		v = p->v;
		lp += v*v;
		rp += v;
		n++;
	    }

    if ((n == 0) || (n == 1)) 
	return ((double) 0);
    nd = (double)n;
    return (sqrt((nd*lp-rp*rp)/(nd*(nd-1))));
}

double
domax(minr, minc, maxr, maxc)
int minr, minc, maxr, maxc;
{
    double v = (double)0;
    register r,c,count;
    register struct ent *p;

    count = 0;
    for (r = minr; r<=maxr; r++)
	for (c = minc; c<=maxc; c++)
	    if ((p = *ATBL(tbl, r, c)) && p->flags&is_valid) {
		if (p->cellerror)
			cellerror = CELLINVALID;

		if (!count) {
		    v = p->v;
		    count++;
		} else if (p->v > v)
		    v = p->v;
	    }

    if (count == 0) 
	return ((double) 0);

    return (v);
}

double
domin(minr, minc, maxr, maxc)
int minr, minc, maxr, maxc;
{
    double v = (double)0;
    register r,c,count;
    register struct ent *p;

    count = 0;
    for (r = minr; r<=maxr; r++)
	for (c = minc; c<=maxc; c++)
	    if ((p = *ATBL(tbl, r, c)) && p->flags&is_valid) {
		if (p->cellerror)
			cellerror = CELLINVALID;

		if (!count) {
		    v = p->v;
		    count++;
		} else if (p->v < v)
		    v = p->v;
	    }

    if (count == 0) 
	return ((double) 0);

    return (v);
}

#define sec_min 60
#define sec_hr  3600L
#define sec_day 86400L
#define sec_yr  31471200L     /* 364.25 days/yr */
#define sec_mo  2622600L       /* sec_yr/12: sort of an average */
int mdays[12]={ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

double
dodts(mo, day, yr)
int mo, day, yr;
{
    long trial;
    register struct tm *tp; 
    register int i;
    register long jdate;

    mdays[1] = 28 + (yr%4 == 0);

    if (mo < 1 || mo > 12 || day < 1 || day > mdays[--mo] ||
		yr > 1999 || yr < 1970) {
	error("@dts: invalid argument");
	cellerror = CELLERROR;
	return(0.0);
    }

    jdate = day-1;
    for (i=0; i<mo; i++)
	    jdate += mdays[i];
    for (i = 1970; i < yr; i++)
	    jdate += 365 + (i%4 == 0);

    trial = jdate * sec_day; 

    yr -= 1900;

    tp = localtime(&trial);

    if (tp->tm_year != yr) {
	    /*
	    * We may fail this test once a year because of time zone
	     * and daylight savings time errors.  This bounces the
	     * trial time past the boundary.  The error introduced is
	     * corrected below.
	     */
	    trial += sec_day*(yr - tp->tm_year);
	    tp = localtime(&trial);
    }
    if (tp->tm_mon != mo) {
	    /* We may fail this test once a month.  */
	    trial += sec_day*(mo - tp->tm_mon);
	    tp = localtime(&trial);
    }
    if (tp->tm_mday + tp->tm_hour + tp->tm_min + tp->tm_sec != day) {
	trial -= (tp->tm_mday - day)*sec_day +  tp->tm_hour*sec_hr
		 + tp->tm_min*sec_min + tp->tm_sec;
    }

#ifdef DEBUGDTS
    tp = localtime(&trial);
    if (tp->tm_mday + tp->tm_hour + tp->tm_min + tp->tm_sec + 
	tp->tm_year + tp->tm_mon != yr+mo+day)
	{	error("Dts broke down");
		cellerror = CELLERROR;
	}
#endif

    return ((double)trial);
}

double
dotts(hr, min, sec)
int hr, min, sec;
{
    if (hr < 0 || hr > 23 || min < 0 || min > 59 || sec < 0 || sec > 59) {
	error ("@tts: Invalid argument");
	cellerror = CELLERROR;
	return ((double)0);
    }
    return ((double)(sec+min*60+hr*3600));
}

double
dotime(which, when)
int which;
double when;
{
	long time();

	static long t_cache;
	static struct tm tm_cache;
	struct tm *tp;
	long tloc;

	if (which == NOW) 
	    return (double)time((long *)0);

	tloc = (long)when;

	if (tloc != t_cache) {
	    tp = localtime(&tloc);
	    tm_cache = *tp;
	    tm_cache.tm_mon += 1;
	    tm_cache.tm_year += 1900;
	    t_cache = tloc;
	}

	switch (which) {
	    case HOUR: return((double)(tm_cache.tm_hour));
	    case MINUTE: return((double)(tm_cache.tm_min));
	    case SECOND: return((double)(tm_cache.tm_sec));
	    case MONTH: return((double)(tm_cache.tm_mon));
	    case DAY: return((double)(tm_cache.tm_mday));
	    case YEAR: return((double)(tm_cache.tm_year));
	}
	/* Safety net */
	cellerror = CELLERROR;
	return ((double)0);
}

double
doston(s)
char *s;
{
#ifndef _AIX
      char *strtof();
#endif
    double v;

    if (!s)
	return((double)0);

    (void)strtof(s, &v);
    scxfree(s);
    return(v);
}

double
doeqs(s1, s2)
char *s1, *s2;
{
    double v;

    if (!s1 && !s2)
	return((double)1.0);

    if (!s1 || !s2)
	v = 0.0;
    else if (strcmp(s1, s2) == 0)
	v = 1.0;
    else
	v = 0.0;

    if (s1)
    	scxfree(s1);

    if (s2)
    	scxfree(s2);

    return(v);
}


/*
 * Given a string representing a column name and a value which is a column
 * number, return a pointer to the selected cell's entry, if any, else NULL.
 * Use only the integer part of the column number.  Always free the string.
 */

struct ent *
getent (colstr, rowdoub)
    char *colstr;
    double rowdoub;
{
    int collen;		/* length of string */
    int row, col;	/* integer values   */
    struct ent *p = (struct ent *)0;	/* selected entry   */

    if (!colstr)
    {	cellerror = CELLERROR;
	return((struct ent *)0);
    }

    if (((row = (int) floor (rowdoub)) >= 0)
     && (row < maxrows)				/* in range */
     && ((collen = strlen (colstr)) <= 2)	/* not too long */
     && ((col = atocol (colstr, collen)) >= 0)
     && (col < maxcols))			/* in range */
    {
	p = *ATBL(tbl, row, col);
	if ((p != NULL) && p->cellerror)
		cellerror = CELLINVALID;
    }
    scxfree (colstr);
    return (p);
}


/*
 * Given a string representing a column name and a value which is a column
 * number, return the selected cell's numeric value, if any.
 */

double
donval (colstr, rowdoub)
    char *colstr;
    double rowdoub;
{
    struct ent *ep;

    return (((ep = getent (colstr, rowdoub)) && ((ep -> flags) & is_valid)) ?
	    (ep -> v) : (double)0);
}


/*
 *	The list routines (e.g. dolmax) are called with an LMAX enode.
 *	The left pointer is a chain of ELIST nodes, the right pointer
 *	is a value.
 */
double
dolmax(ep)
struct enode *ep;
{
	register int count = 0;
	register double maxval = 0; /* Assignment to shut up lint */
	register struct enode *p;
	register double v;

	cellerror = CELLOK;
	for (p = ep; p; p = p->e.o.left) {
		v = eval(p->e.o.right);
		if (!count || v > maxval) {
			maxval = v; count++;
		}
	}
	if (count) return maxval;
	else return (double)0;
}

double
dolmin(ep)
struct enode *ep;
{
	register int count = 0;
	register double minval = 0; /* Assignment to shut up lint */
	register struct enode *p;
	register double v;

	cellerror = CELLOK;
	for (p = ep; p; p = p->e.o.left) {
		v = eval(p->e.o.right);
		if (!count || v < minval) {
			minval = v; count++;
		}
	}
	if (count) return minval;
	else return (double)0;
}

double 
eval(e)
register struct enode *e;
{
    if (e == (struct enode *)0)
    {	cellerror = CELLINVALID;
	return (double)0;
    }
    switch (e->op) {
	case '+':	return (eval(e->e.o.left) + eval(e->e.o.right));
	case '-':	return (eval(e->e.o.left) - eval(e->e.o.right));
	case '*':	return (eval(e->e.o.left) * eval(e->e.o.right));
	case '/':     { double num, denom;
			num = eval(e->e.o.left);
			denom = eval(e->e.o.right);
			if (denom)
/*			if (1)	/* to test num div 0 */
				return(num/denom);
			else
			{	cellerror = CELLERROR;
				return((double) 0);
			}
	}
	case '%':     {	double num, denom;
			num = floor(eval(e->e.o.left));
			denom = floor(eval (e->e.o.right));
			if (denom)
				return(num - floor(num/denom)*denom);
			else
			{	cellerror = CELLERROR;
				return((double) 0);
			}
	}
	case '^':	return (fn2_eval(pow,eval(e->e.o.left),eval(e->e.o.right)));
	case '<':	return (eval(e->e.o.left) < eval(e->e.o.right));
	case '=':	return (eval(e->e.o.left) == eval(e->e.o.right));
	case '>':	return (eval(e->e.o.left) > eval(e->e.o.right));
	case '&':	return (eval(e->e.o.left) && eval(e->e.o.right));
	case '|':	return (eval(e->e.o.left) || eval(e->e.o.right));
	case IF:
	case '?':	return eval(e->e.o.left) ? eval(e->e.o.right->e.o.left)
						: eval(e->e.o.right->e.o.right);
	case 'm':	return (-eval(e->e.o.right));
	case 'f':	return (eval(e->e.o.right));
	case '~':	return (eval(e->e.o.right) == 0.0);
	case O_CONST:	return (e->e.k);
	case O_VAR:	if (e->e.v.vp->cellerror)
				cellerror = CELLINVALID;
			return (e->e.v.vp->v);
	case INDEX:
	case LOOKUP:
	case HLOOKUP:
	case VLOOKUP:
	    {	register r,c;
		register maxr, maxc;
		register minr, minc;
		maxr = e->e.o.right->e.r.right.vp -> row;
		maxc = e->e.o.right->e.r.right.vp -> col;
		minr = e->e.o.right->e.r.left.vp -> row;
		minc = e->e.o.right->e.r.left.vp -> col;
		if (minr>maxr) r = maxr, maxr = minr, minr = r;
		if (minc>maxc) c = maxc, maxc = minc, minc = c;
		switch(e->op){
		case LOOKUP:
		    return dolookup(e->e.o.left, minr, minc, maxr, maxc,
				     minr==maxr, minc==maxc);
		case HLOOKUP:
	            return dolookup(e->e.o.left->e.o.left, minr,minc,maxr,maxc,
			(int) eval(e->e.o.left->e.o.right), 0);
		case VLOOKUP:
	            return dolookup(e->e.o.left->e.o.left, minr,minc,maxr,maxc,
			0, (int) eval(e->e.o.left->e.o.right));
		case INDEX:
		    return doindex(eval(e->e.o.left), minr, minc, maxr, maxc);
		}
	    }
	case REDUCE | '+':
 	case REDUCE | '*':
 	case REDUCE | 'a':
 	case REDUCE | 'c':
 	case REDUCE | 's':
	case REDUCE | MAX:
	case REDUCE | MIN:
	    {	register r,c;
		register maxr, maxc;
		register minr, minc;
		maxr = e->e.r.right.vp -> row;
		maxc = e->e.r.right.vp -> col;
		minr = e->e.r.left.vp -> row;
		minc = e->e.r.left.vp -> col;
		if (minr>maxr) r = maxr, maxr = minr, minr = r;
		if (minc>maxc) c = maxc, maxc = minc, minc = c;
	        switch (e->op) {
	            case REDUCE | '+': return dosum(minr, minc, maxr, maxc);
 	            case REDUCE | '*': return doprod(minr, minc, maxr, maxc);
 	            case REDUCE | 'a': return doavg(minr, minc, maxr, maxc);
 	            case REDUCE | 'c': return docount(minr, minc, maxr, maxc);
 	            case REDUCE | 's': return dostddev(minr, minc, maxr, maxc);
 	            case REDUCE | MAX: return domax(minr, minc, maxr, maxc);
 	            case REDUCE | MIN: return domin(minr, minc, maxr, maxc);
		}
	    }
	case ABS:	 return (fn1_eval( fabs, eval(e->e.o.right)));
	case ACOS:	 return (fn1_eval( acos, eval(e->e.o.right)));
	case ASIN:	 return (fn1_eval( asin, eval(e->e.o.right)));
	case ATAN:	 return (fn1_eval( atan, eval(e->e.o.right)));
	case ATAN2:	 return (fn2_eval( atan2, eval(e->e.o.left), eval(e->e.o.right)));
	case CEIL:	 return (fn1_eval( ceil, eval(e->e.o.right)));
	case COS:	 return (fn1_eval( cos, eval(e->e.o.right)));
	case EXP:	 return (fn1_eval( exp, eval(e->e.o.right)));
	case FABS:	 return (fn1_eval( fabs, eval(e->e.o.right)));
	case FLOOR:	 return (fn1_eval( floor, eval(e->e.o.right)));
	case HYPOT:	 return (fn2_eval( hypot, eval(e->e.o.left), eval(e->e.o.right)));
	case LOG:	 return (fn1_eval( log, eval(e->e.o.right)));
	case LOG10:	 return (fn1_eval( log10, eval(e->e.o.right)));
	case POW:	 return (fn2_eval( pow, eval(e->e.o.left), eval(e->e.o.right)));
	case SIN:	 return (fn1_eval( sin, eval(e->e.o.right)));
	case SQRT:	 return (fn1_eval( sqrt, eval(e->e.o.right)));
	case TAN:	 return (fn1_eval( tan, eval(e->e.o.right)));
	case DTR:	 return (dtr(eval(e->e.o.right)));
	case RTD:	 return (rtd(eval(e->e.o.right)));
	case RND:
		if (rndinfinity)
		{	double temp = eval(e->e.o.right);
			return(temp-floor(temp) < 0.5 ?
					floor(temp) : ceil(temp));
		}
		else
			return rint(eval(e->e.o.right));
 	case ROUND:
		{	int prec = (int) eval(e->e.o.right);
			double	scal = 1;
			if (0 < prec)
				do scal *= 10; while (0 < --prec);
			else if (prec < 0)
				do scal /= 10; while (++prec < 0);

			if (rndinfinity)
			{	double temp = eval(e->e.o.left);
				temp *= scal;
				temp = ((temp-floor(temp)) < 0.5 ?
						floor(temp) : ceil(temp));
				return(temp / scal);
			}
			else
				return(rint(eval(e->e.o.left) * scal) / scal);
		}
	case FV:
	case PV:
	case PMT:	return(finfunc(e->op,eval(e->e.o.left),
				   eval(e->e.o.right->e.o.left),
				      eval(e->e.o.right->e.o.right)));
	case HOUR:	return (dotime(HOUR, eval(e->e.o.right)));
	case MINUTE:	return (dotime(MINUTE, eval(e->e.o.right)));
	case SECOND:	return (dotime(SECOND, eval(e->e.o.right)));
	case MONTH:	return (dotime(MONTH, eval(e->e.o.right)));
	case DAY:	return (dotime(DAY, eval(e->e.o.right)));
	case YEAR:	return (dotime(YEAR, eval(e->e.o.right)));
	case NOW:	return (dotime(NOW, (double)0.0));
	case DTS:	return (dodts((int)eval(e->e.o.left),
				 (int)eval(e->e.o.right->e.o.left),
				 (int)eval(e->e.o.right->e.o.right)));
	case TTS:	return (dotts((int)eval(e->e.o.left),
				 (int)eval(e->e.o.right->e.o.left),
				 (int)eval(e->e.o.right->e.o.right)));
	case STON:	return (doston(seval(e->e.o.right)));
	case EQS:       return (doeqs(seval(e->e.o.right),seval(e->e.o.left)));
	case LMAX:	return dolmax(e);
	case LMIN:	return dolmin(e);
	case NVAL:      return (donval(seval(e->e.o.left),eval(e->e.o.right)));
	case MYROW:	return ((double) gmyrow);
	case MYCOL:	return ((double) gmycol);
	case NUMITER:	return ((double) repct);
	default:	error ("Illegal numeric expression");
			exprerr = 1;
    }
    cellerror = CELLERROR;
    return((double)0.0);
}

#ifdef SIGVOID
void
#else
int
#endif
eval_fpe() /* Trap for FPE errors in eval */
{
#if defined(i386) && !defined(M_XENIX)
	asm("	fnclex");
	asm("	fwait");
#else
#ifdef IEEE_MATH
	(void)fpsetsticky((fp_except)0);	/* Clear exception */
#endif /* IEEE_MATH */
#ifdef PC
	_fpreset();
#endif
#endif
	/* re-establish signal handler for next time */
	(void) signal(SIGFPE, eval_fpe);
	longjmp(fpe_save, 1);
}

double fn1_eval(fn, arg)
double (*fn)();
double arg;
{
	double res;
	errno = 0;
	res = (*fn)(arg);
	if(errno)
		cellerror = CELLERROR;

	return res;
}

double fn2_eval(fn, arg1, arg2)
double (*fn)();
double arg1, arg2;
{
	double res;
	errno = 0;
	res = (*fn)(arg1, arg2);
	if(errno)
		cellerror = CELLERROR;

	return res;
}

/* 
 * Rules for string functions:
 * Take string arguments which they scxfree.
 * All returned strings are assumed to be xalloced.
 */

char *
docat(s1, s2)
register char *s1, *s2;
{
    register char *p;
    char *arg1, *arg2;

    if (!s1 && !s2)
	return((char *)0);
    arg1 = s1 ? s1 : "";
    arg2 = s2 ? s2 : "";
    p = scxmalloc((unsigned)(strlen(arg1)+strlen(arg2)+1));
    (void) strcpy(p, arg1);
    (void) strcat(p, arg2);
    if (s1)
        scxfree(s1);
    if (s2)
        scxfree(s2);
    return(p);
}

char *
dodate(tloc)
long tloc;
{
    char *tp;
    char *p;

    tp = ctime(&tloc);
    tp[24] = '\0';
    p = scxmalloc((unsigned)25);
    (void) strcpy(p, tp);
    return(p);
}


char *
dofmt(fmtstr, v)
char *fmtstr;
double v;
{
    char buff[FBUFLEN];
    char *p;

    if (!fmtstr)
	return((char *)0);
    (void) sprintf(buff, fmtstr, v);
    p = scxmalloc((unsigned)(strlen(buff)+1));
    (void) strcpy(p, buff);
    scxfree(fmtstr);
    return(p);
}


/*
 * Given a command name and a value, run the command with the given value and
 * read and return its first output line (only) as an allocated string, always
 * a copy of prevstr, which is set appropriately first unless external
 * functions are disabled, in which case the previous value is used.  The
 * handling of prevstr and freeing of command is tricky.  Returning an
 * allocated string in all cases, even if null, insures cell expressions are
 * written to files, etc.
 */

#if defined(VMS) || defined(MSDOS)
char *
doext(command, value)
char *command;
double value;
{
    error("Warning: External functions unavailable on VMS");
    cellerror = CELLERROR;	/* not sure if this should be a cellerror */
    if (command)
	scxfree(command);
    return (strcpy (scxmalloc((unsigned) 1), "\0"));
}

#else /* VMS */

char *
doext (command, value)
char   *command;
double value;
{
    static char *prevstr = (char *)0;	/* previous result */
    static unsigned	prevlen = 0;
    char buff[FBUFLEN];		/* command line/return, not permanently alloc */
    extern char *strchr();

    if (!extfunc)    {
	error ("Warning: external functions disabled; using %s value",
		((prevstr == NULL) || (*prevstr == '\0')) ?
			"null" : "previous");

	if (command) scxfree (command);
    } else {
	if ((! command) || (! *command)) {
	    error ("Warning: external function given null command name");
	    cellerror = CELLERROR;
	    if (command) scxfree (command);
	} else {
	    FILE *pp;

	    (void) sprintf (buff, "%s %g", command, value); /* build cmd line */
	    scxfree (command);

	    error ("Running external function...");
	    (void) refresh();

	    if ((pp = popen (buff, "r")) == (FILE *) NULL) {	/* run it */
		error ("Warning: running \"%s\" failed", buff);
		cellerror = CELLERROR;
	    }
	    else {
		if (fgets (buff, sizeof(buff)-1, pp) == NULL)	/* one line */
		    error ("Warning: external function returned nothing");
		else {
		    char *cp;

		    error ("");				/* erase notice */
		    buff[sizeof(buff)-1] = '\0';

		    if (cp = strchr (buff, '\n'))	/* contains newline */
			*cp = '\0';			/* end string there */

		    if (strlen(buff) + 1 > prevlen)
		    {	prevlen = strlen(buff) + 40;
			prevstr = scxrealloc(prevstr, prevlen);
		    }
		    (void) strcpy (prevstr, buff);
			 /* save alloc'd copy */
		}
		(void) pclose (pp);

	    } /* else */
	} /* else */
    } /* else */
    if (prevstr)
	return (strcpy (scxmalloc ((unsigned) (strlen (prevstr) + 1)), prevstr));
    else
	return (strcpy(scxmalloc((unsigned)1), ""));
}

#endif /* VMS */


/*
 * Given a string representing a column name and a value which is a column
 * number, return the selected cell's string value, if any.  Even if none,
 * still allocate and return a null string so the cell has a label value so
 * the expression is saved in a file, etc.
 */

char *
dosval (colstr, rowdoub)
    char *colstr;
    double rowdoub;
{
    struct ent *ep;
    char *llabel;

    llabel = (ep = getent (colstr, rowdoub)) ? (ep -> label) : "";
    return (strcpy (scxmalloc ((unsigned) (strlen (llabel) + 1)), llabel));
}


/*
 * Substring:  Note that v1 and v2 are one-based to users, but zero-based
 * when calling this routine.
 */

char *
dosubstr(s, v1, v2)
char *s;
register int v1,v2;
{
    register char *s1, *s2;
    char *p;

    if (!s)
	return((char *)0);

    if (v2 >= strlen (s))		/* past end */
	v2 =  strlen (s) - 1;		/* to end   */

    if (v1 < 0 || v1 > v2) {		/* out of range, return null string */
	scxfree(s);
	p = scxmalloc((unsigned)1);
	p[0] = '\0';
	return(p);
    }
    s2 = p = scxmalloc((unsigned)(v2-v1+2));
    s1 = &s[v1];
    for(; v1 <= v2; s1++, s2++, v1++)
	*s2 = *s1;
    *s2 = '\0';
    scxfree(s);
    return(p);
}

/*
 * character casing: make upper case, make lower case
 */

char *
docase( acase, s)
int acase;
char *s;
{
    char *p = s;

    if (s == NULL)
	return(NULL);

    if( acase == UPPER ) {
        while( *p != '\0' ) {
           if( islower(*p) )
		*p = toupper(*p);
	   p++;
	}
    }
    else if ( acase == LOWER ) {
	while( *p != '\0' ) {
	    if (isupper(*p))
		*p = tolower(*p);
	    p++;
	}
    }
    return (s);
}

/*
 * make proper capitals of every word in a string
 * if the string has mixed case we say the string is lower
 *	and we will upcase only first letters of words
 * if the string is all upper we will lower rest of words.
 */

char *
docapital( s )
char *s;
{
    char *p;
    int skip = 1;
    int AllUpper = 1;

    if (s == NULL)
	return(NULL);
    for( p = s; *p != '\0' && AllUpper != 0; p++ )
	if( isalpha(*p) && islower(*p) )  AllUpper = 0;
    for (p = s; *p != '\0'; p++) {
	if (!isalnum(*p))
		skip = 1;
	else
	if (skip == 1) {
		skip = 0;
		if (islower(*p))
			*p = toupper(*p);
	}
	else	/* if the string was all upper before */
        if (isupper(*p) && AllUpper != 0)
		*p = tolower(*p);
    }
    return(s);
}

char *
seval(se)
register struct enode *se;
{
    register char *p;

    if (se == (struct enode *)0) return (char *)0;
    switch (se->op) {
	case O_SCONST: p = scxmalloc((unsigned)(strlen(se->e.s)+1));
		     (void) strcpy(p, se->e.s);
		     return(p);
	case O_VAR:    {
			struct ent *ep;
			ep = se->e.v.vp;

			if (!ep->label)
			    return((char *)0);
			p = scxmalloc((unsigned)(strlen(ep->label)+1));
			(void) strcpy(p, ep->label);
			return(p);
		     }
	case '#':    return(docat(seval(se->e.o.left), seval(se->e.o.right)));
	case 'f':    return(seval(se->e.o.right));
	case IF:
	case '?':    return(eval(se->e.o.left) ? seval(se->e.o.right->e.o.left)
					     : seval(se->e.o.right->e.o.right));
	case DATE:   return(dodate((long)(eval(se->e.o.right))));
	case FMT:    return(dofmt(seval(se->e.o.left), eval(se->e.o.right)));
	case UPPER:  return(docase(UPPER, seval(se->e.o.right)));
	case LOWER:  return(docase(LOWER, seval(se->e.o.right)));
	case CAPITAL:return(docapital(seval(se->e.o.right)));
 	case STINDEX:
 		{	register r,c;
 		register maxr, maxc;
 		register minr, minc;
 		maxr = se->e.o.right->e.r.right.vp -> row;
 		maxc = se->e.o.right->e.r.right.vp -> col;
 		minr = se->e.o.right->e.r.left.vp -> row;
 		minc = se->e.o.right->e.r.left.vp -> col;
 		if (minr>maxr) r = maxr, maxr = minr, minr = r;
 		if (minc>maxc) c = maxc, maxc = minc, minc = c;
 	        return dostindex(eval(se->e.o.left), minr, minc, maxr, maxc);
		}
	case EXT:    return(doext(seval(se->e.o.left), eval(se->e.o.right)));
	case SVAL:   return(dosval(seval(se->e.o.left), eval(se->e.o.right)));
	case SUBSTR: return(dosubstr(seval(se->e.o.left),
			    (int)eval(se->e.o.right->e.o.left) - 1,
			    (int)eval(se->e.o.right->e.o.right) - 1));
	case COLTOA: return(strcpy(scxmalloc((unsigned)10),
				   coltoa((int)eval(se->e.o.right)+1)));
	default:
		     error ("Illegal string expression");
		     exprerr = 1;
		     return(NULL);
	}
}

/*
 * The graph formed by cell expressions which use other cells's values is not
 * evaluated "bottom up".  The whole table is merely re-evaluated cell by cell,
 * top to bottom, left to right, in RealEvalAll().  Each cell's expression uses
 * constants in other cells.  However, RealEvalAll() notices when a cell gets a
 * new numeric or string value, and reports if this happens for any cell.
 * EvalAll() repeats calling RealEvalAll() until there are no changes or the
 * evaluation count expires.
 */

int propagation = 10;	/* max number of times to try calculation */
int repct = 1;		/* Make repct a global variable so that the 
				function @numiter can access it */

void
setiterations(i)
int i;
{
	if(i<1) {
		error("iteration count must be at least 1");
		propagation = 1;
		}
	else propagation = i;
}

void
EvalAll () {
     int lastcnt;
  
     repct = 1;
     (void) signal(SIGFPE, eval_fpe);

     while ((lastcnt = RealEvalAll()) && (++repct <= propagation));
     if((propagation>1)&& (lastcnt >0 ))
 	    error("Still changing after %d iterations",propagation-1);

    (void) signal(SIGFPE, doquit);
}

/*
 * Evaluate all cells which have expressions and alter their numeric or string
 * values.  Return the number of cells which changed.
 */

int 
RealEvalAll () {
    register int i,j;
    int chgct = 0;
    register struct ent *p;

    if(calc_order == BYROWS ) {
	for (i=0; i<=maxrow; i++)
	    for (j=0; j<=maxcol; j++)
		if ((p = *ATBL(tbl,i,j)) && !(p->flags&is_locked) && p->expr) RealEvalOne(p,i,j,&chgct);
    }
    else if ( calc_order == BYCOLS ) {
	for (j=0; j<=maxcol; j++)
	{   for (i=0; i<=maxrow; i++)
		if ((p = *ATBL(tbl,i,j)) && !(p->flags&is_locked) && p->expr) RealEvalOne(p,i,j,&chgct);
	}
    }
    else error("Internal error calc_order");
 
    return(chgct);
}

void
RealEvalOne(p, i, j, chgct)
register struct ent *p;
int i, j, *chgct;
{
	if (p->flags & is_strexpr) {
	    char *v;
	    if (setjmp(fpe_save)) {
		error("Floating point exception %s", v_name(i, j));
		cellerror = CELLERROR;
		v = "";
	    } else {
		cellerror = CELLOK;
		v = seval(p->expr);
	    }
	    p->cellerror = cellerror;
	    if (!v && !p->label) /* Everything's fine */
		return;
	    if (!p->label || !v || strcmp(v, p->label) != 0 || cellerror) {
		(*chgct)++;
		p->flags |= is_changed;
		changed++;
	    }
	    if(p->label)
		scxfree(p->label);
	    p->label = v;
	} else {
	    double v;
	    if (setjmp(fpe_save)) {
		error("Floating point exception %s", v_name(i, j));
		cellerror = CELLERROR;
		v = (double)0.0;
	    } else {
		cellerror = CELLOK;
		gmyrow=i; gmycol=j;
		v = eval (p->expr);
	    }
	    if ((p->cellerror = cellerror) || (v != p->v)) {
		p->v = v;
		if (!cellerror)		/* don't keep eval'ing a error */
			(*chgct)++;
		p->flags |= is_changed|is_valid;
		changed++;
	    }
	}
}

struct enode *
new(op, a1, a2)
int	op;
struct enode *a1, *a2;
{
    register struct enode *p;
    if (freeenodes)
    {	p = freeenodes;
	freeenodes = p->e.o.left;
    }
    else
	p = (struct enode *) scxmalloc ((unsigned)sizeof (struct enode));
    p->op = op;
    p->e.o.left = a1;
    p->e.o.right = a2;
    return p;
}

struct enode *
new_var(op, a1)
int	op;
struct ent_ptr a1;
{
    register struct enode *p;
    if (freeenodes)
    {	p = freeenodes;
	freeenodes = p->e.o.left;
    }
    else
	p = (struct enode *) scxmalloc ((unsigned)sizeof (struct enode));
    p->op = op;
    p->e.v = a1;
    return p;
}

struct enode *
new_range(op, a1)
int	op;
struct range_s a1;
{
    register struct enode *p;
    if (freeenodes)
    {	p = freeenodes;
	freeenodes = p->e.o.left;
    }
    else
	p = (struct enode *) scxmalloc ((unsigned)sizeof (struct enode));
    p->op = op;
    p->e.r = a1;
    return p;
}

struct enode *
new_const(op, a1)
int	op;
double a1;
{
    register struct enode *p;
    if (freeenodes)	/* reuse an already free'd enode */
    {	p = freeenodes;
	freeenodes = p->e.o.left;
    }
    else
	p = (struct enode *) scxmalloc ((unsigned)sizeof (struct enode));
    p->op = op;
    p->e.k = a1;
    return p;
}

struct enode *
new_str(s)
char *s;
{
    register struct enode *p;

    if (freeenodes)	/* reuse an already free'd enode */
    {	p = freeenodes;
	freeenodes = p->e.o.left;
    }
    else
	p = (struct enode *) scxmalloc ((unsigned)sizeof(struct enode));
    p->op = O_SCONST;
    p->e.s = s;
    return(p);
}

void
copy(dv1, dv2, v1, v2)
struct ent *dv1, *dv2, *v1, *v2;
{
    int minsr, minsc;
    int maxsr, maxsc;
    int mindr, mindc;
    int maxdr, maxdc;
    int vr, vc;
    int r, c;

    mindr = dv1->row;
    mindc = dv1->col;
    maxdr = dv2->row;
    maxdc = dv2->col;
    if (mindr>maxdr) r = maxdr, maxdr = mindr, mindr = r;
    if (mindc>maxdc) c = maxdc, maxdc = mindc, mindc = c;
    maxsr = v2->row;
    maxsc = v2->col;
    minsr = v1->row;
    minsc = v1->col;
    if (minsr>maxsr) r = maxsr, maxsr = minsr, minsr = r;
    if (minsc>maxsc) c = maxsc, maxsc = minsc, minsc = c;
    checkbounds(&maxdr, &maxdc);

    erase_area(mindr, mindc, maxdr, maxdc);
    if (minsr == maxsr && minsc == maxsc) {
	/* Source is a single cell */
	for(vr = mindr; vr <= maxdr; vr++)
	    for (vc = mindc; vc <= maxdc; vc++)
		copyrtv(vr, vc, minsr, minsc, maxsr, maxsc);
    } else if (minsr == maxsr) {
	/* Source is a single row */
	for (vr = mindr; vr <= maxdr; vr++)
	    copyrtv(vr, mindc, minsr, minsc, maxsr, maxsc);
    } else if (minsc == maxsc) {
	/* Source is a single column */
	for (vc = mindc; vc <= maxdc; vc++)
	    copyrtv(mindr, vc, minsr, minsc, maxsr, maxsc);
    } else {
	/* Everything else */
	copyrtv(mindr, mindc, minsr, minsc, maxsr, maxsc);
    }
    sync_refs();
}

void
copyrtv(vr, vc, minsr, minsc, maxsr, maxsc)
int vr, vc, minsr, minsc, maxsr, maxsc;
{
    register struct ent *p;
    register struct ent *n;
    register int sr, sc;
    register int dr, dc;

    for (dr=vr, sr=minsr; sr<=maxsr; sr++, dr++)
	for (dc=vc, sc=minsc; sc<=maxsc; sc++, dc++) {
	    if (p = *ATBL(tbl, sr, sc))
	    {	n = lookat (dr, dc);
		if (n->flags&is_locked) continue;
		(void) clearent(n);
		copyent( n, p, dr - sr, dc - sc);
	    }
	    else
	    if (n = *ATBL(tbl, dr, dc))
		(void) clearent(n);
	}
}

/* ERASE a Range of cells */
void
eraser(v1, v2)
struct ent *v1, *v2;
{
	FullUpdate++;
	flush_saved();
	erase_area(v1->row, v1->col, v2->row, v2->col);
	sync_refs();
}

/* Goto subroutines */

void
g_free()
{
    switch (gs.g_type) {
    case G_STR: scxfree(gs.g_s); break;
    default: break;
    }
    gs.g_type = G_NONE;
    gs.errsearch = 0;
}

/* repeat the last goto command */
void
go_last()
{
    switch (gs.g_type) {
    case G_NONE:
		error("Nothing to repeat"); break;
    case G_NUM:
		num_search(gs.g_n, gs.errsearch);
		break;
    case  G_CELL:
		moveto(gs.g_row, gs.g_col);
	    	break;
    case  G_STR: 
		gs.g_type = G_NONE;	/* Don't free the string */
   	    	str_search(gs.g_s); 
	   	break;

    default: error("go_last: internal error");
    }
}

/* place the cursor on a given cell */
void
moveto(row, col)
int row, col;
{
    currow = row;
    curcol = col;
    g_free();
    gs.g_type = G_CELL;
    gs.g_row = currow;
    gs.g_col = curcol;
}

/*
 * 'goto' either a given number,'error', or 'invalid' starting at currow,curcol
 */
void
num_search(n, errsearch)
double n;
int	errsearch;
{
    register struct ent *p;
    register int r,c;
    int	endr, endc;

    g_free();
    gs.g_type = G_NUM;
    gs.g_n = n;
    gs.errsearch = errsearch;

    if (currow > maxrow)
	endr = maxrow ? maxrow-1 : 0;
    else
	endr = currow;
    if (curcol > maxcol)
	endc = maxcol ? maxcol-1 : 0;
    else
	endc = curcol;
    r = endr;
    c = endc;
    do {
	if (c < maxcol)
	    c++;
	else {
	    if (r < maxrow) {
		while(++r < maxrow && row_hidden[r]) /* */;
		c = 0;
	    } else {
		r = 0;
		c = 0;
	    }
	}
	if (r == endr && c == endc) {
	    if (errsearch)
		error("no %s cell found", errsearch == CELLERROR ? "ERROR" :
		      "INVALID");
	    else
		error("Number not found");
	    return;
	}
	p = *ATBL(tbl, r, c);
    } while (col_hidden[c] || !p || !(p->flags & is_valid)
	|| (!errsearch && (p->v != n))
	|| (errsearch && !((p->cellerror == errsearch) ||
		(p->cellerror == errsearch))));	/* CELLERROR vs CELLINVALID */
		
    currow = r;
    curcol = c;
}

/* 'goto' a cell containing a matching string */
void
str_search(s)
char *s;
{
    register struct ent *p;
    register int r,c;
    int	endr, endc;
    char *tmp;

#if defined(RE_COMP)
    if ((tmp = re_comp(s)) != (char *)0) {
	scxfree(s);
	error(tmp);
	return;
    }
#endif
#if defined(REGCMP)
    if ((tmp = regcmp(s, (char *)0)) == (char *)0) {
	scxfree(s);
	cellerror = CELLERROR;
	error("Invalid search string");
	return;
    }
#endif
    g_free();
    gs.g_type = G_STR;
    gs.g_s = s;
    if (currow > maxrow)
	endr = maxrow ? maxrow-1 : 0;
    else
	endr = currow;
    if (curcol > maxcol)
	endc = maxcol ? maxcol-1 : 0;
    else
	endc = curcol;
    r = endr;
    c = endc;
    do {
	if (c < maxcol)
	    c++;
	else {
	    if (r < maxrow) {
		while(++r < maxrow && row_hidden[r]) /* */;
		c = 0;
	    } else {
		r = 0;
		c = 0;
	    }
	}
	if (r == endr && c == endc) {
	    error("String not found");
#if defined(REGCMP)
	    free(tmp);
#endif
	    return;
	}
	p = *ATBL(tbl, r, c);
    } while(col_hidden[c] || !p || !(p->label)
#if defined(RE_COMP)
		  			|| (re_exec(p->label) == 0));
#else
#if defined(REGCMP)
				|| (regex(tmp, p->label) == (char *)0));
#else
					|| (strcmp(s, p->label) != 0));
#endif
#endif
    currow = r;
    curcol = c;
#if defined(REGCMP)
    free(tmp);
#endif
}

/* fill a range with constants */
void
fill (v1, v2, start, inc)
struct ent *v1, *v2;
double start, inc;
{
    register r,c;
    register struct ent *n;
    int maxr, maxc;
    int minr, minc;

    maxr = v2->row;
    maxc = v2->col;
    minr = v1->row;
    minc = v1->col;
    if (minr>maxr) r = maxr, maxr = minr, minr = r;
    if (minc>maxc) c = maxc, maxc = minc, minc = c;
    checkbounds(&maxr, &maxc);
    if (minr < 0) minr = 0;
    if (minc < 0) minc = 0;

    FullUpdate++;
    if( calc_order == BYROWS ) {
    for (r = minr; r<=maxr; r++)
	for (c = minc; c<=maxc; c++) {
	    n = lookat (r, c);
	    if (n->flags&is_locked) continue;
	    (void) clearent(n);
	    n->v = start;
	    start += inc;
	    n->flags |= (is_changed|is_valid);
	}
    }
    else if ( calc_order == BYCOLS ) {
    for (c = minc; c<=maxc; c++)
	for (r = minr; r<=maxr; r++) {
	    n = lookat (r, c);
	    (void) clearent(n);
	    n->v = start;
	    start += inc;
	    n->flags |= (is_changed|is_valid);
	}
    }
    else error(" Internal error calc_order");
    changed++;
}

/* lock a range of cells */

void
lock_cells (v1, v2)
struct ent *v1, *v2;
{
    register r,c;
    register struct ent *n;
    int maxr, maxc;
    int minr, minc;

    maxr = v2->row;
    maxc = v2->col;
    minr = v1->row;
    minc = v1->col;
    if (minr>maxr) r = maxr, maxr = minr, minr = r;
    if (minc>maxc) c = maxc, maxc = minc, minc = c;
    checkbounds(&maxr, &maxc);
    if (minr < 0) minr = 0;
    if (minc < 0) minc = 0;

    for (r = minr; r<=maxr; r++)
	for (c = minc; c<=maxc; c++) {
	    n = lookat (r, c);
	    n->flags |= is_locked;
	}
}

/* unlock a range of cells */

void
unlock_cells (v1, v2)
struct ent *v1, *v2;
{
    register r,c;
    register struct ent *n;
    int maxr, maxc;
    int minr, minc;

    maxr = v2->row;
    maxc = v2->col;
    minr = v1->row;
    minc = v1->col;
    if (minr>maxr) r = maxr, maxr = minr, minr = r;
    if (minc>maxc) c = maxc, maxc = minc, minc = c;
    checkbounds(&maxr, &maxc);
    if (minr < 0) minr = 0;
    if (minc < 0) minc = 0;

    for (r = minr; r<=maxr; r++)
	for (c = minc; c<=maxc; c++) {
	    n = lookat (r, c);
	    n->flags &= ~is_locked;
	}
}

/* set the numeric part of a cell */
void
let (v, e)
struct ent *v;
struct enode *e;
{
    double val;
    unsigned isconstant = constant(e);

    if (loading && !isconstant)
	val = (double)0.0;
    else
    {
	exprerr = 0;
	(void) signal(SIGFPE, eval_fpe);
	if (setjmp(fpe_save)) {
	    error ("Floating point exception in cell %s", v_name(v->row, v->col));
	    val = (double)0.0;
	    cellerror = CELLERROR;
	} else {
	    cellerror = CELLOK;
	    val = eval(e);
	}
	if (v->cellerror != cellerror)
	{	v->flags |= is_changed;
		changed++;	modflg++;
		FullUpdate++;
		v->cellerror = cellerror;
	}
	(void) signal(SIGFPE, doquit);
	if (exprerr) {
	    efree(e);
	    return;
	}
    }

    if (isconstant) {
	/* prescale input unless it has a decimal */
#if defined(IEEE_MATH) && !defined(NO_FMOD)
	if (!loading && (prescale < (double)0.9999999) &&
				(fmod(val, (double)1.0) == (double)0))
#else
	if (!loading && (prescale < (double)0.9999999) &&
			((val - floor(val)) == (double)0))
#endif
	    val *= prescale;

	v->v = val;

	if (!(v->flags & is_strexpr)) {
            efree(v->expr);
	    v->expr = (struct enode *)0;
	}
	efree(e);
    }
    else
    {
	efree(v->expr);
	v->expr = e;
	v->flags &= ~is_strexpr;
    }

    changed++; modflg++;
    v->flags |= (is_changed|is_valid);
}

void
slet (v, se, flushdir)
struct ent *v;
struct enode *se;
int flushdir;
{
    char *p;

    exprerr = 0;
    (void) signal(SIGFPE, eval_fpe);
    if (setjmp(fpe_save)) {
	error ("Floating point exception in cell %s", v_name(v->row, v->col));
	cellerror = CELLERROR;
	p = "";
    } else {
	cellerror = CELLOK;
	p = seval(se);
    }
    if (v->cellerror != cellerror)
    {	v->flags |= is_changed;
	changed++;	modflg++;
	FullUpdate++;
	v->cellerror = cellerror;
    }
    (void) signal(SIGFPE, doquit);
    if (exprerr) {
	efree(se);
	return;
    }
    if (constant(se)) {
	label(v, p, flushdir);
	if (p)
	    scxfree(p);
	efree(se);
	if (v->flags & is_strexpr) {
            efree(v->expr);
	    v->expr = (struct enode *)0;
	    v->flags &= ~is_strexpr;
	}
	return;
    }
    efree(v->expr);
    v->expr = se;
    v->flags |= (is_changed|is_strexpr);
    if (flushdir<0) v->flags |= is_leftflush;

    if (flushdir==0)
	v->flags |= is_label;
    else v->flags &= ~is_label;

    FullUpdate++;
    changed++;
    modflg++;
}

void
format_cell(v1, v2, s)
struct ent *v1, *v2;
char *s;
{
    register r,c;
    register struct ent *n;
    int maxr, maxc;
    int minr, minc;

    maxr = v2->row;
    maxc = v2->col;
    minr = v1->row;
    minc = v1->col;
    if (minr>maxr) r = maxr, maxr = minr, minr = r;
    if (minc>maxc) c = maxc, maxc = minc, minc = c;
    checkbounds(&maxr, &maxc);
    if (minr < 0) minr = 0;
    if (minc < 0) minc = 0;

    FullUpdate++;
	modflg++;
    for (r = minr; r <= maxr; r++)
	for (c = minc; c <= maxc; c++) {
	    n = lookat (r, c);
	    if (n->flags&is_locked) {
		error("Cell %s%d is locked", coltoa(n->col), n->row);
		continue;
	    }
	    if (n->format)
		scxfree(n->format);
	    n->format = 0;
	    if (s && *s != '\0')
		n->format = strcpy(scxmalloc((unsigned)(strlen(s)+1)), s);
	    n->flags |= is_changed;
       }
}

void
hide_row(arg)
int arg;
{
    if (arg < 0) {
	error("Invalid Range");
	return;
    }
    if (arg >= maxrows-1)
    {
	if (!growtbl(GROWROW, arg+1, 0))
	{	error("You can't hide the last row");
		return;
	}
    }
    FullUpdate++;
    row_hidden[arg] = 1;
}

void
hide_col(arg)
int arg;
{
    if (arg < 0) {
	error("Invalid Range");
	return;
    }
    if (arg >= maxcols-1)
    {	if ((arg >= ABSMAXCOLS-1) || !growtbl(GROWCOL, 0, arg+1))
	{	error("You can't hide the last col");
		return;
	}
    }
    FullUpdate++;
    col_hidden[arg] = TRUE;
}

void
clearent (v)
struct ent *v;
{
    if (!v)
	return;
    label(v,"",-1);
    v->v = (double)0;
    if (v->expr)
	efree(v->expr);
    v->expr = (struct enode *)0;
    if (v->format)
	scxfree(v->format);
    v->format = (char *)0;
    v->flags |= (is_changed);
    v->flags &= ~(is_valid);
    changed++;
    modflg++;
}

/*
 * Say if an expression is a constant (return 1) or not.
 */
int
constant (e)
    register struct enode *e;
{
    return (
	 e == (struct enode *)0
	 || e -> op == O_CONST
	 || e -> op == O_SCONST
	 || (
	     e -> op != O_VAR
	     && (e -> op & REDUCE) != REDUCE
	     && constant (e -> e.o.left)
	     && constant (e -> e.o.right)
	     && e -> op != EXT	 /* functions look like constants but aren't */
	     && e -> op != NVAL
	     && e -> op != SVAL
	     && e -> op != NOW
	     && e -> op != MYROW
	     && e -> op != MYCOL
	     && e -> op != NUMITER
	)
    );
}

void
efree (e)
struct enode *e;
{
    if (e) {
	if (e->op != O_VAR && e->op !=O_CONST && e->op != O_SCONST
		&& (e->op & REDUCE) != REDUCE) {
	    efree(e->e.o.left);
	    efree(e->e.o.right);
	}
	if (e->op == O_SCONST && e->e.s)
	    scxfree(e->e.s);
	e->e.o.left = freeenodes;
	freeenodes = e;
    }
}

void
label (v, s, flushdir)
register struct ent *v;
register char *s;
int	flushdir;
{
    if (v) {
	if (flushdir==0 && v->flags&is_valid) {
	    register struct ent *tv;
	    if (v->col>0 && ((tv=lookat(v->row,v->col-1))->flags&is_valid)==0)
		v = tv, flushdir = 1;
	    else if (((tv=lookat (v->row,v->col+1))->flags&is_valid)==0)
		v = tv, flushdir = -1;
	    else flushdir = -1;
	}
	if (v->label) scxfree((char *)(v->label));
	if (s && s[0]) {
	    v->label = scxmalloc ((unsigned)(strlen(s)+1));
	    (void) strcpy (v->label, s);
	} else
	    v->label = (char *)0;
	if (flushdir<0) v->flags |= is_leftflush;
	else v->flags &= ~is_leftflush;
	if (flushdir==0) v->flags |= is_label;
	else v->flags &= ~is_label;
	FullUpdate++;
	modflg++;
    }
}

void
decodev (v)
struct ent_ptr v; 
{
	register struct range *r;

	if (!v.vp) (void) sprintf (line+linelim,"VAR?");
	else if ((r = find_range((char *)0, 0, v.vp, v.vp)) && !r->r_is_range)
	    (void) sprintf(line+linelim, "%s", r->r_name);
	else
	    (void) sprintf (line+linelim, "%s%s%s%d",
			v.vf & FIX_COL ? "$" : "",
			coltoa(v.vp->col),
			v.vf & FIX_ROW ? "$" : "",
			v.vp->row);
	linelim += strlen (line+linelim);
}

char *
coltoa(col)
int col;
{
    static char rname[3];
    register char *p = rname;

    if (col > 25) {
	*p++ = col/26 + 'A' - 1;
	col %= 26;
    }
    *p++ = col+'A';
    *p = '\0';
    return(rname);
}

/*
 *	To make list elements come out in the same order
 *	they were entered, we must do a depth-first eval
 *	of the ELIST tree
 */
static void
decompile_list(p)
struct enode *p;
{
	if (!p) return;
	decompile_list(p->e.o.left);	/* depth first */
        decompile(p->e.o.right, 0);
	line[linelim++] = ',';
}

void
decompile(e, priority)
register struct enode *e;
int	priority;
{
    register char *s;
    if (e) {
	int mypriority;
	switch (e->op) {
	default: mypriority = 99; break;
	case '?': mypriority = 1; break;
	case ':': mypriority = 2; break;
	case '|': mypriority = 3; break;
	case '&': mypriority = 4; break;
	case '<': case '=': case '>': mypriority = 6; break;
	case '+': case '-': case '#': mypriority = 8; break;
	case '*': case '/': case '%': mypriority = 10; break;
	case '^': mypriority = 12; break;
	}
	if (mypriority<priority) line[linelim++] = '(';
	switch (e->op) {
	case 'f':	for (s="fixed "; line[linelim++] = *s++;);
			linelim--;
			decompile (e->e.o.right, 30);
			break;
	case 'm':	line[linelim++] = '-';
			decompile (e->e.o.right, 30);
			break;
	case '~':	line[linelim++] = '~';
			decompile (e->e.o.right, 30);
			break;
	case O_VAR:	decodev (e->e.v);
			break;
	case O_CONST:	(void) sprintf (line+linelim,"%.15g",e->e.k);
			linelim += strlen (line+linelim);
			break;
	case O_SCONST:	(void) sprintf (line+linelim, "\"%s\"", e->e.s);
			linelim += strlen(line+linelim);
			break;

	case REDUCE | '+': range_arg( "@sum(", e); break;
	case REDUCE | '*': range_arg( "@prod(", e); break;
	case REDUCE | 'a': range_arg( "@avg(", e); break;
	case REDUCE | 'c': range_arg( "@count(", e); break;
	case REDUCE | 's': range_arg( "@stddev(", e); break;
	case REDUCE | MAX: range_arg( "@max(", e); break;
	case REDUCE | MIN: range_arg( "@min(", e); break;

	case ABS:		one_arg( "@abs(", e); break;
	case ACOS:	one_arg( "@acos(", e); break;
	case ASIN:	one_arg( "@asin(", e); break;
	case ATAN:	one_arg( "@atan(", e); break;
	case ATAN2:	two_arg( "@atan2(", e); break;
	case CEIL:	one_arg( "@ceil(", e); break;
	case COS:	one_arg( "@cos(", e); break;
	case EXP:	one_arg( "@exp(", e); break;
	case FABS:	one_arg( "@fabs(", e); break;
	case FLOOR:	one_arg( "@floor(", e); break;
	case HYPOT:	two_arg( "@hypot(", e); break;
	case LOG:	one_arg( "@ln(", e); break;
	case LOG10:	one_arg( "@log(", e); break;
	case POW:	two_arg( "@pow(", e); break;
	case SIN:	one_arg( "@sin(", e); break;
	case SQRT:	one_arg( "@sqrt(", e); break;
	case TAN:	one_arg( "@tan(", e); break;
	case DTR:	one_arg( "@dtr(", e); break;
	case RTD:	one_arg( "@rtd(", e); break;
	case RND:	one_arg( "@rnd(", e); break;
	case ROUND:	two_arg( "@round(", e); break;
	case HOUR:	one_arg( "@hour(", e); break;
	case MINUTE:	one_arg( "@minute(", e); break;
	case SECOND:	one_arg( "@second(", e); break;
	case MONTH:	one_arg( "@month(", e); break;
	case DAY:	one_arg( "@day(", e); break;
	case YEAR:	one_arg( "@year(", e); break;
	case DATE:	one_arg( "@date(", e); break;
	case UPPER:	one_arg( "@upper(", e); break;
	case LOWER:	one_arg( "@lower(", e); break;
	case CAPITAL:	one_arg( "@capital(", e); break;
	case DTS:	three_arg( "@dts(", e); break;
	case TTS:	three_arg( "@tts(", e); break;
	case STON:	one_arg( "@ston(", e); break;
	case FMT:	two_arg( "@fmt(", e); break;
	case EQS:	two_arg( "@eqs(", e); break;
	case NOW:	for ( s = "@now"; line[linelim++] = *s++;);
			linelim--;
			break;
	case LMAX:	list_arg("@max(", e); break;
	case LMIN: 	list_arg("@min(", e); break;
	case FV:	three_arg("@fv(", e); break;
	case PV:	three_arg("@pv(", e); break;
	case PMT:	three_arg("@pmt(", e); break;
	case NVAL:	two_arg("@nval(", e); break;
	case SVAL:	two_arg("@sval(", e); break;
	case EXT:	two_arg("@ext(", e); break;
	case SUBSTR:	three_arg("@substr(", e); break;
	case STINDEX:	index_arg("@stindex(", e); break;
	case INDEX:	index_arg("@index(", e); break;
	case LOOKUP:	index_arg("@lookup(", e); break;
	case HLOOKUP:	two_arg_index("@hlookup(", e); break;
	case VLOOKUP:	two_arg_index("@vlookup(", e); break;
	case IF:	three_arg("@if(", e); break;
	case MYROW:	for ( s = "@myrow"; line[linelim++] = *s++;);
			linelim--;
			break;
	case MYCOL:	for ( s = "@mycol"; line[linelim++] = *s++;);
			linelim--;
			break;
	case COLTOA:	one_arg( "@coltoa(", e); break;
	case NUMITER:	for ( s = "@numiter"; line[linelim++] = *s++;);
                        linelim--;
                        break;
	default:	decompile (e->e.o.left, mypriority);
			line[linelim++] = e->op;
			decompile (e->e.o.right, mypriority+1);
			break;
	}
	if (mypriority<priority) line[linelim++] = ')';
    } else line[linelim++] = '?';
}

void
index_arg(s, e)
char *s;
struct enode *e;
{
    for (; line[linelim++] = *s++;);
    linelim--;
    decompile( e-> e.o.left, 0 );
    range_arg(", ", e->e.o.right);
}

void
two_arg_index(s, e)
char *s;
struct enode *e;
{
    for (; line[linelim++] = *s++;);
    linelim--;
    decompile( e->e.o.left->e.o.left, 0 );
    range_arg(",", e->e.o.right);
    linelim--;
    line[linelim++] = ',';
    decompile( e->e.o.left->e.o.right, 0 );
    line[linelim++] = ')';
}

void
list_arg(s, e)
char *s;
struct enode *e;
{
    for (; line[linelim++] = *s++;);
    linelim--;

    decompile (e->e.o.right, 0);
    line[linelim++] = ',';
    decompile_list(e->e.o.left);
    line[linelim - 1] = ')';
}

void
one_arg(s, e)
char *s;
struct enode *e;
{
    for (; line[linelim++] = *s++;);
    linelim--;
    decompile (e->e.o.right, 0);
    line[linelim++] = ')';
}

void
two_arg(s,e)
char *s;
struct enode *e;
{
    for (; line[linelim++] = *s++;);
    linelim--;
    decompile (e->e.o.left, 0);
    line[linelim++] = ',';
    decompile (e->e.o.right, 0);
    line[linelim++] = ')';
}

void
three_arg(s,e)
char *s;
struct enode *e;
{
    for (; line[linelim++] = *s++;);
    linelim--;
    decompile (e->e.o.left, 0);
    line[linelim++] = ',';
    decompile (e->e.o.right->e.o.left, 0);
    line[linelim++] = ',';
    decompile (e->e.o.right->e.o.right, 0);
    line[linelim++] = ')';
}

void
range_arg(s,e)
char *s;
struct enode *e;
{
    struct range *r;

    for (; line[linelim++] = *s++;);
    linelim--;
    if ((r = find_range((char *)0, 0, e->e.r.left.vp,
			     e->e.r.right.vp)) && r->r_is_range) {
	(void) sprintf(line+linelim, "%s", r->r_name);
	linelim += strlen(line+linelim);
    } else {
	decodev (e->e.r.left);
	line[linelim++] = ':';
	decodev (e->e.r.right);
    }
    line[linelim++] = ')';
}

void
editfmt (row, col)
int row, col;
{
    register struct ent *p;

    p = lookat (row, col);
    if (p->format) {
        (void) sprintf (line, "fmt %s \"%s\"", v_name(row, col), p->format);
	linelim = strlen(line);
    }
}

void
editv (row, col)
int row, col;
{
    register struct ent *p;

    p = lookat (row, col);
    (void) sprintf (line, "let %s = ", v_name(row, col));
    linelim = strlen(line);
    if (p->flags & is_strexpr || p->expr == 0) {
	(void) sprintf (line+linelim, "%.15g", p->v);
	linelim += strlen (line+linelim);
    } else {
        editexp(row,col);
    }
}

void
editexp(row,col)
int row, col;
{
    register struct ent *p;

    p = lookat (row, col);
    decompile (p->expr, 0);
    line[linelim] = '\0';
}

void
edits (row, col)
int row, col;
{
    register struct ent *p;

    p = lookat (row, col);
    if( p->flags&is_label )
	(void) sprintf( line, "label %s = ", v_name(row, col));
    else
    (void) sprintf (line, "%sstring %s = ",
			((p->flags&is_leftflush) ? "left" : "right"),
			v_name(row, col));
    linelim = strlen(line);
    if (p->flags & is_strexpr && p->expr) {
	editexp(row, col);
    } else if (p->label) {
        (void) sprintf (line+linelim, "\"%s\"", p->label);
        linelim += strlen (line+linelim);
    } else {
        (void) sprintf (line+linelim, "\"");
        linelim += 1;
    }
}

#ifdef RINT
/*	round-to-even, also known as ``banker's rounding''.
	With round-to-even, a number exactly halfway between two values is
	rounded to whichever is even; e.g. rnd(0.5)=0, rnd(1.5)=2,
	rnd(2.5)=2, rnd(3.5)=4.  This is the default rounding mode for
	IEEE floating point, for good reason: it has better numeric
	properties.  For example, if X+Y is an integer,
	then X+Y = rnd(X)+rnd(Y) with round-to-even,
	but not always with sc's rounding (which is
	round-to-positive-infinity).  I ran into this problem when trying to
	split interest in an account to two people fairly.
*/
double rint(d) double d;
{
	/* as sent */
	double fl = floor(d),  fr = d-fl;
	return
	    fr<0.5  ||  fr==0.5 && fl==floor(fl/2)*2   ?   fl   :   ceil(d);
}
#endif
