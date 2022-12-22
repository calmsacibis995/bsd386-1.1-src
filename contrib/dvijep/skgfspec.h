/* -*-C-*- skgfspec.h */
/*-->skgfspec*/
/**********************************************************************/
/****************************** skgfspec ******************************/
/**********************************************************************/

void
skgfspec()	/* Skip GF font file specials */
{
    BYTE the_byte;

    the_byte = (BYTE)nosignex(fontfp,(BYTE)1);
    while ((the_byte >= (BYTE)GFXXX1) && (the_byte != GFPOST))
    {
	switch(the_byte)
	{
	case GFXXX1:
	    (void)FSEEK(fontfp,(long)nosignex(fontfp,(BYTE)1),1);
	    break;

	case GFXXX2:
	    (void)FSEEK(fontfp,(long)nosignex(fontfp,(BYTE)2),1);
	    break;

	case GFXXX3:
	    (void)FSEEK(fontfp,(long)nosignex(fontfp,(BYTE)3),1);
	    break;

	case GFXXX4:
	    (void)FSEEK(fontfp,(long)nosignex(fontfp,(BYTE)4),1);
	    break;

	case GFYYY:
	    (void)nosignex(fontfp,(BYTE)4);
	    break;

	case GFNOOP:
	    break;

	default:
	    (void)sprintf(message,"skgfspec():  Bad GF font file [%s]",
		fontptr->name);
	    (void)fatal(message);
	}
	the_byte = (BYTE)nosignex(fontfp,(BYTE)1);
    }
    (void)UNGETC((char)the_byte,fontfp);	/* put back lookahead byte */
}
