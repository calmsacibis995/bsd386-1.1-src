/* $Header: /bsdi/MASTER/BSDI_OS/contrib/psgraph/psgraph/psgraph.h,v 1.1.1.1 1994/01/03 23:14:16 polk Exp $ */

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
 * psgraph.h - Definitions for psgraph
 * 
 * Author:	Christopher A. Kent
 * 		Western Research Laboratory
 * 		Digital Equipment Corporation
 * Date:	Wed Jan  4 1989
 */

/*
 * $Log: psgraph.h,v $
 * Revision 1.1.1.1  1994/01/03  23:14:16  polk
 * New contrib utilities
 *
 * Revision 1.8  92/08/04  17:55:13  mogul
 * undo RCS botch
 * 
 * Revision 1.7  1992/07/08  16:43:46  mogul
 * Stupid Ul tricks.
 * (math.h doesn't declare copysign() as returning double)
 *
 * Revision 1.6  92/07/08  15:50:32  mogul
 * PROLOG can now be defined in Makefile.
 * 
 * Revision 1.5  1992/04/02  00:45:01  kent
 * Changes to handle lots of points; when using dataticks, the axis
 * routines could get too big and overflow the operand stack. As
 * a result, the output PostScript code is even uglier.
 *
 * Revision 1.4  1992/04/01  23:28:24  kent
 *  Added datalabel verb, fixed a bug in handling blank input lines.
 *
 * Revision 1.3  1992/03/31  00:07:39  kent
 * Added markerscale verb.
 *
 * Revision 1.2  1992/03/30  23:54:25  kent
 * Added halfopen, halfticks grid styles, range frames, and gray.
 *
 * Revision 1.1  1992/03/20  21:29:05  kent
 * Initial revision
 *
 * Revision 1.10  92/02/21  17:13:23  mogul
 * Added Digital license info
 * 
 * Revision 1.9  91/02/04  16:48:11  mogul
 * Fixed text, marker colors
 * 
 * Revision 1.8  90/12/11  20:42:27  reid
 * Added new enum type members for LINEWIDTH and LINECOLOR
 * 
 * Revision 1.7  89/01/27  15:59:38  kent
 * Need to get the prolog from the standard place!
 * 
 * Revision 1.6  89/01/10  18:20:01  kent
 * Moved marker code to prolog, added error checking and messages.
 * 
 * Revision 1.5  89/01/09  22:18:49  kent
 * Added log scales.
 * 
 * Revision 1.4  89/01/04  18:12:35  kent
 * Added -p flag to specify alternate prologue file.
 * 
 * Revision 1.3  89/01/04  17:29:47  kent
 * Made command line arguments override compiled-in defaults for
 * all plots in a run, not just the first one. 
 * 
 * Revision 1.2  89/01/04  15:22:10  kent
 * Massive renaming. No functional change.
 * 
 * Revision 1.1  89/01/04  13:58:10  kent
 * Initial revision
 * 
 */

#include <ctype.h>
#include <math.h>

#ifdef	ultrix
extern double copysign();	/* grrrr */
#endif	ultrix

typedef	char	bool;
#define	TRUE	1
#define	FALSE	0

#define TOKENINC	16
#define ARGC	16
#define EOS	'\0'

#ifndef	PROLOG
#define	PROLOG	"/usr/local/lib/ps/psgraph.pro"
#endif	PROLOG

#define	TEXTFONT	"Times-Roman10"

#define	MIN(a,b)	((a)<(b)?(a):(b))
#define	MAX(a,b)	((a)>(b)?(a):(b))

typedef enum {IDENT, LOG10}		tform_t;
typedef enum {NONE,OPEN,TICKS,FULL,HALFOPEN,HALFTICKS}	grid_t;
typedef	enum {NORTH, SOUTH, EAST, WEST}	dir_t;

typedef struct _title {
	char	*title;		/* title string	*/
	char	*font;		/* font for the title */
} title_t;

typedef struct _arg {		/* plot-specific parameters that can also be
				 * specified on the command line */
	bool	breakAfterLabel;/* automatic break after input label */
	float	center;		/* center point of baseline */
} arg_t;

typedef struct _axis {		/* axis-specific parameters */
	bool	minflag;	/* TRUE=>minimum specified explicitly	*/
	bool	maxflag;	/* TRUE=>maximum specified explicitly	*/
	bool	distf;		/* TRUE=>user-supplied grid spacing	*/
	bool	tickflag;	/* TRUE=>user supplied tick positions	*/
	bool	rangeframe;	/* TRUE=>axis only covers data range    */
	bool	datatick;	/* TRUE=>ticks at real datapoints	*/
	bool	datalabel;	/* TRUE=>labels at real datapoints	*/
	float	tick;		/* user-supplied tick spacing		*/
	float	tickgray;	/* percent gray in which to draw ticks  */
	grid_t	gridtype;	/* grid type in this dimension		*/
	bool	halfgrid;	/* don't draw a full frame for this axis*/
	float	axisgray;	/* percent gray in which to draw axis   */
	tform_t	tform;		/* transformation type			*/
	float	min, max;	/* data minimum and maximum		*/
	int	intervals;	/* number of intervals on axis		*/
	float	pmin, pmax;	/* computed plot minimum and maximum	*/
	float	distp;		/* computed distance between grid lines	*/
	float	gmin, gmax;	/* min and max to use on the grid	*/
        float	lgmin, lgmax;	/* log of gmin, gmax for log scales	*/
	float	distg;		/* grid spacing to actually use		*/
	float	dist;		/* grid line spacing			*/
	float	offset;		/* displacement				*/
	float	size;		/* grid size in inches			*/
	char	*label;		/* label to place on the axis		*/
	char	*font;		/* font to label axis in		*/
} axis_t;

typedef struct _limit {		/* axis limit argument */
	tform_t	tform;		/* transformation type			*/
	bool	minflag;	/* TRUE=>minimum specified explicitly	*/
	bool	maxflag;	/* TRUE=>maximum specified explicitly	*/
	bool	distf;		/* TRUE=>user-supplied grid spacing	*/
	float	min, max;	/* data minimum and maximum		*/
	float	dist;		/* grid line spacing			*/
} limit_t;

typedef enum tokenType {	/* internalized graph tokens */
	POINT, BREAK, LINETYPE, LINECOLOR, LINEWIDTH, MARKER, MARKERGRAY,
	MARKERSCALE, FONT, TEXT, SPLINE, TRANS, IGNORE
} type_t;
typedef struct _token {
	type_t	type;
	float	val[2];
	int	ival;
	char	*label;
}token_t;
#define xval val[0]
#define yval val[1]

typedef struct _fontName {	/* the chain of fonts that were used */
	char *name;
	struct _fontName *next;
}fontName_t;

arg_t	Args;			/* command-line arguments override defaults */
char	*Prolog;
grid_t	GridType;
bool	HalfGrid;
float	Height;
bool	Preview;
float	Xoffset;
float	Yoffset;
bool	TransposeAxes;
float	Width;
limit_t	Xlim, Ylim;

bool	BreakAfterLabel;	/* plot-specific values */
float	ClipDist;
bool	DoAxisLabels;
int	PointSize;
char	*TextFont;
float	TickLen;
float	Tick2Len;
bool	TransparentLabels;
float	Xcenter;
axis_t	AxisArgs[2];

char	**File;			/* global state */
int	Files;
int	CurrentPage;
float	MinX, MinY, MaxX, MaxY;
float	minX, minY, maxX, maxY;
char	*LineType;
char	*LineColor;
char	*LineWidth;
bool	UseSpline;
title_t	Title;
token_t	*Token;
int	SizeofToken, NumTokens;
char	*UseMarker;
fontName_t	*FontList;
fontName_t	*CurrentFont;
char	*TextColor;
char	*MarkColor;
int	CurrentTemp;
bool	TempOpen;
int	LinesInTemp;

#define	Xaxis	AxisArgs[0]
#define	Yaxis	AxisArgs[1]

char	*newstr();
grid_t	gridval();
float	sx(), sy(), SX(), SY();
float	plotx(), ploty();
float	ipow();
char	*calloc(), *malloc(), *realloc();
double	atof();

