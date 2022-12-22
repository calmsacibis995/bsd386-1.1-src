/* 
 * tkGeometry.c --
 *
 *	This file contains code generic Tk code for geometry
 *	management, plus code to manage the geometry of top-level
 *	windows (by reflecting information up to the window
 *	manager).
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
static char rcsid[] = "$Header: /bsdi/MASTER/BSDI_OS/contrib/tk/libtk/tkGeometry.c,v 1.1.1.1 1994/01/03 23:15:25 polk Exp $ SPRITE (Berkeley)";
#endif

#include "tkConfig.h"
#include "tkInt.h"

/*
 *--------------------------------------------------------------
 *
 * Tk_ManageGeometry --
 *
 *	Arrange for a particular procedure to handle geometry
 *	requests for a given window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Proc becomes the new geometry manager for tkwin, replacing
 *	any previous geometry manager.  In the future, whenever
 *	Tk_GeometryRequest is called for tkwin, proc will be
 *	invoked to handle the request.  Proc should have the
 *	following structure:
 *
 *	void
 *	proc(clientData, tkwin)
 *	{
 *	}
 *
 *	The clientData argument will be the same as the clientData
 *	argument to this procedure, and the tkwin arguments will
 *	be the same as the corresponding argument to
 *	Tk_GeometryRequest.  Information about the desired
 *	geometry for tkwin is avilable to proc using macros such
 *	as Tk_ReqWidth.  Proc should do the best it can to meet
 *	the request within the constraints of its geometry-management
 *	algorithm, but it is not obligated to meet the request.
 *
 *--------------------------------------------------------------
 */

void
Tk_ManageGeometry(tkwin, proc, clientData)
    Tk_Window tkwin;		/* Window whose geometry is to
				 * be managed by proc.  */
    Tk_GeometryProc *proc;	/* Procedure to manage geometry.
				 * NULL means make tkwin unmanaged. */
    ClientData clientData;	/* Arbitrary one-word argument to
				 * pass to proc. */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;

    winPtr->geomProc = proc;
    winPtr->geomData = clientData;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_GeometryRequest --
 *
 *	This procedure is invoked by widget code to indicate
 *	its preferences about the size of a window it manages.
 *	In general, widget code should call this procedure
 *	rather than Tk_ResizeWindow.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The geometry manager for tkwin (if any) is invoked to
 *	handle the request.  If possible, it will reconfigure
 *	tkwin and/or other windows to satisfy the request.  The
 *	caller gets no indication of success or failure, but it
 *	will get X events if the window size was actually
 *	changed.
 *
 *--------------------------------------------------------------
 */

void
Tk_GeometryRequest(tkwin, reqWidth, reqHeight)
    Tk_Window tkwin;		/* Window that geometry information
				 * pertains to. */
    int reqWidth, reqHeight;	/* Minimum desired dimensions for
				 * window, in pixels. */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;

    if ((reqWidth == winPtr->reqWidth) && (reqHeight == winPtr->reqHeight)) {
	return;
    }
    winPtr->reqWidth = reqWidth;
    winPtr->reqHeight = reqHeight;
    if (winPtr->geomProc != NULL) {
	(*winPtr->geomProc)(winPtr->geomData, tkwin);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_SetInternalBorder --
 *
 *	Notify relevant geometry managers that a window has an internal
 *	border of a given width and that child windows should not be
 *	placed on that border.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The border width is recorded for the window, and all geometry
 *	managers of all children are notified so that can re-layout, if
 *	necessary.
 *
 *----------------------------------------------------------------------
 */

void
Tk_SetInternalBorder(tkwin, width)
    Tk_Window tkwin;		/* Window that will have internal border. */
    int width;			/* Width of internal border, in pixels. */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;

    if (width == winPtr->internalBorderWidth) {
	return;
    }
    if (width < 0) {
	width = 0;
    }
    winPtr->internalBorderWidth = width;
    for (winPtr = winPtr->childList; winPtr != NULL;
	    winPtr = winPtr->nextPtr) {
	if (winPtr->geomProc != NULL) {
	    (*winPtr->geomProc)(winPtr->geomData, (Tk_Window) winPtr);
	}
    }
}
