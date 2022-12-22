/* -*-C-*- gendefs.h */
/*-->gendefs*/
/**********************************************************************/
/****************************** gendefs *******************************/
/**********************************************************************/

/**********************************************************************/
/************************  General Definitions  ***********************/
/**********************************************************************/

/***********************************************************************
This section should not require modification for either new hosts or new
output devices.
***********************************************************************/

#define  ABS(x)		((x) < 0 ? -(x) : (x))
#define  DBGOPT(flag)	(debug_code & (flag))
#define  DBG_PAGE_DUMP	0x0001
#define  DBG_CHAR_DUMP	0x0002
#define  DBG_POS_CHAR	0x0004
#define  DBG_OKAY_OPEN	0x0008
#define  DBG_FAIL_OPEN	0x0010
#define  DBG_OFF_PAGE	0x0020
#define  DBG_FONT_CACHE	0x0040
#define  DBG_SET_TEXT	0x0080
#define  DEBUG_OPEN(fp,fname,openmode) dbgopen(fp,fname,openmode)
#define  DVIFORMAT	  2
#define  FIRSTPXLCHAR	  0
#define  FT_GF		0
#define  FT_PK		1
#define  FT_PXL		2
#define  IN(a,b,c)	(((a) <= (b)) && ((b) <= (c)))

/* Computer Modern has 128 characters (0..127), but Japanese fonts and
extended European Computer Moderns may have up to 256 (0..255)
characters */
#define  LASTPXLCHAR	255

#define  MAGSIZE(f)	((UNSIGN32)(1000.0*(f) + 0.5))

#ifdef MAX
#undef MAX
#endif

#define  MAX(a,b)	((a) > (b) ? (a) : (b))
#define  MAXFONTS       32   /* number of fonts per job (HPLJ, Canon A2) */
#define  MAXMSG		1024 /* message[] size--big enough for 2 file names*/
#define  MAXPAGE	999  /* limit on number of pages in a DVI file */
#define  MAXREQUEST	256  /* limit on number of explicit page print
				requests */
#define  MAXSPECIAL	500  /* limit on \special{} string size; it need not
				be larger than TeX's compile-time parameter
				buf_siz, which is 500 in Standard TeX-82 */
#define  MAXSTR		257  /* DVI file text string size */

#define  MAXFORMATS	 12  /* number of font file naming formats */
#define  MIN_M		-500 /* GF character image extents */
#define  MAX_M		1500
#define  MIN_N		-500
#define  MAX_N		1500

#ifdef MIN
#undef MIN
#endif

#define  MIN(a,b)	((a) < (b) ? (a) : (b))

#if    (OS_ATARI | OS_PCDOS | OS_TOPS20)
#define NEWLINE(fp) {(void)putc((char)'\r',fp);(void)putc((char)'\n',fp);}
					/* want <CR><LF> for these systems */
#else
#define NEWLINE(fp) (void)putc((char)'\n',fp)	/* want bare <LF> */
#endif

#define  NPXLCHARS	256
#define  ONES		 ~0  /* a word of all one bits */
#define  OUTC(c)	(void)putc((char)(c),plotfp)
#define  OUTF(fmt,v)	(void)fprintf(plotfp,fmt,v)
#define  OUTF2(fmt,u,v)	(void)fprintf(plotfp,fmt,u,v)
#define  OUTF3(fmt,u,v,w)	(void)fprintf(plotfp,fmt,u,v,w)
#define  OUTS(s)	(void)fputs(s,plotfp)

#if    CANON_A2

#ifdef CANON_TEST
#define  FOUTC(c)	(void)putc((char)(c),savefp);
#define  FOUTS(s)	(void)fputs(s,savefp)
#endif /* CANON_TEST */

#endif

#define  PIXROUND(n,c)  ((COORDINATE)(((float)(n))*(c) + 0.5))

#undef	 PXLID
#define  PXLID	       1001

#define  SETBIT(m) img_row[(m-min_m) >> 5] |= img_mask[(m-min_m) & 0x1f]
#define  TESTBIT(m) img_row[(m-min_m) >> 5] & img_mask[(m-min_m) & 0x1f]

#if    STDRES
#define  STDMAG		1000
#else
#define  STDMAG		603  /* 1500/(1.2**5) to stay in magstep family */
#endif

#define  RESOLUTION	(((float)STDMAG)/5.0)	/* dots per inch */
#define  STACKSIZE	100

#undef	 USEGLOBALMAG
#define  USEGLOBALMAG	  1		/* allow dvi global magnification */

#define  VISIBLE(t) ((t->wp > 0) && (t->hp > 0))
		/* true if both pixel height and width are non-zero */
