/* -*-C-*- openfont.h */
/*-->openfont*/
/**********************************************************************/
/****************************** openfont ******************************/
/**********************************************************************/

void
openfont(fontname)
char *fontname;

/***********************************************************************
    The original version of this DVI driver reopened the font file  each
    time the font changed, resulting in an enormous number of relatively
    expensive file  openings.	This version  keeps  a cache  of  up  to
    MAXOPEN open files,  so that when  a font change  is made, the  file
    pointer, fontfp, can  usually be  updated from the  cache.  When the
    file is not found in  the cache, it must  be opened.  In this  case,
    the next empty slot  in the cache  is assigned, or  if the cache  is
    full, the least used font file is closed and its slot reassigned for
    the new file.  Identification of the least used file is based on the
    counts of the number  of times each file  has been "opened" by  this
    routine.  On return, the file pointer is always repositioned to  the
    beginning of the file.

    If the first open attempt  fails, an attempt will  be made to use  a
    substitute font, then neighboring magnifications (with the same font
    name), or substitutes for them.
***********************************************************************/

{
    register INT16 i,j,k;	/* loop indices */
    INT16 current;
    INT16 least_used;
    INT16 maxopen = MAXOPEN;

#if    OS_VAXVMS
    long *jpi;
#endif

    struct font_entry *tfontptr;
    char subfont[MAXFNAME];
    int submag;
    char* filelist[MAXFORMATS];	/* pointers to templist[][] */
    char templist[MAXFORMATS][MAXFNAME];

#if    VIRTUAL_FONTS
    struct stat statbuf;		/* so fstat() can get file size */
    char *p;
#endif

    if ((pfontptr != (struct font_entry *)NULL) && (pfontptr == fontptr))
	return;			/* we need not have been called */

    for (j = 0; j < MAXFORMATS; ++j)	/* initialize fontlist pointers */
	filelist[j] = &templist[j][0];

#if    IBM_PC_MICROSOFT
    /* This code required rewriting to get around a fatal compiler
    assertion error occasioned by the code in the #else part */
    for (current = 1; current <= nopen; ++current)
    {
    	FILE* pfp;

	pfp = font_files[current].font_id;
	if (pfp == fontptr->font_file_id)
	    break;
    }
#else
    for (current = 1;
	(current <= nopen) &&
	    (font_files[current].font_id != fontptr->font_file_id);
	++current)
	;			/* try to find file in open list */
#endif

    if (current <= nopen)	/* file already open, lookup its id */
	fontfp = font_files[current].font_id;
    else
    {
	/***************************************************************
	The file was  not in list  of open  files.  If the  list is  not
	full, add it to  the list; otherwise  close the least-used  file
	and remove it from the font_entry containing it.  Finally,  open
	the file, or its closest  neighbor in the magnification  family.
	A warning is issued  if no file can  be opened.  The caller  can
	then proceed with zero font metrics if desired.
	***************************************************************/

#if    OS_VAXVMS
        /***************************************************************
	VAX VMS has many user quotas, one of which is the maximum number
	of files  that can be open, which  need have no relation  to the
	number  that C  permits.  If  we do  not determine the  limit at
	run-time, the drivers may attempt to open too many files, and in
	such a case, will fail.   There  are two relevant  quotas, FILLM
	(open file limit), and  FILCNT (remaining open file quota).   We
	use the latter,  and leave  one available file  for a   possible
	error log.
	***************************************************************/

	jpi = (long*)getjpi(JPI$_FILCNT);
	if (jpi == (long*)NULL)
	    maxopen = MAXOPEN;		/* should never happen */
	else
	    maxopen = nopen - 1 + *jpi;
	maxopen = MIN(maxopen,MAXOPEN);	/* we have arrays of size MAXFONT */
					/* so do not exceed that limit */
#endif /* OS_VAXVMS */

	if (nopen < maxopen)	/* just add it to list */
	    current = ++nopen;
	else			/* list full -- find least used file, */
	{			/* close it, and reuse slot for new file */
	    least_used = 1;
	    for (i = 2; i <= maxopen; ++i)
		if (font_files[least_used].use_count >
		    font_files[i].use_count)
		    least_used = i;

	    fontfp = font_files[least_used].font_id;
	    tfontptr = hfontptr;
	    while (tfontptr != (struct font_entry*)NULL)
	    {			/* remove file pointer from its font_entry */
		if (tfontptr->font_file_id == fontfp)
		{
		    tfontptr->font_file_id = (FILE*)NULL;
		    break;
		}
		tfontptr = tfontptr->next;
	    }

#if    VIRTUAL_FONTS
	    if (virt_font && (fontfp != (FILE*)NULL))
	        (void)virtfree(fontfp);
#endif

	    (void)fclose(fontfp);
	    fontfp = (FILE*)NULL;
	    current = least_used;
	}
	(void)actfact(fontptr->font_mag);	/* Get global mag_index */

	fontfp = (FILE*)NULL;

	/***************************************************************
	Try the requested font, then any substitute font, then for  each
	neighboring magnification  from  nearest to  furthest,  try  the
	requested font, and then any substitute font.
	***************************************************************/

	for (k = 0; (fontfp == (FILE*)NULL) && (k < MAGTABSIZE); ++k)
	{				/* loop over mag family */
	    for (i = -k; (fontfp == (FILE*)NULL) && (i <= k); i += MAX(1,k+k))
	    {				/* try smaller, then larger */
		if (IN(0,mag_index+i,MAGTABSIZE-1))
		{
		    (void)fontfile(filelist, ((fontptr->a==0)?fontpath:""),
			fontname, (int)MAGSIZE(mag_table[mag_index+i]));
		    for (j = 0; (j < MAXFORMATS) && *filelist[j]; ++j)
		    {
			fontfp = FOPEN(filelist[j],RB_OPEN);
			DEBUG_OPEN(fontfp,filelist[j],RB_OPEN);
			if (fontfp != (FILE *)NULL)
			{
			    strcpy(fontptr->name,filelist[j]);
			    break;
			}
		    }
		    if ((k > 0) && (fontfp != (FILE*)NULL))
		    {
			(void)sprintf(message,
			    "Font file [%s [mag %d]] could not be opened.\n\
---using nearest neighbor [%s [mag %d]] instead.",
			    fontname,(int)MAGSIZE(mag_table[mag_index]),
			    fontptr->name,
			    (int)MAGSIZE(mag_table[mag_index+i]));
			(void)warning(message);
		    }

		    if ((fontfp == (FILE*)NULL) && fontsub(subfont,&submag,
			fontname,(int)MAGSIZE(mag_table[mag_index+i])))
		    {
			(void)fontfile(filelist,((fontptr->a==0)?fontpath:""),
			    subfont,(submag ? submag :
			    (int)MAGSIZE(mag_table[mag_index+i])));
			for (j = 0; (j < MAXFORMATS) && *filelist[j]; ++j)
			{
			    fontfp = FOPEN(filelist[j],RB_OPEN);
			    DEBUG_OPEN(fontfp,filelist[j],RB_OPEN);
			    if (fontfp != (FILE *)NULL)
			    {
				strcpy(fontptr->name,filelist[j]);
				break;
			    }
			}

			if (fontfp != (FILE*)NULL)
			{
			    (void)sprintf(message,
				"Substituting font file [%s [mag %d]] \
by [%s [mag %d]]",
				fontname,(int)MAGSIZE(mag_table[mag_index]),
				fontptr->name,
				(int)MAGSIZE(mag_table[mag_index+i]));
			    (void)warning(message);
			}
		    }
		}
	    } /* end for (i) -- loop over smaller and larger neighbors  */
	} /* end for (k) -- loop over mag family */

	if (fontfp == (FILE*)NULL)
	{
	    --nopen;			/* don't count this failed open */
	    (void)sprintf(message,"Font file [%s [mag %d]] could not be \
opened; %d font files are open\n\
Proceeding with zero size characters for this font",
		fontname,(int)MAGSIZE(mag_table[mag_index]),nopen);
	    (void)warning(message);
	}

	font_files[current].font_id = fontfp;
	font_files[current].use_count = 0;

#if    VIRTUAL_FONTS
	/*
	****************************************************************
	This code  is implementation-dependent.   On many  C  compilers,
	FILE points to a struct of the form

	struct	_iobuf {
		char	*_ptr;	// pointer to next available char
		int	_cnt;	// number of chars left in buffer
		char	*_base;	// pointer to start of buffer
		int	_flag;	// assorted flags
		int	_file;	// file number handle
		}

	To implement virtual fonts,  we save the  pointers in a  private
	global array, get the file size from fstat(), malloc() a  buffer
	for it, read the entire file in, and replace the pointers in the
	FILE variable with ones  for  the  new buffer.  Just before  the
	file is  closed, the space is  freed  and the   old pointers are
	restored.   Although many C implementations use  malloc() to get
	the buffer  space  in the  first place, which  would  make  this
	saving unnecessary, not all do; see  the implementation given in
	Kernighan and Ritchie "The C Programming Language", p. 168.

	In  implementing   this   code   on   any   new   machine,   the
	implementations of fread(), read(), fseek(), and rewind() should
	be checked to  choose the  most efficient one  (e.g. one  system
	call for entire file  read, and no  double buffering), and  make
	sure that  fseek() and  rewind()  do not  unnecessarily  discard
	input buffer  contents.  fseek()  and  rewind() are  defined  in
	terms of macros FSEEK() and REWIND() in machdefs.h to facilitate
	replacement.
	****************************************************************
	*/
	if (virt_font)
	{
	    virt_save[fileno(fontfp)].base = (char *)NULL;
	    (void)fstat(fileno(fontfp),&statbuf);	/* get file size */
	    p = (char *)MALLOC((int)statbuf.st_size);	/* get file buffer */
	    if (p != (char *)NULL)
	    {
		if (READ(fileno(fontfp),p,(int)statbuf.st_size)
		     == (int)statbuf.st_size)
		{			/* read successful */
		    virt_save[fileno(fontfp)].ptr = FILE_PTR(fontfp);
		    virt_save[fileno(fontfp)].cnt = FILE_CNT(fontfp);
		    virt_save[fileno(fontfp)].base = FILE_BASE(fontfp);
		    FILE_PTR(fontfp) = p;
		    FILE_BASE(fontfp) = p;
		    FILE_CNT(fontfp) = (int)statbuf.st_size;
		}
		else			/* failure */
		{
		    (void)REWIND(fontfp);
		    (void)free(p);	/* free dynamic buffer */
		}
	    }
	    if (DBGOPT(DBG_FONT_CACHE))
	    {
	        (void)fprintf(stderr,"\nopenfont(): Font file %d [%s] ",
			(int)fileno(fontfp),fontptr->name);
	        if (p == (char *)NULL)
		    (void)fprintf(stderr,
			"cannot callocate buffer for entire file\n");
		else
		    (void)fprintf(stderr,
			"buffer length 0x%x\told buffer at 0x%lx\t\
new buffer at 0x%lx\n",
			(int)statbuf.st_size,
			(long)virt_save[fileno(fontfp)].base,
			(long)FILE_BASE(fontfp));
	    }
	}
#endif

    } /* end if (file is in open list) */

    pfontptr = fontptr;			/* make previous = current font */
    fontptr->font_file_id = fontfp;	/* set file identifier */
    font_files[current].use_count++;	/* update reference count */
}

#if    VIRTUAL_FONTS
void
virtfree(fp)				/* free buffer space before close */
FILE *fp;
{
    if (virt_font && (fp != (FILE*)NULL) &&
	(virt_save[fileno(fp)].base != (char *)NULL))
    {
	(void)fflush(fp);
	(void)free(FILE_BASE(fp));
	FILE_PTR(fp) = virt_save[fileno(fp)].ptr;
	FILE_CNT(fp) = virt_save[fileno(fp)].cnt;
	FILE_BASE(fp) = virt_save[fileno(fp)].base;
	virt_save[fileno(fp)].base = (char *)NULL;
    }
}
#endif

