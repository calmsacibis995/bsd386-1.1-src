/* -*-C-*- readpost.h */
/*-->readpost*/
/**********************************************************************/
/****************************** readpost ******************************/
/**********************************************************************/

void
readpost()

/***********************************************************************
This routine is used  to read in the  postamble values.  It  initializes
the magnification and checks the stack height prior to starting printing
the document.
***********************************************************************/

{
    long lastpageptr;		/* byte pointer to last physical page */
    int the_page_count;		/* page count from DVI file */

    findpost();
    if ((BYTE)nosignex(dvifp,(BYTE)1) != POST)
	(void)fatal("readpost():  POST missing at head of postamble");

    lastpageptr = (long)nosignex(dvifp,(BYTE)4);
    num = nosignex(dvifp,(BYTE)4);
    den = nosignex(dvifp,(BYTE)4);
    mag = nosignex(dvifp,(BYTE)4);
    conv = ((float)num/(float)den) *
	   ((float)runmag/(float)STDMAG) *

#if    USEGLOBALMAG
	    actfact(mag) *
#endif

	    ((float)RESOLUTION/254000.0);
    /* denominator/numerator here since will be dividing by the conversion
	   factor */
    (void) nosignex(dvifp,(BYTE)4);	/* height-plus-depth of tallest page */
    (void) nosignex(dvifp,(BYTE)4);	/* width of widest page */
    if ((int)nosignex(dvifp,(BYTE)2) >= STACKSIZE)
	(void)fatal("readpost():  Stack size is too small");

    the_page_count = (int)nosignex(dvifp,(BYTE)2);

    if (!quiet)
    {
	(void)fprintf(stderr,"[%d pages]",the_page_count);
	NEWLINE(stderr);

	(void)fprintf(stderr,"[%d magnification]",(int)runmag);
	NEWLINE(stderr);
    }

    if (preload)
        getfntdf();
    getpgtab(lastpageptr);
}


