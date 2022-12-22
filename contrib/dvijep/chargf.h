/* -*-C-*- chargf.h */
/*-->chargf*/
/**********************************************************************/
/******************************* chargf *******************************/
/**********************************************************************/

int
chargf(c,outfcn)	/* return 0 on success, and EOF on failure */
BYTE c;			/* current character value */
void (*outfcn)();	/* (possibly NULL) function to output current row */
{
    UNSIGN16 d;			/* step in m index */
    BOOLEAN do_output;		/* FALSE if outfcn is NULL */
    register UNSIGN16 k;	/* loop step */
    register INT16 m,n;		/* column,row indices in image[n][m] */
    long p;			/* pointer into font file */
    BYTE paint_switch;		/* alternates between BLACK and WHITE */
    struct char_entry *tcharptr;/* temporary char_entry pointer */
    register BYTE the_byte;	/* current command byte */
    UNSIGN32 the_word;		/* temporary result holder */

/* NB: We only test for equality with BLACK.   WHITE is any non-zero
bit pattern, so we cannot test for equality with it */
#define BLACK ((BYTE)0)
#define WHITE ((BYTE)(!BLACK))

    /*******************************************************************
    This function is called to process a single character description in
    the GF font file,  and to set the  character metrics hp, wp,  xoffp,
    and yoffp for it.  The character  description may start either at  a
    special (XXX) command, or at a beginning-of-character (BOC or  BOC1)
    command.   It  processes  the  character  description  up  to,   and
    including, the end-of-character (EOC) command which terminates it.

    The GF  raster description  is encoded  in a  complex form,  but  is
    guaranteed to step across raster rows  from left to right, and  down
    from the  top  row  to  bottom  row,  such  that  in  references  to
    image[n][m], m never decreases in a row, and n never increases in  a
    character.  This means that we  only require enough memory space  to
    hold one row, provided that outfcn(c,yoff) is called each time a row
    is	 completed.    This   is   an	important   economization    for
    high-resolution output  devices --  e.g. a  10pt character  at  2400
    dots/inch would require about  8Kb for the  entire image, but  fewer
    than 25 bytes for a  single row.  A 72pt  character (such as in  the
    aminch font) would need  about 430Kb for the  image, but only  about
    200 bytes for a row.

    Access to bit m in the current row is controlled through the  macros
    SETBIT(m) and  TESTBIT(m) so  we  need not  be concerned  about  the
    details of bit masking.  Too bad C does not have a bit data type!

    The row image is recorded in such a way that bits min_m .. max_m are
    mapped onto  bits  0  ..  (max_m -  min_m)  in  img_row[],  so  that
    outfcn(c,yoff) should be relieved of any shifting operations.

    outfcn(c,yoff) is NEVER called if either of hp or wp is <= 0, or if
    if it is NULL.
    *******************************************************************/

    if ((c < FIRSTPXLCHAR) || (LASTPXLCHAR < c))
    {
	(void)warning(
	    "chargf():  Character value out of range for GF font file");
	return(EOF);
    }
    tcharptr = &(fontptr->ch[c]);

    p = (long)tcharptr->fontrp;		/* font file raster pointer */
    if (p < 0L)
    {
	(void)warning(
	    "chargf():  Requested character not found in GF font file");
	return(EOF);
    }
    if (FSEEK(fontfp,p,0))
    {
	(void)warning(
	    "chargf():  FSEEK() failure for GF font file character raster");
	return(EOF);
    }

    do_output = (BOOLEAN)(outfcn != (void(*)())NULL);

    (void)skgfspec();			/* skip any GF special commands */

    the_byte = (BYTE)nosignex(fontfp,(BYTE)1);
    if ((the_byte != GFBOC) && (the_byte != GFBOC1))
    {
	(void)warning(
	    "chargf():  GF font file not positioned at BOC or BOC1 command");
	return(EOF);
    }

    for (;;)	/* loop with exit at EOC, or at BOC or BOC1 if no rasters */
    {
	switch (the_byte)
	{
	case GFPAINT0:
	    paint_switch = (BYTE)(!paint_switch);
	    break;

	case GFPAINT1:
	    d = (UNSIGN16)nosignex(fontfp,(BYTE)1);
	    if (do_output && (paint_switch == BLACK))
	    {
	        for (k = 0; k < d; (++m, ++k))
		    SETBIT(m);
	    }
	    else
		m += (INT16)d;
	    paint_switch = (BYTE)(!paint_switch);
	    break;

	case GFPAINT2:
	    d = (UNSIGN16)nosignex(fontfp,(BYTE)2);
	    if (do_output && (paint_switch == BLACK))
	    {
	        for (k = 0; k < d; (++m, ++k))
		    SETBIT(m);
	    }
	    else
		m += (INT16)d;
	    paint_switch = (BYTE)(!paint_switch);
	    break;

	case GFPAINT3:			/* METAFONT never needs this */
	    d = (UNSIGN16)nosignex(fontfp,(BYTE)3);	/* NOTE: truncation */
	    if (do_output && (paint_switch == BLACK))
	    {
	        for (k = 0; k < d; (++m, ++k))
		    SETBIT(m);
	    }
	    else
		m += (INT16)d;
	    paint_switch = (BYTE)(!paint_switch);
	    break;

	case GFBOC:	/* beginning-of-character -- long form */
	    the_word = (UNSIGN32)nosignex(fontfp,(BYTE)4);
	    p = (long)signex(fontfp,(BYTE)4);
	    if ((UNSIGN32)c != the_word)
	    {
		if (p < 0L)
		{
		    (void)warning(
    "chargf():	Requested character not found in back chain in GF font file");
		    return(EOF);
		}
		else
 		    (void)FSEEK(fontfp,p,0);	/* must follow back chain */
		break;
	    }
	    else
	    {
		min_m = (INT16)signex(fontfp,(BYTE)4);
		max_m = (INT16)signex(fontfp,(BYTE)4);
		min_n = (INT16)signex(fontfp,(BYTE)4);
		max_n = (INT16)signex(fontfp,(BYTE)4);
	    }
	    if ((min_m < MIN_M) || (MAX_M < max_m) ||
		(min_n < MIN_N) || (MAX_N < max_n))
	    {
		(void)warning(
		    "chargf():  GF font file character box too large for me");
		return(EOF);
	    }
	    m = min_m;
	    n = max_n;
	    paint_switch = WHITE;
	    img_words = (UNSIGN16)((max_m - min_m + 1 + 31) >> 5);
	    if (do_output)
		(void)clrrow();
	    tcharptr->hp = (COORDINATE)(max_n - min_n + 1);
	    tcharptr->wp = (COORDINATE)(max_m - min_m + 1);
	    tcharptr->xoffp = -(COORDINATE)(min_m);
	    tcharptr->yoffp = (COORDINATE)(max_n);
	    if (!VISIBLE(tcharptr))
		return(0); /* empty character raster -- nothing to output */
	    break;

	case GFBOC1:		/* beginning-of-character -- short form */
	    if (c != (BYTE)nosignex(fontfp,(BYTE)1))
	    {
		(void)warning(
		    "chargf():	Requested character not found in GF font file");
		return(EOF);
	    }
	    else
	    {
		min_m = (INT16)nosignex(fontfp,(BYTE)1);
		max_m = (INT16)nosignex(fontfp,(BYTE)1);
		min_m = max_m - min_m;
		min_n = (INT16)nosignex(fontfp,(BYTE)1);
		max_n = (INT16)nosignex(fontfp,(BYTE)1);
		min_n = max_n - min_n;
	    }
	    if ((min_m < MIN_M) || (MAX_M < max_m) ||
		(min_n < MIN_N) || (MAX_N < max_n))
	    {
		(void)warning(
		    "chargf():  GF font file character box too large for me");
		return(EOF);
	    }
	    m = min_m;
	    n = max_n;
	    paint_switch = WHITE;
	    img_words = (UNSIGN16)((max_m - min_m + 1 + 31) >> 5);
	    if (do_output)
		(void)clrrow();
	    tcharptr->hp = (COORDINATE)(max_n - min_n + 1);
	    tcharptr->wp = (COORDINATE)(max_m - min_m + 1);
	    tcharptr->xoffp = -(COORDINATE)(min_m);
	    tcharptr->yoffp = (COORDINATE)(max_n);
	    if (!VISIBLE(tcharptr))
		return(0);/* empty character raster -- nothing to output */
	    break;

	case GFEOC:			/* end-of-character */
	    if (do_output)
	    {
		(void)(*outfcn)(c,max_n-n);	/* output current row */
		(void)clrrow();		/* clear next one */
		--n;			/* advance to next row below */
		for ( ; n >= min_n ; --n)	/* output any remaining rows */
		    (void)(*outfcn)(c,max_n-n);
	    }
	    return(0);			/* exit outer loop */

	case GFSKIP0:
	    if (do_output)
	    {
		(void)(*outfcn)(c,max_n-n);	/* output current row */
		(void)clrrow();		/* clear next one */
	    }
	    --n;			/* advance to next row below */
	    m = min_m;
	    paint_switch = WHITE;
	    break;

	case GFSKIP1:
	    if (do_output)
	    {
		(void)(*outfcn)(c,max_n-n);	/* output current row */
		(void)clrrow();		/* clear next one */
	    }
	    --n;			/* advance to next row below */
	    d = (UNSIGN16)nosignex(fontfp,(BYTE)1);
	    if (do_output)
	    {
		for (k = 0; k < d; (--n,++k))	/* output d white rows */
		    (void)(*outfcn)(c,max_n-n);
	    }
	    else
		n -= (INT16)d;
	    m = min_m;
	    paint_switch = WHITE;
	    break;

	case GFSKIP2:
	    if (do_output)
	    {
		(void)(*outfcn)(c,max_n-n);	/* output current row */
		(void)clrrow();		/* clear next one */
	    }
	    --n;			/* advance to next row below */
	    d = (UNSIGN16)nosignex(fontfp,(BYTE)2);
	    if (do_output)
	    {
		for (k = 0; k < d; (--n,++k))	/* output d white rows */
		    (void)(*outfcn)(c,max_n-n);
	    }
	    else
		n -= (INT16)d;
	    m = min_m;
	    paint_switch = WHITE;
	    break;

	case GFSKIP3:			/* METAFONT never needs this */
	    if (do_output)
	    {
		(void)(*outfcn)(c,max_n-n);	/* output current row */
		(void)clrrow();		/* clear next one */
	    }
	    --n;			/* advance to next row below */
	    d = (UNSIGN16)nosignex(fontfp,(BYTE)3);	/* NOTE truncation */
	    if (do_output)
	    {
		for (k = 0; k < d; (++k, --n))	/* output d white rows */
		    (void)(*outfcn)(c,max_n-n);
	    }
	    else
		n -= (INT16)d;
	    m = min_m;
	    paint_switch = WHITE;
	    break;

	case GFXXX1:
	case GFXXX2:
	case GFXXX3:
	case GFXXX4:
	case GFYYY:
	case GFNOOP:
	    (void)UNGETC((char)the_byte,fontfp);
	    (void)skgfspec();
	    break;

	default:
	    if (((BYTE)GFPAINT0 < the_byte) && (the_byte < GFPAINT1))
	    {
		d = (UNSIGN16)(the_byte - GFPAINT0);
		if (do_output && (paint_switch == BLACK))
		{
		    for (k = 0; k < d; (++m, ++k))
		        SETBIT(m);
		}
		else
		    m += (INT16)d;
		paint_switch = (BYTE)(!paint_switch);
	    }
	    else if (((BYTE)GFNROW0 <= the_byte) && (the_byte <= GFNROWMAX))
	    {
		if (do_output)
		{
		    (void)(*outfcn)(c,max_n-n);	/* output current row */
		    (void)clrrow();	/* clear next one */
		}
		n--;			/* advance to next row below */
		paint_switch = BLACK;
		m = min_m + (UNSIGN16)(the_byte - GFNROW0);
	    }
	    else
	    {
		(void)sprintf(message,
		    "chargf():  Unexpected byte %d (0x%02x) in GF font file \
at position %ld",
		    the_byte,the_byte,(long)FTELL(fontfp));
		(void)warning(message);
		return(EOF);
	    }
	    break;
	}				/* end switch */
	the_byte = (BYTE)nosignex(fontfp,(BYTE)1);
    }					/* end for() */
}
