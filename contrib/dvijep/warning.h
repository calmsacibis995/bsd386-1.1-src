/* -*-C-*- warning.h */
/*-->warning*/
/**********************************************************************/
/****************************** warning *******************************/
/**********************************************************************/

void
warning(msg)				/* issue a warning */
char *msg;				/* message string  */
{

#if    BBNBITGRAPH
    if (quiet)				/* no warnings in quiet mode */
        return;
#endif

    if (g_dolog && (g_logfp == (FILE *)NULL) && g_logname[0])
    {
	g_logfp = fopen(g_logname,"w+");
	DEBUG_OPEN(g_logfp,g_logname,"w+");
	if (g_logfp != (FILE *)NULL)
	{
	    (void)fprintf(stderr,"[Log file [%s] created]",g_logname);
	    NEWLINE(stderr);

#if    OS_TOPS20
#if    PCC_20
	    /* Because of the PCC-20 arithmetic (instead of */
	    /* logical) shift bug wherein a constant expression */
	    /* (1<<35) evaluates to 0 instead of 400000,,000000, */
	    /* we must set CF_nud using a variable, instead of */
	    /* just using a constant "ac1 = makefield(CF_nud,1);" */
	    ac1 = 1;
	    ac1 = makefield(CF_nud,ac1);
	    setfield(ac1,CF_jfn,jfnof(fileno(g_logfp)));	/* jfn */
	    setfield(ac1,CF_dsp,FBbyv);	/* generation number index in fdb */
	    ac2 = makefield(FB_ret,-1);
	    ac3 = makefield(FB_ret,0);/* set generation count to 0 (infinite) */
	    (void)jsys(JSchfdb,acs);	/* ignore any errors */
#endif /* PCC_20 */
#endif

	}
    }

    if (g_dolog && (g_logfp != (FILE *)NULL) && g_logname[0])
    {
	NEWLINE(g_logfp);

#if    (OS_TOPS20 | OS_VAXVMS)
	(void)putc('%',g_logfp);	/* percent at start of line */
#endif

	(void)fputs(msg,g_logfp);
	NEWLINE(g_logfp);
	(void)fprintf(g_logfp,"Current TeX page counters: [%s]",tctos());
	NEWLINE(g_logfp);
    }

    if (g_dolog && (g_logfp == (FILE *)NULL) && g_logname[0])
    {
	(void)sprintf(message,"Cannot open log file [%s]",g_logname);
	fatal(message);
    }
    NEWLINE(stderr);

#if    (OS_TOPS20 | OS_VAXVMS)
    (void)putc('%',stderr);	/* percent at start of line */
#endif

    (void)fputs(msg,stderr);
    NEWLINE(stderr);
    if (g_logname[0])
    {
	(void)fprintf(stderr,"Current TeX page counters: [%s]",tctos());
	NEWLINE(stderr);
    }
}
