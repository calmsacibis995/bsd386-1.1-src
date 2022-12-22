
static char rcsid[] = "@(#)Id: pmalloc.c,v 5.3 1992/12/07 04:28:03 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.2 $   $State: Exp $
 *
 *			Copyright (c) 1988-1992 USENET Community Trust
 *			Copyright (c) 1986,1987 Dave Taylor
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 *******************************************************************************
 * $Log: pmalloc.c,v $
 * Revision 1.2  1994/01/14  00:55:28  cp
 * 2.4.23
 *
 * Revision 5.3  1992/12/07  04:28:03  syd
 * change include from defs to headers as now needs LINES
 * From: Syd
 *
 * Revision 5.2  1992/11/26  00:46:13  syd
 * changes to first change screen back (Raw off) and then issue final
 * error message.
 * From: Syd
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** This routine contains a cheap permanent version of the malloc call to 
    speed up the initial allocation of the weedout headers and the uuname 
    data.  

      This routine is originally from Jim Davis of HP Labs, with some 
    mods by me.
**/

#include <stdio.h>
#include "headers.h"
#include "s_elm.h"

extern nl_catd elm_msg_cat;	/* message catalog	    */
/*VARARGS0*/

char *pmalloc(size)
int size; 
{
	/** return the address of a specified block **/

	static char *our_block = NULL;
	static int   free_mem  = 0;

	char   *return_value;

	/** if bigger than our threshold, just do the real thing! **/

	if (size > PMALLOC_THRESHOLD) 
	   return((char *) malloc(size));

	/** if bigger than available space, get more, tossing what's left **/

	if (size > free_mem) {
	  if ((our_block = (char *) malloc(PMALLOC_BUFFER_SIZE)) == NULL) {
	    MoveCursor(LINES,0);
	    Raw(OFF);
	    fprintf(stderr, catgets(elm_msg_cat, ElmSet, ElmCouldntMallocBytes,
		    "\n\nCouldn't malloc %d bytes!!\n\n"),
		    PMALLOC_BUFFER_SIZE);
	    leave(0);	
          }
	  our_block += 4;  /* just for safety, don't give back true address */
	  free_mem = PMALLOC_BUFFER_SIZE-4;
	}
	
	return_value  = our_block;	/* get the memory */
	size = ((size+3)/4)*4;		/* Go to quad byte boundary */
	our_block += size;		/* use it up      */
	free_mem  -= size;		/*  and decrement */

	return( (char *) return_value);
}
