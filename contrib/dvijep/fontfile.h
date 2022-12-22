/* -*-C-*- fontfile.h */
/*-->fontfile*/
/**********************************************************************/
/****************************** fontfile ******************************/
/**********************************************************************/

#if 1					/* new code supporting path search */

#if    ANSI_PROTOTYPES
char	*findfile(const char *pathlist, const char *name);
#else
char	*findfile();
#endif

#define FINDFILE(fmt,a,b) \
			do \
			{ \
			    char *p; \
			    (void)sprintf(filelist[m], fmt, a, b); \
			    p = findfile(font_path,filelist[m]); \
			    if (p != (char*)NULL) \
			        (void)strcpy(filelist[m++],p); \
			} while (0)
#endif

void
fontfile(filelist,font_path,font_name,magnification)
char *filelist[MAXFORMATS];	/* output filename list */
char *font_path;		/* host font file pathname */
char *font_name;		/* TeX font_name */
int magnification;		/* magnification value */
{

    /*
    Given a TeX font_name and a magnification value, construct a list of
    system-dependent  host  filenames,  returning  them  in  the   first
    argument.  The files are not guaranteed to exist.  The font names by
    default contains names for the PK,  GF, and PXL font files, but  the
    selection, and search order, may be changed at run time by  defining
    a value for the environment  variable FONTLIST.  See initglob()  for
    details.
    */

    register int m;			/* index into filelist[] */
    register INT16 k;			/* loop index */
    float fltdpi;			/* dots/inch */
    int dpi;				/* dots/inch for filename */

#if    OS_VAXVMS
    char *tcp;			/* temporary character pointer */
    char *tcp2;			/* temporary character pointer */
    char temp_name[MAXFNAME];	/* temporary filename */
#endif

    /*
    We need to convert an old-style ROUNDED integer magnification based
    on 1000 units = 200 dpi to an actual rounded dpi value.

    We have
	magnification = round(real_mag) = trunc(real_mag + 0.5)
    which implies
	float(magnification) - 0.5 <= real_mag < float(magnification) + 0.5

    We want
	dpi = round(real_mag/5.0)
    which implies
	float(dpi) - 0.5 <= real_mag/5.0 < float(dpi) + 0.5
    or
	5.0*float(dpi) - 2.5 <= real_mag < 5.0*float(dpi) + 2.5

    We can combine the two to conclude that
	5.0*float(dpi) - 2.5 < float(magnification) + 0.5
    or
	5.0*float(dpi) - 3 < float(magnification)
    which gives the STRICT inequality
	float(dpi) < (float(magnification) + 3.0)/5.0
    */

    fltdpi = (((float)magnification) + 3.0)/5.0;
    dpi = (int)fltdpi;
    if (fltdpi == (float)dpi)
	dpi--;				/* enforce inequality */

    for (k = 0; k < MAXFORMATS; ++k) /* loop over possible file types */
      *filelist[k] = '\0';	/* Initially, all filenames are empty */

    m = 0;				/* index in filelist[] */
    for (k = 0; k < MAXFORMATS; ++k) /* loop over possible file types */
    {

#if    OS_TOPS20

	/* Typical results:
	    texfonts:amr10.300gf
	    texfonts:amr10.300pk
	    texfonts:amr10.1500pxl
	*/

	if (k == gf_index)
	    (void)sprintf(filelist[m++], "%s%s.%dgf",
		font_path, font_name, dpi);
	else if (k == pk_index)
	    (void)sprintf(filelist[m++], "%s%s.%dpk",
		font_path, font_name, dpi);
	else if (k == pxl_index)
	    (void)sprintf(filelist[m++], "%s%s.%dpxl",
		font_path, font_name, magnification);

#endif /* OS_TOPS20 */

#if    (OS_ATARI | OS_PCDOS)

	/* Typical results:
	    d:\tex\fonts\300\amr10.gf
	    d:\tex\fonts\300\amr10.pk
	    d:\tex\fonts\1500\amr10.pxl
	*/

#if 0					/* old single-path code */
	if (k == gf_index)
	    (void)sprintf(filelist[m++], "%s%d\\%s.gf",
		font_path, dpi, font_name);
	else if (k == pk_index)
	    (void)sprintf(filelist[m++], "%s%d\\%s.pk",
		font_path, dpi, font_name);
	else if (k == pxl_index)
	    (void)sprintf(filelist[m++], "%s%d\\%s.pxl",
		font_path, magnification, font_name);
#else					/* new code supporting path search */
	if (k == gf_index)
	{
	    FINDFILE("%d\\%s.gf", dpi, font_name);
	}
	else if (k == pk_index)
	{
	    FINDFILE("%d\\%s.pk", dpi, font_name);
	}
	else if (k == pxl_index)
	{
	    FINDFILE("%d\\%s.pxl", magnification, font_name);
	}
#endif

#endif /* (OS_ATARI | OS_PCDOS) */

#if    OS_UNIX

	/* Typical results (both naming styles are tried):
	    /usr/lib/tex/fonts/300/amr10.gf
	    /usr/lib/tex/fonts/amr10.300gf
	    /usr/lib/tex/fonts/300/amr10.pk
	    /usr/lib/tex/fonts/1500/amr10.pxl
	    /usr/lib/tex/fonts/amr10.1500pxl
	    /usr/lib/tex/fonts/amr10.300pk
	*/

#if 0					/* old single-path code */
	if (k == gf_index)
	{
	    (void)sprintf(filelist[m++], "%s%d/%s.gf",
		font_path, dpi, font_name);
	    (void)sprintf(filelist[m++], "%s%s.%dgf",
		font_path, font_name, dpi);
	}
	else if (k == pk_index)
	{
	    (void)sprintf(filelist[m++], "%s%d/%s.pk",
		font_path, dpi, font_name);
	    (void)sprintf(filelist[m++], "%s%s.%dpk",
		font_path, font_name, dpi);
	}
	else if (k == pxl_index)
	{
	    (void)sprintf(filelist[m++], "%s%d/%s.pxl",
		font_path, magnification, font_name);
	    (void)sprintf(filelist[m++], "%s%s.%dpxl",
		font_path, font_name, magnification);
	}
#else					/* new code supporting path search */
	if (k == gf_index)
	{
	    FINDFILE("%d/%s.gf", dpi, font_name);
	    FINDFILE("%s.%dgf", font_name, dpi);
	}
	else if (k == pk_index)
	{
	    FINDFILE("%d/%s.pk", dpi, font_name);
	    FINDFILE("%s.%dpk", font_name, dpi);
	}
	else if (k == pxl_index)
	{
	    FINDFILE("%d/%s.pxl", magnification, font_name);
	    FINDFILE("%s.%dpxl", font_name, magnification);
	}
#endif
	/*(void)fprintf(stderr,"In fontfile: Path = %s\n",filelist[m - 1]);*/

#endif /* OS_UNIX */


#if    OS_VAXVMS

	/* Typical results (both naming styles are tried):
	    [tex.fonts.300]amr10.gf
	    [tex.fonts]amr10.300gf
	    [tex.fonts.300]amr10.pk
	    [tex.fonts]amr10.300pk
	    [tex.fonts.1500]amr10.pxl
	    [tex.fonts]amr10.1500pxl


	We try a translation of  a logical name here if  what we have in
	fontpath[]    is   not     a   directory  specification     like
	"device:[tex.fonts]" or a rooted logical  name like "tex_fonts:"
	translating to something like "device:[tex.fonts.]"
	*/


	/* Use a straightforward expansion first, in case fontpath is
	   a search list. */
	if (k == gf_index)
	{
	    (void)sprintf(filelist[m++], "%s[%d]%s.gf", fontpath,
		dpi, font_name);
	    (void)sprintf(filelist[m++], "%s%s.%dgf", fontpath,
		font_name, dpi);
	}
	else if (k == pk_index)
	{
	    (void)sprintf(filelist[m++], "%s[%d]%s.pk", fontpath,
		dpi, font_name);
	    (void)sprintf(filelist[m++], "%s%s.%dpk", fontpath,
		font_name, dpi);
	}
	else if (k == pxl_index)
	{
	    (void)sprintf(filelist[m++], "%s[%d]%s.pxl", fontpath,
		magnification, font_name);
	    (void)sprintf(filelist[m++], "%s%s.%dpxl", fontpath,
		font_name, magnification);
	}

	(void)strcpy(temp_name,fontpath);
	tcp = strrchr(temp_name,']'); /* search for last right bracket */
	if (tcp == (char *)NULL) /* then try logical name translation */
	{
	    tcp = GETENV(fontpath);
	    if (tcp != (char *)NULL)
	    {
	        tcp2 = strrchr(tcp,']');
		if (tcp2 == (char *)NULL)
		  tcp = (char *)NULL; /* translates to another logical name */
		else
		{
		    if (*(tcp2-1) == '.')
		        tcp = (char *)NULL; /* looks like rooted logical name */
		    else	/* looks like directory name */
		    {
		        (void)strcpy(temp_name,tcp);
		        tcp = strrchr(temp_name,']');
		    }
		}
	    }
	}
	if (tcp != (char *)NULL)
	{    /* fontpath is something like [tex.fonts] */
	    *tcp = '\0';	    /* clobber right bracket */
	    if (k == gf_index)
	    {
		(void)sprintf(filelist[m++], "%s.%d]%s.gf", temp_name,
		    dpi, font_name);
		(void)sprintf(filelist[m++], "%s]%s.%dgf", temp_name,
		    font_name, dpi);
	    }
	    else if (k == pk_index)
	    {
		(void)sprintf(filelist[m++], "%s.%d]%s.pk", temp_name,
		    dpi, font_name);
		(void)sprintf(filelist[m++], "%s]%s.%dpk", temp_name,
		    font_name, dpi);
	    }
	    else if (k == pxl_index)
	    {
		(void)sprintf(filelist[m++], "%s.%d]%s.pxl", temp_name,
		    magnification, font_name);
		(void)sprintf(filelist[m++], "%s]%s.%dpxl", temp_name,
		    font_name, magnification);
	    }
	}
#endif /* OS_VAXVMS */
    }

}

