/* -*-C-*- charpk.h */
/*-->charpk*/
/**********************************************************************/
/******************************* charpk *******************************/
/**********************************************************************/

/* This file departs from all others in the DVIxxx family in that it
declares some private functions and globals which are required nowhere
else.  Only charpk() is called from outside. */

BYTE get_bit();			/* return next bit from file */
BYTE get_nybble();		/* return next 4-bit nybble from file */
INT32 pk_packed_num();		/* get packed number from file */

BYTE bit_weight;		/* weight of current bit in next_byte */
BYTE dyn_f;			/* dynamic packing flag */
INT32 repeat_count;		/* how many times to repeat next row */
BYTE next_byte;			/* current input byte */
BYTE word_weight;		/* weight of current bit in *pword */

/**********************************************************************/

int
charpk(c,outfcn)	/* return 0 on success, and EOF on failure */
BYTE c;			/* current character value */
void (*outfcn)();	/* (possibly NULL) function to output current row */
{
    INT32 count;		/* how many bits of current color left */
    UNSIGN16 c_width;		/* character width in dots */
    UNSIGN16 c_height;		/* character height in dots */
    BOOLEAN do_output;		/* FALSE if outfcn is NULL */
    register BYTE flag_byte;	/* current file flag byte */
    INT32 h_bit;		/* horizontal position in row */
    register INT32 i;		/* loop index */
    register UNSIGN16 k;	/* loop index */
    long offset;		/* offset into font file */
    long p;			/* pointer into font file */
    UNSIGN32 packet_length;	/* packet length */
    UNSIGN32* pword;		/* pointer into img_row[] */
    INT32 rows_left;		/* how many rows left */
    struct char_entry *tcharptr;/* temporary char_entry pointer */
    BYTE turn_on;		/* initial black/white flag */

    /*******************************************************************
    This function is called to process a single character description in
    the	 PK  font  file.   The	 character  packet  is	pointed	 to   by
    fontptr->ch[c].fontrp, and was set by readpk(), which moved past any
    special commands that precede the packet.

    The PK  raster description	is encoded  in a  complex form,	 but  is
    guaranteed to step across raster rows  from left to right, and  down
    from the  top  row	to  bottom  row,  such	that  in  references  to
    image[n][m], m never decreases in a row, and n never increases in  a
    character.	This means that we  only require enough memory space  to
    hold one row, provided that outfcn(c,yoff) is called each time a row
    is	 completed.    This   is   an	important   economization    for
    high-resolution output  devices --	e.g. a	10pt character	at  2400
    dots/inch would require about  8Kb for the	entire image, but  fewer
    than 25 bytes for a	 single row.  A 72pt  character (such as in  the
    aminch font) would need  about 430Kb for the  image, but only  about
    200 bytes for a row.

    The code  follows  PKtoPX  quite closely,  particular  the	latter's
    sections 37-43  for	 preamble  processing, and  sections  44-51  for
    raster decoding.

    Access to bit m in the current row is controlled through the  macros
    SETBIT(m) and  TESTBIT(m) so  we  need not	be concerned  about  the
    details of bit masking.  Too bad C does not have a bit data type!

    The row image is recorded in such a way that bits min_m .. max_m are
    mapped onto	 bits  0  ..  (max_m -	min_m)	in  img_row[],	so  that
    outfcn(c,yoff) should be relieved of any shifting operations.

    outfcn(c,yoff) is NEVER called if either of hp or wp is <= 0, or  if
    it is NULL.
    *******************************************************************/

    if ((c < FIRSTPXLCHAR) || (LASTPXLCHAR < c))
    {
	(void)warning(
	    "charpk():	Character value out of range for PK font file");
	return(EOF);
    }
    tcharptr = &(fontptr->ch[c]);

    if (!VISIBLE(tcharptr))
	return(0); /* empty character raster -- nothing to output */

    c_width = (UNSIGN16)tcharptr->wp;	/* local copies of these to save */
    c_height = (UNSIGN16)tcharptr->hp;	/* pointer dereferencing in loops */

    p = (long)tcharptr->fontrp; /* font file raster pointer */
    if (p < 0L)
    {
	(void)warning(
	    "charpk():  Requested character not found in PK font file");
	return(EOF);
    }
    if (FSEEK(fontfp,p,0))
    {
	(void)warning(
	    "charpk():  FSEEK() failure for PK font file character raster");
	return(EOF);
    }

    /* Since readpk() has already done error checking on the file, and
    recorded the character metrics, we need not repeat that work, and
    concentrate instead on getting to the encoded raster description
    which follows the character packet.  */

    img_words = (UNSIGN16)(c_width + 31) >> 5;
    flag_byte = (BYTE)nosignex(fontfp,(BYTE)1); /* get packet flag byte */
    dyn_f = flag_byte >> 4;	/* dyn_f is high 4 bits from flag_byte */
    turn_on = (BYTE)(flag_byte & 0x08); /* turn_on is 8-bit from flag_byte */
    flag_byte &= 0x07;		/* final flag_byte is low 3 bits */
    do_output = (BOOLEAN)(outfcn != (void(*)())NULL);

    switch(flag_byte)		/* flag_byte is in 0..7 */
    {
    case 0:
    case 1:
    case 2:
    case 3:		/* short character preamble */
	packet_length = (UNSIGN32)flag_byte;
	packet_length <<= 8;
	packet_length += (UNSIGN32)nosignex(fontfp,(BYTE)1);
	packet_length -= 8L;
	offset = 9L;
	break;

    case 4:
    case 5:
    case 6:		/* extended short character preamble */
	packet_length = (UNSIGN32)(flag_byte & 0x03);
	packet_length <<= 16;
	packet_length += (UNSIGN32)nosignex(fontfp,(BYTE)2);
	packet_length -= 13L;
	offset = 14L;
	break;

    case 7:		/* long character preamble */
	packet_length = (UNSIGN32)nosignex(fontfp,(BYTE)4);
	packet_length -= 28L;
	offset = 32L;
	break;

    } /* end switch */

    if (FSEEK(fontfp,offset,1)) /* position to start of raster data */
    {
	(void)warning(
	    "charpk():  FSEEK() failure for PK font file character raster");
	return(EOF);
    }

    bit_weight = 0;

    if (dyn_f == (BYTE)14)	/* <PKtoPX: Get raster by bits> */
    {		/* uncompressed character image */
	for (i = 0; i < c_height; ++i)
	{
	    pword = img_row;
	    if (do_output)
		(void)clrrow();
	    word_weight = (BYTE)31;
	    for (k = 0; k < c_width; ++k)
	    {
		if (get_bit())
		    *pword |= power[word_weight];
		if (word_weight)
		    --word_weight;
		else
		{
		    ++pword;
		    word_weight = (BYTE)31;
		}
	    } /* end for (k) */
	    if (do_output)
		(void)(*outfcn)(c,i);
	} /* end for (i) */
    }
    else	/* <PKtoPX: Create normally packed raster> */
    {	/* run-length encoded character image */
	rows_left = (INT32)c_height;
	h_bit = (INT32)c_width;
	repeat_count = 0;
	word_weight = (BYTE)32;
	pword = img_row;
	if (do_output)
	    (void)clrrow();		/* clear img_row[] */
	while (rows_left > 0)
	{
	    count = pk_packed_num();
	    while (count > 0)
	    {
		if ( (count < (INT32)word_weight) && (count < h_bit) )
		{
		    if (turn_on)
		    {
			*pword |= gpower[word_weight];
			*pword &= ~gpower[word_weight - count];
		    }
		    h_bit -= count;
		    word_weight -= (BYTE)count;
		    count = 0;
		}
		else if ( (count >= h_bit) && (h_bit <= (INT32)word_weight) )
		{
		    if (turn_on)
		    {
			*pword |= gpower[word_weight];
			*pword &= ~gpower[word_weight - h_bit];
		    }
		    if (do_output)
		    {
			for (i = 0; i <= repeat_count; (--rows_left, ++i))
			    (void)(*outfcn)(c,(UNSIGN16)(c_height-rows_left));
		    }
		    else
			rows_left -= repeat_count + 1;
		    repeat_count = 0;
		    pword = img_row;
		    if (do_output)
			(void)clrrow();
		    word_weight = (BYTE)32;
		    count -= h_bit;
		    h_bit = (INT32)c_width;
		}
		else
		{
		    if (turn_on)
			*pword |= gpower[word_weight];
		    pword++;
		    count -= (INT32)word_weight;
		    h_bit -= (INT32)word_weight;
		    word_weight = (BYTE)32;
		}
	    } /* end while (count > 0) */
	    turn_on = (BYTE)(!turn_on);
	} /* end while (rows_left > 0) */
	if ((rows_left > 0) || (h_bit != (INT32)c_width))
	{
	    (void)warning(
		"charpk():  Bad PK font file--more bits than required");
	    return(EOF);
	}
    } /* end if (run-length encoded image) */

    return(0);
}

/*-->get_bit*/
/**********************************************************************/
/****************************** get_bit *******************************/
/**********************************************************************/

BYTE
get_bit()	/* return 0, or non-zero (not necessarily 1) */
{
    register BYTE temp;

    bit_weight >>= 1;
    if (bit_weight == 0)	/* then must get new byte from font file */
    {
	next_byte = (BYTE)nosignex(fontfp,(BYTE)1);
	bit_weight = (BYTE)128;
    }
    temp = next_byte & bit_weight;
    next_byte &= ~bit_weight;	/* clear the just-read bit */

    return (temp);
}

/*-->get_nybble*/
/**********************************************************************/
/***************************** get_nybble *****************************/
/**********************************************************************/

BYTE
get_nybble()
{
    register BYTE temp;

    if (bit_weight == 0)	/* get high nybble */
    {
	next_byte = (BYTE)nosignex(fontfp,(BYTE)1);
	temp = next_byte >> 4;
	next_byte &= 0x0f;
	bit_weight = (BYTE)1;
    }
    else	/* get low nybble */
    {
	temp = next_byte;	/* this has only 4 bits in it */
	bit_weight = (BYTE)0;
    }
    return(temp);
}

/*-->pk_packed_num*/
/**********************************************************************/
/*************************** pk_packed_num ****************************/
/**********************************************************************/

INT32
pk_packed_num() /* return a (non-negative) packed number from the font file */
{	/* this function is recursive to one level only */
    INT32 i,j;

    i = (INT32)get_nybble();
    if (i == 0)
    {
	do
	{
	    j = (INT32)get_nybble();
	    ++i;
	} while (j == 0);
	while (i > 0)
	{
	    j <<= 4;
	    j += (INT32)get_nybble();
	    --i;
	}
	return((INT32)(j - 15 + ((13 - dyn_f) << 4) + dyn_f));
    }
    else if (i <= (INT32)dyn_f)
	return((INT32)i);
    else if (i < 14)
	return((INT32)(((i - dyn_f - 1) << 4) + get_nybble() + dyn_f + 1));
    else if (i == 14)
    {
	repeat_count = pk_packed_num();
	return(pk_packed_num());
    }
    else
    {
	repeat_count = 1;
	return(pk_packed_num());
    }
}


