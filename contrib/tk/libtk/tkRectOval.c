/* 
 * tkRectOval.c --
 *
 *	This file implements rectangle and oval items for canvas
 *	widgets.
 *
 * Copyright (c) 1991-1993 The Regents of the University of California.
 * All rights reserved.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

#ifndef lint
static char rcsid[] = "$Header: /bsdi/MASTER/BSDI_OS/contrib/tk/libtk/tkRectOval.c,v 1.1.1.1 1994/01/03 23:15:28 polk Exp $ SPRITE (Berkeley)";
#endif

#include <stdio.h>
#include "tkConfig.h"
#include "tkInt.h"
#include "tkCanvas.h"

/*
 * The structure below defines the record for each rectangle/oval item.
 */

typedef struct RectOvalItem  {
    Tk_Item header;		/* Generic stuff that's the same for all
				 * types.  MUST BE FIRST IN STRUCTURE. */
    double bbox[4];		/* Coordinates of bounding box for rectangle
				 * or oval (x1, y1, x2, y2).  Item includes
				 * x1 and x2 but not y1 and y2. */
    int width;			/* Width of outline. */
    XColor *outlineColor;	/* Color for outline. */
    XColor *fillColor;		/* Color for filling rectangle/oval. */
    Pixmap fillStipple;		/* Stipple bitmap for filling item. */
    GC outlineGC;		/* Graphics context for outline. */
    GC fillGC;			/* Graphics context for filling item. */
} RectOvalItem;

/*
 * Information used for parsing configuration specs:
 */

static Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_COLOR, "-fill", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(RectOvalItem, fillColor), TK_CONFIG_NULL_OK},
    {TK_CONFIG_COLOR, "-outline", (char *) NULL, (char *) NULL,
	"black", Tk_Offset(RectOvalItem, outlineColor), TK_CONFIG_NULL_OK},
    {TK_CONFIG_BITMAP, "-stipple", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(RectOvalItem, fillStipple), TK_CONFIG_NULL_OK},
    {TK_CONFIG_CUSTOM, "-tags", (char *) NULL, (char *) NULL,
	(char *) NULL, 0, TK_CONFIG_NULL_OK, &tkCanvasTagsOption},
    {TK_CONFIG_PIXELS, "-width", (char *) NULL, (char *) NULL,
	"1", Tk_Offset(RectOvalItem, width), TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0}
};

/*
 * Prototypes for procedures defined in this file:
 */

static void		ComputeRectOvalBbox _ANSI_ARGS_((Tk_Canvas *canvasPtr,
			    RectOvalItem *rectOvalPtr));
static int		ConfigureRectOval _ANSI_ARGS_((
			    Tk_Canvas *canvasPtr, Tk_Item *itemPtr, int argc,
			    char **argv, int flags));
static int		CreateRectOval _ANSI_ARGS_((Tk_Canvas *canvasPtr,
			    struct Tk_Item *itemPtr, int argc, char **argv));
static void		DeleteRectOval _ANSI_ARGS_((Tk_Canvas *canvasPtr,
			    Tk_Item *itemPtr));
static void		DisplayRectOval _ANSI_ARGS_((Tk_Canvas *canvasPtr,
			    Tk_Item *itemPtr, Drawable dst));
static int		OvalToArea _ANSI_ARGS_((Tk_Canvas *canvasPtr,
			    Tk_Item *itemPtr, double *areaPtr));
static double		OvalToPoint _ANSI_ARGS_((Tk_Canvas *canvasPtr,
			    Tk_Item *itemPtr, double *pointPtr));
static int		RectOvalCoords _ANSI_ARGS_((Tk_Canvas *canvasPtr,
			    Tk_Item *itemPtr, int argc, char **argv));
static int		RectOvalToPostscript _ANSI_ARGS_((Tk_Canvas *canvasPtr,
			    Tk_Item *itemPtr, Tk_PostscriptInfo *psInfoPtr));
static int		RectToArea _ANSI_ARGS_((Tk_Canvas *canvasPtr,
			    Tk_Item *itemPtr, double *areaPtr));
static double		RectToPoint _ANSI_ARGS_((Tk_Canvas *canvasPtr,
			    Tk_Item *itemPtr, double *pointPtr));
static void		ScaleRectOval _ANSI_ARGS_((Tk_Canvas *canvasPtr,
			    Tk_Item *itemPtr, double originX, double originY,
			    double scaleX, double scaleY));
static void		TranslateRectOval _ANSI_ARGS_((Tk_Canvas *canvasPtr,
			    Tk_Item *itemPtr, double deltaX, double deltaY));

/*
 * The structures below defines the rectangle and oval item types
 * by means of procedures that can be invoked by generic item code.
 */

Tk_ItemType TkRectangleType = {
    "rectangle",			/* name */
    sizeof(RectOvalItem),		/* itemSize */
    CreateRectOval,			/* createProc */
    configSpecs,			/* configSpecs */
    ConfigureRectOval,			/* configureProc */
    RectOvalCoords,			/* coordProc */
    DeleteRectOval,			/* deleteProc */
    DisplayRectOval,			/* displayProc */
    0,					/* alwaysRedraw */
    RectToPoint,			/* pointProc */
    RectToArea,				/* areaProc */
    RectOvalToPostscript,		/* postscriptProc */
    ScaleRectOval,			/* scaleProc */
    TranslateRectOval,			/* translateProc */
    (Tk_ItemIndexProc *) NULL,		/* indexProc */
    (Tk_ItemCursorProc *) NULL,		/* icursorProc */
    (Tk_ItemSelectionProc *) NULL,	/* selectionProc */
    (Tk_ItemInsertProc *) NULL,		/* insertProc */
    (Tk_ItemDCharsProc *) NULL,		/* dTextProc */
    (Tk_ItemType *) NULL		/* nextPtr */
};

Tk_ItemType TkOvalType = {
    "oval",				/* name */
    sizeof(RectOvalItem),		/* itemSize */
    CreateRectOval,			/* createProc */
    configSpecs,			/* configSpecs */
    ConfigureRectOval,			/* configureProc */
    RectOvalCoords,			/* coordProc */
    DeleteRectOval,			/* deleteProc */
    DisplayRectOval,			/* displayProc */
    0,					/* alwaysRedraw */
    OvalToPoint,			/* pointProc */
    OvalToArea,				/* areaProc */
    RectOvalToPostscript,		/* postscriptProc */
    ScaleRectOval,			/* scaleProc */
    TranslateRectOval,			/* translateProc */
    (Tk_ItemIndexProc *) NULL,		/* indexProc */
    (Tk_ItemCursorProc *) NULL,		/* cursorProc */
    (Tk_ItemSelectionProc *) NULL,	/* selectionProc */
    (Tk_ItemInsertProc *) NULL,		/* insertProc */
    (Tk_ItemDCharsProc *) NULL,		/* dTextProc */
    (Tk_ItemType *) NULL		/* nextPtr */
};

/*
 *--------------------------------------------------------------
 *
 * CreateRectOval --
 *
 *	This procedure is invoked to create a new rectangle
 *	or oval item in a canvas.
 *
 * Results:
 *	A standard Tcl return value.  If an error occurred in
 *	creating the item, then an error message is left in
 *	canvasPtr->interp->result;  in this case itemPtr is
 *	left uninitialized, so it can be safely freed by the
 *	caller.
 *
 * Side effects:
 *	A new rectangle or oval item is created.
 *
 *--------------------------------------------------------------
 */

static int
CreateRectOval(canvasPtr, itemPtr, argc, argv)
    register Tk_Canvas *canvasPtr;	/* Canvas to hold new item. */
    Tk_Item *itemPtr;			/* Record to hold new item;  header
					 * has been initialized by caller. */
    int argc;				/* Number of arguments in argv. */
    char **argv;			/* Arguments describing rectangle. */
{
    register RectOvalItem *rectOvalPtr = (RectOvalItem *) itemPtr;

    if (argc < 4) {
	Tcl_AppendResult(canvasPtr->interp, "wrong # args:  should be \"",
		Tk_PathName(canvasPtr->tkwin), "\" create ",
		itemPtr->typePtr->name, " x1 y1 x2 y2 ?options?",
		(char *) NULL);
	return TCL_ERROR;
    }

    /*
     * Carry out initialization that is needed in order to clean
     * up after errors during the the remainder of this procedure.
     */

    rectOvalPtr->width = 1;
    rectOvalPtr->outlineColor = NULL;
    rectOvalPtr->fillColor = NULL;
    rectOvalPtr->fillStipple = None;
    rectOvalPtr->outlineGC = None;
    rectOvalPtr->fillGC = None;

    /*
     * Process the arguments to fill in the item record.
     */

    if ((TkGetCanvasCoord(canvasPtr, argv[0], &rectOvalPtr->bbox[0]) != TCL_OK)
	    || (TkGetCanvasCoord(canvasPtr, argv[1],
		&rectOvalPtr->bbox[1]) != TCL_OK)
	    || (TkGetCanvasCoord(canvasPtr, argv[2],
		    &rectOvalPtr->bbox[2]) != TCL_OK)
	    || (TkGetCanvasCoord(canvasPtr, argv[3],
		    &rectOvalPtr->bbox[3]) != TCL_OK)) {
	return TCL_ERROR;
    }

    if (ConfigureRectOval(canvasPtr, itemPtr, argc-4, argv+4, 0) != TCL_OK) {
	DeleteRectOval(canvasPtr, itemPtr);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * RectOvalCoords --
 *
 *	This procedure is invoked to process the "coords" widget
 *	command on rectangles and ovals.  See the user documentation
 *	for details on what it does.
 *
 * Results:
 *	Returns TCL_OK or TCL_ERROR, and sets canvasPtr->interp->result.
 *
 * Side effects:
 *	The coordinates for the given item may be changed.
 *
 *--------------------------------------------------------------
 */

static int
RectOvalCoords(canvasPtr, itemPtr, argc, argv)
    register Tk_Canvas *canvasPtr;	/* Canvas containing item. */
    Tk_Item *itemPtr;			/* Item whose coordinates are to be
					 * read or modified. */
    int argc;				/* Number of coordinates supplied in
					 * argv. */
    char **argv;			/* Array of coordinates: x1, y1,
					 * x2, y2, ... */
{
    register RectOvalItem *rectOvalPtr = (RectOvalItem *) itemPtr;
    char c0[TCL_DOUBLE_SPACE], c1[TCL_DOUBLE_SPACE];
    char c2[TCL_DOUBLE_SPACE], c3[TCL_DOUBLE_SPACE];

    if (argc == 0) {
	Tcl_PrintDouble(canvasPtr->interp, rectOvalPtr->bbox[0], c0);
	Tcl_PrintDouble(canvasPtr->interp, rectOvalPtr->bbox[1], c1);
	Tcl_PrintDouble(canvasPtr->interp, rectOvalPtr->bbox[2], c2);
	Tcl_PrintDouble(canvasPtr->interp, rectOvalPtr->bbox[3], c3);
	Tcl_AppendResult(canvasPtr->interp, c0, " ", c1, " ",
		c2, " ", c3, (char *) NULL);
    } else if (argc == 4) {
	if ((TkGetCanvasCoord(canvasPtr, argv[0],
		    &rectOvalPtr->bbox[0]) != TCL_OK)
		|| (TkGetCanvasCoord(canvasPtr, argv[1],
		    &rectOvalPtr->bbox[1]) != TCL_OK)
		|| (TkGetCanvasCoord(canvasPtr, argv[2],
			&rectOvalPtr->bbox[2]) != TCL_OK)
		|| (TkGetCanvasCoord(canvasPtr, argv[3],
			&rectOvalPtr->bbox[3]) != TCL_OK)) {
	    return TCL_ERROR;
	}
	ComputeRectOvalBbox(canvasPtr, rectOvalPtr);
    } else {
	sprintf(canvasPtr->interp->result,
		"wrong # coordinates:  expected 0 or 4, got %d",
		argc);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * ConfigureRectOval --
 *
 *	This procedure is invoked to configure various aspects
 *	of a rectangle or oval item, such as its border and
 *	background colors.
 *
 * Results:
 *	A standard Tcl result code.  If an error occurs, then
 *	an error message is left in canvasPtr->interp->result.
 *
 * Side effects:
 *	Configuration information, such as colors and stipple
 *	patterns, may be set for itemPtr.
 *
 *--------------------------------------------------------------
 */

static int
ConfigureRectOval(canvasPtr, itemPtr, argc, argv, flags)
    Tk_Canvas *canvasPtr;	/* Canvas containing itemPtr. */
    Tk_Item *itemPtr;		/* Rectangle item to reconfigure. */
    int argc;			/* Number of elements in argv.  */
    char **argv;		/* Arguments describing things to configure. */
    int flags;			/* Flags to pass to Tk_ConfigureWidget. */
{
    register RectOvalItem *rectOvalPtr = (RectOvalItem *) itemPtr;
    XGCValues gcValues;
    GC newGC;
    unsigned long mask;

    if (Tk_ConfigureWidget(canvasPtr->interp, canvasPtr->tkwin,
	    configSpecs, argc, argv, (char *) rectOvalPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * A few of the options require additional processing, such as
     * graphics contexts.
     */

    if (rectOvalPtr->outlineColor == NULL) {
	newGC = None;
    } else {
	gcValues.foreground = rectOvalPtr->outlineColor->pixel;
	gcValues.cap_style = CapProjecting;
	if (rectOvalPtr->width < 0) {
	    rectOvalPtr->width = 1;
	}
	gcValues.line_width = rectOvalPtr->width;
	mask = GCForeground|GCCapStyle|GCLineWidth;
	newGC = Tk_GetGC(canvasPtr->tkwin, mask, &gcValues);
    }
    if (rectOvalPtr->outlineGC != None) {
	Tk_FreeGC(canvasPtr->display, rectOvalPtr->outlineGC);
    }
    rectOvalPtr->outlineGC = newGC;

    if (rectOvalPtr->fillColor == NULL) {
	newGC = None;
    } else {
	gcValues.foreground = rectOvalPtr->fillColor->pixel;
	if (rectOvalPtr->fillStipple != None) {
	    gcValues.stipple = rectOvalPtr->fillStipple;
	    gcValues.fill_style = FillStippled;
	    mask = GCForeground|GCStipple|GCFillStyle;
	} else {
	    mask = GCForeground;
	}
	newGC = Tk_GetGC(canvasPtr->tkwin, mask, &gcValues);
    }
    if (rectOvalPtr->fillGC != None) {
	Tk_FreeGC(canvasPtr->display, rectOvalPtr->fillGC);
    }
    rectOvalPtr->fillGC = newGC;
    ComputeRectOvalBbox(canvasPtr, rectOvalPtr);

    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * DeleteRectOval --
 *
 *	This procedure is called to clean up the data structure
 *	associated with a rectangle or oval item.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resources associated with itemPtr are released.
 *
 *--------------------------------------------------------------
 */

static void
DeleteRectOval(canvasPtr, itemPtr)
    Tk_Canvas *canvasPtr;		/* Info about overall widget. */
    Tk_Item *itemPtr;			/* Item that is being deleted. */
{
    register RectOvalItem *rectOvalPtr = (RectOvalItem *) itemPtr;

    if (rectOvalPtr->outlineColor != NULL) {
	Tk_FreeColor(rectOvalPtr->outlineColor);
    }
    if (rectOvalPtr->fillColor != NULL) {
	Tk_FreeColor(rectOvalPtr->fillColor);
    }
    if (rectOvalPtr->fillStipple != None) {
	Tk_FreeBitmap(canvasPtr->display, rectOvalPtr->fillStipple);
    }
    if (rectOvalPtr->outlineGC != None) {
	Tk_FreeGC(canvasPtr->display, rectOvalPtr->outlineGC);
    }
    if (rectOvalPtr->fillGC != None) {
	Tk_FreeGC(canvasPtr->display, rectOvalPtr->fillGC);
    }
}

/*
 *--------------------------------------------------------------
 *
 * ComputeRectOvalBbox --
 *
 *	This procedure is invoked to compute the bounding box of
 *	all the pixels that may be drawn as part of a rectangle
 *	or oval.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The fields x1, y1, x2, and y2 are updated in the header
 *	for itemPtr.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static void
ComputeRectOvalBbox(canvasPtr, rectOvalPtr)
    Tk_Canvas *canvasPtr;		/* Canvas that contains item. */
    register RectOvalItem *rectOvalPtr;	/* Item whose bbox is to be
					 * recomputed. */
{
    int bloat;

    /*
     * Make sure that the first coordinates are the lowest ones.
     */

    if (rectOvalPtr->bbox[1] > rectOvalPtr->bbox[3]) {
	double tmp;
	tmp = rectOvalPtr->bbox[3];
	rectOvalPtr->bbox[3] = rectOvalPtr->bbox[1];
	rectOvalPtr->bbox[1] = tmp;
    }
    if (rectOvalPtr->bbox[0] > rectOvalPtr->bbox[2]) {
	double tmp;
	tmp = rectOvalPtr->bbox[2];
	rectOvalPtr->bbox[2] = rectOvalPtr->bbox[0];
	rectOvalPtr->bbox[0] = tmp;
    }

    bloat = (rectOvalPtr->width+1)/2 + 1;
    rectOvalPtr->header.x1 = rectOvalPtr->bbox[0] - bloat;
    rectOvalPtr->header.y1 = rectOvalPtr->bbox[1] - bloat;
    rectOvalPtr->header.x2 = rectOvalPtr->bbox[2] + bloat;
    rectOvalPtr->header.y2 = rectOvalPtr->bbox[3] + bloat;
}

/*
 *--------------------------------------------------------------
 *
 * DisplayRectOval --
 *
 *	This procedure is invoked to draw a rectangle or oval
 *	item in a given drawable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	ItemPtr is drawn in drawable using the transformation
 *	information in canvasPtr.
 *
 *--------------------------------------------------------------
 */

static void
DisplayRectOval(canvasPtr, itemPtr, drawable)
    register Tk_Canvas *canvasPtr;	/* Canvas that contains item. */
    Tk_Item *itemPtr;			/* Item to be displayed. */
    Drawable drawable;			/* Pixmap or window in which to draw
					 * item. */
{
    register RectOvalItem *rectOvalPtr = (RectOvalItem *) itemPtr;
    Display *display = Tk_Display(canvasPtr->tkwin);
    int x1, y1, x2, y2;

    /*
     * Compute the screen coordinates of the bounding box for the item.
     * Make sure that the bbox is at least one pixel large, since some
     * X servers will die if it isn't.
     */

    x1 = SCREEN_X(canvasPtr, rectOvalPtr->bbox[0]);
    y1 = SCREEN_Y(canvasPtr, rectOvalPtr->bbox[1]);
    x2 = SCREEN_X(canvasPtr, rectOvalPtr->bbox[2]);
    y2 = SCREEN_Y(canvasPtr, rectOvalPtr->bbox[3]);
    if (x2 <= x1) {
	x2 = x1+1;
    }
    if (y2 <= y1) {
	y2 = y1+1;
    }

    /*
     * Display filled part first (if wanted), then outline.  If we're
     * stippling, then modify the stipple offset in the GC.  Be sure to
     * reset the offset when done, since the GC is supposed to be
     * read-only.
     */

    if (rectOvalPtr->fillGC != None) {
	if (rectOvalPtr->fillStipple != None) {
	    XSetTSOrigin(display, rectOvalPtr->fillGC,
		    -canvasPtr->drawableXOrigin, -canvasPtr->drawableYOrigin);
	}
	if (rectOvalPtr->header.typePtr == &TkRectangleType) {
	    XFillRectangle(display, drawable, rectOvalPtr->fillGC,
		    x1, y1, (unsigned int) (x2-x1), (unsigned int) (y2-y1));
	} else {
	    XFillArc(display, drawable, rectOvalPtr->fillGC,
		    x1, y1, (x2-x1), (y2-y1), 0, 360*64);
	}
	if (rectOvalPtr->fillStipple != None) {
	    XSetTSOrigin(display, rectOvalPtr->fillGC, 0, 0);
	}
    }
    if (rectOvalPtr->outlineGC != None) {
	if (rectOvalPtr->header.typePtr == &TkRectangleType) {
	    XDrawRectangle(display, drawable, rectOvalPtr->outlineGC,
		    x1, y1, (x2-x1-1), (y2-y1-1));
	} else {
	    XDrawArc(display, drawable, rectOvalPtr->outlineGC,
		    x1, y1, (x2-x1-1), (y2-y1-1), 0, 360*64);
	}
    }
}

/*
 *--------------------------------------------------------------
 *
 * RectToPoint --
 *
 *	Computes the distance from a given point to a given
 *	rectangle, in canvas units.
 *
 * Results:
 *	The return value is 0 if the point whose x and y coordinates
 *	are coordPtr[0] and coordPtr[1] is inside the rectangle.  If the
 *	point isn't inside the rectangle then the return value is the
 *	distance from the point to the rectangle.  If itemPtr is filled,
 *	then anywhere in the interior is considered "inside"; if
 *	itemPtr isn't filled, then "inside" means only the area
 *	occupied by the outline.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static double
RectToPoint(canvasPtr, itemPtr, pointPtr)
    Tk_Canvas *canvasPtr;	/* Canvas containing item. */
    Tk_Item *itemPtr;		/* Item to check against point. */
    double *pointPtr;		/* Pointer to x and y coordinates. */
{
    register RectOvalItem *rectPtr = (RectOvalItem *) itemPtr;
    double xDiff, yDiff, x1, y1, x2, y2, inc, tmp;

    /*
     * Generate a new larger rectangle that includes the border
     * width, if there is one.
     */

    x1 = rectPtr->bbox[0];
    y1 = rectPtr->bbox[1];
    x2 = rectPtr->bbox[2];
    y2 = rectPtr->bbox[3];
    if (rectPtr->outlineGC != None) {
	inc = rectPtr->width/2.0;
	x1 -= inc;
	y1 -= inc;
	x2 += inc;
	y2 += inc;
    }

    /*
     * If the point is inside the rectangle, handle specially:
     * distance is 0 if rectangle is filled, otherwise compute
     * distance to nearest edge of rectangle and subtract width
     * of edge.
     */

    if ((pointPtr[0] >= x1) && (pointPtr[0] < x2)
		&& (pointPtr[1] >= y1) && (pointPtr[1] < y2)) {
	if ((rectPtr->fillGC != None) || (rectPtr->outlineGC == None)) {
	    return 0.0;
	}
	xDiff = pointPtr[0] - x1;
	tmp = x2 - pointPtr[0];
	if (tmp < xDiff) {
	    xDiff = tmp;
	}
	yDiff = pointPtr[1] - y1;
	tmp = y2 - pointPtr[1];
	if (tmp < yDiff) {
	    yDiff = tmp;
	}
	if (yDiff < xDiff) {
	    xDiff = yDiff;
	}
	xDiff -= rectPtr->width;
	if (xDiff < 0.0) {
	    return 0.0;
	}
	return xDiff;
    }

    /*
     * Point is outside rectangle.
     */

    if (pointPtr[0] < x1) {
	xDiff = x1 - pointPtr[0];
    } else if (pointPtr[0] > x2)  {
	xDiff = pointPtr[0] - x2;
    } else {
	xDiff = 0;
    }

    if (pointPtr[1] < y1) {
	yDiff = y1 - pointPtr[1];
    } else if (pointPtr[1] > y2)  {
	yDiff = pointPtr[1] - y2;
    } else {
	yDiff = 0;
    }

    return hypot(xDiff, yDiff);
}

/*
 *--------------------------------------------------------------
 *
 * OvalToPoint --
 *
 *	Computes the distance from a given point to a given
 *	oval, in canvas units.
 *
 * Results:
 *	The return value is 0 if the point whose x and y coordinates
 *	are coordPtr[0] and coordPtr[1] is inside the oval.  If the
 *	point isn't inside the oval then the return value is the
 *	distance from the point to the oval.  If itemPtr is filled,
 *	then anywhere in the interior is considered "inside"; if
 *	itemPtr isn't filled, then "inside" means only the area
 *	occupied by the outline.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static double
OvalToPoint(canvasPtr, itemPtr, pointPtr)
    Tk_Canvas *canvasPtr;	/* Canvas containing item. */
    Tk_Item *itemPtr;		/* Item to check against point. */
    double *pointPtr;		/* Pointer to x and y coordinates. */
{
    register RectOvalItem *ovalPtr = (RectOvalItem *) itemPtr;
    double width;
    int filled;

    width = ovalPtr->width;
    filled = ovalPtr->fillGC != None;
    if (ovalPtr->outlineGC == None) {
	width = 0.0;
	filled = 1;
    }
    return TkOvalToPoint(ovalPtr->bbox, width, filled, pointPtr);
}

/*
 *--------------------------------------------------------------
 *
 * RectToArea --
 *
 *	This procedure is called to determine whether an item
 *	lies entirely inside, entirely outside, or overlapping
 *	a given rectangle.
 *
 * Results:
 *	-1 is returned if the item is entirely outside the area
 *	given by rectPtr, 0 if it overlaps, and 1 if it is entirely
 *	inside the given area.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static int
RectToArea(canvasPtr, itemPtr, areaPtr)
    Tk_Canvas *canvasPtr;	/* Canvas containing item. */
    Tk_Item *itemPtr;		/* Item to check against rectangle. */
    double *areaPtr;		/* Pointer to array of four coordinates
				 * (x1, y1, x2, y2) describing rectangular
				 * area.  */
{
    register RectOvalItem *rectPtr = (RectOvalItem *) itemPtr;
    double halfWidth;

    halfWidth = rectPtr->width/2.0;
    if (rectPtr->outlineGC == None) {
	halfWidth = 0.0;
    }

    if ((areaPtr[2] <= (rectPtr->bbox[0] - halfWidth))
	    || (areaPtr[0] >= (rectPtr->bbox[2] + halfWidth))
	    || (areaPtr[3] <= (rectPtr->bbox[1] - halfWidth))
	    || (areaPtr[1] >= (rectPtr->bbox[3] + halfWidth))) {
	return -1;
    }
    if ((rectPtr->fillGC == None) && (rectPtr->outlineGC != None)
	    && (areaPtr[0] >= (rectPtr->bbox[0] + halfWidth))
	    && (areaPtr[1] >= (rectPtr->bbox[1] + halfWidth))
	    && (areaPtr[2] <= (rectPtr->bbox[2] - halfWidth))
	    && (areaPtr[3] <= (rectPtr->bbox[3] - halfWidth))) {
	return -1;
    }
    if ((areaPtr[0] <= (rectPtr->bbox[0] - halfWidth))
	    && (areaPtr[1] <= (rectPtr->bbox[1] - halfWidth))
	    && (areaPtr[2] >= (rectPtr->bbox[2] + halfWidth))
	    && (areaPtr[3] >= (rectPtr->bbox[3] + halfWidth))) {
	return 1;
    }
    return 0;
}

/*
 *--------------------------------------------------------------
 *
 * OvalToArea --
 *
 *	This procedure is called to determine whether an item
 *	lies entirely inside, entirely outside, or overlapping
 *	a given rectangular area.
 *
 * Results:
 *	-1 is returned if the item is entirely outside the area
 *	given by rectPtr, 0 if it overlaps, and 1 if it is entirely
 *	inside the given area.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static int
OvalToArea(canvasPtr, itemPtr, areaPtr)
    Tk_Canvas *canvasPtr;	/* Canvas containing item. */
    Tk_Item *itemPtr;		/* Item to check against oval. */
    double *areaPtr;		/* Pointer to array of four coordinates
				 * (x1, y1, x2, y2) describing rectangular
				 * area.  */
{
    register RectOvalItem *ovalPtr = (RectOvalItem *) itemPtr;
    double oval[4], halfWidth;
    int result;

    /*
     * Expand the oval to include the width of the outline, if any.
     */

    halfWidth = ovalPtr->width/2.0;
    if (ovalPtr->outlineGC == None) {
	halfWidth = 0.0;
    }
    oval[0] = ovalPtr->bbox[0] - halfWidth;
    oval[1] = ovalPtr->bbox[1] - halfWidth;
    oval[2] = ovalPtr->bbox[2] + halfWidth;
    oval[3] = ovalPtr->bbox[3] + halfWidth;

    result = TkOvalToArea(oval, areaPtr);

    /*
     * If the rectangle appears to overlap the oval and the oval
     * isn't filled, do one more check to see if perhaps all four
     * of the rectangle's corners are totally inside the oval's
     * unfilled center, in which case we should return "outside".
     */

    if ((result == 0) && (ovalPtr->outlineGC != None)
	    && (ovalPtr->fillGC == None)) {
	double centerX, centerY, width, height;
	double xDelta1, yDelta1, xDelta2, yDelta2;

	centerX = (ovalPtr->bbox[0] + ovalPtr->bbox[2])/2.0;
	centerY = (ovalPtr->bbox[1] + ovalPtr->bbox[3])/2.0;
	width = (ovalPtr->bbox[2] - ovalPtr->bbox[0])/2.0 - halfWidth;
	height = (ovalPtr->bbox[3] - ovalPtr->bbox[1])/2.0 - halfWidth;
	xDelta1 = (areaPtr[0] - centerX)/width;
	xDelta1 *= xDelta1;
	yDelta1 = (areaPtr[1] - centerY)/height;
	yDelta1 *= yDelta1;
	xDelta2 = (areaPtr[2] - centerX)/width;
	xDelta2 *= xDelta2;
	yDelta2 = (areaPtr[3] - centerY)/height;
	yDelta2 *= yDelta2;
	if (((xDelta1 + yDelta1) < 1.0)
		&& ((xDelta1 + yDelta2) < 1.0)
		&& ((xDelta2 + yDelta1) < 1.0)
		&& ((xDelta2 + yDelta2) < 1.0)) {
	    return -1;
	}
    }
    return result;
}

/*
 *--------------------------------------------------------------
 *
 * ScaleRectOval --
 *
 *	This procedure is invoked to rescale a rectangle or oval
 *	item.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The rectangle or oval referred to by itemPtr is rescaled
 *	so that the following transformation is applied to all
 *	point coordinates:
 *		x' = originX + scaleX*(x-originX)
 *		y' = originY + scaleY*(y-originY)
 *
 *--------------------------------------------------------------
 */

static void
ScaleRectOval(canvasPtr, itemPtr, originX, originY, scaleX, scaleY)
    Tk_Canvas *canvasPtr;		/* Canvas containing rectangle. */
    Tk_Item *itemPtr;			/* Rectangle to be scaled. */
    double originX, originY;		/* Origin about which to scale rect. */
    double scaleX;			/* Amount to scale in X direction. */
    double scaleY;			/* Amount to scale in Y direction. */
{
    register RectOvalItem *rectOvalPtr = (RectOvalItem *) itemPtr;

    rectOvalPtr->bbox[0] = originX + scaleX*(rectOvalPtr->bbox[0] - originX);
    rectOvalPtr->bbox[1] = originY + scaleY*(rectOvalPtr->bbox[1] - originY);
    rectOvalPtr->bbox[2] = originX + scaleX*(rectOvalPtr->bbox[2] - originX);
    rectOvalPtr->bbox[3] = originY + scaleY*(rectOvalPtr->bbox[3] - originY);
    ComputeRectOvalBbox(canvasPtr, rectOvalPtr);
}

/*
 *--------------------------------------------------------------
 *
 * TranslateRectOval --
 *
 *	This procedure is called to move a rectangle or oval by a
 *	given amount.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The position of the rectangle or oval is offset by
 *	(xDelta, yDelta), and the bounding box is updated in the
 *	generic part of the item structure.
 *
 *--------------------------------------------------------------
 */

static void
TranslateRectOval(canvasPtr, itemPtr, deltaX, deltaY)
    Tk_Canvas *canvasPtr;		/* Canvas containing item. */
    Tk_Item *itemPtr;			/* Item that is being moved. */
    double deltaX, deltaY;		/* Amount by which item is to be
					 * moved. */
{
    register RectOvalItem *rectOvalPtr = (RectOvalItem *) itemPtr;

    rectOvalPtr->bbox[0] += deltaX;
    rectOvalPtr->bbox[1] += deltaY;
    rectOvalPtr->bbox[2] += deltaX;
    rectOvalPtr->bbox[3] += deltaY;
    ComputeRectOvalBbox(canvasPtr, rectOvalPtr);
}

/*
 *--------------------------------------------------------------
 *
 * RectOvalToPostscript --
 *
 *	This procedure is called to generate Postscript for
 *	rectangle and oval items.
 *
 * Results:
 *	The return value is a standard Tcl result.  If an error
 *	occurs in generating Postscript then an error message is
 *	left in canvasPtr->interp->result, replacing whatever used
 *	to be there.  If no error occurs, then Postscript for the
 *	rectangle is appended to the result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
RectOvalToPostscript(canvasPtr, itemPtr, psInfoPtr)
    Tk_Canvas *canvasPtr;		/* Information about overall canvas. */
    Tk_Item *itemPtr;			/* Item for which Postscript is
					 * wanted. */
    Tk_PostscriptInfo *psInfoPtr;	/* Information about the Postscript;
					 * must be passed back to Postscript
					 * utility procedures. */
{
    char pathCmd[500], string[100];
    register RectOvalItem *rectOvalPtr = (RectOvalItem *) itemPtr;
    double y1, y2;

    y1 = TkCanvPsY(psInfoPtr, rectOvalPtr->bbox[1]);
    y2 = TkCanvPsY(psInfoPtr, rectOvalPtr->bbox[3]);

    /*
     * Generate a string that creates a path for the rectangle or oval.
     * This is the only part of the procedure's code that is type-
     * specific.
     */


    if (rectOvalPtr->header.typePtr == &TkRectangleType) {
	sprintf(pathCmd, "%.15g %.15g moveto %.15g 0 rlineto 0 %.15g rlineto %.15g 0 rlineto closepath\n",
		rectOvalPtr->bbox[0], y1,
		rectOvalPtr->bbox[2]-rectOvalPtr->bbox[0], y2-y1,
		rectOvalPtr->bbox[0]-rectOvalPtr->bbox[2]);
    } else {
	sprintf(pathCmd, "matrix currentmatrix\n%.15g %.15g translate %.15g %.15g scale 1 0 moveto 0 0 1 0 360 arc\nsetmatrix\n",
		(rectOvalPtr->bbox[0] + rectOvalPtr->bbox[2])/2, (y1 + y2)/2,
		(rectOvalPtr->bbox[2] - rectOvalPtr->bbox[0])/2, (y1 - y2)/2);
    }

    /*
     * First draw the filled area of the rectangle.
     */

    if (rectOvalPtr->fillColor != NULL) {
	Tcl_AppendResult(canvasPtr->interp, pathCmd, (char *) NULL);
	if (TkCanvPsColor(canvasPtr, psInfoPtr, rectOvalPtr->fillColor)
		!= TCL_OK) {
	    return TCL_ERROR;
	}
	if (rectOvalPtr->fillStipple != None) {
	    if (TkCanvPsStipple(canvasPtr, psInfoPtr,
		    rectOvalPtr->fillStipple, 1) != TCL_OK) {
		return TCL_ERROR;
	    }
	} else {
	    Tcl_AppendResult(canvasPtr->interp, "fill\n", (char *) NULL);
	}
    }

    /*
     * Now draw the outline, if there is one.
     */

    if (rectOvalPtr->outlineColor != NULL) {
	Tcl_AppendResult(canvasPtr->interp, pathCmd, (char *) NULL);
	sprintf(string, "%d setlinewidth", rectOvalPtr->width);
	Tcl_AppendResult(canvasPtr->interp, string,
		" 0 setlinejoin 2 setlinecap\n", (char *) NULL);
	if (TkCanvPsColor(canvasPtr, psInfoPtr, rectOvalPtr->outlineColor)
		!= TCL_OK) {
	    return TCL_ERROR;
	}
	Tcl_AppendResult(canvasPtr->interp, "stroke\n", (char *) NULL);
    }
    return TCL_OK;
}
