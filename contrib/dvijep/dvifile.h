/* -*-C-*- dvifile.h */
/*-->dvifile*/
/**********************************************************************/
/****************************** dvifile *******************************/
/**********************************************************************/

void
dvifile(argc,argv,filestr)
int   argc;			/* argument count */
char* argv[];			/* argument strings */
char* filestr;			/* DVI filename to process */
{

#define PAGENUMBER(p) ((p) < 0 ? (page_count + 1 + (p)) : (p))

    register INT16 i,j,m;	/* loop indices */
    register INT16 swap;	/* swap storage */

    INT16 m_begin,m_end,m_step;	/* loop limits and increment */
    INT16 list[3];		/* index list for sort */
    char tempstr[MAXSTR];	/* temporary string storage */


    /*
    Establish the default  font file  search order.  This  is done  here
    instead of  in  initglob()  or  option(),  because  both  could  set
    fontlist[].

    By default, the search list contains  names for the PK, GF, and  PXL
    font  files;  that  order   corresponds  to  increasing  size   (and
    therefore, presumably, increasing  processing cost)  of font  raster
    information.  This search  order may  be overridden at  run time  by
    defining an alternate one in the environment variable FONTLIST;  the
    order of  the  strings  "PK",  "GF",  and  "PXL"  in  that  variable
    determines the search  order, and  letter case  is NOT  significant.
    The  substrings  may  be  separated   by  zero  or  more   arbitrary
    characters, so  the  values  "PK-GF-PXL", "PK  GF  PXL",  "PKGFPXL",
    "PK;GF;PXL", "PK/GF/PXL"  are all  acceptable,  and all  choose  the
    default search  order.   This  flexibility  in  separator  character
    choice is occasioned  by the  requirement on some  systems that  the
    environment variable have the syntax of a file name, or be a  single
    "word".  If  any  substring  is  omitted, then  that  font  will  be
    excluded from consideration.  Thus, "GF" permits the use only of  GF
    font files.

    The indexes gf_index, pk_index, and pxl_index are in -1 .. 2, and at
    least one must be non-negative.  A negative index excludes that font
    file type from the search.
    */

    /* Note that non-negative entries in list[] are UNIQUE. */
    list[0] = gf_index = (INT16)strid2(fontlist,"GF");
    list[1] = pk_index = (INT16)strid2(fontlist,"PK");
    list[2] = pxl_index = (INT16)strid2(fontlist,"PXL");

    for (i = 0; i < 3; ++i)	/* put list in non-decreasing order */
	for (j = i+1; j < 3; ++j)
	    if (list[i] > list[j])
	    {
		swap = list[i];
		list[i] = list[j];
		list[j] = swap;
	    }
    for (i = 0; i < 3; ++i)	/* assign indexes 0,1,2 */
	if (list[i] >= 0)
	{
            if (list[i] == gf_index)
		gf_index = i;
            else if (list[i] == pk_index)
		pk_index = i;
            else if (list[i] == pxl_index)
		pxl_index = i;
	}

    if ((gf_index < 0) && (pk_index < 0) && (pxl_index < 0))
	(void)fatal("dvifile():  FONTLIST does not define at least one of \
GF, PK, or PXL fonts");

    (void)dviinit(filestr);	/* initialize DVI file processing */

    (void)devinit(argc,argv);	/* initialize device output */

    (void)readpost();
    (void)FSEEK(dvifp, 14L, 0); /* position past preamble */
    (void)getbytes(dvifp, tempstr,
	(BYTE)nosignex(dvifp,(BYTE)1)); /* flush DVI comment */

    cur_page_number = 0;


#if    (HPLASERJET|HPJETPLUS|GOLDENDAWNGL100|POSTSCRIPT|IMPRESS|CANON_A2)
    /* print pages in reverse order because of laser printer */
    /* page stacking */
    if (backwards)
    {
	m_begin = 1;
	m_end = page_count;
	m_step = 1;
    }
    else	/* normal device order */
    {
	m_begin = page_count;
	m_end = 1;
	m_step = -1;
    }

#else
  /* NOT (HPLASERJET|HPJETPLUS|GOLDENDAWNGL100|POSTSCRIPT|IMPRESS|CANON_A2) */
    /* print pages in forward order for most devices */
    if (backwards)
    {
	m_begin = page_count;
	m_end = 1;
	m_step = -1;
    }
    else
    {
	m_begin = 1;
	m_end = page_count;
	m_step = 1;
    }
#endif /* (HPLASERJET|HPJETPLUS|GOLDENDAWNGL100|POSTSCRIPT|IMPRESS|CANON_A2) */

    for (i = 0; i < npage; ++i)		/* make page numbers positive */
    {					/* and order pairs non-decreasing */
	page_begin[i] = PAGENUMBER(page_begin[i]);
	page_end[i] = PAGENUMBER(page_end[i]);
	if (page_begin[i] > page_end[i])
	{
	    swap = page_begin[i];
	    page_begin[i] = page_end[i];
	    page_end[i] = swap;
	}
    }

    for (m = m_begin; ; m += m_step)
    {
	for (i = 0; i < npage; ++i)	/* search page list */
	    if ( IN(page_begin[i],m,page_end[i]) &&
	        (((m - page_begin[i]) % page_step[i]) == 0) )
	    {
		if (!quiet)		/* start progress report */
		    (void)fprintf(stderr,"[%d", m);
		cur_page_number++;/* sequential page number 1..N */
		cur_index = m-1;	/* remember index globally */
		prtpage(page_ptr[cur_index]);
		if (!quiet)		/* finish progress report */
		    (void)fprintf(stderr,"] ");
		break;
	    }
	if (m == m_end)			/* exit loop after last page */
	    break;
    }

    (void)devterm();		/* terminate device output */

    (void)dviterm();		/* terminate DVI file processing */
}
