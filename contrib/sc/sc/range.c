
/*	SC	A Spreadsheet Calculator
 *		Range Manipulations
 *
 *              Robert Bond, 4/87
 *
 *		$Revision: 1.1.1.1 $
 */

#include <sys/types.h>
#ifdef BSD42
#include <strings.h>
#else
#ifndef SYSIII
#include <string.h>
#endif
#endif

#include <stdio.h>
#include <curses.h>
#include <ctype.h>
#include "sc.h"

static struct range *rng_base;

void
add_range(name, left, right, is_range)
char *name;
struct ent_ptr left, right;
int is_range;
{
    struct range *r;
    register char *p;
    int len;
    int minr,minc,maxr,maxc;
    int minrf, mincf, maxrf, maxcf;
    register struct ent *rcp;

    if (left.vp->row < right.vp->row) {
	minr = left.vp->row; minrf = left.vf & FIX_ROW;
	maxr = right.vp->row; maxrf = right.vf & FIX_ROW;
    } else {
	minr = right.vp->row; minrf = right.vf & FIX_ROW;
	maxr = left.vp->row; maxrf = right.vf & FIX_ROW;
    } 

    if (left.vp->col < right.vp->col) {
	minc = left.vp->col; mincf = left.vf & FIX_COL;
	maxc = right.vp->col; maxcf = right.vf & FIX_COL;
    } else {
	minc = right.vp->col; mincf = right.vf & FIX_COL;
	maxc = left.vp->col; maxcf = left.vf & FIX_COL;
    } 

    left.vp = lookat(minr, minc);
    left.vf = minrf | mincf;
    right.vp = lookat(maxr, maxc);
    right.vf = maxrf | maxcf;

    if (find_range(name, strlen(name), (struct ent *)0, (struct ent *)0)) {
	error("Error: range name already defined");
	scxfree(name);
	return;
    }

    if (strlen(name) <= 2) {
	error("Invalid range name - too short");
	scxfree(name);
	return;
    }

    for(p=name, len=0; *p; p++, len++)
	if (!((isalpha(*p) && (len<=2)) ||
	    ((isdigit(*p) || isalpha(*p) || (*p == '_')) && (len>2)))) {
	    error("Invalid range name - illegal combination");
	    scxfree(name);
	    return;
	}
 
    if(autolabel && minc>0 && !is_range) {
	rcp = lookat(minr,minc-1);
	if (rcp->label==0 && rcp->expr==0 && rcp->v==0)
		label(rcp, name, 0);
    }

    r = (struct range *)scxmalloc((unsigned)sizeof(struct range));
    r->r_name = name;
    r->r_left = left;
    r->r_right = right;
    r->r_next = rng_base;
    r->r_prev = (struct range *)0;
    r->r_is_range = is_range;
    if (rng_base)
        rng_base->r_prev = r;
    rng_base = r;
}

void
del_range(left, right)
struct ent *left, *right;
{
    register struct range *r;
    int minr,minc,maxr,maxc;

    minr = left->row < right->row ? left->row : right->row;
    minc = left->col < right->col ? left->col : right->col;
    maxr = left->row > right->row ? left->row : right->row;
    maxc = left->col > right->col ? left->col : right->col;

    left = lookat(minr, minc);
    right = lookat(maxr, maxc);

    if (!(r = find_range((char *)0, 0, left, right))) 
	return;

    if (r->r_next)
        r->r_next->r_prev = r->r_prev;
    if (r->r_prev)
        r->r_prev->r_next = r->r_next;
    else
	rng_base = r->r_next;
    scxfree((char *)(r->r_name));
    scxfree((char *)r);
}

void
clean_range()
{
    register struct range *r;
    register struct range *nextr;

    r = rng_base;
    rng_base = (struct range *)0;

    while (r) {
	nextr = r->r_next;
	scxfree((char *)(r->r_name));
	scxfree((char *)r);
	r = nextr;
    }
}

/* Match on name or lmatch, rmatch */

struct range *
find_range(name, len, lmatch, rmatch)
char *name;
int len;
struct ent *lmatch;
struct ent *rmatch;
{
    struct range *r;
    register char *rp, *np;
    register int c;

    if (name) {
	for (r = rng_base; r; r = r->r_next) {
	    for (np = name, rp = r->r_name, c = len;
		 c && *rp && (*rp == *np);
		 rp++, np++, c--) /* */;
	    if (!c && !*rp)
		return(r);
	}
	return((struct range *)0);
    }

    for (r = rng_base; r; r= r->r_next) {
	if ((lmatch == r->r_left.vp) && (rmatch == r->r_right.vp)) 
	    return(r);
    }
    return((struct range *)0);
}

void
sync_ranges()
{
    register struct range *r;

    r = rng_base;
    while(r) {
	r->r_left.vp = lookat(r->r_left.vp->row, r->r_left.vp->col);
	r->r_right.vp = lookat(r->r_right.vp->row, r->r_right.vp->col);
	r = r->r_next;
    }
}

void
write_range(f)
FILE *f;
{
    register struct range *r;

    for (r = rng_base; r; r = r->r_next) {
	(void) fprintf(f, "define \"%s\" %s%s%s%d",
			r->r_name,
			r->r_left.vf & FIX_COL ? "$":"",
			coltoa(r->r_left.vp->col), 
			r->r_left.vf & FIX_ROW ? "$":"",
			r->r_left.vp->row);
	if (r->r_is_range)
	    (void) fprintf(f, ":%s%s%s%d\n",
			    r->r_right.vf & FIX_COL ? "$":"",
			    coltoa(r->r_right.vp->col), 
			    r->r_right.vf & FIX_ROW ? "$":"",
			    r->r_right.vp->row);
	else
	    (void) fprintf(f, "\n");
    }
}

void
list_range(f)
FILE *f;
{
    register struct range *r;

    (void) fprintf(f, "%-30s %s\n\n","Name","Definition");

    for (r = rng_base; r; r = r->r_next) {
	(void) fprintf(f, "%-30s %s%s%s%d",
			    r->r_name,
			    r->r_left.vf & FIX_COL ? "$":"",
			    coltoa(r->r_left.vp->col), 
			    r->r_left.vf & FIX_ROW ? "$":"",
			    r->r_left.vp->row);
	if (r->r_is_range)
	    (void) fprintf(f, ":%s%s%s%d\n",
			    r->r_right.vf & FIX_COL ? "$":"",
			    coltoa(r->r_right.vp->col), 
			    r->r_right.vf & FIX_ROW ? "$":"",
			    r->r_right.vp->row);
	else
	    (void) fprintf(f, "\n");
    }
}

char *
v_name(row, col)
int row, col;
{
    struct ent *v;
    struct range *r;
    static char buf[20];

    v = lookat(row, col);
    if (r = find_range((char *)0, 0, v, v)) {
	return(r->r_name);
    } else {
        (void) sprintf(buf, "%s%d", coltoa(col), row);
	return(buf);
    }
}

char *
r_name(r1, c1, r2, c2)
int r1, c1, r2, c2;
{
    struct ent *v1, *v2;
    struct range *r;
    static char buf[100];

    v1 = lookat(r1, c1);
    v2 = lookat(r2, c2);
    if (r = find_range((char *)0, 0, v1, v2)) {
	return(r->r_name);
    } else {
        (void) sprintf(buf, "%s", v_name(r1, c1));
	(void) sprintf(buf+strlen(buf), ":%s", v_name(r2, c2));
	return(buf);
    }
}

int
are_ranges()
{
	return (rng_base != 0);
}
