/* -*-C-*- readgf.h */
/*-->readgf*/
/**********************************************************************/
/******************************* readgf *******************************/
/**********************************************************************/

int
readgf()	/* return 0 on success, EOF on failure */
{
    UNSIGN32 checksum;
    register struct char_entry *tcharptr;/* temporary char_entry pointer */
    register BYTE the_byte;
    register UNSIGN16 the_char;	/* loop index */

    /*
    Read a GF  file postamble, extracting  character metrics and  raster
    locations.  The character xoffp, yoffp,  wp, and hp values will  not
    be filled in until a character raster description is actually called
    for.
    */

    /******************************************************************/

    /* Process the post-postamble */

#if    OS_VAXVMS
    /* VMS binary files are stored with NUL padding to the next 512 byte
    multiple.  We therefore search backwards to the last non-NULL byte
    to the find real end-of-file, then move back 4 bytes from that. */
    FSEEK(fontfp,0L,2);		/* seek to end-of-file */
    while (FSEEK(fontfp,-1L,1) == 0)
    {
        the_char = (UNSIGN16)fgetc(fontfp);
	if (the_char)
	  break;		/* exit leaving pointer PAST last non-NUL */
	UNGETC((char)the_char,fontfp);
    }
    if (FSEEK(fontfp,-4L,1))	/* start 4 bytes before last non-NUL */
#else
    if (FSEEK(fontfp,-4L,2))	/* start 4 bytes from end-of-file */
#endif
    {
	(void)warning("readgf():  FSEEK() failed--GF font file may be empty");
	return(EOF);
    }

    while ((the_byte = (BYTE)nosignex(fontfp,(BYTE)1)) == GFEOF)
    {
	if (FSEEK(fontfp,-2L,1))/* backup 2 bytes to read previous one */
	{
	    (void)warning(
	    "readgf():  FSEEK() failed to find GF font file EOF trailer bytes");
	    return(EOF);
	}
    }

    if (the_byte != (BYTE)GFID)
    {
	(void)warning("readgf():  GF font file ID byte not found");
	return(EOF);
    }

    if (FSEEK(fontfp,-6L,1))	/* backup to read post_post */
    {
	(void)warning(
	    "readgf():  FSEEK() failed to find GF font file POSTPOST byte");
	return(EOF);
    }

    if ((BYTE)nosignex(fontfp,(BYTE)1) != (BYTE)GFPOSTPOST)
    {
	(void)warning("readgf():  GF font file POSTPOST byte not found");
	return(EOF);
    }

    if (FSEEK(fontfp,(long)nosignex(fontfp,(BYTE)4),0))
    {		/* position to start of postamble */
	(void)warning(
	    "readgf():  FSEEK() failed to find postamble in GF font file");
	return(EOF);
    }

    /******************************************************************/

    /* Process the postamble */

    if ((BYTE)nosignex(fontfp,(BYTE)1) != (BYTE)GFPOST)
    {
	(void)warning("readgf():  GF font file POST byte not found");
	return(EOF);
    }

    (void)nosignex(fontfp,(BYTE)4);	/* discard special pointer p[4] */

    fontptr->designsize = nosignex(fontfp,(BYTE)4);

    checksum = nosignex(fontfp,(BYTE)4);
    if ((fontptr->c != 0L) && (checksum != 0L) && (fontptr->c != checksum))
    {
	(void)sprintf(message,
	"readgf():  font [%s] has checksum = 10#%010lu [16#%08lx] [8#%011lo] \
different from DVI checksum = 10#%010lu [16#%08lx] [8#%011lo].  \
TeX preloaded .fmt file is probably out-of-date with respect to new fonts.",
	    fontptr->name, fontptr->c, fontptr->c, fontptr->c,
	    checksum, checksum, checksum);
	(void)warning(message);
    }

    fontptr->hppp = (UNSIGN32)nosignex(fontfp,(BYTE)4);
    fontptr->vppp = (UNSIGN32)nosignex(fontfp,(BYTE)4);
    fontptr->min_m = (INT32)signex(fontfp,(BYTE)4);
    fontptr->max_m = (INT32)signex(fontfp,(BYTE)4);
    fontptr->min_n = (INT32)signex(fontfp,(BYTE)4);
    fontptr->max_n = (INT32)signex(fontfp,(BYTE)4);

    fontptr->magnification = (UNSIGN32)( 0.5 + 5.0 * 72.27 *
	(float)(fontptr->hppp) / 65536.0 );	/* from GFtoPXL Section 53 */

    /******************************************************************/

    /* Process the character locators */

    the_byte = (BYTE)GFNOOP;
    while (the_byte == (BYTE)GFNOOP)
    {
	switch(the_byte = (BYTE)nosignex(fontfp,(BYTE)1))
	{
	case GFCHLOC:
	case GFCHLOC0:
	    the_char = (UNSIGN16)nosignex(fontfp,(BYTE)1);
	    			/* character number MOD 256 */
	    if (the_char >= NPXLCHARS)
	    {
		(void)warning(
		"readgf():  GF font file character number is too big for me");
		return(EOF);
	    }
	    tcharptr = &(fontptr->ch[the_char]);
	    if (the_byte == (BYTE)GFCHLOC)	/* long form */
	    {
		tcharptr->dx = (INT32)signex(fontfp,(BYTE)4);
		tcharptr->dy = (INT32)signex(fontfp,(BYTE)4);
	    }
	    else	/* short form */
	    {
		tcharptr->dx = (INT32)(((UNSIGN32)nosignex(fontfp,(BYTE)1)) <<
		    16);
		tcharptr->dy = 0L;
	    }
	    tcharptr->tfmw = (UNSIGN32)(((float)nosignex(fontfp,(BYTE)4) *
		(float)fontptr->s) / (float)(1L << 20));
	    tcharptr->pxlw = (UNSIGN16)PIXROUND(tcharptr->dx, 1.0/65536.0);
	    tcharptr->fontrp = (long)signex(fontfp,(BYTE)4);

	    the_byte = (BYTE)GFNOOP;	/* to force loop continuation */
	    break;

	case GFNOOP:
	    break;

	default:	/* this causes loop exit */
	    break;
	}	/* end switch */
    }	/* end while */

    for (the_char = FIRSTPXLCHAR; the_char <= LASTPXLCHAR; the_char++)
    {	/* Get remaining character metrics, and ignore error returns for now. */
	tcharptr = &(fontptr->ch[the_char]);
	if (tcharptr->fontrp >= 0L)
	    (void)chargf((BYTE)the_char,(void(*)())NULL);
    }

#if    (BBNBITGRAPH | HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2)
    (void)newfont();
#endif

    return(0);
}
