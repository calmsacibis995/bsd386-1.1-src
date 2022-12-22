/* -*-C-*- readpxl.h */
/*-->readpxl*/
/**********************************************************************/
/****************************** readpxl *******************************/
/**********************************************************************/

int
readpxl()	/* return 0 on success, EOF on failure */
{
    UNSIGN32 checksum;
    register struct char_entry *tcharptr;/* temporary char_entry pointer */
    register UNSIGN16 the_char;		/* loop index */

#if    OS_VAXVMS
    /* VMS binary files are stored with NUL padding to the next 512 byte
    multiple.  We therefore search backwards to the last non-NULL byte
    to the find real end-of-file, then move back 20 bytes from that. */
    FSEEK(fontfp,0L,2);		/* seek to end-of-file */
    while (FSEEK(fontfp,-1L,1) == 0)
    {
        the_char = (UNSIGN16)fgetc(fontfp);
	if (the_char)
	  break;		/* exit leaving pointer PAST last non-NUL */
	UNGETC((char)the_char,fontfp);
    }
    if (FSEEK(fontfp,-20L,1))	/* 20 bytes before last non-NUL for checksum */
#else
    if (FSEEK(fontfp,-20L,2))	/* 20 bytes before end-of-file for checksum */
#endif
    {
	(void)warning("readpxl():  FSEEK() failed--PXL font file may be empty");
	return(EOF);
    }

    checksum = nosignex(fontfp,(BYTE)4);
    if ((fontptr->c != 0L) && (checksum != 0L) && (fontptr->c != checksum))
    {
	(void)sprintf(message,
	"readpxl():  font [%s] has checksum = 10#%010lu [16#%08lx] [8#%011lo] \
different from DVI checksum = 10#%010lu [16#%08lx] [8#%011lo].  \
TeX preloaded .fmt file is probably out-of-date with respect to new fonts.",
	    fontptr->name, fontptr->c, fontptr->c, fontptr->c,
	    checksum, checksum, checksum);
	(void)warning(message);
    }
    fontptr->magnification = nosignex(fontfp,(BYTE)4);
    fontptr->designsize = nosignex(fontfp,(BYTE)4);
    if (FSEEK(fontfp, (long)(nosignex(fontfp,(BYTE)4) << 2), 0))
    {
	(void)warning(
	"readpxl():  FSEEK() did not find PXL font file character directory");
	return(EOF);
    }

    for (the_char = FIRSTPXLCHAR; the_char <= 127; the_char++)
    {
	tcharptr = &(fontptr->ch[the_char]);
	tcharptr->wp = (COORDINATE)nosignex(fontfp,(BYTE)2);
	tcharptr->hp = (COORDINATE)nosignex(fontfp,(BYTE)2);
	tcharptr->xoffp = (COORDINATE)signex(fontfp,(BYTE)2);
	tcharptr->yoffp = (COORDINATE)signex(fontfp,(BYTE)2);

	/* convert (32-bit) word pointer to byte pointer */
	tcharptr->fontrp = (long)(nosignex(fontfp,(BYTE)4) << 2);

	tcharptr->tfmw = (UNSIGN32)(((float)nosignex(fontfp,(BYTE)4) *
	    (float)fontptr->s) / (float)(1L<<20));
	tcharptr->pxlw = (UNSIGN16)PIXROUND((INT32)(tcharptr->tfmw), conv);
	tcharptr->refcount = 0;			/* character unused */
	tcharptr->rasters = (UNSIGN32*)NULL;	/* no raster description */
    }

#if    (BBNBITGRAPH | HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2)
    (void)newfont();
#endif

    return(0);
}
