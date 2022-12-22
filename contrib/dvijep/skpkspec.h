/* -*-C-*- skpkspec.h */
/*-->skpkspec*/
/**********************************************************************/
/****************************** skpkspec ******************************/
/**********************************************************************/

void
skpkspec()	/* Skip PK font file specials */
{
    BYTE the_byte;

    the_byte = (BYTE)nosignex(fontfp,(BYTE)1);
    while ((the_byte >= (BYTE)PKXXX1) && (the_byte != PKPOST))
    {
	switch(the_byte)
	{
	case PKXXX1:
	    (void)FSEEK(fontfp,(long)nosignex(fontfp,(BYTE)1),1);
	    break;

	case PKXXX2:
	    (void)FSEEK(fontfp,(long)nosignex(fontfp,(BYTE)2),1);
	    break;

	case PKXXX3:
	    (void)FSEEK(fontfp,(long)nosignex(fontfp,(BYTE)3),1);
	    break;

	case PKXXX4:
	    (void)FSEEK(fontfp,(long)nosignex(fontfp,(BYTE)4),1);
	    break;

	case PKYYY:
	    (void)nosignex(fontfp,(BYTE)4);
	    break;

	case PKNOOP:
	    break;

	default:
	    (void)sprintf(message,"skpkspec():  Bad PK font file [%s]",
		fontptr->name);
	    (void)fatal(message);
	}
	the_byte = (BYTE)nosignex(fontfp,(BYTE)1);
    }
    (void)UNGETC((char)the_byte,fontfp);	/* put back lookahead byte */
}
