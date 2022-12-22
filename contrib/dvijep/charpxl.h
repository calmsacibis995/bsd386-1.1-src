/* -*-C-*- charpxl.h */
/*-->charpxl*/
/**********************************************************************/
/******************************* charpxl ******************************/
/**********************************************************************/

int
charpxl(c,outfcn)	/* return 0 on success, and EOF on failure */
BYTE c;			/* current character value */
void (*outfcn)();	/* (possibly NULL) function to output current row */
{
    UNSIGN16 i,j;		/* loop index */
    long p;			/* offset into font file */
    register UNSIGN32 *q;	/* pointer into rasters area */
    struct char_entry *tcharptr;/* temporary char_entry pointer */

    if ((c < FIRSTPXLCHAR) || (LASTPXLCHAR < c))
    {
	(void)warning(
	    "charpxl():  Character value out of range for PXL font file");
	return(EOF);
    }
    tcharptr = &(fontptr->ch[c]);

    if (!VISIBLE(tcharptr))
	return(0);	/* do nothing for empty characters */

    p = (long)tcharptr->fontrp;	/* font file raster pointer */
    if (p < 0L)
    {
	(void)warning(
	    "charpxl():  Requested character not found in PXL font file");
	return(EOF);
    }

    if (FSEEK(fontfp,p,0))
    {
	(void)warning(
	    "charpxl():  FSEEK() failure for PXL font file character raster");
	return(EOF);
    }

    if (outfcn != (void(*)())NULL)
    {
	img_words = ((UNSIGN16)(tcharptr->wp) + 31) >> 5;
	for (i = 0; i < (UNSIGN16)(tcharptr->hp); ++i)
	{
	    q = img_row;
	    for (j = 0; j < img_words; ++j)
		*q++ = (UNSIGN32)nosignex(fontfp,(BYTE)4);
	    (void)(*outfcn)(c,i);
	}
    }
    return(0);
}
