/* -*-C-*- readpk.h */
/*-->readpk*/
/**********************************************************************/
/******************************* readpk *******************************/
/**********************************************************************/

int
readpk()	/* return 0 on success, EOF on failure */
{
    UNSIGN32 checksum;
    long end_of_packet;	/* pointer to byte following character packet */
    register BYTE flag_byte;
    UNSIGN32 packet_length;
    long start_of_packet;	/* pointer to start of character packet */
    register struct char_entry *tcharptr;/* temporary char_entry pointer */
    register UNSIGN32 the_char;

    /* Read a PK file, extracting character metrics and raster
       locations. */

    /******************************************************************/

    /* Process the preamble parameters */

    (void)REWIND(fontfp);	/* start at beginning of file */
    if ((BYTE)nosignex(fontfp,(BYTE)1) != PKPRE)
    {
	(void)warning("readpk():  PK font file does not start with PRE byte");
	return(EOF);
    }
    if ((BYTE)nosignex(fontfp,(BYTE)1) != PKID)
    {
	(void)warning(
	    "readpk():  PK font file PRE byte not followed by ID byte");
	return(EOF);
    }

    if (FSEEK(fontfp,(long)nosignex(fontfp,(BYTE)1),1))
    {
	(void)warning("readpk():  PK font file has garbled comment string");
	return(EOF);		/* skip comment string */
    }

    fontptr->designsize = nosignex(fontfp,(BYTE)4);

    checksum = nosignex(fontfp,(BYTE)4);	/* checksum */
    if ((fontptr->c != 0L) && (checksum != 0L) && (fontptr->c != checksum))
    {
	(void)sprintf(message,
	"readpk():  font [%s] has checksum = 10#%010lu [16#%08lx] [8#%011lo] \
different from DVI checksum = 10#%010lu [16#%08lx] [8#%011lo].  \
TeX preloaded .fmt file is probably out-of-date with respect to new fonts.",
	    fontptr->name, fontptr->c, fontptr->c, fontptr->c,
	    checksum, checksum, checksum);
	(void)warning(message);
    }

    fontptr->hppp = (UNSIGN32)nosignex(fontfp,(BYTE)4);
    fontptr->vppp = (UNSIGN32)nosignex(fontfp,(BYTE)4);

    fontptr->min_m = 0L;	/* these are unused in PK format */
    fontptr->max_m = 0L;
    fontptr->min_n = 0L;
    fontptr->max_n = 0L;

    fontptr->magnification = (UNSIGN32)( 0.5 + 5.0 * 72.27 *
	(float)(fontptr->hppp) / 65536.0 );	/* from GFtoPXL Section 53 */

    /******************************************************************/
    /* Process the characters until the POST byte is reached */

    (void)skpkspec();	/* skip any PK specials */

    start_of_packet = (long)FTELL(fontfp);
    while ((flag_byte = (BYTE)nosignex(fontfp,(BYTE)1)) != PKPOST)
    {
	flag_byte &= 0x07;
	switch(flag_byte)	/* flag_byte is in 0..7 */
	{
	case 0:
	case 1:
	case 2:
	case 3:		/* short character preamble */
	    packet_length = (UNSIGN32)flag_byte;
	    packet_length <<= 8;
	    packet_length += (UNSIGN32)nosignex(fontfp,(BYTE)1);
	    the_char = (UNSIGN32)nosignex(fontfp,(BYTE)1);
	    if (the_char >= NPXLCHARS)
	    {
		(void)warning(
		"readpk():  PK font file character number is too big for me");
	        return(EOF);
	    }
	    tcharptr = &(fontptr->ch[the_char]);
	    end_of_packet = (long)FTELL(fontfp) + (long)packet_length;
	    tcharptr->tfmw = (UNSIGN32)(((float)nosignex(fontfp,(BYTE)3) *
		(float)fontptr->s) / (float)(1L << 20));
	    tcharptr->dx = (INT32)nosignex(fontfp,(BYTE)1) << 16;
	    tcharptr->dy = 0L;
	    tcharptr->pxlw = (UNSIGN16)PIXROUND(tcharptr->dx, 1.0/65536.0);
	    tcharptr->wp = (COORDINATE)nosignex(fontfp,(BYTE)1);
	    tcharptr->hp = (COORDINATE)nosignex(fontfp,(BYTE)1);
	    tcharptr->xoffp = (COORDINATE)signex(fontfp,(BYTE)1);
	    tcharptr->yoffp = (COORDINATE)signex(fontfp,(BYTE)1);
	    break;

	case 4:
	case 5:
	case 6:		/* extended short character preamble */
	    packet_length = (UNSIGN32)(flag_byte & 0x03);
	    packet_length <<= 16;
	    packet_length += (UNSIGN32)nosignex(fontfp,(BYTE)2);
	    the_char = (UNSIGN32)nosignex(fontfp,(BYTE)1);
	    if (the_char >= NPXLCHARS)
	    {
		(void)warning(
		"readpk():  PK font file character number is too big for me");
	        return(EOF);
	    }
	    tcharptr = &(fontptr->ch[the_char]);
	    end_of_packet = (long)FTELL(fontfp) + (long)packet_length;
	    tcharptr->tfmw = (UNSIGN32)(((float)nosignex(fontfp,(BYTE)3) *
		(float)fontptr->s) / (float)(1L << 20));
	    tcharptr->dx = (INT32)nosignex(fontfp,(BYTE)2) << 16;
	    tcharptr->dy = 0L;
	    tcharptr->pxlw = (UNSIGN16)PIXROUND(tcharptr->dx, 1.0/65536.0);
	    tcharptr->wp = (COORDINATE)nosignex(fontfp,(BYTE)2);
	    tcharptr->hp = (COORDINATE)nosignex(fontfp,(BYTE)2);
	    tcharptr->xoffp = (COORDINATE)signex(fontfp,(BYTE)2);
	    tcharptr->yoffp = (COORDINATE)signex(fontfp,(BYTE)2);
	    break;

	case 7:		/* long character preamble */
	    packet_length = (UNSIGN32)nosignex(fontfp,(BYTE)4);
	    the_char = (UNSIGN32)nosignex(fontfp,(BYTE)4);
	    if (the_char >= NPXLCHARS)
	    {
		(void)warning(
		"readpk():  PK font file character number is too big for me");
	        return(EOF);
	    }
	    tcharptr = &(fontptr->ch[the_char]);
	    end_of_packet = (long)FTELL(fontfp) + (long)packet_length;
	    tcharptr->tfmw = (UNSIGN32)(((float)nosignex(fontfp,(BYTE)4) *
		(float)fontptr->s) / (float)(1L << 20));
	    tcharptr->dx = (INT32)signex(fontfp,(BYTE)4);
	    tcharptr->dy = (INT32)signex(fontfp,(BYTE)4);
	    tcharptr->pxlw = (UNSIGN16)PIXROUND(tcharptr->dx, 1.0/65536.0);
	    tcharptr->wp = (COORDINATE)nosignex(fontfp,(BYTE)4);
	    tcharptr->hp = (COORDINATE)nosignex(fontfp,(BYTE)4);
	    tcharptr->xoffp = (COORDINATE)signex(fontfp,(BYTE)4);
	    tcharptr->yoffp = (COORDINATE)signex(fontfp,(BYTE)4);
	    break;
	} /* end switch */
	tcharptr->fontrp = start_of_packet;
	(void)FSEEK(fontfp,end_of_packet,0); /* position to end of packet */
	(void)skpkspec();	/* skip any PK specials */
	start_of_packet = (long)FTELL(fontfp);
    } /* end while */

#if    (BBNBITGRAPH | HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2)
    (void)newfont();
#endif

    return(0);
}
