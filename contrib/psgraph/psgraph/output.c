/* $Header: /bsdi/MASTER/BSDI_OS/contrib/psgraph/psgraph/output.c,v 1.1.1.1 1994/01/03 23:14:16 polk Exp $ */

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
 * output.c - Spit out the PostScript
 * 
 * Author:	Christopher A. Kent
 * 		Western Research Laboratory
 * 		Digital Equipment Corporation
 * Date:	Wed Jan  4 1989
 */

/*
 * $Log: output.c,v $
 * Revision 1.1.1.1  1994/01/03  23:14:16  polk
 * New contrib utilities
 *
 * Revision 1.10  92/08/04  17:55:10  mogul
 * undo RCS botch
 * 
 * Revision 1.9  1992/07/16  20:07:09  cak
 * Fix markergray implementation so it doesn't blow away the effects of color.
 *
 * Revision 1.8  1992/06/17  22:14:41  kent
 * Make axis specs with max < min work, fix some bugs with centering
 * and multiple args.
 *
 * Revision 1.7  1992/06/16  01:48:11  kent
 * Make y positioning and centering and such work right.
 *
 * Revision 1.6  1992/04/06  21:14:03  kent
 * Broke the bounding box computation last time 'round. Fixed.
 *
 * Revision 1.5  1992/04/02  00:45:01  kent
 * Changes to handle lots of points; when using dataticks, the axis
 * routines could get too big and overflow the operand stack. As
 * a result, the output PostScript code is even uglier.
 *
 * Revision 1.4  1992/03/31  02:31:34  kent
 * Added markergray verb and fixed inverted gray values.
 *
 * Revision 1.3  1992/03/31  00:07:39  kent
 * Added markerscale verb.
 *
 * Revision 1.2  1992/03/30  23:33:47  kent
 * Added halfopen, halfticks grid styles, range frames, and gray.
 *
 * Revision 1.1  1992/03/20  21:25:43  kent
 * Initial revision
 *
 * Revision 1.16  92/02/21  17:13:23  mogul
 * Added Digital license info
 * 
 * Revision 1.15  91/02/04  17:03:22  mogul
 * Break up long sets of text commands to avoid producing
 * PostScript with excessively long lines (hard to email).
 * 
 * 
 * Revision 1.14  91/02/04  16:48:27  mogul
 * Don't emit color changes when not necessary (saves space in
 * output file).
 * 
 * Revision 1.13  91/02/04  16:31:05  mogul
 * fixed color support for text, markers
 * 
 * Revision 1.12  90/12/11  20:41:55  reid
 * Added support for new "color" and "linewidth" options.
 * 
 * Revision 1.11  90/11/05  11:11:44  reid
 * Checking in Chris Kent's changes of June 1989
 * 
 * Revision 1.10  89/03/01  10:44:40  kent
 * NORTH and WEST axis text must be adjusted based on point size, since the
 * code in the prologue "unadjusts" it based on the point size.
 * 
 * Revision 1.9  89/02/03  09:33:23  kent
 * Make splines work on log scales.
 * 
 * Revision 1.8  89/01/27  15:56:15  kent
 * Line style "off" has to be handled in code, since there's no way I can
 * cleanly use setdash to draw an "empty" line. Also fixed problems with
 * axisText on axes with non-zero origins.
 * 
 * Revision 1.7  89/01/11  09:14:27  kent
 * Removed some internal knowledge about the semantics of line types. This
 * is all in the PostScript now.
 * 
 * Revision 1.6  89/01/10  18:19:55  kent
 * Moved marker code to prolog, added error checking and messages.
 * 
 * Revision 1.5  89/01/09  22:18:45  kent
 * Added log scales.
 * 
 * Revision 1.4  89/01/04  17:40:56  kent
 * Moved font stuff from main.c to output.c.
 * newfont() sets PS fontsize variable so white background is the right size.
 * 
 * Revision 1.3  89/01/04  17:30:31  kent
 * Made command line arguments override compiled-in defaults for
 * all plots in a run, not just the first one. 
 * 
 * Revision 1.2  89/01/04  15:22:08  kent
 * Massive renaming. No functional change.
 * 
 * Revision 1.1  89/01/04  13:57:59  kent
 * Initial revision
 * 
 */

static char rcs_ident[] = "$Header: /bsdi/MASTER/BSDI_OS/contrib/psgraph/psgraph/output.c,v 1.1.1.1 1994/01/03 23:14:16 polk Exp $";

#include <stdio.h>
#include <assert.h>

#include "psgraph.h"

typedef struct _plotpoint {
	float x, y;
}plotpoint_t;

/*
 * doplot - generate all the output text for the plot.
 *
 * BUG: leaks storage if called multiple times.
 */

doplot()
{
	int	 i, numpoints, sizeofpoints;
	plotpoint_t	*points;

	CurrentPage++;
	printf("%%%%Page: %d %d\n", CurrentPage, CurrentPage);
	printf("StartPSGraph\n");
	printf("/saveIt save def\ngsave\n/solid f\n");
	TextFont = TEXTFONT;
	CurrentFont = NULL;
	newfont(TextFont);

/**/

	setupAxes();

	doXaxis();

	doYaxis();

	printf("/drawTitle [\n");
	doTitle();
	printf("] cvx bind def\n");

	if (Preview)
		printf("1 1 translate\n");

	if (Xcenter != 0.0) {		/* do centering */
		printf("%f %f sub 2 div 0 translate\n", 
			Xcenter, Xaxis.size);
		/* must adjust bbox */
		minX += (Xcenter - Xaxis.size) / 2.0;
		maxX += (Xcenter - Xaxis.size) / 2.0;
		/* move Y origin to 0 */
		printf("0 %f translate\n", -minY);
		maxY -= minY; minY = 0.0;
	} else {
		printf("%f %f translate\n",	/* move origin to 0,0 */
			-minX, -minY);
		maxX -= minX; minX = 0.0;
		maxY -= minY; minY = 0.0;
	}
	printf("drawXaxis drawYaxis drawTitle\n");

	MinX = MIN(minX, MinX);
	MinY = MIN(minY, MinY);
	MaxX = MAX(maxX, MaxX);
	MaxY = MAX(maxY, MaxY);

	sizeofpoints = 10;
	points=(plotpoint_t *)calloc(10,sizeof(plotpoint_t));
	numpoints = 0;
	cliplimits(Xaxis.gmin-ClipDist/Xaxis.size*(Xaxis.gmax-Xaxis.gmin),
		   Yaxis.gmin-ClipDist/Yaxis.size*(Yaxis.gmax-Yaxis.gmin),
		   Xaxis.gmax+ClipDist/Xaxis.size*(Xaxis.gmax-Xaxis.gmin),
		   Yaxis.gmax+ClipDist/Yaxis.size*(Yaxis.gmax-Yaxis.gmin),
		   Xaxis.distg < 0, Yaxis.distg < 0);
	for(i=0 ; i<NumTokens; i++) {
		switch(Token[i].type) {
		case TRANS:
			if (numpoints > 0) {
				dumppoints(points, numpoints);
				numpoints = 0;
			}
			TransparentLabels = Token[i].ival;
			break;
		case BREAK:
			if ( numpoints > 0 ) {
				dumppoints(points,numpoints);
				numpoints = 0;
			}
			break;
		case LINETYPE:
			if ( numpoints > 0 ) {
				dumppoints(points,numpoints);
				numpoints = 0;
				UseSpline = FALSE;
			}
			if ( Token[i].label != NULL )
				LineType = Token[i].label;
			break;
		case LINECOLOR:
			if ( numpoints > 0 ) {
				dumppoints(points,numpoints);
				numpoints = 0;
				UseSpline = FALSE;
			}
			if ( Token[i].label != NULL ) {
				LineColor = Token[i].label;
				TextColor = Token[i].label;
				MarkColor = Token[i].label;
				graymarker(0.0);
			}
			break;
		case LINEWIDTH:
			if ( numpoints > 0 ) {
				dumppoints(points,numpoints);
				numpoints = 0;
				UseSpline = FALSE;
			}
			if ( Token[i].label != NULL )
				LineWidth = Token[i].label;
			break;
		case MARKER:
			UseMarker = Token[i].label;
			setmarker(UseMarker);
			break;
		case MARKERSCALE:
			scalemarker(Token[i].val[0]);
			break;
		case MARKERGRAY:
			graymarker(Token[i].val[0]);
			break;
		case FONT:
			if ( numpoints > 0 ) {
				dumppoints(points,numpoints);
				points[0] = points[numpoints-1];
				numpoints = 1;
			}
			if ( Token[i].label != NULL ) {
				newfont(Token[i].label);
				TextFont = Token[i].label;
			}
			break;
		case TEXT:
			if ( numpoints > 0 ) {
				dumppoints(points,numpoints);
				numpoints = 0;
			}
			if ( code(Token[i].xval,Token[i].yval)==0 )
				text(Token[i].label,sx(Token[i].xval),sy(Token[i].yval),"ljust");
			break;
		case SPLINE:
			dumppoints(points,numpoints);
			numpoints = 0;
			UseSpline = TRUE;
			break;
		case POINT:
			if ( numpoints >= sizeofpoints ) {
				sizeofpoints += 10;
				points = (plotpoint_t *)realloc((char *)points,sizeofpoints*sizeof(plotpoint_t));
			}
			points[numpoints].x = Token[i].val[0];
			points[numpoints++].y = Token[i].val[1];
			if ( Token[i].label != NULL ) {
				if ( code(Token[i].xval,Token[i].yval)==0 )
					if (TransparentLabels)
					    text(Token[i].label,
						sx(Token[i].val[0])+0.1,
						sy(Token[i].val[1]),"ljust");
					else
					    wtext(Token[i].label,
						sx(Token[i].val[0])+0.1,
						sy(Token[i].val[1]),"ljust");
				if (BreakAfterLabel && numpoints > 0) {
					dumppoints(points,numpoints);
					numpoints = 0;
				}
			}
			if ( UseMarker != NULL )
				domarker(Token[i].val[0],Token[i].val[1]);
			break;
		}
	}
	dumppoints(points,numpoints);
	printf("grestore saveIt restore\n");
	if (Preview)
/*	    printf("gsave showpage grestore\n");*/
	    printf("showpage\n");
	/*
	 * bah ... these can be supplied with command line arguments, but
	 * they shouldn't leak through to a second graph...
	 */
	
	Title.title = NULL;
	Title.font = NULL;
}

/*
 * dumppoints - generate the commands for the data stored in the points array.
 */

dumppoints(points, numpoints)
plotpoint_t *points;
int numpoints;
{
	int i;
	float xs, ys, xe, ye;
	float dxi, dyi, dxi1, dyi1;

	if (numpoints < 2 ||
	    strcmp(LineType, "off") == 0 ||
	    strcmp(LineType, "none") == 0)
		return;
	if ( UseSpline ) {
		i = 1;
		while (!clip(points[i-1].x, points[i-1].y,
				points[i].x, points[i].y,
				&xs, &ys, &xe, &ye)) 
			i++;		/* remove invisibles at beginning */

		startline(xs, ys);
		printf(" %f %f rlineto\n",
			(SX(xe) - SX(xs))/2, (SY(ye) - SY(ys))/2);

		for (; i < numpoints - 1; i++){
			if ( clip(points[i-1].x, points[i-1].y,
				  points[i].x, points[i].y,
				  &xs, &ys, &xe, &ye)) {
				dxi = SX(xe) - SX(xs);
				dyi = SY(ye) - SY(ys);
			} else {/* handle intermediate clipping */}
			if ( clip(points[i].x, points[i].y,
				  points[i+1].x, points[i+1].y,
				  &xs, &ys, &xe, &ye)) {
				dxi1 = SX(xe) - SX(xs);
				dyi1 = SY(ye) - SY(ys);
			} else {/* ditto. it's ugly */}
			printf(" %f %f %f %f %f %f rcurveto\n",
				dxi/3, dyi/3,
				(3*dxi+dxi1)/6, (3*dyi+dyi1)/6,
				(dxi+dxi1)/2, (dyi+dyi1)/2);
		}
		contline(xe, ye);		
	} else for ( i=1 ; i<numpoints ; i++ ) {
		if ( clip(points[i-1].x, points[i-1].y,
			  points[i].x,points[i].y, &xs, &ys, &xe, &ye)) {
			if ( i==1 || xs!=points[i-1].x && ys!=points[i-1].y) {
				if ( i != 1 )
					printf("\n");
				startline(xs,ys);
			}
			contline(xe,ye);
		}
	}
}

startGridTemp()
{
    assert(!TempOpen);
    printf("/gt%03d[\n", CurrentTemp++);
    TempOpen = TRUE;
    LinesInTemp = 0;
}

endGridTemp()
{
    assert(TempOpen);
    printf("]cvx bind def\n");
    TempOpen = FALSE;
}

gridLine(x1, y1, x2, y2, g)
float	x1, y1, x2, y2, g;
{
    	if (LinesInTemp > 50) { endGridTemp(); startGridTemp(); }
	LinesInTemp++;
	    
	putchar('{');
	if (g != 0.0)
	    printf("gsave %f setgray ", g);
	stroke(sx(x1), sy(y1), sx(x2), sy(y2));
	if (g != 0.0)
	    printf("grestore");
	printf("\t}EX\n");
}

tick(x, y, d, big, g)
float	x, y, g;
dir_t	d;
bool	big;
{
	float	len;

    	if (LinesInTemp > 50) { endGridTemp(); startGridTemp(); }
	LinesInTemp++;

	len = big ? TickLen : Tick2Len;
	printf("{ ");
	if (g != 0.0)
	    printf("gsave %f setgray ", g);
	switch(d) {
	case	NORTH:
		stroke(sx(x), sy(y), sx(x), sy(y) + len);
		break;
	case	EAST:
		stroke(sx(x), sy(y), sx(x) + len, sy(y));
		break;
	case	SOUTH:
		stroke(sx(x), sy(y) - len, sx(x), sy(y));
		break;
	case	WEST:
		stroke(sx(x) - len, sy(y), sx(x), sy(y));
		break;
	}
	if (g != 0.0)
	    printf("grestore");
	printf("\t}EX\n");
}

static int	maxWest = 0;	/* most char positions west */

tickText(x, y, val, d, font)
float	x, y;
float	val;
dir_t	d;
char	*font;
{
	char	buf[32];
	int	len;

    	if (LinesInTemp > 50) { endGridTemp(); startGridTemp(); }
	LinesInTemp++;

	sprintf(buf, "%g", val);
	printf("{ ");
	newfont(font);
	switch(d){
	case SOUTH:
		text(buf, sx(x), sy(y) - (0.8 * PointSize/72.0), "");
		minY = MIN(minY, sy(Yaxis.gmin)-(0.8*PointSize/72.0));
		break;
	case WEST:
		text(buf, sx(x) - 0.05, sy(y), "rjust");
		len = strlen(buf);
		maxWest = MAX(len, maxWest);
		minX = MIN(minX, -(0.75 * len*PointSize/72.0));
		break;
	}
	printf("\t}EX\n");
}

axisText(buf, d, font)
char	*buf;
dir_t	d;
char	*font;
{
	float	xc, yc;

	xc = sx(Xaxis.gmin) + (SX(Xaxis.gmax) - SX(Xaxis.gmin)) / 2.0;
	yc = sy(Yaxis.gmin) + (SY(Yaxis.gmax) - SY(Yaxis.gmin)) / 2.0;

	printf("{ ");
	newfont(font);
	switch(d) {
	case NORTH:
		printf("%f %f m (%s) /cjust t ", 
			xc, sy(Yaxis.gmax) + 0.1 + 0.3 * PointSize/72.0,
			buf);
		maxY = MAX(maxY, sy(Yaxis.gmax) + 0.1 + 1.3*PointSize/72.0);
		break;

	case SOUTH:
		printf("%f %f m (%s) /cjust t ",
			xc, sy(Yaxis.gmin) - 1.8*PointSize/72.0, buf);
		minY = MIN(minY, sy(Yaxis.gmin) - 2.0*PointSize/72.0);
		break;

	case WEST:
		printf("%f %f m (%s) /vjust t ", 
			sx(Xaxis.gmin) - (0.75*maxWest * PointSize/72.0
					  + 0.1 + 0.3 * PointSize/72.0),
			yc, 
			buf);
		minX = MIN(minX, sx(Xaxis.gmin) - 
			(0.75*maxWest*PointSize/72.0 
			 + 0.1 + 1.3 * PointSize/72.0));
		break;		
	}
	printf("\t}EX\n");
}

/*
 * Plot routines -
 *
 * These routines all assume that their input is already scaled to inches
 */

stroke(x1,y1,x2,y2)
float x1,y1,x2,y2;
{
	printf("%f %f %f %f l ", x1, y1, x2, y2);
}

startline(x,y)
float x,y;
{
	setcolor(LineColor);
	if (LineType != NULL && strlen(LineType)>0)
		printf("/%s f ", LineType);
	else
		printf("/solid f ");
	if (LineWidth != NULL && strlen(LineType)>0)
		printf(" %s fw ", LineWidth);
	else
		printf(" 0.6 setlinewidth ");
	printf("%f %f m\n",sx(x),sy(y));
}

contline(x,y)
float x,y;
{
	printf(" %f %f n\n",sx(x),sy(y));
}

text(s,x,y,mod)
float x,y;
char *s, *mod;
{
	setcolor(TextColor);
	printf("%f %f m (%s) ", x, y, s);
	if ((strlen(mod) == 0) || (mod == NULL))
		printf("/cjust ");
	else 
		printf ("/%s ", mod);
	printf("t\n");
}

wtext(s,x,y,mod)
float x,y;
char *s, *mod;
{
	setcolor(TextColor);
	printf("%f %f m (%s) ", x, y, s);
	if ((strlen(mod) == 0) || (mod == NULL))
		printf("/cjust ");
	else 
		printf ("/%s ", mod);
	printf("w\n");
}

newfont(font)
register char *font;
{
	char name[100];
	int size;
	register char *np = name;
	register char *s = font;
	register fontName_t *fp;

	while (isascii(*s) && (isalpha(*s) || (*s == '-')))
		*np++ = *s++;
	*np++ = NULL;
	if (isascii(*s) && isdigit(*s)) {
		size = 0;
		do
			size = size * 10 + *s++ - '0';
		while ('0' <= *s && *s <= '9');
	}
	if (*s || !size || !name[0]) {
		fprintf (stderr, "Poorly formed font name: \"%s\"\n", name);
		return;
	}
	if (CurrentFont)
		if (size == PointSize && !strcmp(CurrentFont->name, name))
			return;		/* no change */
	printf("/%s findfont %d scalefont setfont ", name, size);
	printf("/fontsize %d def\n", size);
	PointSize = size;

	for (fp = FontList; fp; fp = fp->next)
		if (!strcmp(fp->name, name)) {
			CurrentFont = fp;
			return;		/* already there */
		}
	fp = (fontName_t *) malloc(sizeof (fontName_t));
	fp->name = newstr(name);
	fp->next = FontList;
	FontList = fp;
	CurrentFont = fp;
}

/*
 * setmarker - set the current marker type
 */

setmarker(m)
char	*m;
{
	printf("/%s sm\n", m);
}

/*
 * domarker - print the marker at the given point.
 */

domarker(x,y)
float	x,y;
{
    if ( code(x,y) )
	return;
    /*
     * note that the marker gray stuff is done inside mk (in PS), so
     * the color may not actually take effect.
     */
    setcolor(MarkColor);
    printf("%f %f mk\n", sx(x), sy(y));
}

/*
 * graymarker - set the marker gray value
 */

graymarker(s)
float s;
{
    printf("/mg %f def\n", s);
}

/*
 * scalemarker - set the marker scale value
 */

scalemarker(s)
float	s;
{
    printf("/ms %f def\n", s);
}

/*
 * setcolor - set current color; remembers what is already set
 */

char *curColor = NULL;

setcolor(color)
char *color;
{
	if (curColor && color && (strcmp(curColor, color) == 0))
		return;
	curColor = color;
	if (color != NULL && strlen(color)>0)
		printf("/%s fc\n",color);
	else
		printf("/black fc\n");
}

/*
 *----------------------------------------------------------------------
 *
 * DATA CLIPPING ROUTINES
 *
 *	2D clipper 	Cohen-Sutherland clipper from Newman and Sproull(pg 67).
 *
 */

#define LEFT	0x1
#define RIGHT   0x2
#define BOTTOM  0x4
#define TOP     0x8

float   Clipxl, Clipxr, Clipyb, Clipyt;
int	Clipxf, Clipyf;
unsigned int c;


cliplimits( xmin, ymin, xmax, ymax, xflip, yflip)
float xmin, ymin, xmax, ymax;
int   xflip, yflip;
{
	extern float Clipxl, Clipxr, CLipyb, Clipyt;

	Clipxl = xmin;
	Clipxr = xmax;
	Clipxf = xflip;
	Clipyb = ymin;
	Clipyt = ymax;
	Clipyf = yflip;
}


code( x, y )
float x,y; /* point to encode */

{

	c = 0;

	if ( Clipxf ? x > Clipxl : x < Clipxl )
		c = LEFT;
	else if ( Clipxf ? x < Clipxr : x > Clipxr )
		c = RIGHT;

	if ( Clipyf ? y > Clipyb : y < Clipyb )
		c = c | BOTTOM;
	else if ( Clipyf ? y < Clipyt : y > Clipyt )
		c = c | TOP;

	return c;

}


clip( x1, y1, x2, y2, xs, ys, xe, ye )
float x1, y1, x2, y2;      /* line to test against box */
float *xs, *ys, *xe, *ye;  /* returned line within region */ 
/*
 *	Description:	Clips a line against the rectangular clipping region set by
 *			Cliplimits. 
 *
 *	Input:
 *			endpoints of a line to be tested. No order assumed.
 *
 *	Output:
 *			Returns the new endpoints of the line clipped to the edges
 *			of the box in xs, ys, xe, ye.
 *
 *	    RETURNS:    0  if the the line does not pass through the clipping region.
 *		        1  if the line passes through the clipping region.
 *
 */

{
	unsigned int c1,c2;
	float x,y;

	c1 = code( x1,y1 );
	c2 = code( x2,y2 );

	/*  if both c = 0, then both within window : trivial accept */
	while ( (c1 != 0) || (c2 != 0) ) {
			

		/* if intersection of c1 and c2 is non-zero, then the line lies
		   completely off of the screen */
		if ( c1 & c2 ) {
			*xs = x1;
			*ys = y1;
			*xe = x2;
			*ye = y2;
			return FALSE;
		}

		c = c1;
		if ( c == 0 ) c = c2;


		if ( c & LEFT ) { /* crosses left edge */
			y = y1 + (y2-y1) * (Clipxl - x1) / (x2-x1);
			x = Clipxl;
		} else if ( c & RIGHT ) { /* crosses right edge */
			y = y1 + (y2-y1) * (Clipxr - x1) / (x2-x1);
			x = Clipxr;
		} else if ( c & BOTTOM ) { /* crosses bottom edge */
			x = x1 + (x2-x1) * ( Clipyb - y1 ) / (y2-y1);
			y = Clipyb;
		} else if ( c & TOP ) { /* crosses top edge */
			x = x1 + (x2-x1) * ( Clipyt - y1 ) / (y2-y1);		
			y = Clipyt;
		}

		if ( c = c1 ) {
			x1 = x;
			y1 = y;
			c1 = code(x,y);	
		} else {
			x2 = x;
			y2 = y;
			c2 = code(x,y);
		}

	} /* end while */

	/* line is visible */

	*xs = x1;
	*ys = y1;
	*xe = x2;
	*ye = y2;
	return TRUE;
}

/*
 * Scaling routines - these convert user data into inches
 */

float 
sx(x)
float x;
{
	register axis_t *argp = &Xaxis;

	if (argp->tform == IDENT)
		return (x-argp->gmin)/(argp->gmax-argp->gmin)*argp->size
			+ argp->offset;
	else
		return (log10(x)-argp->lgmin)/(argp->lgmax - argp->lgmin)
			* argp->size + argp->offset;
}
float
sy(y)
float y;
{
	register axis_t *argp = &Yaxis;

	if (argp->tform == IDENT)
		return (y-argp->gmin)/(argp->gmax-argp->gmin)*argp->size
			+ argp->offset;
	else
		return (log10(y)-argp->lgmin)/(argp->lgmax - argp->lgmin)
			* argp->size + argp->offset;
}

/* these just scale, don't translate */

float 
SX(x)
float x;
{
	register axis_t *argp = &Xaxis;

	if (argp->tform == IDENT)
		return (x)/(argp->gmax-argp->gmin)*argp->size;
	else
		return (log10(x)-argp->lgmin)/(argp->lgmax - argp->lgmin)
			* argp->size;
}
float 
SY(y)
float y;
{
	register axis_t *argp = &Yaxis;

	if (argp->tform == IDENT)
		return (y)/(argp->gmax-argp->gmin)*argp->size;
	else
		return (log10(y)-argp->lgmin)/(argp->lgmax - argp->lgmin)
			* argp->size;
}

