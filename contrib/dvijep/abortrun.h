/* -*-C-*- abortrun.h */
/*-->abortrun*/
/**********************************************************************/
/***************************  abortrun  *******************************/
/**********************************************************************/

void
abortrun(code)
int code;

{
    /*******************************************************************
    This routine  is called  on both  success and  failure to  terminate
    execution.  All open files (except stdin, stdout, stderr) are closed
    before calling EXIT() to quit.
    *******************************************************************/

    UNSIGN16 k;

    for (k = 0; k < (UNSIGN16)nopen; ++k)
	if (font_files[k].font_id != (FILE*)NULL)
	    (void)fclose(font_files[k].font_id);

    if (dvifp != (FILE*)NULL)
        (void)fclose(dvifp);
    if (plotfp != (FILE*)NULL)
        (void)fclose(plotfp);
    if (g_dolog && (g_logfp != (FILE *)NULL))
	(void)fclose(g_logfp);

#if    (OS_TOPS20 | OS_VAXVMS)
    if (code)
    {
	NEWLINE(stderr);
	(void)fprintf(stderr,"?Aborted with error code %d",code);
	NEWLINE(stderr);
	(void)perror("?perror() says");
    }
#endif

    (void)EXIT(code);
}

