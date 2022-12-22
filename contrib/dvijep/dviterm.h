/* -*-C-*- dviterm.h */
/*-->dviterm*/
/**********************************************************************/
/****************************** dviterm *******************************/
/**********************************************************************/

void
dviterm()			/* terminate DVI file processing */
{
    register INT16 k;		/* loop index */

#if    OS_VAXVMS
    /* Stupid VMS fflush() raises error and loses last data block
       unless it is full for a fixed-length record binary file.
       Pad it here with IM_EOF characters, sigh... */
    for (k = (INT16)((*plotfp)->_cnt); k > 0; --k)
#if    IMPRESS
        OUTC(IM_EOF);
#else
        OUTC('\0');
#endif
#endif /* OS_VAXVMS */

    if (fflush(plotfp) == EOF)
	(void)fatal("dviterm(): Output error -- disk storage probably full");

    /* Close all files and free dynamically-allocated font space.  Stdin
       is never closed, however, because it might be needed later for
       ioctl(). */

    if ((dvifp != (FILE*)NULL) && (dvifp != stdin))
        (void)fclose(dvifp);
    if (plotfp != (FILE*)NULL)
        (void)fclose(plotfp);

    for (k = 1; k <= (INT16)nopen; ++k)
    {
	if (font_files[k].font_id != (FILE *)NULL)
	{

#if    VIRTUAL_FONTS
	    if (virt_font)
	        (void)virtfree(font_files[k].font_id);
#endif /* VIRTUAL_FONTS */

	    (void)fclose(font_files[k].font_id);
	    font_files[k].font_id = (FILE *)NULL;
	}
    }

    fontptr = hfontptr;
    while (fontptr != (struct font_entry *)NULL)
    {
	for (k = 0; k < NPXLCHARS; ++k)
	    if ((fontptr->ch[k].rasters) != (UNSIGN32 *)NULL)
		(void)free((char *)fontptr->ch[k].rasters);
	pfontptr = fontptr;
	fontptr = fontptr->next;
	(void)free((char *)pfontptr);
    }
    if (!quiet)
    {
	(void)fprintf(stderr," [OK]");
	NEWLINE(stderr);
    }

#if    (BSD42 | OS_TOPS20)
    if (spool_output)
    {
	register char *p;

#if    BSD42
	(void)sprintf(message,"DVISPOOL %s\n",dvoname);
#endif

#if    OS_TOPS20
	(void)sprintf(message,"DVISPOOL: %s\n",dvoname);
#endif

	for (p = message; *p; ++p)
	{
#if    OS_TOPS20
	    ac1 = 0100;		/* .priin (primary input) */
	    ac2 = *p;		/* next character from string */
	    jsys(JSsti, acs);	/* stuff it into terminal input buffer */
#endif

#if    BSD42
	    (void)ioctl(fileno(stdin),TIOCSTI,p);
#endif

	}
    }
#endif /* (BSD42 | OS_TOPS20) */

}
