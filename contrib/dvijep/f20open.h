/* -*-C-*- f20open.h */
/*-->f20open*/
/**********************************************************************/
/****************************** f20open *******************************/
/**********************************************************************/

#if    OS_TOPS20
FILE*
f20open(filename,mode)
char *filename;
char *mode;

/***********************************************************************
Input files are opened in bytesize 8, since this is what is expected for
dvi files.  Output files are opened in bytesize BYTE_SIZE, which will be
7 or 8, depending on the output device.
***********************************************************************/
{
    int fp;

    if (g_dolog && (g_logfp != (FILE *)NULL))
    {
	(void)fprintf(g_logfp,"%%Opening %d-bit file %s for mode %s",
	    ((mode[0] == 'r') ? 8 : BYTE_SIZE),filename,mode);
	NEWLINE(g_logfp);
    }

    /* Copy file in bytesize BYTE_SIZE with binary flag set to prevent
       CRLF <--> LF mapping; the "rb" or "wb" flag is not sufficient for
       this because PCC-20 maintains two places internally
       where the binary flag is set, and both are used!  */

    if (mode[0] == 'r')

#if    KCC_20
        return(fopen(filename,RB_OPEN));
#endif /* KCC_20 */

#if    PCC_20
	if ((fp = open(filename,FATT_RDONLY | FATT_SETSIZE | FATT_BINARY,
	    8)) >= 0)
	    return(fdopen(fp,RB_OPEN));
	else
	    return((FILE*)NULL);
#endif /* PCC_20 */

    else if (mode[0] == 'w')

#if    KCC_20
#if    BYTE_SIZE == 7
        return (fopen(filename,"wb7"));
#else
#if    BYTE_SIZE == 8
        return (fopen(filename,"wb8"));
#else
	fatal("Illegal bytesize set for KCC-20 implementation");
#endif
#endif
#endif /* KCC_20 */

#if    PCC_20
	if ((fp = open(filename, FATT_WRONLY | FATT_SETSIZE | FATT_BINARY,
	    BYTE_SIZE)) >= 0)
	    return(fdopen(fp,WB_OPEN));
	else
	    return((FILE*)NULL);
#endif /* PCC_20 */

    else
    {
	(void)sprintf(message,
	    "f20open does not understand open mode %s for file %s",
	    mode,filename);
	fatal(message);
    }
}
#endif /* OS_TOPS20 */
