/* -*-C-*- getbytes.h */
/*-->getbytes*/
/**********************************************************************/
/****************************** getbytes ******************************/
/**********************************************************************/

void
getbytes(fp, cp, n)	/* get n bytes from file fp */
register FILE *fp;	/* file pointer	 */
register char *cp;	/* character pointer */
register BYTE n;	/* number of bytes */

{
    while (n--)
	*cp++ = (char)getc(fp);
}


