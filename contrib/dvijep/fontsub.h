/* -*-C-*- fontsub.c */
/*-->fontsub*/
/**********************************************************************/
/****************************** fontsub *******************************/
/**********************************************************************/

BOOLEAN
fontsub(subfont,submag,TeXfont,TeXmag)
char *subfont;		/* new fontname returned */
int  *submag;		/* new magnification returned (0 if entire family) */
char *TeXfont;		/* TeX requested fontname */
int TeXmag;		/* TeX requested magnification value */

/***********************************************************************

Given a  TeX  fontname  and  a  magnification  value,  search  the  font
substitution file (if  any) for  the first pattern  defining a  possible
replacement, and return  their values  in subfont[] and  submag, with  a
function value of TRUE.   If no substitution  is possible, subfont[]  is
set to a null string, submag to 0, and the function value to FALSE.

The substitution file is searched for under the names

	(1) subfile (by default, or from command line)
	(2) subname subext
	(3) subpath subname subext

allowing the user to have document-file specific substitutions, TeX-wide
private substitutions, and system TeX-wide substitutions.  For  example,
these	 files	  might    be	 "sample.sub",	  "texfonts.sub",    and
"texinputs:texfonts.sub" for a document "sample.tex".

Font substitution lines have the form:

% comment
oldname.oldmag	->	subname.submag	% comment
oldname oldmag	->	subname submag	% comment
oldname 	->	subname		% comment

Examples are:

% These provide replacements for some LaTeX invisible fonts:
iamr10 1500	->	amr10 1500	% comment
iamr10.1500	->	amr10.1500	% comment
iamssb8		->	amssb8		% comment

These are  easily readable  by sscanf().   The first  two forms  request
substitution of a  particular font  and magnification.	 The third  form
substitutes an entire font  family; the closest available  magnification
to the required one will be  used.  Any dots in the non-comment  portion
will be converted  to spaces, and  therefore, cannot be  part of a  name
field.

The first  matching substitution  will  be selected,  so  magnification-
specific  substitutions   should   be	given	first,	 before   family
substitutions.

Comments are introduced by percent and continue to end-of-line, just  as
for TeX.   One  whitespace character  is  equivalent to  any  amount  of
whitespace.  Whitespace and comments are optional.
***********************************************************************/

{
    static FILE* subfp = (FILE *)NULL;	/* file pointer for sub file */
    static BOOLEAN have_file = TRUE;	/* memory of open success */
    char line[MAXSTR];			/* for sub file lines */
    char *pline;			/* pointer to line[] */
    static char fname[MAXFNAME];	/* sub file name */
    char oldfont[MAXFNAME];		/* working storage for font names */
    int oldmag;				/* font magnification */
    int k;				/* sscanf() result */
    int line_number;			/* for error messages */

    if (!have_file)			/* no file available */
    {
	*subfont = '\0';
	*submag = 0;
	return (FALSE);
    }
    else if (subfp == (FILE *)NULL)	/* happens only first time  */
    {
	(void)strcpy(fname,subfile);	/* try document specific: */
	subfp = fopen(fname,"r");	/* e.g. foo.sub (from foo.dvi) */
	DEBUG_OPEN(subfp,fname,"r");
	if (subfp == (FILE *)NULL)
	{
	    (void)strcpy(fname,subname);/* try path specific: */
	    (void)strcat(fname,subext);	/* e.g. texfonts.sub */
	    subfp = fopen(fname,"r");
	    DEBUG_OPEN(subfp,fname,"r");
	}

	if (subfp == (FILE *)NULL)
	{
#if 0					/* old single-path code */
	    (void)strcpy(fname,subpath);/* try system specific: */
	    (void)strcat(fname,subname);/* e.g. texinputs:texfonts.sub */
	    (void)strcat(fname,subext);
	    subfp = fopen(fname,"r");
	    DEBUG_OPEN(subfp,fname,"r");
#else					/* new code supporting path search */
	    char *p;
	    (void)strcpy(fname,subname);/* e.g. texinputs:texfonts.sub */
	    (void)strcat(fname,subext);
	    p = (char*)findfile(subpath,fname);
	    if (p != (char*)NULL)
	    {
		(void)strcpy(fname,p);
		subfp = fopen(fname,"r");
		DEBUG_OPEN(subfp,fname,"r");
	    }
#endif
	}

	if (subfp == (FILE *)NULL) /* could not open any substitution file */
	{
	    have_file = FALSE;
	    *subfont = '\0';
	    *submag = 0;
	    return (FALSE);
	}
    }

    line_number = 0;
    (void)REWIND(subfp);
    while (fgets(line,MAXSTR,subfp) != (char *)NULL)
    {
	line_number++;		/* count lines */

	pline = &line[0];	/* truncate at comment start or end-of-line */
	while ((*pline) && (*pline != '%') &&
	    (*pline != '\r') && (*pline != '\n'))
	{
	    if (*pline == '.')	/* convert dots to whitespace for sscanf() */
		*pline = ' ';
	    ++pline;
	}
	*pline = '\0';		/* replace terminator by NUL */

	if ((k = (int)sscanf(line," %s %d -> %s %d",oldfont,&oldmag,
	    subfont,submag)) == 4)
	{
	    if ((TeXmag == oldmag) && (strcm2(oldfont,TeXfont) == 0))
		return (TRUE);
	}
	else if ((k = (int)sscanf(line," %s -> %s",oldfont,subfont)) == 2)
	{
	    *submag = 0;
	    if (strcm2(oldfont,TeXfont) == 0)
		return (TRUE);
	}
	else if (k != EOF)	/* EOF means no fields, so have empty line */
	{
	    (void)sprintf(message,
		"fontsub():  Bad font substitution at line %d in file \
[%s]: [%s]",
		line_number,fname,line);
	    (void)warning(message);
	}
    }
    *subfont = '\0';
    *submag = 0;
    return (FALSE);
}
