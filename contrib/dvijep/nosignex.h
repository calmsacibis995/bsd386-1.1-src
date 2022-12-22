/* -*-C-*- nosignex.h */
/*-->nosignex*/
/**********************************************************************/
/****************************** nosignex ******************************/
/**********************************************************************/

UNSIGN32
nosignex(fp, n)	/* return n byte quantity from file fd */
register FILE *fp;	/* file pointer    */
register BYTE n;	/* number of bytes (1..4) */

{
    register UNSIGN32 number;	/* number being constructed */

    number = 0;
    while (n--)
    {
	number <<= 8;
	number |= getc(fp);
    }
    return((UNSIGN32)number);
}
