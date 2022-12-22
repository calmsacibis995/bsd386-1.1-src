/* -*-C-*- setfntnm.h */
/*-->setfntnm*/
/**********************************************************************/
/****************************** setfntnm ******************************/
/**********************************************************************/

void
setfntnm(k)
register INT32 k;

/***********************************************************************
This routine is used to specify the  font to be used in printing  future
characters.
***********************************************************************/

{
    register struct font_entry *p;

    p = hfontptr;
    while ((p != (struct font_entry *)NULL) && (p->k != k))
	p = p->next;
    if (p == (struct font_entry *)NULL)
    {
	(void)sprintf(message,"setfntnm():  font %ld undefined", k);
	(void)fatal(message);
    }
    else
	fontptr = p;

#if    (HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2)
    font_switched = TRUE;
#endif

}
