/* $Header: /bsdi/MASTER/BSDI_OS/contrib/psgraph/psgraph/grid.c,v 1.1.1.1 1994/01/03 23:14:15 polk Exp $ */

/*
 *               Copyright 1989, 1992 Digital Equipment Corporation
 *                          All Rights Reserved
 * 
 * 
 * Permission to use, copy, and modify this software and its documentation
 * is hereby granted only under the following terms and conditions.  Both
 * the above copyright notice and this permission notice must appear in
 * all copies of the software, derivative works or modified versions, and
 * any portions threof, and both notices must appear in supporting
 * documentation.
 * 
 * Users of this software agree to the terms and conditions set forth
 * herein, and hereby grant back to Digital a non-exclusive, unrestricted,
 * royalty-free right and license under any changes, enhancements or
 * extensions made to the core functions of the software, including but
 * not limited to those affording compatibility with other hardware or
 * software environments, but excluding applications which incorporate
 * this software.  Users further agree to use their best efforts to return
 * to Digital any such changes, enhancements or extensions that they make
 * and inform Digital of noteworthy uses of this software.  Correspondence
 * should be provided to Digital at:
 * 
 *                       Director of Licensing
 *                       Western Research Laboratory
 *                       Digital Equipment Corporation
 *                       250 University Avenue
 *                       Palo Alto, California  94301  
 * 
 * This software may be distributed (but not offered for sale or
 * transferred for compensation) to third parties, provided such third
 * parties agree to abide by the terms and conditions of this notice.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL
 * EQUIPMENT CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* 
 * grid.c - Compute and generate grids and axes
 * 
 * Author:	Christopher A. Kent
 * 		Western Research Laboratory
 * 		Digital Equipment Corporation
 * Date:	Wed Jan  4 1989
 */

/*
 * $Log: grid.c,v $
 * Revision 1.1.1.1  1994/01/03  23:14:15  polk
 * New contrib utilities
 *
 * Revision 1.13  92/08/25  12:53:37  mogul
 * Don't accept non-positive log-scale minima
 * 
 * Revision 1.12  92/08/04  17:55:02  mogul
 * undo RCS botch
 * 
 * Revision 1.11  1992/06/17  22:14:41  kent
 * Make axis specs with max < min work, fix some bugs with centering
 * and multiple args.
 *
 * Revision 1.10  1992/06/16  01:48:11  kent
 * Make y positioning and centering and such work right.
 *
 * Revision 1.9  1992/06/16  00:41:35  kent
 * Hokey fix for a bug where min and max of a scale end up the same.
 *
 * Revision 1.8  1992/04/15  22:24:34  kent
 * log Y label was being placed incorrectly
 *
 * Revision 1.7  1992/04/07  18:11:09  kent
 * Made log scales support datalabels.
 *
 * Revision 1.6  1992/04/07  18:01:19  kent
 * Can't draw the title until after the ticks are labelled, or the
 * placement isn't right.
 *
 * Revision 1.5  1992/04/02  00:45:01  kent
 * Changes to handle lots of points; when using dataticks, the axis
 * routines could get too big and overflow the operand stack. As
 * a result, the output PostScript code is even uglier.
 *
 * Revision 1.4  1992/04/01  23:27:34  kent
 * Added datalabel verb, fixed a bug in handling blank input lines.
 *
 * Revision 1.3  1992/03/31  23:13:12  kent
 * Added "dataticks" verb.
 *
 * Revision 1.2  1992/03/30  23:33:47  kent
 * Added halfopen, halfticks grid styles, range frames, and gray.
 *
 * Revision 1.1  1992/03/20  21:25:43  kent
 * Initial revision
 *
 * Revision 1.9  92/02/21  17:13:20  mogul
 * Added Digital license info
 * 
 * Revision 1.8  91/02/04  16:46:25  mogul
 * set variable "font" before using it, even when not labelling axes.
 * 
 * Revision 1.7  90/11/05  11:09:51  reid
 * Checking in Chris Kent's changes before I start making some.
 * 
 * Revision 1.6  89/01/27  15:54:19  kent
 * lint.
 * 
 * Revision 1.5  89/01/11  09:22:56  kent
 * Use floor() to properly handle axis limit computations.
 * 
 * Revision 1.4  89/01/09  22:18:24  kent
 * Added log scales.
 * 
 * Revision 1.3  89/01/04  17:30:24  kent
 * Made command line arguments override compiled-in defaults for
 * all plots in a run, not just the first one. 
 * 
 * Revision 1.2  89/01/04  15:21:52  kent
 * Massive renaming. No functional change.
 * 
 * Revision 1.1  89/01/04  13:57:40  kent
 * Initial revision
 * 
 */

static char rcs_ident[] = "$Header: /bsdi/MASTER/BSDI_OS/contrib/psgraph/psgraph/grid.c,v 1.1.1.1 1994/01/03 23:14:15 polk Exp $";

#include <stdio.h>

#include "psgraph.h"

grid_t
gridval(cp)
char *cp;
{
	if ( isdigit(*cp)) {
		switch(atoi(cp)) {
		case 0: return NONE;
		case 1:	return OPEN;
		case 2: return TICKS;
		case 3: return FULL;
		case 4: return HALFOPEN;
		case 5: return HALFTICKS;
		default: return FULL;
		}
	} else if ( strcmp(cp,"none")==0 )
		return NONE;
	else if ( strcmp(cp,"open")==0 )
		return OPEN;
	else if ( strncmp(cp,"tick",4)==0 )
		return TICKS;
	else if ( strcmp(cp,"full")==0 )
		return FULL;
	else if ( strcmp(cp,"halfopen")==0 )
		return HALFOPEN;
	else if ( strncmp(cp,"halftick",8)==0 )
		return HALFTICKS;
	return FULL;
}

limargs(p,argc,argv,arg)
limit_t	*p;
int	argc, arg;
char	**argv;
{
	if ( arg < argc ** argv[arg] == 'l' ) {
		p->tform = LOG10;
		arg++;
	}
	if ( isfloat(argv[arg]) ) {
		p->minflag = TRUE;
		p->min = atof(argv[arg++]);
		if ( isfloat(argv[arg]) ) {
			p->maxflag = TRUE;
			p->max = atof(argv[arg++]);
			if ( isfloat(argv[arg]) ) {
				p->distf = TRUE;
				p->dist = atof(argv[arg++]);
			}
		}
	}
	return arg;
}

/*
 * draw the axes - define routines to draw the plot axes based on the
 * parameters.  Because we may want to do a translate (to center the graph)
 * based on the max and min, we can't just emit the commands here; because this
 * could get arbitrarily large and overflow the operand stack, we use a hack to
 * collapse that stack, at the cost of more obscure code.
 */

setupAxes()
{
	minX = 0;
	minY = 0;
	maxX = MAX(maxX, sx(Xaxis.gmax));
	maxY = MAX(maxY, sy(Yaxis.gmax));
}

doXaxis()
{
    	int i, startTemp = CurrentTemp;
	
	startGridTemp();
	
	if (Xaxis.tform == IDENT)
		linearX();
	else
		logX();

	endGridTemp();
	printf("/drawXaxis {\n");	/* hack hack */
	for (i = startTemp; i < CurrentTemp; i++)
	    printf("\tgt%03d\n", i);
	printf("}def\n");
}

doYaxis()
{
    	int i, startTemp = CurrentTemp;
	
	startGridTemp();
	
	if (Yaxis.tform == IDENT)
		linearY();
	else
		logY();

	endGridTemp();
	printf("/drawYaxis {\n");	/* hack hack */
	for (i = startTemp; i < CurrentTemp; i++)
	    printf("\tgt%03d\n", i);
	printf("}def\n");
}

doTitle()
{
	if (Title.title != NULL)
		axisText(Title.title, NORTH, 
			Title.font ? Title.font : TextFont);
}

#define	GT(a,b)		(argp->distg>0 ? ((a)-(b))>0 : ((a)-(b))<0)
#define LT(a,b)		(argp->distg>0 ? ((a)-(b))<0 : ((a)-(b))>0)

linearX()
{
	register axis_t *argp;
	char *font;
	float x;
	float amin, amax;
	float agray, tgray;
	int i;

	argp = &Xaxis;
	agray = argp->axisgray;
	tgray = argp->tickgray;
	font = argp->font ? argp->font : TextFont;
	amin = argp->gmin; amax = argp->gmax;

	/*
	 * Determine how much of the frame to draw
	 */
	if (argp->rangeframe) {
	    amin = argp->min; amax = argp->max;
	}
	
	/*
	 * Draw the frame 
	 */
	if ( argp->gridtype != NONE ) {
		gridLine(amin, Yaxis.gmin, amax, Yaxis.gmin, agray);
		if (!argp->halfgrid)
		    gridLine(amin, Yaxis.gmax, amax, Yaxis.gmax, agray);
	}

	/*
	 * Label the endpoints
	 */
	if (DoAxisLabels) {
		tickText(Xaxis.gmin, Yaxis.gmin, Xaxis.gmin, SOUTH, font);
		tickText(Xaxis.gmax, Yaxis.gmin, Xaxis.gmax, SOUTH, font);
	}

	/*
	 * Do data-only ticks
	 */
	if (argp->datatick) {
	    for (i = 0; i < NumTokens; i++) {
		if (Token[i].type != POINT)
		    continue;
		x = Token[i].val[0];
		if (argp->gridtype == FULL)
		    gridLine(x, Yaxis.gmin, x, Yaxis.gmax, tgray);
		else {
		    tick(x, Yaxis.gmin, NORTH, TRUE, tgray);
		    if (!argp->halfgrid)
			tick(x, Yaxis.gmax, SOUTH, TRUE, tgray);
		}
		if (argp->datalabel)
		    tickText(x, Yaxis.gmin, x, SOUTH, font);
	    }
	}

	/*
	 * Draw the ticks that will have labels on them (or just the labels
	 * in the case of dataticks, ugh)
	 */
	x = argp->gmin;

	while ( LT(x, argp->gmax) ) {
	    if ( GT(x, argp->gmin) ) {
		if (!argp->datatick)
		    if ( argp->gridtype==FULL )
			gridLine(x, Yaxis.gmin, x, Yaxis.gmax, tgray);
		    else if ( argp->gridtype==TICKS ) {
			tick(x, Yaxis.gmin, NORTH, TRUE, tgray);
			if (!argp->halfgrid)
			    tick(x, Yaxis.gmax, SOUTH, TRUE, tgray);
		    }
		if ( DoAxisLabels )
		    tickText(x, Yaxis.gmin, x, SOUTH, font);
	    }
	    x += argp->distg;
	}

	/*
	 * Draw the "title" of the axis
	 */
	if ( argp->label != NULL )
		axisText(argp->label, SOUTH, font);

	if (argp->datatick)
	    return;

	/*
	 * Draw the ticks for the endpoints
	 */
	if ( argp->gridtype == FULL ) {
	    gridLine(Xaxis.gmin, Yaxis.gmin, Xaxis.gmin, Yaxis.gmax, agray);
	    gridLine(Xaxis.gmax, Yaxis.gmin, Xaxis.gmax, Yaxis.gmax, agray);
	} else if ( argp->gridtype==TICKS ) {
	    tick(Xaxis.gmin, Yaxis.gmin, NORTH, TRUE, tgray);
	    tick(Xaxis.gmax, Yaxis.gmin, NORTH, TRUE, tgray);
	    if (!argp->halfgrid) {
		tick(Xaxis.gmin, Yaxis.gmax, SOUTH, TRUE, tgray);
		tick(Xaxis.gmax, Yaxis.gmax, SOUTH, TRUE, tgray);
	    }
	}

	/*
	 * Draw the secondary ticks
	 */
	if ( argp->tickflag ) {
		if (argp->minflag && argp->distf)
			x = argp->gmin;
		else
			x = argp->pmin;
		while ( LT(x, argp->gmax) ) {
			if ( GT(x, argp->gmin) ) {
				tick(x, Yaxis.gmin, NORTH, FALSE, tgray);
				if (!argp->halfgrid)
				    tick(x, Yaxis.gmax, SOUTH, FALSE, tgray);
			}
			x += argp->tick;
		}
	}
}

logX()
{
	register axis_t *argp;
	char	*font;
	float	x, d1, d2, dist;
	float	amin, amax;
	float agray, tgray;
	int i;

	argp = &Xaxis;
	agray = argp->axisgray;
	tgray = argp->tickgray;
	font = argp->font ? argp->font : TextFont;
	amin = argp->gmin; amax = argp->gmax;

	if (argp->rangeframe) {
	    amin = argp->min; amax = argp->max;
	}

	if ( argp->gridtype != NONE ) {
		gridLine(amin, Yaxis.gmin, amax, Yaxis.gmin, agray);
		if (!argp->halfgrid)
		    gridLine(amin, Yaxis.gmax, amax, Yaxis.gmax, agray);
	}
	if (DoAxisLabels) {
		tickText(argp->gmin, Yaxis.gmin, argp->gmin, SOUTH, font);
		tickText(argp->gmax, Yaxis.gmin, argp->gmax, SOUTH, font);
	}

	if ( argp->gridtype == FULL ) {
	    gridLine(Xaxis.gmin, Yaxis.gmin, Xaxis.gmin, Yaxis.gmax, agray);
	    gridLine(Xaxis.gmax, Yaxis.gmin, Xaxis.gmax, Yaxis.gmax, agray);
	} else if ( argp->gridtype==TICKS ) {
	    tick(Xaxis.gmin, Yaxis.gmin, NORTH, TRUE, tgray);
	    tick(Xaxis.gmax, Yaxis.gmin, NORTH, TRUE, tgray);
	    if (!argp->halfgrid) {
		tick(Xaxis.gmin, Yaxis.gmax, SOUTH, TRUE, tgray);
		tick(Xaxis.gmax, Yaxis.gmax, SOUTH, TRUE, tgray);
	    }
	}

	if (argp->datatick) {
	    for (i = 0; i < NumTokens; i++) {
		if (Token[i].type != POINT)
		    continue;
		x = Token[i].val[0];
		if (argp->gridtype == FULL)
		    gridLine(x, Yaxis.gmin, x, Yaxis.gmax, tgray);
		else {
		    tick(x, Yaxis.gmin, NORTH, TRUE, tgray);
		    if (!argp->halfgrid)
			tick(x, Yaxis.gmax, SOUTH, TRUE, tgray);
		}
		if (argp->datalabel)
		    tickText(x, Yaxis.gmin, x, SOUTH, font);
	    }
	}

	if ( argp->label != NULL )
		axisText(argp->label, SOUTH, font);

	x = argp->gmin;

	if (x <= 0.0) {
	    fprintf(stderr, "log scale: illegal x min %g\n", x);
	    exit(-1);
	}

	d1 = ipow(10.0, (int)floor(log10(x)));
	while ( LT(d1, argp->gmax) ) {
	    d2 = (argp->distg > 0) ? 10.0 * d1 : d1 / 10.0;
	    if ( GT(d1, argp->gmin) ) {
		if (!argp->datatick)
		    if ( argp->gridtype==FULL )
			gridLine(d1, Yaxis.gmin, d1, Yaxis.gmax, tgray);
		    else if ( argp->gridtype==TICKS ) {
			tick(d1, Yaxis.gmin, NORTH, TRUE, tgray);
			if (!argp->halfgrid)
			    tick(d1, Yaxis.gmax, SOUTH, TRUE, tgray);
		    }
		if ( DoAxisLabels )
		    tickText(d1, Yaxis.gmin, d1, SOUTH, font);
	    }
	    if ( argp->intervals && !argp->datatick ) {
		dist = copysign(d2 / argp->intervals, argp->distg);
		x = d1;
		while (LT(x, d2) && LT(x, argp->gmax)) {
		    if ( GT(x, argp->gmin) ) {
			tick(x, Yaxis.gmin, NORTH, FALSE, tgray);
			if (!argp->halfgrid)
			    tick(x, Yaxis.gmax, SOUTH, FALSE, tgray);
		    }
		    x += dist;
		}
	    }
	    d1 = d2;
	}
}

linearY()
{
	register axis_t *argp;
	char *font;
	float y;
	float amin, amax;
	float agray, tgray;
	int i;

	argp = &Yaxis;
	font = argp->font ? argp->font : TextFont;
	agray = argp->axisgray;
	tgray = argp->tickgray;
	amin = argp->gmin; amax = argp->gmax;

	if (argp->rangeframe) {
	    amin = argp->min; amax = argp->max;
	}

	if ( argp->gridtype != NONE ) {
		gridLine(Xaxis.gmin, amin, Xaxis.gmin, amax, agray);
		if (!argp->halfgrid)
		    gridLine(Xaxis.gmax, amin, Xaxis.gmax, amax, agray);
	}

	if (DoAxisLabels) {
		tickText(Xaxis.gmin, Yaxis.gmin, Yaxis.gmin, WEST, font);
		tickText(Xaxis.gmin, Yaxis.gmax, Yaxis.gmax, WEST, font);
	}

	if (argp->datatick) {
	    for (i = 0; i < NumTokens; i++) {
		if (Token[i].type != POINT)
		    continue;
		y = Token[i].val[1];
		if (argp->gridtype == FULL)
		    gridLine(Xaxis.gmin, y, Xaxis.gmax, y, tgray);
		else {
		    tick(Xaxis.gmin, y, EAST, TRUE, tgray);
		    if (!argp->halfgrid)
			tick(Xaxis.gmax, y, WEST, TRUE, tgray);
		}
		if (argp->datalabel)
		    tickText(Xaxis.gmin, y, y, WEST, font);
	    }
	}

	y = argp->gmin;
	
	while ( LT(y, argp->gmax) ) {
	    if ( GT(y, argp->gmin) ) {
		if (!argp->datatick)
		    if ( argp->gridtype==FULL )
			gridLine(Xaxis.gmin, y, Xaxis.gmax, y, tgray);
		    else if ( argp->gridtype==TICKS ) {
			tick(Xaxis.gmin, y, EAST, TRUE, tgray);
			if (!argp->halfgrid)
			    tick(Xaxis.gmax, y, WEST, TRUE, tgray);
		    }
		if ( DoAxisLabels )
		    tickText(Xaxis.gmin, y, y, WEST, font);
	    }
	    y += argp->distg;
	}

	if (argp->label != NULL)
		axisText(argp->label, WEST, font);
	
	if (argp->datatick)
	    return;

	if ( argp->gridtype == FULL ) {
	    gridLine(Xaxis.gmin, Yaxis.gmin, Xaxis.gmax, Yaxis.gmin, agray);
	    gridLine(Xaxis.gmin, Yaxis.gmax, Xaxis.gmax, Yaxis.gmax, agray);
	} else if ( argp->gridtype==TICKS ) {
	    tick(Xaxis.gmin, Yaxis.gmin, EAST, TRUE, tgray);
	    tick(Xaxis.gmin, Yaxis.gmax, EAST, TRUE, tgray);
	    if (!argp->halfgrid) {
		tick(Xaxis.gmax, Yaxis.gmin, WEST, TRUE, tgray);
		tick(Xaxis.gmax, Yaxis.gmax, WEST, TRUE, tgray);
	    }
	}
	    
	if ( argp->tickflag ) {
		if ( argp->minflag && argp->distf )
			y = argp->gmin;
		else
			y = argp->pmin;
		while ( LT(y, argp->gmax) ) {
			if ( GT(y, argp->gmin) ) {
				tick(Xaxis.gmin, y, EAST, FALSE, tgray);
				if (!argp->halfgrid)
				    tick(Xaxis.gmax, y, WEST, FALSE, tgray);
			}
			y += argp->tick;
		}
	}
}

logY()
{
	register axis_t *argp;
	char *font;
	float y, d1, d2, dist;
	float	amin, amax;
	float agray, tgray;
	int i;

	argp = &Yaxis;
	agray = argp->axisgray;
	tgray = argp->tickgray;
	font = argp->font ? argp->font : TextFont;
	amin = argp->gmin; amax = argp->gmax;

	if (argp->rangeframe) {
	    amin = argp->min; amax = argp->max;
	}

	if ( argp->gridtype != NONE ) {
		gridLine(Xaxis.gmin, amin, Xaxis.gmin, amax, agray);
		if (!argp->halfgrid)
		    gridLine(Xaxis.gmax, amin, Xaxis.gmax, amax, agray);
	}
	if (DoAxisLabels) {
		tickText(Xaxis.gmin, Yaxis.gmin, Yaxis.gmin, WEST, font);
		tickText(Xaxis.gmin, Yaxis.gmax, Yaxis.gmax, WEST, font);
	}

	if ( argp->gridtype == FULL ) {
	    gridLine(Xaxis.gmin, Yaxis.gmin, Xaxis.gmax, Yaxis.gmin, agray);
	    gridLine(Xaxis.gmin, Yaxis.gmax, Xaxis.gmax, Yaxis.gmax, agray);
	} else if ( argp->gridtype==TICKS ) {
	    tick(Xaxis.gmin, Yaxis.gmin, EAST, TRUE, tgray);
	    tick(Xaxis.gmin, Yaxis.gmax, EAST, TRUE, tgray);
	    if (!argp->halfgrid) {
		tick(Xaxis.gmax, Yaxis.gmin, WEST, TRUE, tgray);
		tick(Xaxis.gmax, Yaxis.gmax, WEST, TRUE, tgray);
	    }
	}

	if (argp->datatick) {
	    for (i = 0; i < NumTokens; i++) {
		if (Token[i].type != POINT)
		    continue;
		y = Token[i].val[1];
		if (argp->gridtype == FULL)
 		    gridLine(Xaxis.gmin, y, Xaxis.gmax, y, tgray);
		else {
		    tick(Xaxis.gmin, y, EAST, TRUE, tgray);
		    if (!argp->halfgrid)
			tick(Xaxis.gmax, y, WEST, TRUE, tgray);
		}
		if (argp->datalabel)
		    tickText(Xaxis.gmin, y, y, WEST, font);
	    }
	}

	if ( argp->label != NULL )
		axisText(argp->label, WEST, font);

	y = argp->gmin;
		
	if (y <= 0.0) {
	    fprintf(stderr, "log scale: illegal y min %g\n", y);
	    exit(-1);
	}

	d1 = ipow(10.0, (int)floor(log10(y)));
	while (LT(d1, argp->gmax)) {
	    d2 = (argp->distg > 0) ? 10.0 * d1 : d1 / 10.0;
	    if ( GT(d1, argp->gmin) ) {
		if (!argp->datatick)
		    if ( argp->gridtype==FULL )
			gridLine(Xaxis.gmin, d1, Xaxis.gmax, d1, tgray);
		    else if ( argp->gridtype==TICKS ) {
			tick(Xaxis.gmin, d1, EAST, TRUE, tgray);
			if (!argp->halfgrid)
			    tick(Xaxis.gmax, d1, WEST, TRUE, tgray);
		    }
		if ( DoAxisLabels )
		    tickText(Xaxis.gmin, d1, d1, WEST, font);
	    }
	    if ( argp->intervals && !argp->datatick ) {
		dist = copysign(d2 / argp->intervals, argp->distg);
		y = d1;
		while ( LT(y, d2) && LT(y, argp->gmax)) {
		    if ( GT(y, argp->gmin) ) {
			tick(Xaxis.gmin, y, EAST, FALSE, tgray);
			if (!argp->halfgrid)
			    tick(Xaxis.gmax, y, WEST, FALSE, tgray);
		    }
		    y += dist;
		}
	    }
	    d1 = d2;
	}
}

/*
 *----------------------------------------------------------------------
 *
 * GRID COMPUTATION ROUTINES
 *
 * These routines are from ACM Collected Algorithms, Number 463.
 *
 * Author: C.R.Lewart
 */

float ipow(x,i)
float x;
int i;
{
	float	res;
	
	res = 1.0;
	if ( i > 0 )
		while ( i-- > 0 )
			res *= x;
	else
		while ( i++ < 0 )
			res /= x;
	return res;
}			

/*
 * scale1
 *
 * Given data extremes xmin and xmax, and number of desired grid lines n
 * (advisory only), compute plot minimum and maximum xpmin and xpmax with
 * distance between grid lines dist.
 *
 * Translated from FORTRAN to C by Bob Brown.
 */

scale1(xmin, xmax, n, xpmin, xpmax, dist)
float	xmin, xmax;
int	n;
float	*xpmin, *xpmax, *dist;
{
	static float vint[] = { 1.0, 2.0, 5.0, 10.0 };
	static float sqr[] = { 1.414213562373, 3.162277660168, 7.071067811865 };
	static float del = 0.000001;
	float a, al, b, fm;
	int nal, m, i;

	if (xmax == xmin)
	    xmax += 1.0;
	a = fabs((xmax-xmin)/(float)n);
	al = log10(a);
	nal = (int)al;
	if ( a < 1.0 )
		nal -= 1;
	b = a/ipow(10.0, nal);
	for ( i=0 ; i<3 ; i++ )
		if ( b < sqr[i] )
			break;
	*dist = copysign(vint[i] * ipow(10.0, nal), (xmax - xmin));
	fm = xmin / *dist;
	m = fm < 0.0 ? (int)fm - 1 : (int)fm;
	if ( fabs((float)m+1.0-fm) < del )
		m += 1;
	*xpmin = *dist * (float)m;
	fm = xmax / *dist;
	m = fm < -1.0 ? (int)fm : (int)fm + 1;
	if ( fabs(fm+1.0-(float)m) < del )
		m -= 1;
	*xpmax = *dist * (float)m;
	if ( *xpmin > xmin )
		*xpmin = xmin;
	if ( *xpmax < xmax )
		*xpmax = xmax;
}
