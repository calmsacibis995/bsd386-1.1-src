/* -*-C-*- skipfont.h */
/*-->skipfont*/
/**********************************************************************/
/****************************** skipfont ******************************/
/**********************************************************************/

void
skipfont(k)
INT32 k;				/* UNUSED */

{
    BYTE a, l;
    char n[MAXSTR];

    (void) nosignex(dvifp,(BYTE)4);
    (void) nosignex(dvifp,(BYTE)4);
    (void) nosignex(dvifp,(BYTE)4);
    a = (BYTE) nosignex(dvifp,(BYTE)1);
    l = (BYTE) nosignex(dvifp,(BYTE)1);
    getbytes(dvifp, n, (BYTE)(a+l));
}


