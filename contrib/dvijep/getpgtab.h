/* -*-C-*- getpgtab.h */
/*-->getpgtab*/
/**********************************************************************/
/****************************** getpgtab ******************************/
/**********************************************************************/

void
getpgtab(lastpageptr)
long lastpageptr;

{
    register long p;
    register INT16 i,k;

    (void) FSEEK (dvifp,lastpageptr,0);
    p = lastpageptr;

    for (k = 0; (p != (-1)) && (k < MAXPAGE); ++k)
    {
        page_ptr[MAXPAGE-1-k] = p;
        (void) FSEEK (dvifp,(long) p, 0);

        if ((BYTE)nosignex(dvifp,(BYTE)1) != BOP)
            (void)fatal(
		"getpgtab():  Invalid BOP (beginning-of-page) back chain");

        for (i = 0; i <= 9; ++i)
            (void) nosignex(dvifp,(BYTE)4);   /* discard count0..count9 */
        p = (long)signex(dvifp,(BYTE)4);
    }
    page_count = k;
    if (k >= MAXPAGE)
        (void)warning("getpgtab():  Page table full...rebuild driver with \
larger MAXPAGE");
    else	/* move pointer table to front of array */
        for (k = 0; k < page_count; ++k)
	    page_ptr[k] = page_ptr[MAXPAGE-page_count+k];
}


