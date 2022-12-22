/* 
 * tkPreserve.c --
 *
 *	This file contains a collection of procedures that are used
 *	to make sure that widget records and other data structures
 *	aren't reallocated when there are nested procedures that
 *	depend on their existence.
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
static char rcsid[] = "$Header: /bsdi/MASTER/BSDI_OS/contrib/tk/libtk/tkPreserve.c,v 1.1.1.1 1994/01/03 23:15:28 polk Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "tkConfig.h"
#include "tk.h"

/*
 * The following data structure is used to keep track of all the
 * Tk_Preserve calls that are still in effect.  It grows as needed
 * to accommodate any number of calls in effect.
 */

typedef struct {
    ClientData clientData;	/* Address of preserved block. */
    int refCount;		/* Number of Tk_Preserve calls in effect
				 * for block. */
    int mustFree;		/* Non-zero means Tk_EventuallyFree was
				 * called while a Tk_Preserve call was in
				 * effect, so the structure must be freed
				 * when refCount becomes zero. */
    Tk_FreeProc *freeProc;	/* Procedure to call to free. */
} Reference;

static Reference *refArray;	/* First in array of references. */
static int spaceAvl = 0;	/* Total number of structures available
				 * at *firstRefPtr. */
static int inUse = 0;		/* Count of structures currently in use
				 * in refArray. */
#define INITIAL_SIZE 2

/*
 *----------------------------------------------------------------------
 *
 * Tk_Preserve --
 *
 *	This procedure is used by a procedure to declare its interest
 *	in a particular block of memory, so that the block will not be
 *	reallocated until a matching call to Tk_Release has been made.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information is retained so that the block of memory will
 *	not be freed until at least the matching call to Tk_Release.
 *
 *----------------------------------------------------------------------
 */

void
Tk_Preserve(clientData)
    ClientData clientData;	/* Pointer to malloc'ed block of memory. */
{
    register Reference *refPtr;
    int i;

    /*
     * See if there is already a reference for this pointer.  If so,
     * just increment its reference count.
     */

    for (i = 0, refPtr = refArray; i < inUse; i++, refPtr++) {
	if (refPtr->clientData == clientData) {
	    refPtr->refCount++;
	    return;
	}
    }

    /*
     * Make a reference array if it doesn't already exist, or make it
     * bigger if it is full.
     */

    if (inUse == spaceAvl) {
	if (spaceAvl == 0) {
	    refArray = (Reference *) ckalloc((unsigned)
		    (INITIAL_SIZE*sizeof(Reference)));
	    spaceAvl = INITIAL_SIZE;
	} else {
	    Reference *new;

	    new = (Reference *) ckalloc((unsigned)
		    (2*spaceAvl*sizeof(Reference)));
	    memcpy((VOID *) new, (VOID *) refArray, spaceAvl*sizeof(Reference));
	    ckfree((char *) refArray);
	    refArray = new;
	    spaceAvl *= 2;
	}
    }

    /*
     * Make a new entry for the new reference.
     */

    refPtr = &refArray[inUse];
    refPtr->clientData = clientData;
    refPtr->refCount = 1;
    refPtr->mustFree = 0;
    inUse += 1;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_Release --
 *
 *	This procedure is called to cancel a previous call to
 *	Tk_Preserve, thereby allowing a block of memory to be
 *	freed (if no one else cares about it).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If Tk_EventuallyFree has been called for clientData, and if
 *	no other call to Tk_Preserve is still in effect, the block of
 *	memory is freed.
 *
 *----------------------------------------------------------------------
 */

void
Tk_Release(clientData)
    ClientData clientData;	/* Pointer to malloc'ed block of memory. */
{
    register Reference *refPtr;
    int i;

    for (i = 0, refPtr = refArray; i < inUse; i++, refPtr++) {
	if (refPtr->clientData != clientData) {
	    continue;
	}
	refPtr->refCount--;
	if (refPtr->refCount == 0) {
	    if (refPtr->mustFree) {
		if (refPtr->freeProc == (Tk_FreeProc *) free) {
		    ckfree((char *) refPtr->clientData);
		} else {
		    (*refPtr->freeProc)(refPtr->clientData);
		}
	    }

	    /*
	     * Copy down the last reference in the array to fill the
	     * hole left by the unused reference.
	     */

	    inUse--;
	    if (i < inUse) {
		refArray[i] = refArray[inUse];
	    }
	}
	return;
    }

    /*
     * Reference not found.  This is a bug in the caller.
     */

    panic("Tk_Release couldn't find reference for 0x%x", clientData);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_EventuallyFree --
 *
 *	Free up a block of memory, unless a call to Tk_Preserve is in
 *	effect for that block.  In this case, defer the free until all
 *	calls to Tk_Preserve have been undone by matching calls to
 *	Tk_Release.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Ptr may be released by calling free().
 *
 *----------------------------------------------------------------------
 */

void
Tk_EventuallyFree(clientData, freeProc)
    ClientData clientData;	/* Pointer to malloc'ed block of memory. */
    Tk_FreeProc *freeProc;	/* Procedure to actually do free. */
{
    register Reference *refPtr;
    int i;

    /*
     * See if there is a reference for this pointer.  If so, set its
     * "mustFree" flag (the flag had better not be set already!).
     */

    for (i = 0, refPtr = refArray; i < inUse; i++, refPtr++) {
	if (refPtr->clientData != clientData) {
	    continue;
	}
	if (refPtr->mustFree) {
	    panic("Tk_EventuallyFree called twice for 0x%x\n", clientData);
        }
        refPtr->mustFree = 1;
	refPtr->freeProc = freeProc;
        return;
    }

    /*
     * No reference for this block.  Free it now.
     */

    if (freeProc == (Tk_FreeProc *) free) {
	ckfree((char *) clientData);
    } else {
	(*freeProc)(clientData);
    }
}
