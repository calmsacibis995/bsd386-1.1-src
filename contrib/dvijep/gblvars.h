/* -*-C-*- gblvars.h */
/*-->gblvars*/
/**********************************************************************/
/****************************** gblvars *******************************/
/**********************************************************************/

/**********************************************************************/
/*********************  General Global Variables  *********************/
/**********************************************************************/

char curpath[MAXFNAME];			/* current file area */
char curext[MAXFNAME];			/* current file extension */
char curname[MAXFNAME];			/* current file name */

UNSIGN16 debug_code;			/* 0 for no debug output */

char dviname[MAXFNAME];			/* DVI filespec */
char dvoname[MAXFNAME];			/* device output filespec */

char fontlist[MAXSTR];			/* FONTLIST environment string */
char fontpath[MAXFNAME];		/* font directory path */

char helpcmd[MAXSTR];			/* where to look for help */

char message[MAXMSG];			/* for formatting error messages */

/***********************************************************************
Magnification table for 144dpi, 200dpi, and 300dpi devices, computed
to 20 figures and sorted by magnitude.

    Column 1	     Column 2	     Column 3
0.72*sqrt(1.2)**i  sqrt(1.2)**I  1.5*sqrt(1.2)**I	(I = -16,16)

***********************************************************************/

static float mag_table[] =
	{
	0.16744898601451165028, 0.18343117374303022733, 0.20093878321741398034,
	0.22011740849163627280, 0.23256803936137783874, 0.24112653986089677641,
	0.25476552262595201888, 0.26414089018996352736, 0.27908164723365340649,
	0.28935184783307613169, 0.30571862715114242265, 0.31696906822795623283,
	0.33489797668038408779, 0.34722221739969135802, 0.34885205904206675812,
	0.36686235258137090718, 0.38036288187354747940, 0.38214828393892802832,
	0.40187757201646090535, 0.41666666087962962963, 0.41862247085048010974,
	0.44023482309764508862, 0.45643545824825697527, 0.45857794072671363398,
	0.48225308641975308642, 0.49999999305555555556, 0.50234696502057613169,
	0.52828178771717410634, 0.54772254989790837033, 0.55029352887205636077,
	0.57870370370370370370, 0.59999999166666666667, 0.60281635802469135802,
	0.63393814526060892761, 0.65726705987749004440, 0.66035223464646763293,
	0.69444444444444444444, 0.71999999000000000000, 0.72337962962962962963,
	0.76072577431273071313, 0.78872047185298805327, 0.79242268157576115952,
	0.83333333333333333333, 0.86399998800000000000, 0.86805555555555555556,
	0.91287092917527685576, 0.94646456622358566393, 0.95090721789091339142,
	1.00000000000000000000, 1.03679998560000000000, 1.04166666666666666670,
	1.09544511501033222690, 1.13575747946830279670, 1.14108866146909606970,
	1.20000000000000000000, 1.24415998272000000000, 1.25000000000000000000,
	1.31453413801239867230, 1.36290897536196335610, 1.36930639376291528360,
	1.44000000000000000000, 1.49299197926400000000, 1.50000000000000000000,
	1.57744096561487840680, 1.63549077043435602730, 1.64316767251549834040,
	1.72800000000000000000, 1.79159037511680000000, 1.80000000000000000000,
	1.89292915873785408810, 1.96258892452122723270, 1.97180120701859800840,
	2.07360000000000000000, 2.14990845014016000000, 2.16000000000000000000,
	2.27151499048542490570, 2.35510670942547267930, 2.36616144842231761010,
	2.48832000000000000000, 2.57989014016819200000, 2.59200000000000000000,
	2.72581798858250988690, 2.82612805131056721510, 2.83939373810678113220,
	2.98598400000000000000, 3.09586816820183040000, 3.11040000000000000000,
	3.27098158629901186430, 3.40727248572813735860, 3.58318080000000000000,
	3.73248000000000000000, 3.92517790355881423710, 4.08872698287376483030,
	4.29981696000000000000, 4.47897600000000000000, 4.90647237944851779640,
	5.37477120000000000000, 5.88776685533822135560, 6.44972544000000000000
	};

INT16 mag_index;		/* set by actfact */

#define MAGTABSIZE (sizeof(mag_table) / sizeof(float))

int g_errenc = 0;		/* has an error been encountered?	  */
char g_logname[MAXSTR];		/* name of log file, if created		  */
BOOLEAN g_dolog = TRUE;		/* allow log file creation		  */
FILE *g_logfp = (FILE*)NULL;	/* log file pointer (for errors)	  */
char g_progname[MAXSTR];	/* program name				  */

FILE *plotfp = (FILE*)NULL;	/* plot file pointer			  */

struct char_entry
{				/* character entry			  */
   COORDINATE wp, hp;		/* width and height in pixels		  */
   COORDINATE xoffp, yoffp;	/* x offset and y offset in pixels	  */
   long fontrp;			/* font file raster pointer		  */
   UNSIGN32 tfmw;		/* TFM width				  */
   INT32 dx, dy;		/* character escapements		  */
   UNSIGN16 pxlw;		/* pixel width == round(TFM width in	  */
				/* pixels for .PXL files, or		  */
				/* float(char_dx)/65536.0 for .GF and .PK */
				/* files)				  */
   INT16 refcount;		/* reference count for memory management  */
   UNSIGN32 *rasters;		/* raster description (dynamically loaded) */

#if    (HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2)
   BOOLEAN isloaded;		/* is the character already downloaded?   */
#endif /* (HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2) */

#if    HPJETPLUS
   BOOLEAN istoobig;		/* Too big (or too odd) to be loaded */
#endif

#if    CANON_A2
   BOOLEAN isknown;		/* Character is known */
   BOOLEAN istoobig;		/* Too big to be loaded */
#endif

#if    BBNBITGRAPH
   BOOLEAN istoobig;		/* is the character too big for BitGraph? */
   BOOLEAN isloaded;		/* is the character loaded in the BitGraph?*/
   INT16 bgfont, bgchar;	/* BitGraph font and character		  */
#endif /* BBNBITGRAPH */

};

struct font_entry
{
    struct font_entry *next;	/* pointer to next font entry		   */
    void (*charxx)();		/* pointer to chargf(), charpk(), charpxl()*/
    FILE *font_file_id;		/* file identifier (NULL if none)	   */
    INT32 k;			/* font number                             */
    UNSIGN32 c;			/* checksum                                */
    UNSIGN32 d;			/* design size                             */
    UNSIGN32 s;			/* scale factor                            */
    INT32 font_space;		/* computed from FNT_DEF s parameter	   */
    UNSIGN32 font_mag;		/* computed from FNT_DEF s and d parameters*/
    UNSIGN32 magnification;	/* magnification read from PXL file	   */
    UNSIGN32 designsize;	/* design size read from PXL file	   */
    UNSIGN32 hppp;		/* horizontal pixels/point * 2**16	   */
    UNSIGN32 vppp;		/* vertical pixels/point * 2**16	   */
    INT32 min_m;		/* GF bounding box values		   */
    INT32 max_m;
    INT32 min_n;
    INT32 max_n;

#if    (CANON_A2 | HPJETPLUS | IMPRESS | POSTSCRIPT)
    UNSIGN16 font_number;	/* font number (0..32767) */
#endif /* (CANON_A2 | HPJETPLUS | IMPRESS | POSTSCRIPT) */

#if    CANON_A2
    COORDINATE cell_w;
    COORDINATE cell_h;
    COORDINATE cell_d;
    UNSIGN16 nloaded;
    UNSIGN32 storage;
#endif /* CANON_A2 */

    BYTE font_type;		/* GF, PK, or PXL font file		   */
    BYTE a;			/* length of font area in n[]              */
    BYTE l;			/* length of font name in n[]              */
    char n[MAXSTR];		/* font area and name                      */
    char name[MAXSTR];		/* full name of PXL file		   */
    struct char_entry ch[NPXLCHARS];/* character information		   */
};

struct font_list
{
    FILE *font_id;		/* file identifier			   */
    INT16 use_count;		/* count of "opens"			   */
};

INT32 cache_size;		/* record of how much character raster	    */
				/* is actually used			    */
float conv;			/* converts DVI units to pixels		    */
UNSIGN16 copies;		/* number of copies to print of each page   */
INT16 cur_page_number;		/* sequential output page number in 1..N    */
INT16 cur_index;		/* current index in page_ptr[]              */

COORDINATE xcp,ycp;		/* current position			    */
UNSIGN32 den;			/* denominator specified in preamble	    */
FILE *dvifp = (FILE*)NULL;	/* DVI file pointer			    */
struct font_entry *fontptr;	/* font_entry pointer			    */
struct font_entry *hfontptr = (struct font_entry *)NULL;
				/* head font_entry pointer		    */

#if    (CANON_A2 | HPJETPLUS | IMPRESS | POSTSCRIPT)
UNSIGN16 font_count;		/* used to assign unique font numbers	    */
struct font_entry *font_table[MAXFONTS];
#endif /* (CANON_A2 | HPJETPLUS | IMPRESS | POSTSCRIPT) */

#if    (HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2)
BOOLEAN font_switched;		/* current font has changed		    */
#endif /* (HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2) */

INT16 gf_index, pk_index, pxl_index;
				/* indexes into filelist[] in fontfile();   */
				/* they define the search order, and are    */
				/* in initglob().			    */
UNSIGN32 gpower[33];		/* gpower[k] = 2**k-1 (k = 0..32)	    */
INT32 h;			/* current horizontal position		    */
COORDINATE hh;			/* current horizontal position in pixels    */

#if    HPLASERJET
INT16 hpres;			/* output resolution (75, 100, 150, 300 dpi) */
#endif /* HPLASERJET */

#if    HPJETPLUS
BOOLEAN landscape = FALSE;	/* landscape mode for HP LaserJet+          */
#endif /* HPJETPLUS */

UNSIGN32 img_mask[32];		/* initialized at run-time so that bit k    */
				/* (counting from high end) is one	    */
UNSIGN32 img_row[(MAX_M - MIN_M + 1 + 31) >> 5];
				/* current character image row of bits	    */
INT16 max_m, min_m, max_n, min_n;
				/* current character matrix extents	    */
UNSIGN16 img_words;		/* number of words in use in img_row[]	    */
float leftmargin;		/* left margin in inches		    */
COORDINATE lmargin;		/* left margin offset in pixels		    */
INT16 nopen;			/* number of open PXL files		    */
INT16 page_count;		/* number of entries in page_ptr[]	    */

#if    HPJETPLUS
INT16 page_fonts;		/* count of fonts used on current page      */
#endif /* HPJETPLUS */

long page_ptr[MAXPAGE+1];	/* byte pointers to pages (reverse order)   */

#if    POSTSCRIPT
long page_loc[MAXPAGE+1];	/* byte pointers to output pages	    */
INT32 page_tex[MAXPAGE+1];	/* TeX's \count0 page numbers		    */
#endif /* POSTSCRIPT */

INT16 page_begin[MAXREQUEST+1],
    page_end[MAXREQUEST+1],
    page_step[MAXREQUEST+1];	/* explicit page range requests		    */
INT16 npage;			/* number of explicit page range requests   */
struct font_list font_files[MAXOPEN+1];
				/* list of open PXL file identifiers	    */

UNSIGN32 power[32];		/* power[k] = 1 << k			    */

#if    POSTSCRIPT
BOOLEAN ps_vmbug;		/* reload fonts on each page when TRUE	    */
#endif /* POSTSCRIPT */

UNSIGN32 rightones[HOST_WORD_SIZE];/* bit masks */

#if    (APPLEIMAGEWRITER | EPSON | DECLA75 | DECLN03PLUS)
BOOLEAN runlengthcode = FALSE;		/* this is runtime option '-r' */
#endif /* (APPLEIMAGEWRITER | EPSON | DECLA75 | DECLN03PLUS) */

#if    (GOLDENDAWNGL100 | TOSHIBAP1351)
BOOLEAN runlengthcode = FALSE;		/* this is runtime option '-r' */
#endif /* (GOLDENDAWNGL100 | TOSHIBAP1351) */

UNSIGN32 runmag;		/* runtime magnification		    */
UNSIGN32 mag;			/* magnification specified in preamble	    */
UNSIGN32 num;			/* numerator specified in preamble	    */
struct font_entry *pfontptr = (struct font_entry *)NULL;
				/* previous font_entry pointer		    */
BOOLEAN preload = TRUE;		/* preload the font descriptions?	    */
FILE *fontfp = (FILE*)NULL;	/* font file pointer			    */

BOOLEAN quiet = FALSE;		/* suppress status display when TRUE	    */

BOOLEAN backwards = FALSE;	/* print in backwards order		    */

#if    (CANON_A2 | HPJETPLUS | IMPRESS | POSTSCRIPT)
COORDINATE rule_height;		/* parameters of last rule set */
COORDINATE rule_width;
COORDINATE str_ycp;		/* last string ycp value */
UNSIGN16 size_limit;		/* character size limit in pixels -- larger */
				/* characters are downloaded each time they */
				/* are required to avoid PostScript ROM bugs */
#endif /* (CANON_A2 | HPJETPLUS | IMPRESS | POSTSCRIPT) */

#if    (BSD42 | OS_TOPS20)
BOOLEAN spool_output = FALSE;	/* offer to send output to spooler */
#endif /* (BSD42 | OS_TOPS20) */

char subpath[MAXFNAME];		/* font substitution file path		    */
char subname[MAXFNAME];		/* font substitution file name field	    */
char subext[MAXFNAME];		/* font substitution file extension field   */
char subfile[MAXFNAME];		/* font substitution filename		    */

INT32 tex_counter[10];		/* TeX c0..c9 counters on current page      */
float topmargin;		/* top margin in inches			    */
COORDINATE tmargin;		/* top margin offset in pixels		    */

INT32 v;			/* current vertical position		    */

#if    VIRTUAL_FONTS
BOOLEAN virt_font;		/* virtual font cache flag                  */
struct virt_data
    {
	int	cnt;
	char	*ptr;
	char	*base;
    };
struct virt_data virt_save[_NFILE];/* space for saving old FILE values      */
#endif /* VIRTUAL_FONTS */

COORDINATE vv;			/* current vertical position in pixels	    */

#if    BBNBITGRAPH
struct char_entry *bgcp[NBGFONTS+(NBGFONTS+2)/3][NPXLCHARS];
	/* Pointer to corresponding char_entry for this BitGraph font */
	/* and character.  These are used to set the char_entry's */
	/* status to "not loaded" when we have to reuse the BitGraph */
	/* character.  The array is cleared initially in devinit(). */

INT16 fullfont = 0;		/* full font to load in BitGraph	    */
BOOLEAN g_interactive=TRUE;	/* is the program running interactively   */
				/* (i.e., standard output not redirected)? */
INT16 partchar = FIRSTBGCHAR;	/* partial font character to load in BitGraph*/
INT16 partfont = NBGFONTS;	/* partial font to load in BitGraph	    */
INT16 pbghpos;			/* previous BitGraph horizontal position    */
INT16 pbgvpos;			/* previous BitGraph vertical position	    */
INT16 pbgf = -1;		/* previous BitGraph font		    */
COORDINATE xdiff;		/* x difference				    */
COORDINATE xscreen;		/* x screen adjustment			    */
COORDINATE ydiff;		/* y difference				    */
COORDINATE yscreen;		/* y screen adjustment			    */
long cpagep;			/* pointer to current page in DVI file	    */
long ppagep;			/* pointer to previous page in DVI file     */

#if    OS_TOPS20
#define jfn_plotfp (jfnof(fileno(plotfp)))

int bg_length,bg_width,bg_1ccoc,bg_2ccoc,bg_modeword,bg_sysmsg;
#endif /* OS_TOPS20 */

#endif /* BBNBITGRAPH */
