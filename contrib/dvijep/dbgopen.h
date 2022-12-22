/* -*-C-*- dbgopen.h */
/*-->dbgopen*/
/**********************************************************************/
/****************************** dbgopen *******************************/
/**********************************************************************/

/* This used to be a long in-line macro, but some compilers could not */
/* handle it. */

void
dbgopen(fp, fname, openmode)
FILE* fp;				/* file pointer */
char* fname;				/* file name */
char* openmode;				/* open mode flags */
{
    if (DBGOPT(DBG_OKAY_OPEN) && (fp != (FILE *)NULL))
    {
	(void)fprintf(stderr,"%%Open [%s] mode [%s]--[OK]",fname,openmode);
	NEWLINE(stderr);
    }
    if (DBGOPT(DBG_FAIL_OPEN) && (fp == (FILE *)NULL))
    {
	(void)fprintf(stderr,"%%Open [%s] mode [%s]--[FAILED]",fname,openmode);
	NEWLINE(stderr);
    }
}
