/* -*-C-*- alldone.h */
/*-->alldone*/
/**********************************************************************/
/****************************** alldone  ******************************/
/**********************************************************************/

void
alldone()

{
    register int t;

    if ((g_errenc != 0) && g_dolog && (g_logfp != (FILE *)NULL))
    {		/* errors occurred - copy log file to stderr */
	(void)fflush(g_logfp);		/* make sure file is up-to-date */
	(void)REWIND(g_logfp);		/* rewind it */
	while ((t=(int)getc(g_logfp)) != EOF)	/* copy to stderr */
	    (void)putc((char)t,stderr);
	(void)fclose(g_logfp);		/* close it */
	g_logfp = (FILE *)NULL;
    }
    abortrun(g_errenc);
}
