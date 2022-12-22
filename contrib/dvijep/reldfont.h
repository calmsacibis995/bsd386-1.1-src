/* -*-C-*- reldfont.h */
/*-->reldfont*/
/**********************************************************************/
/****************************** reldfont ******************************/
/**********************************************************************/

void
reldfont(tfontptr)		/* load (or reload) font parameters */
struct font_entry *tfontptr;
{
    register UNSIGN16 the_char;	/* loop index */
    int err_code;
    register struct char_entry *tcharptr;/* temporary char_entry pointer */

    tfontptr->font_mag = (UNSIGN32)((actfact(
        MAGSIZE((float)tfontptr->s/(float)tfontptr->d)) *
	((float)runmag/(float)STDMAG) *

#if    USEGLOBALMAG
	actfact(mag) *
#endif

	(float)RESOLUTION * 5.0) + 0.5);

    tfontptr->designsize = (UNSIGN32)0L;
    tfontptr->hppp = (UNSIGN32)0L;
    tfontptr->vppp = (UNSIGN32)0L;
    tfontptr->min_m = (INT32)0L;
    tfontptr->max_m = (INT32)0L;
    tfontptr->min_n = (INT32)0L;
    tfontptr->max_n = (INT32)0L;

    for (the_char = FIRSTPXLCHAR; the_char <= LASTPXLCHAR; the_char++)
    {
	tcharptr = &(tfontptr->ch[the_char]);
	tcharptr->dx = (INT32)0L;
	tcharptr->dy = (INT32)0L;
	tcharptr->hp = (COORDINATE)0;
	tcharptr->fontrp = -1L;
	tcharptr->pxlw = (UNSIGN16)0;
	tcharptr->rasters = (UNSIGN32*)NULL;
	tcharptr->refcount = 0;
	tcharptr->tfmw = 0L;
	tcharptr->wp = (COORDINATE)0;
	tcharptr->xoffp = (COORDINATE)0;
	tcharptr->yoffp = (COORDINATE)0;
    }

    if (tfontptr != pfontptr)
	(void)openfont(tfontptr->n);

    if (fontfp == (FILE *)NULL)	/* have empty font with zero metrics */
	return;

    for (;;)		/* fake one-trip loop */
    {	/* test for font types PK, GF, and PXL in order of preference */
    	(void)REWIND(fontfp);	/* position to beginning-of-file */
    	if ( ((BYTE)nosignex(fontfp,(BYTE)1) == (BYTE)PKPRE) &&
    	    ((BYTE)nosignex(fontfp,(BYTE)1) == (BYTE)PKID) )
    	{
    	    tfontptr->font_type = (BYTE)FT_PK;
	    tfontptr->charxx = (void(*)())charpk;
    	    err_code = readpk();
	    break;
    	}

        (void)REWIND(fontfp);	/* position to beginning-of-file */
        if ( ((BYTE)nosignex(fontfp,(BYTE)1) == (BYTE)GFPRE) &&
	    ((BYTE)nosignex(fontfp,(BYTE)1) == (BYTE)GFID) )
        {
	    tfontptr->font_type = (BYTE)FT_GF;
	    tfontptr->charxx = (void(*)())chargf;
	    err_code = readgf();
	    break;
        }

	(void)REWIND(fontfp);	/* position to beginning-of-file */
	if (nosignex(fontfp,(BYTE)4) == (UNSIGN32)PXLID)
	{
	    tfontptr->font_type = (BYTE)FT_PXL;
	    tfontptr->charxx = (void(*)())charpxl;
	    err_code = readpxl();
	    break;
	}

	err_code = (int)EOF;
	break;
    } /* end one-trip loop */

    if (err_code)
    {
        (void)sprintf(message,
	    "reldfont():  Font file [%s] is not a valid GF, PK, or PXL file",
	    tfontptr->name);
    	(void)fatal(message);
    }
}
