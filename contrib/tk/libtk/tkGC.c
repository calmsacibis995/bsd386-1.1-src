/* 
 * tkGC.c --
 *
 *	This file maintains a database of read-only graphics contexts 
 *	for the Tk toolkit, in order to allow GC's to be shared.
 *
 * Copyright (c) 1990-1993 The Regents of the University of California.
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
static char rcsid[] = "$Header: /bsdi/MASTER/BSDI_OS/contrib/tk/libtk/tkGC.c,v 1.1.1.1 1994/01/03 23:15:25 polk Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "tkConfig.h"
#include "tk.h"

/*
 * One of the following data structures exists for each GC that is
 * currently active.  The structure is indexed with two hash tables,
 * one based on font name and one based on XFontStruct address.
 */

typedef struct {
    GC gc;			/* Graphics context. */
    Display *display;		/* Display to which gc belongs. */
    int refCount;		/* Number of active uses of gc. */
    Tcl_HashEntry *valueHashPtr;/* Entry in valueTable (needed when deleting
				 * this structure). */
} TkGC;

/*
 * Hash table to map from a GC's values to a TkGC structure describing
 * a GC with those values (used by Tk_GetGC).
 */

static Tcl_HashTable valueTable;
typedef struct {
    XGCValues values;		/* Desired values for GC. */
    Display *display;		/* Display for which GC is valid. */
    int screenNum;		/* screen number of display */
    int depth;			/* and depth for which GC is valid. */
} ValueKey;

/*
 * Hash table for <display + GC> -> TkGC mapping. This table is used by
 * Tk_FreeGC.
 */

static Tcl_HashTable idTable;
typedef struct {
    Display *display;		/* Display for which GC was allocated. */
    GC gc;			/* X's identifier for GC. */
} IdKey;

static int initialized = 0;	/* 0 means static structures haven't been
				 * initialized yet. */

/*
 * Forward declarations for procedures defined in this file:
 */

static void		GCInit _ANSI_ARGS_((void));

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetGC --
 *
 *	Given a desired set of values for a graphics context, find
 *	a read-only graphics context with the desired values.
 *
 * Results:
 *	The return value is the X identifer for the desired graphics
 *	context.  The caller should never modify this GC, and should
 *	call Tk_FreeGC when the GC is no longer needed.
 *
 * Side effects:
 *	The GC is added to an internal database with a reference count.
 *	For each call to this procedure, there should eventually be a call
 *	to Tk_FreeGC, so that the database can be cleaned up when GC's
 *	aren't needed anymore.
 *
 *----------------------------------------------------------------------
 */

GC
Tk_GetGC(tkwin, valueMask, valuePtr)
    Tk_Window tkwin;		/* Window in which GC will be used. */
    register unsigned long valueMask;
				/* 1 bits correspond to values specified
				 * in *valuesPtr;  other values are set
				 * from defaults. */
    register XGCValues *valuePtr;
				/* Values are specified here for bits set
				 * in valueMask. */
{
    ValueKey valueKey;
    IdKey idKey;
    Tcl_HashEntry *valueHashPtr, *idHashPtr;
    register TkGC *gcPtr;
    int new;
    Drawable d, freeDrawable;

    if (!initialized) {
	GCInit();
    }

    /*
     * Must zero valueKey at start to clear out pad bytes that may be
     * part of structure on some systems.
     */

    memset((VOID *) &valueKey, 0, sizeof(valueKey));

    /*
     * First, check to see if there's already a GC that will work
     * for this request (exact matches only, sorry).
     */

    if (valueMask & GCFunction) {
	valueKey.values.function = valuePtr->function;
    } else {
	valueKey.values.function = GXcopy;
    }
    if (valueMask & GCPlaneMask) {
	valueKey.values.plane_mask = valuePtr->plane_mask;
    } else {
	valueKey.values.plane_mask = ~0;
    }
    if (valueMask & GCForeground) {
	valueKey.values.foreground = valuePtr->foreground;
    } else {
	valueKey.values.foreground = 0;
    }
    if (valueMask & GCBackground) {
	valueKey.values.background = valuePtr->background;
    } else {
	valueKey.values.background = 1;
    }
    if (valueMask & GCLineWidth) {
	valueKey.values.line_width = valuePtr->line_width;
    } else {
	valueKey.values.line_width = 0;
    }
    if (valueMask & GCLineStyle) {
	valueKey.values.line_style = valuePtr->line_style;
    } else {
	valueKey.values.line_style = LineSolid;
    }
    if (valueMask & GCCapStyle) {
	valueKey.values.cap_style = valuePtr->cap_style;
    } else {
	valueKey.values.cap_style = CapButt;
    }
    if (valueMask & GCJoinStyle) {
	valueKey.values.join_style = valuePtr->join_style;
    } else {
	valueKey.values.join_style = JoinMiter;
    }
    if (valueMask & GCFillStyle) {
	valueKey.values.fill_style = valuePtr->fill_style;
    } else {
	valueKey.values.fill_style = FillSolid;
    }
    if (valueMask & GCFillRule) {
	valueKey.values.fill_rule = valuePtr->fill_rule;
    } else {
	valueKey.values.fill_rule = EvenOddRule;
    }
    if (valueMask & GCArcMode) {
	valueKey.values.arc_mode = valuePtr->arc_mode;
    } else {
	valueKey.values.arc_mode = ArcPieSlice;
    }
    if (valueMask & GCTile) {
	valueKey.values.tile = valuePtr->tile;
    } else {
	valueKey.values.tile = None;
    }
    if (valueMask & GCStipple) {
	valueKey.values.stipple = valuePtr->stipple;
    } else {
	valueKey.values.stipple = None;
    }
    if (valueMask & GCTileStipXOrigin) {
	valueKey.values.ts_x_origin = valuePtr->ts_x_origin;
    } else {
	valueKey.values.ts_x_origin = 0;
    }
    if (valueMask & GCTileStipYOrigin) {
	valueKey.values.ts_y_origin = valuePtr->ts_y_origin;
    } else {
	valueKey.values.ts_y_origin = 0;
    }
    if (valueMask & GCFont) {
	valueKey.values.font = valuePtr->font;
    } else {
	valueKey.values.font = None;
    }
    if (valueMask & GCSubwindowMode) {
	valueKey.values.subwindow_mode = valuePtr->subwindow_mode;
    } else {
	valueKey.values.subwindow_mode = ClipByChildren;
    }
    if (valueMask & GCGraphicsExposures) {
	valueKey.values.graphics_exposures = valuePtr->graphics_exposures;
    } else {
	valueKey.values.graphics_exposures = True;
    }
    if (valueMask & GCClipXOrigin) {
	valueKey.values.clip_x_origin = valuePtr->clip_x_origin;
    } else {
	valueKey.values.clip_x_origin = 0;
    }
    if (valueMask & GCClipYOrigin) {
	valueKey.values.clip_y_origin = valuePtr->clip_y_origin;
    } else {
	valueKey.values.clip_y_origin = 0;
    }
    if (valueMask & GCClipMask) {
	valueKey.values.clip_mask = valuePtr->clip_mask;
    } else {
	valueKey.values.clip_mask = None;
    }
    if (valueMask & GCDashOffset) {
	valueKey.values.dash_offset = valuePtr->dash_offset;
    } else {
	valueKey.values.dash_offset = 0;
    }
    if (valueMask & GCDashList) {
	valueKey.values.dashes = valuePtr->dashes;
    } else {
	valueKey.values.dashes = 4;
    }
    valueKey.display = Tk_Display(tkwin);
    valueKey.screenNum = Tk_ScreenNumber(tkwin);
    valueKey.depth = Tk_Depth(tkwin);
    valueHashPtr = Tcl_CreateHashEntry(&valueTable, (char *) &valueKey, &new);
    if (!new) {
	gcPtr = (TkGC *) Tcl_GetHashValue(valueHashPtr);
	gcPtr->refCount++;
	return gcPtr->gc;
    }

    /*
     * No GC is currently available for this set of values.  Allocate a
     * new GC and add a new structure to the database.
     */

    gcPtr = (TkGC *) ckalloc(sizeof(TkGC));

    /*
     * Find or make a drawable to use to specify the screen and depth
     * of the GC.  We may have to make a small pixmap, to avoid doing
     * Tk_MakeWindowExist on the window.
     */

    freeDrawable = None;
    if (Tk_WindowId(tkwin) != None) {
	d = Tk_WindowId(tkwin);
    } else if (valueKey.depth ==
	    DefaultDepth(valueKey.display, valueKey.screenNum)) {
	d = RootWindow(valueKey.display, valueKey.screenNum);
    } else {
	d = XCreatePixmap(valueKey.display,
		RootWindow(valueKey.display, valueKey.screenNum),
		1, 1, valueKey.depth);
	freeDrawable = d;
    }

    gcPtr->gc = XCreateGC(valueKey.display, d, valueMask, &valueKey.values);
    gcPtr->display = valueKey.display;
    gcPtr->refCount = 1;
    gcPtr->valueHashPtr = valueHashPtr;
    idKey.display = valueKey.display;
    idKey.gc = gcPtr->gc;
    idHashPtr = Tcl_CreateHashEntry(&idTable, (char *) &idKey, &new);
    if (!new) {
	panic("GC already registered in Tk_GetGC");
    }
    Tcl_SetHashValue(valueHashPtr, gcPtr);
    Tcl_SetHashValue(idHashPtr, gcPtr);
    if (freeDrawable != None) {
	XFreePixmap(valueKey.display, freeDrawable);
    }

    return gcPtr->gc;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_FreeGC --
 *
 *	This procedure is called to release a font allocated by
 *	Tk_GetGC.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The reference count associated with gc is decremented, and
 *	gc is officially deallocated if no-one is using it anymore.
 *
 *----------------------------------------------------------------------
 */

void
Tk_FreeGC(display, gc)
    Display *display;		/* Display for which gc was allocated. */
    GC gc;			/* Graphics context to be released. */
{
    IdKey idKey;
    Tcl_HashEntry *idHashPtr;
    register TkGC *gcPtr;

    if (!initialized) {
	panic("Tk_FreeGC called before Tk_GetGC");
    }

    idKey.display = display;
    idKey.gc = gc;
    idHashPtr = Tcl_FindHashEntry(&idTable, (char *) &idKey);
    if (idHashPtr == NULL) {
	panic("Tk_FreeGC received unknown gc argument");
    }
    gcPtr = (TkGC *) Tcl_GetHashValue(idHashPtr);
    gcPtr->refCount--;
    if (gcPtr->refCount == 0) {
	XFreeGC(gcPtr->display, gcPtr->gc);
	Tcl_DeleteHashEntry(gcPtr->valueHashPtr);
	Tcl_DeleteHashEntry(idHashPtr);
	ckfree((char *) gcPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GCInit --
 *
 *	Initialize the structures used for GC management.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Read the code.
 *
 *----------------------------------------------------------------------
 */

static void
GCInit()
{
    initialized = 1;
    Tcl_InitHashTable(&valueTable, sizeof(ValueKey)/sizeof(int));
    Tcl_InitHashTable(&idTable, sizeof(IdKey)/sizeof(int));
}
