/* -*-C-*- readfont.h */
/*-->readfont*/
/**********************************************************************/
/****************************** readfont ******************************/
/**********************************************************************/

void
readfont(font_k)
INT32 font_k;
{
    BYTE a, l;
    UNSIGN32 c;				/* checksum */
    UNSIGN32 d;				/* design size */
    char n[MAXSTR];
    UNSIGN32 s;				/* scale factor */
    struct font_entry *tfontptr;	 /* temporary font_entry pointer */

    c = nosignex(dvifp,(BYTE)4);
    s = nosignex(dvifp,(BYTE)4);
    d = nosignex(dvifp,(BYTE)4);
    a = (BYTE)nosignex(dvifp,(BYTE)1);
    l = (BYTE)nosignex(dvifp,(BYTE)1);
    (void)getbytes(dvifp, n, (BYTE)(a+l));
    n[a+l] = '\0';
    tfontptr = (struct font_entry*)MALLOC((unsigned)sizeof(struct font_entry));
    if (tfontptr == (struct font_entry *)NULL)
	(void)fatal(
	    "readfont():  No allocable memory space left for font_entry");
    tfontptr->next = hfontptr;

    fontptr = hfontptr = tfontptr;
    fontptr->k = font_k;
    fontptr->c = c;
    fontptr->s = s;
    fontptr->d = d;
    fontptr->a = a;
    fontptr->l = l;
    (void)strcpy(fontptr->n, n);
    fontptr->font_space = (INT32)(s/6);
    (void)reldfont(fontptr);
}
