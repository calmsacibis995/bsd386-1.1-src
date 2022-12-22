/* -*-C-*- dviinit.h */
/*-->dviinit*/
/**********************************************************************/
/****************************** dviinit *******************************/
/**********************************************************************/

void
dviinit(filestr)		/* initialize DVI file processing */
char *filestr;			/* command line filename string  */
{
    int i;			/* temporary */
    INT16 k;			/* index in curpath[] and curname[] */
    char* tcp;			/* temporary string pointers */
    char* tcp1;			/* temporary string pointers */

#if    OS_UNIX
/***********************************************************************
Unix allows binary I/O on stdin and stdout; most other operating systems
do not.  In order  to support this,  we use the  convention that a  null
filestr (NOT  a (char*)NULL)  means the  dvifile is  on stdin,  and  the
translated output is on stdout.  Error logging will be to dvixxx.err  or
dvixxx.dvi-err.  Because random access is required of the dvifile, stdin
may NOT be a pipe; it must be a disk file.
***********************************************************************/
#define NO_FILENAME (filestr[0] == '\0')
#endif

#if    CANON_A2

#ifdef CANON_TEST
    char modestring[10];	/* for dynamic fopen() mode string */
#endif /* CANON_TEST */

#endif

#if    (OS_TOPS20 | OS_VAXVMS)
    char* tcp2;			/* temporary string pointers */
#endif

    for (k = 0; k < 10; ++k)
	tex_counter[k] = (INT32)0L;

    fontptr = (struct font_entry *)NULL;
    hfontptr = (struct font_entry *)NULL;
    pfontptr = (struct font_entry *)NULL;
    fontfp = (FILE *)NULL;
    cache_size = 0;
    nopen = 0;

    /***********************************************************
    Set up file names and open dvi and plot files.  Simple
    filename parsing assumes forms like:

    Unix:	name
		name.ext
		/dir/subdir/.../subdir/name
		/dir/subdir/.../subdir/name.ext
    TOPS-20:	any Unix style (supported by PCC-20)
		<directory>name
		<directory>name.ext
		<directory>name.ext.gen
		device:<directory>name
		device:<directory>name.ext
		device:<directory>name.ext.gen
		[directory]name
		[directory]name.ext
		[directory]name.ext.gen
		device:[directory]name
		device:[directory]name.ext
		device:[directory]name.ext.gen
		logname:name
		logname:name.ext
		logname:name.ext.gen

    The Unix style should work  under IBM PC DOS (backslash  can
    usually be  replaced for  forward  slash), and  the  TOPS-20
    style contains VAX VMS as  a subset.  Fancier TOPS-20  names
    are possible (control-V quoting of arbitrary characters, and
    attributes), but they are rare enough that we do not support
    them.  For  TOPS-20  and  VAX VMS,  generation  numbers  are
    recognized as a digit following  the last dot, and they  are
    preserved for the DVI file only.  For the output files,  the
    highest generation  will always  be  assumed.  This  is  for
    convenience with the  TOPS-20 TeX  implementation, which  on
    completion types  in a  command "TeXspool:  foo.dvi.13"  and
    waits for the user to type a carriage return.

    We only  need  to  extract the  directory  path,  name,  and
    extension, so the parsing is simple-minded.

    ***********************************************************/

    tcp = strrchr(filestr,'/'); /* find end of Unix file path */

#if    (OS_ATARI | OS_PCDOS)
    if (tcp == (char*)NULL)	/* no Unix-style file path */
	tcp = strrchr(filestr, '\\');	/* try \dos\path\ */
#endif

#if    OS_TOPS20
    if (tcp == (char*)NULL)	/* no Unix-style file path */
    {
	tcp = strrchr(filestr, '>');	/* try <dir> */
	if (tcp == (char*)NULL)			/* no <dir> */
	    tcp = strrchr(filestr, ']');	/* try [dir] */
	if (tcp == (char*)NULL)			/* no [dir] */
	    tcp = strrchr(filestr, ':');	/* try logname: */
    }
#endif

#if    OS_VAXVMS
    if (tcp == (char*)NULL)	/* no Unix-style file path */
    {
	tcp = strrchr(filestr, ']');	/* try [dir] */
	if (tcp == (char*)NULL)			/* no [dir] */
	    tcp = strrchr(filestr, ':');	/* try logname: */
    }
#endif

    if (tcp == (char*)NULL)	/* no file path */
    {
	curpath[0] = '\0';	/* empty path */
	tcp = filestr;	/* point to start of name */
    }
    else			/* save path for later use */
    {
	k = (INT16)(tcp-filestr+1);
	(void)strncpy(curpath, filestr, (int)k);
	curpath[k] = '\0';
	tcp++;			/* point to start of name */
    }

    tcp1 = strrchr(tcp, '.');	/* find last dot in filename */

#if    (OS_TOPS20 | OS_VAXVMS)
    if ((tcp1 != (char*)NULL) && isdigit(*(tcp1+1)))
    {				/* then assume generation number */
	tcp2 = tcp1;		/* remember dot position */
	*tcp1 = '\0';		/* discard generation number */
	tcp1 = strrchr(tcp,'.');/* find last dot in filename */
	*tcp2 = '.';		/* restore dot */
    }
#endif

    if (tcp1 == (char*)NULL)
    {				/* no dot, so name has no extension */
	(void)strcpy(curname, tcp);	/* save name */
	tcp1 = strchr(tcp, '\0');	/* set empty extension */
    }
    else			/* save name part */
    {
	k = (INT16)(tcp1-tcp);
	(void)strncpy(curname, tcp, (int)k);
	curname[k] = '\0';
    }

    (void)strcpy(curext, tcp1);	/* save extension */


    /* DVI file must always have extension DVIEXT; supply one if
    necessary (e.g /u/jones/foo.dvi) */

    (void)strcpy(dviname, curpath);
    (void)strcat(dviname, curname);
    if (curext[0] == '\0')
	(void)strcat(dviname, DVIEXT);
    else
	(void)strcat(dviname, curext);

    /* Device output file is PATH NAME . DVIPREFIX OUTFILE_EXT (e.g.
       /u/jones/foo.dvi-alw) */

    (void)strcpy(dvoname, curpath);
    (void)strcat(dvoname, curname);
    (void)strcat(dvoname, ".");
    (void)strcat(dvoname, DVIPREFIX);
    (void)strcat(dvoname, OUTFILE_EXT);

    /* Font substitution file is PATH NAME SUBEXT (e.g.
       /u/jones/foo.sub).  We do not tell user (via stderr)
       about it until it is needed. */

    if (subfile[0] == '\0')	/* no -fsubfile; make default */
    {
	(void)strcpy(subfile, curpath);
	(void)strcat(subfile, curname);
	(void)strcat(subfile, subext);
    }


#if    OS_UNIX
    if (NO_FILENAME)
    {
	dvifp = stdin;		/* already open, so no DEBUG_OPEN call */
	if (fseek(dvifp,0L,0))
	{
	    (void)fprintf(stderr,
		"%s: Unable to fseek() on stdin--was it a pipe?\n",
		g_progname);
	    (void)EXIT(1);
	}
    }
    else
    {
	dvifp = FOPEN(dviname,RB_OPEN);
	DEBUG_OPEN(dvifp,dviname,RB_OPEN);
    }
#else

    dvifp = FOPEN(dviname,RB_OPEN);
    DEBUG_OPEN(dvifp,dviname,RB_OPEN);
#endif

    if (dvifp == (FILE*)NULL)
    {
	(void)sprintf(message,"dviinit(): %s: can't open [%s]",
	    g_progname, dviname);
	(void)fatal(message);
    }

    /* We try both
	PATH NAME . DVIPREFIX OUTFILE_EXT and
	     NAME . DVIPREFIX OUTFILE_EXT,
        in case the user does not have write access to the directory PATH */
    for (i = 1; i <= 2; ++i)		/* two tries here */
    {

#if    BBNBITGRAPH
	strcpy(dvoname,"stdout");	/* BitGraph always goes to stdout */
	plotfp = stdout;
#else /* NOT BBNBITGRAPH */

#if    OS_VAXVMS

#if    POSTSCRIPT
	plotfp = FOPEN(dvoname,WB_OPEN);
#else
	/* Create binary files for other devices; they cause less trouble
        with VMS spooler programs. */
	plotfp = FOPEN(dvoname,WB_OPEN,"rfm=fix","bls=512","mrs=512");
#endif /* POSTSCRIPT */

#else /* NOT OS_VAXVMS */

#if    OS_UNIX
	plotfp = NO_FILENAME ? stdout : FOPEN(dvoname,WB_OPEN);
#else
	plotfp = FOPEN(dvoname,WB_OPEN);
#endif /* OS_UNIX */

#endif /* OS_VAXVMS */

#endif /* BBNBITGRAPH */

#if    OS_UNIX
	if (!NO_FILENAME)
#endif

	DEBUG_OPEN(plotfp,dvoname,WB_OPEN);
	if (plotfp != (FILE*)NULL)
	    break;			/* open okay */
	if (i == 2)
	{
	    (void)strcpy(dvoname, curname);
	    (void)strcat(dvoname, ".");
	    (void)strcat(dvoname, DVIPREFIX);
	    (void)strcat(dvoname, OUTFILE_EXT);
	}
    }

    if (plotfp == (FILE*)NULL)
    {
	(void)sprintf(message,"dviinit(): %s: can't open [%s]",
	    g_progname, dvoname);
	(void)fatal(message);
    }

#if    OS_TOPS20
    /* Because of the PCC-20 arithmetic (instead of logical)
       shift bug wherein a constant expression (1<<35) evaluates
       to 0 instead of 400000,,000000, we must set CF_nud using a
       variable, instead of just using a constant */
    /* "ac1 = makefield(CF_nud,1);" */
    ac1 = 1;
    ac1 = makefield(CF_nud,ac1);
    setfield(ac1,CF_jfn,jfnof(fileno(plotfp)));	/* jfn */
    setfield(ac1,CF_dsp,FBbyv);	/* generation number index in fdb */
    ac2 = makefield(FB_ret,-1);
    ac3 = makefield(FB_ret,0);	/* set generation count to 0 (infinite) */
    (void)jsys(JSchfdb,acs);	/* ignore any errors */
#endif /* OS_TOPS20 */

#if    CANON_A2

#ifdef CANON_TEST

#if    OS_ATARI
    strcpy(tmoname,"\\ca2XXX\.");
#else
    strcpy(tmoname,"ca2XXXXXX");
#endif

    mktemp(tmoname);
    savefp = plotfp;
    (void)sprintf(modestring,"%s+",WB_OPEN);
    plotfp = FOPEN(tmoname, modestring);
    stor_total = 0;
#endif /* CANON_TEST */

#endif /* CANON_A2 */

    if (!quiet

#if    OS_UNIX
	&& !NO_FILENAME
#endif

    )
    {
	(void)fprintf(stderr,"[Input from DVI file %s]",dviname);
	NEWLINE(stderr);
	(void)fprintf(stderr,"[Output on file %s]",dvoname);
	NEWLINE(stderr);
    }

    if (strncmp(curpath,dvoname,strlen(curpath)) == 0)
	(void)strcpy(g_logname, curpath);	/* used PATH on output file */
    else
	g_logname[0] = '\0';		/* no PATH on output file */

#if    OS_UNIX
    if (NO_FILENAME)			/* use dvixxx as file prefix */
    {
	(void)strcpy(g_logname, "dvi");
	(void)strcat(g_logname, OUTFILE_EXT);
    }
#endif

    (void)strcat(g_logname, curname);
    (void)strcat(g_logname, ".");
    (void)strcat(g_logname, DVIPREFIX);

#if    (OS_ATARI | OS_PCDOS | OS_UNIX)
    (void)strcat(g_logname, "err");
#endif

#if    (OS_TOPS20 | OS_VAXVMS)
    /* Force new generation; otherwise fopen(,"w+") reuses existing file */
    (void)strcat(g_logname, "err.-1");
#endif

    lmargin = (COORDINATE)(leftmargin*((float)XDPI));
    tmargin = (COORDINATE)(topmargin*((float)YDPI));

#if    (CANON_A2 | IMPRESS | HPJETPLUS)
    /* Printer has (0,0) origin at (XORIGIN,YORIGIN) near, but not at,
    upper left corner */
#if	HPJETPLUS
    lmargin -= landscape ? XORIGIN_LANDSCAPE : XORIGIN;
    tmargin -= landscape ? YORIGIN_LANDSCAPE : YORIGIN;
#else
    lmargin -= XORIGIN;
    tmargin -= YORIGIN;
#endif
#endif /* (CANON_A2 | IMPRESS | HPJETPLUS) */

    if ((BYTE)nosignex(dvifp,(BYTE)1) != PRE)
	(void)fatal("dviinit(): PRE doesn't occur first--are you sure \
this is a DVI file?");

    i = (int)signex(dvifp,(BYTE)1);
    if (i != DVIFORMAT)
    {
	(void)sprintf(message,
	    "dviinit(): DVI format = %d, can only process DVI format %d files.",
	    i, (int)DVIFORMAT);
	(void)fatal(message);
    }
}
