/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/textfmt/textfmt.c++,v 1.1.1.1 1994/01/14 23:10:58 torek Exp $
/*
 * Copyright (c) 1993 Sam Leffler
 * Copyright (c) 1993 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */

/*
 * A sort of enscript clone.  This program is derived from the
 * lptops program by Nelson Beebe.  Any bugs are my fault...
 */
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "Types.h"
#include "Array.h"
#include "PageSize.h"
extern "C" {
#include <locale.h>
}

typedef long Coord;		// local coordinates
#define LUNIT 	(72*20)		// local coord system is .05 scale
#define	ICVT(x) ((Coord)((x)*LUNIT))	// scale inches to local coordinates
#define	CVTI(x)	(float(x)/LUNIT)	// convert coords to inches

inline Coord fxmin(Coord a, Coord b)	{ return (a < b) ? a : b; }
inline Coord fxmax(Coord a, Coord b)	{ return (a > b) ? a : b; }

#define COLFRAC		35	// 1/fraction of col width for margin

char*	tempfile;		// temp filename
FILE*	tf;			// temporary output file
#define NCHARS 256            	// number of chars per font
long	CharWidth[NCHARS];	// font width array
char*	prog;			// program name

fxDECLARE_PrimArray(SizetArray, size_t);
fxIMPLEMENT_PrimArray(SizetArray, size_t);

fxBool	gaudy		= FALSE;// emit gaudy headers
fxBool	landscape	= FALSE;// horizontal landscape mode output
fxBool	useISO8859	= TRUE;	// use the ISO 8859-1 character encoding
fxBool	reverse		= FALSE;// page reversal flag
fxBool	wrapLines	= TRUE;	// wrap/truncate lines
fxBool	headers		= TRUE;	// emit page headers
const char* font_name;		// text font name
int	firstPageNum	= 1;	// starting page number
int	numcol		= 1;	// number of text columns
SizetArray* pageOff;		// page offset table
float	physPageHeight;		// physical page height (inches)
float	physPageWidth;		// physical page width (inches)
Coord	pointSize	= -1;	// font point size in big points
Coord	lm, rm;			// left, right margins in local coordinates
Coord	tm, bm;			// top, bottom margin in local coordinates
const char* curFile;		// current input file
char	modDate[80];		// date of last modification to input file
char	modTime[80];		// time of last modification to input file
int	tabStop		= 8;	// n-column tab stop

Coord	lineHeight	= 0;	// inter-line spacing
fxBool	boc;			// at beginning of a column
fxBool	bop;			// at beginning of a page
int	column		= 1;	// current text column # (1..numcol)
Coord	col_margin	= 0L;	// inter-column margin
Coord	col_width;		// column width in local coordinates
int	level;			// PS string parenthesis level
Coord	outline		= 0L;	// page and column outline linewidth
Coord	pageHeight;		// page height in local coordinates
int	pageNum		= 1;	// current page number
Coord	pageWidth;		// page width in local coordinates
Coord	right_x;		// column width (right hand side x)
Coord	tabWidth;		// tab stop width in local units
Coord	x, y;			// current coordinate

static void Copy_Block(long ,long);
static void doFile(FILE *);
static void endLine(void);
static void endTextLine();
static void endTextCol();
static Coord inch(const char *);
static void readMetrics(void);
static fxBool findFont(const char* name);
static void setDefaultFont();
static void usage();
static void fatal(const char* fmt...);
static void emitPrologue(FILE*, int argc, char* argv[]);
static void emitTrailer(FILE*);
static void setupPageSize(const char* name);
static void setupMargins(const char* s);

int
main(int argc, char* argv[])
{
    extern int optind;
    extern char* optarg;
    int c;

#ifdef LC_CTYPE
    setlocale(LC_CTYPE, "");
#endif
    setDefaultFont();
    pageOff = new SizetArray;

    lm = rm = inch("0.25in");
    tm = inch("0.85in");
    bm = inch("0.5in");
    tabStop = 8;
    setupPageSize("default");

    prog = argv[0];
    while ((c = getopt(argc, argv, "f:m:M:o:p:s:V:12BcDGrRU")) != -1)
	switch(c) {
	case '1':		// 1-column output
	case '2':		// 2-column output
	    numcol = c - '0';
	    break;
	case 'B':
	    headers = FALSE;
	    break;
	case 'c':		// clip/cut instead of wrapping lines
	    wrapLines = FALSE;
	    break;
	case 'D':		// don't use ISO 8859-1 encoding
	    useISO8859 = FALSE; 
	    break;
	case 'f':		// body font
	    if (!findFont(optarg)) {
		fprintf(stderr, "%s: Unknown font \"%s\".\n", prog, optarg);
		usage();
	    }
	    font_name = optarg;
	    break;
	case 'G':		// gaudy mode
	    gaudy = headers = TRUE;
	    break;
	case 'm':		// multi-column output
	    numcol = atoi(optarg);
	    break;
	case 'M':		// margin(s)
	    setupMargins(optarg);
	    break;
	case 'o':		// outline columns
	    outline = inch(optarg);
	    break;
	case 'p':		// text point size
	    pointSize = inch(optarg);
	    break;
	case 'r':		// rotate page (landscape)
	    landscape = TRUE;
	    break;
	case 'R':		// don't rotate page (portrait)
	    landscape = FALSE;
	    break;
	case 's':		// page size
	    setupPageSize(optarg);
	    break;
	case 'U':		// reverse page collation
	    reverse = TRUE;
	    break;
	case 'V':		// vertical line height+spacing
	    lineHeight = inch(optarg);
	    break;
	default:
	    fprintf(stderr,"Unrecognized option \"%c\".\n", c);
	    usage();
	}
    pageHeight = ICVT(physPageHeight);
    pageWidth = ICVT(physPageWidth);

    if (reverse) {
	/*
	 * Open the file w+ so that we can reread the temp file.
	 */
	tempfile = tmpnam(NULL);
	tf = fopen(tempfile, "w+");
	if (tf == NULL)
	    fatal("Cannot open temporary file \"%s\"", tempfile);
	unlink(tempfile);	/* remove temp file in case we get killed */
    } else
	tf = stdout;

    numcol = fxmax(1,numcol);
    if (pointSize == -1)
	pointSize = inch(numcol > 1 ? "7bp" : "10bp");
    else
	pointSize = fxmax(inch("3bp"), pointSize);
    if (pointSize > inch("18bp"))
	fprintf(stderr, "%s: Warning, point size is unusually large (>18pt).\n",
	    prog);
    readMetrics();			// read font metrics
    outline = fxmax(0L,outline);
    tabWidth = tabStop * CharWidth[' '];

    if (landscape) {
	Coord t = pageWidth; pageWidth = pageHeight; pageHeight = t;
    }
    if (lm+rm >= pageWidth || tm+bm >= pageHeight) {
	fprintf(stderr,"\n?Margin values too large for page\n");
	exit(1);
    }

    col_width = (pageWidth - (lm + rm))/numcol;
    if (numcol > 1 || outline)
	col_margin = col_width/COLFRAC;
    else
	col_margin = 0;
    /*
     * TeX's baseline skip is 12pt for
     * 10pt type, we preserve that ratio.
     */
    if (lineHeight <= 0)
	lineHeight = (pointSize * 12L) / 10L;

    emitPrologue(tf, argc, argv);
    if (optind < argc) {
	for (; optind < argc; optind++) {
	    curFile = argv[optind];
	    FILE* fp = fopen(curFile, "r");
	    if (fp != NULL) {
		struct stat sb;
		if (fstat(fileno(fp), &sb) >= 0) {
		    struct tm* tm = localtime(&sb.st_mtime);
		    strftime(modTime, sizeof (modTime), "%X", tm);
		    strftime(modDate, sizeof (modDate), "%D", tm);
		}
		doFile(fp);
		fclose(fp);
	    } else
		fprintf(stderr,"%s: Unable to open file.\n",  argv[optind]);
	}
    } else {
	curFile = "";
	doFile(stdin);
    }
    if (reverse) {
	/*
	 * Now rewind temporary file and write
	 * pages to stdout in reverse order.
	 */
	rewind(tf);
	Copy_Block(0L, (*pageOff)[0]-1);	/* 1st page is header stuff */
	size_t last = (*pageOff)[pageOff->length()-1];
	for (int k = pageNum-firstPageNum; k >= 0; --k) {
	    /* copy remainder in reverse order */
	    size_t next = (size_t) ftell(stdout);
	    Copy_Block((*pageOff)[k],last-1);
	    last = (*pageOff)[k];
	    (*pageOff)[k] = next;
	}
	if (fclose(tf))
	    fatal("Close failure on temporary file \"%s\"", tempfile);
    }
    emitTrailer(stdout);
    return (0);
}

static void
setupPageSize(const char* name)
{
    PageSizeInfo* info = PageSizeInfo::getPageSizeByName(name);
    if (!info)
	fatal("Unknown page size \"%s\"", name);
    physPageWidth = info->width() / 25.4;
    physPageHeight = info->height() / 25.4;
    delete info;
}

/*
 * Parse margin syntax: l=#,r=#,t=#,b=#
 */
static void
setupMargins(const char* s)
{
    for (const char* cp = s; cp && cp[0]; cp = strchr(cp, ',')) {
	if (cp[1] != '=') {
	    fprintf(stderr, "Bad margin syntax, expecting \"=\".\n");
	    usage();
	}
	Coord v = inch(&cp[2]);
	switch (tolower(cp[0])) {
	case 'b': bm = v; break;
	case 'l': lm = v; break;
	case 'r': rm = v; break;
	case 't': tm = v; break;
	default:
	    fprintf(stderr,"Unrecognized margin identifier \"%c\".\n", cp[0]);
	    usage();
	}
    }
}

static void
Copy_Block(long b1,long b2)		/* copy bytes b1..b2 to stdout */
{
    char buf[16*1024];

    for (long k = b1; k <= b2; k += sizeof (buf)) {
	size_t cc = fxmin(sizeof (buf), (u_int) (b2-k+1));
	fseek(tf, k, 0);		/* position to desired block */
	if (fread(buf, 1, cc, tf) != cc)
	    fatal("Input block length error on temporary file [%s]", tempfile);
	if (fwrite(buf, 1, cc, stdout) != cc)
	    fatal("Output block length error on stdout");
    }
    fflush(stdout);
}

const char* ISOprologue1 = "\
/ISOLatin1Encoding where{pop save true}{false}ifelse\n\
/ISOLatin1Encoding[\n\
 /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n\
 /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n\
 /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n\
 /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n\
 /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n\
 /.notdef /.notdef /space /exclam /quotedbl /numbersign\n\
 /dollar /percent /ampersand /quoteright /parenleft\n\
 /parenright /asterisk /plus /comma /minus /period\n\
 /slash /zero /one /two /three /four /five /six /seven\n\
 /eight /nine /colon /semicolon /less /equal /greater\n\
 /question /at /A /B /C /D /E /F /G /H /I /J /K /L /M\n\
 /N /O /P /Q /R /S /T /U /V /W /X /Y /Z /bracketleft\n\
 /backslash /bracketright /asciicircum /underscore\n\
 /quoteleft /a /b /c /d /e /f /g /h /i /j /k /l /m\n\
 /n /o /p /q /r /s /t /u /v /w /x /y /z /braceleft\n\
 /bar /braceright /asciitilde /guilsinglright /fraction\n\
 /florin /quotesingle /quotedblleft /guilsinglleft /fi\n\
 /fl /endash /dagger /daggerdbl /bullet /quotesinglbase\n\
 /quotedblbase /quotedblright /ellipsis /trademark\n\
 /perthousand /grave /scaron /circumflex /Scaron /tilde\n\
 /breve /zcaron /dotaccent /dotlessi /Zcaron /ring\n\
 /hungarumlaut /ogonek /caron /emdash /space /exclamdown\n\
 /cent /sterling /currency /yen /brokenbar /section\n\
 /dieresis /copyright /ordfeminine /guillemotleft\n\
 /logicalnot /hyphen /registered /macron /degree\n\
 /plusminus /twosuperior /threesuperior /acute /mu\n\
 /paragraph /periodcentered /cedilla /onesuperior\n\
 /ordmasculine /guillemotright /onequarter /onehalf\n\
 /threequarters /questiondown /Agrave /Aacute\n\
 /Acircumflex /Atilde /Adieresis /Aring /AE /Ccedilla\n\
 /Egrave /Eacute /Ecircumflex /Edieresis /Igrave /Iacute\n\
 /Icircumflex /Idieresis /Eth /Ntilde /Ograve /Oacute\n\
 /Ocircumflex /Otilde /Odieresis /multiply /Oslash\n\
 /Ugrave /Uacute /Ucircumflex /Udieresis /Yacute /Thorn\n\
 /germandbls /agrave /aacute /acircumflex /atilde\n\
 /adieresis /aring /ae /ccedilla /egrave /eacute\n\
 /ecircumflex /edieresis /igrave /iacute /icircumflex\n\
 /idieresis /eth /ntilde /ograve /oacute /ocircumflex\n\
 /otilde /odieresis /divide /oslash /ugrave /uacute\n\
 /ucircumflex /udieresis /yacute /thorn /ydieresis\n\
]def{restore}if\n\
";
const char* ISOprologue2 = "\
/reencodeISO{\n\
  dup length dict begin\n\
    {1 index /FID ne {def}{pop pop} ifelse} forall\n\
    /Encoding ISOLatin1Encoding def\n\
    currentdict\n\
  end\n\
}def\n\
/findISO{\n\
  dup /FontType known{\n\
    dup /FontType get 3 ne\n\
    1 index /CharStrings known{\n\
      1 index /CharStrings get /Thorn known\n\
    }{false}ifelse\n\
    and\n\
  }{false}ifelse\n\
}def\n\
";
const char* normalISOFont = "\
/N{/%s findfont\
  findISO{reencodeISO /%s-ISO exch definefont}if\
  %d UP scalefont setfont\
}def\n\
";
const char* normalFont = "\
/N{/%s findfont %d UP scalefont setfont}def\n\
";

const char* defPrologue = "\
/B{gsave}def\n\
/LN{show}def\n\
/EL{grestore 0 -%d rmoveto}def\n\
/M{0 rmoveto}def\n\
/O{gsave show grestore}def\n\
/LandScape{90 rotate 0 -%ld translate}def\n\
/U{%d mul}def\n\
/UP{U 72 div}def\n\
/S{show grestore 0 -%d rmoveto}def\n\
";

const char* headerPrologue = "\
/InitGaudyHeaders{\n\
  /TwoColumn exch def /HeaderY exch def /BarLength exch def\n\
  /ftD /Times-Bold findfont 12 UP scalefont def\n\
  /ftF /Times-Roman findfont 14 UP scalefont def\n\
  /ftP /Helvetica-Bold findfont 30 UP scalefont def\n\
  /fillbox{ % w h x y => -\n\
    moveto 1 index 0 rlineto 0 exch rlineto neg 0 rlineto\n\
    closepath fill\n\
  }def\n\
  /LB{ % x y w h (label) font labelColor boxColor labelPtSize => -\n\
    gsave\n\
    /pts exch UP def /charcolor exch def /boxcolor exch def\n\
    /font exch def /label exch def\n\
    /h exch def /w exch def\n\
    /y exch def /x exch def\n\
    boxcolor setgray w h x y fillbox\n\
    /lines label length def\n\
    /ly y h add h lines pts mul sub 2 div sub pts .85 mul sub def\n\
    font setfont charcolor setgray\n\
    label {\n\
      dup stringwidth pop\n\
      2 div x w 2 div add exch sub ly moveto\n\
      show\n\
      /ly ly pts sub def\n\
    } forall\n\
    grestore\n\
  }def\n\
  /Header{ % (file) [(date)] (page) => -\n\
    /Page exch def /Date exch def /File exch def\n\
    .25 U HeaderY U BarLength .1 sub U .25 U [File] ftF .97 0 14 LB\n\
    .25 U HeaderY .25 add U BarLength .1 sub U .25 U [()] ftF 1 0 14 LB\n\
    .25 U HeaderY U 1 U .5 U Date ftD .7 0 12 LB\n\
    BarLength .75 sub U HeaderY U 1 U .5 U [Page] ftP .7 1 30 LB\n\
    TwoColumn{\n\
      BarLength 2 div .19 add U HeaderY U moveto 0 -10 U rlineto stroke\n\
    }if\n\
  }def\n\
}def\n\
/InitNormalHeaders{\n\
  /TwoColumn exch def /HeaderY exch def /BarLength exch def\n\
  /ftF /Times-Roman findfont 14 UP scalefont def\n\
  /ftP /Helvetica-Bold findfont 14 UP scalefont def\n\
  /LB{ % x y w h (label) font labelColor labelPtSize => -\n\
    gsave\n\
    /pts exch UP def /charcolor exch def\n\
    /font exch def /label exch def\n\
    /h exch def /w exch def\n\
    /y exch def /x exch def\n\
    /ly y h add h pts sub 2 div sub pts .85 mul sub def\n\
    font setfont charcolor setgray\n\
    label stringwidth pop 2 div x w 2 div add exch sub ly moveto\n\
    label show\n\
    grestore\n\
  }def\n\
  /Header{ % (file) [(date)] (page) => -\n\
    /Page exch def pop /File exch def\n\
    .25 U HeaderY U BarLength 2 div U .5 U File ftF 0 14 LB\n\
    BarLength .75 sub U HeaderY U 1 U .5 U Page ftP 0 14 LB\n\
    TwoColumn{\n\
      BarLength 2 div .19 add U HeaderY U moveto 0 -10 U rlineto stroke\n\
    }if\n\
  }def\n\
}def\n\
/InitNullHeaders{/Header{3{pop}repeat}def Header}def\n\
";

/*
 * Emit the DSC header comments and prologue.
 */
static void
emitPrologue(FILE* fd, int argc, char* argv[])
{
    fputs("%!PS-Adobe-3.0\n", fd);
    fprintf(fd, "%%%%Creator: %s\n", prog);
    fputs("%%Title:", fd);			/* command line */
    for (; argc > 0; argc--, argv++)
	fprintf(fd, " %s", argv[0]);
    putc('\n', fd);
    time_t t = time(0);
    fprintf(fd, "%%%%CreationDate: %s", ctime(&t));
    char* cp = cuserid(NULL);
    fprintf(fd, "%%%%For: %s\n", cp ? cp : "");
    fputs("%%Origin: 0 0\n", fd);
    fprintf(fd, "%%%%BoundingBox: 0 0 %.0f %.0f\n",
	physPageHeight*72, physPageWidth*72);
    fputs("%%Pages: (atend)\n", fd);
    fprintf(fd, "%%%%PageOrder: %s\n", reverse ? "Descend" : "Ascend");
    fprintf(fd, "%%%%Orientation: %s\n", landscape ? "Landscape" : "Portrait");
    fprintf(fd, "%%%%DocumentNeededResources: font %s\n", font_name);
    if (gaudy) {
	fputs("%%+ font Times-Bold\n", fd);
	fputs("%%+ font Times-Roman\n", fd);
	fputs("%%+ font Helvetica-Bold\n", fd);
    }
    fprintf(fd, "%%%%EndComments\n");

    fprintf(fd, "%%%%BeginProlog\n");
    fputs("/$printdict 50 dict def $printdict begin\n", fd);
    if (useISO8859) {
	fputs(ISOprologue1, fd);
	fputs(ISOprologue2, fd);
    }
    fprintf(fd, defPrologue, lineHeight, pageHeight, LUNIT, lineHeight);
    if (useISO8859)
	fprintf(tf, normalISOFont,
	    font_name, font_name, pointSize/20, pointSize/20); 
    else
	fprintf(tf, normalFont, font_name, pointSize/20);
    fputs(headerPrologue, tf);
    fprintf(tf, "%.2f %.2f %s Init%sHeaders\n"
	, CVTI(pageWidth - (lm+rm))
	, CVTI(pageHeight - tm)
	, (numcol > 1 ? "true" : "false")
	, (gaudy ? "Gaudy" : headers ? "Normal" : "Null")
    );
    fputs("end\n", fd);
    fputs("%%EndProlog\n", fd);
}

/*
 * Emit the DSC trailer comments.
 */
static void
emitTrailer(FILE* fd)
{
    fputs("%%Trailer\n", fd);
    fprintf(fd, "%%%%Pages: %d\n", pageNum-firstPageNum);
    printf("%%%%EOF\n");
}

static void
newPage()
{
    x = lm;					// x starts at left margin
    right_x = col_width - col_margin/2;		// right x, 0-relative
    y = pageHeight - tm - lineHeight;		// y at page top
    level = 0;					// string paren level reset
    column = 1;
    boc = TRUE;
    bop = TRUE;
}

static void
newCol()
{
    x += col_width;				// x, shifted
    right_x += col_width;			// right x, shifted
    y = pageHeight - tm - lineHeight;		// y at page top
    level = 0;
    column++;
    boc = TRUE;
}

static void
beginCol()
{
    if (column == 1) {				// new page
	if (reverse)  {
	    int k = pageNum-firstPageNum;
	    size_t off = (size_t) ftell(tf);
	    if (k < pageOff->length())
		(*pageOff)[k] = off;
	    else
		pageOff->append(off);
	}
	fprintf(tf,"%%%%Page: \"%d\" %d\n",pageNum-firstPageNum+1,pageNum);
	fputs("save $printdict begin\n", tf);
	fprintf(tf, ".05 dup scale N\n");
	if (landscape)
	    fputs("LandScape\n", tf);
	fprintf(tf, "(%s)[(%s)(%s)](%d)Header\n",
	    curFile, modDate, modTime, pageNum);
    }
    fprintf(tf, "%ld %ld moveto\n",x,y);
}

static void
beginLine()
{
    if (boc)
	beginCol(), boc = FALSE, bop = FALSE;
    fputs("B", tf);
}

static void
beginText()
{
    putc('(', tf);
    level++;
}

static void
closeStrings(const char* cmd)
{
    int l = level;
    for (; level > 0; level--)
	putc(')', tf);
    if (l > 0)
	fputs(cmd, tf);
}

static void
doFile(FILE* fp)
{
    newPage();				// each file starts on a new page

    fxBool bol = TRUE;			// force line start
    fxBool bot = TRUE;			// force text start
    int c;
    if ((c = getc(fp)) != '\f')	// discard initial form feeds
	ungetc(c, fp);
    Coord xoff = col_width * (column-1);
    while ((c = getc(fp)) != EOF) {
	switch (c) {
	case '\0':			// discard nulls
	    break;
	case '\f':			// form feed
	    endTextCol();
	    bol = bot = TRUE;
	    break;
	case '\n':			// line break
	    if (bol)
		beginLine();
	    if (bot)
		beginText();
	    endTextLine();
	    bol = bot = TRUE;
	    xoff = col_width * (column-1);
	    break;
	case '\r':			// check for overstriking
	    if ((c = getc(fp)) == '\n') {
		ungetc(c,fp);		// collapse \r\n => \n
		break;
	    }
	    closeStrings("O\n");	// do overstriking
	    bot = TRUE;			// start new string
	    break;
	default:
	    Coord hm;
	    if (c == '\t' || c == ' ') {
		/*
		 * Coalesce white space into one relative motion.
		 * The offset calculation below is to insure that
		 * 0 means that start of the line (no matter what
		 * column we're in).
		 */
		hm = 0;
		int cc = c;
		Coord off = xoff - col_width*(column-1);
		do {
		    if (cc == '\t')
			hm += tabWidth - (off+hm) % tabWidth;
		    else
			hm += CharWidth[' '];
		} while ((cc = getc(fp)) == '\t' || cc == ' ');
		if (cc != EOF)
		    ungetc(cc, fp);
		/*
		 * If the motion is one space's worth, either
		 * a single blank or a tab that causes a blank's
		 * worth of horizontal motion, then force it
		 * to be treated as a blank below.
		 */
		if (hm == CharWidth[' '])
		    c = ' ';
		else
		    c = '\t';
	    } else
		hm = CharWidth[c];
	    if (xoff + hm > right_x) {
		if (!wrapLines)		// discard line overflow
		    break;
		if (c == '\t')		// adjust white space motion
		    hm -= right_x - xoff;
		endTextLine();
		bol = bot = TRUE;
		xoff = col_width * (column-1);
	    }
	    if (bol)
		beginLine(), bol = FALSE;
	    if (c == '\t') {		// close open PS string and do motion
		if (hm > 0) {
		    closeStrings("LN");
		    fprintf(tf, " %ld M ", hm);
		    bot = TRUE;		// force new string
		}
	    } else {			// append to open PS string
		if (bot)
		    beginText(), bot = FALSE;
		if (040 <= c && c <= 0176) {
		    if (c == '(' || c == ')' || c == '\\')
			putc('\\',tf);
		    putc(c,tf);
		} else
		    fprintf(tf, "\\%03o", c);
	    }
	    xoff += hm;
	    break;
	}
    }
    if (!bol)
	endLine();
    if (!bop) {
	column = numcol;			// force page end action
	endTextCol();
    }
    if (reverse) {
	/*
	 * Normally, beginCol sets the pageOff entry.  Since this is
	 * the last output for this file, we must set it here manually.
	 * If more files are printed, this entry will be overwritten (with
	 * the same value) at the next call to beginCol.
	 */
	size_t off = (size_t) ftell(tf);
	pageOff->append(off);
    }
}

const char* outlineCol = "\n\
gsave\
    %ld setlinewidth\
    newpath %ld %ld moveto\
    %ld %ld rlineto\
    %ld %ld rlineto\
    %ld %ld rlineto\
    closepath stroke \
grestore\n\
";

static void
endCol()
{
    if (outline > 0) {
	fprintf(tf, outlineCol, outline,
	    x - col_margin, bm,		col_width, 0, 0,
	    pageHeight-bm-tm,		-col_width, 0);
    }
    if (column == numcol) {		// columns filled, start new page
	pageNum++;
	fputs("showpage\nend restore\n", tf);
	fflush(tf);
	if (ferror(tf) && errno == ENOSPC)
	    fatal("Output error -- disk storage probably full");
	newPage();
    } else
	newCol();
}

static void
endLine()
{
    fputs("EL\n", tf);
    if ((y -= lineHeight) < bm)
	endCol();
}

static void
endTextCol()
{
    closeStrings("LN");
    putc('\n', tf);
    endCol();
}

static void
endTextLine()
{
    closeStrings("S\n");
    if ((y -= lineHeight) < bm)
	endCol();
}

/*
 * Convert a value of the form:
 * 	#.##bp		big point (1in = 72bp)
 * 	#.##cc		cicero (1cc = 12dd)
 * 	#.##cm		centimeter
 * 	#.##dd		didot point (1157dd = 1238pt)
 * 	#.##in		inch
 * 	#.##mm		millimeter (10mm = 1cm)
 * 	#.##pc		pica (1pc = 12pt)
 * 	#.##pt		point (72.27pt = 1in)
 * 	#.##sp		scaled point (65536sp = 1pt)
 * to inches, returning it as the function value.  The case of
 * the dimension name is ignored.  No space is permitted between
 * the number and the dimension.
 */
static Coord
inch(const char* s)
{
    char* cp;
    double v = strtod(s, &cp);
    if (cp == NULL) {
	fprintf(stderr,
	    "%s: Unable to convert \"%s\" to a floating point number.\n",
	    prog, s);
	exit(-1);
    }
    if (strncasecmp(cp,"in",2) == 0)		// inches
	;
    else if (strncasecmp(cp,"cm",2) == 0)	// centimeters
	v /= 2.54;
    else if (strncasecmp(cp,"pt",2) == 0)	// points
	v /= 72.27;
    else if (strncasecmp(cp,"cc",2) == 0)	// cicero
	v *= 12.0 * (1238.0 / 1157.0) / 72.27;
    else if (strncasecmp(cp,"dd",2) == 0)	// didot points
	v *= (1238.0 / 1157.0) / 72.27;
    else if (strncasecmp(cp,"mm",2) == 0)	// millimeters
	v /= 25.4;
    else if (strncasecmp(cp,"pc",2) == 0)	// pica
	v *= 12.0 / 72.27;
    else if (strncasecmp(cp,"sp",2) == 0)	// scaled points
	v /= (65536.0 * 72.27);
    else					// big points
	v /= 72.0;
    return ICVT(v);
}

static FILE*
OpenAFMFile(char* font_path)
{
    FILE* fd;
    sprintf(font_path, "%s/%s.afm", FONTDIR, font_name);
    fd = fopen(font_path, "r");
    if (fd != NULL || errno != ENOENT)
	return (fd);
    font_path[strlen(font_path)-4] = '\0';
    return (fopen(font_path, "r"));
}

static fxBool
getAFMLine(FILE* fp, char* buf, int bsize)
{
    if (fgets(buf, bsize, fp) == NULL)
	return (FALSE);
    char* cp = strchr(buf, '\n');
    if (cp == NULL) {			// line too long, skip it
	int c;
	while ((c = getc(fp)) != '\n')	// skip to end of line
	    if (c == EOF)
		return (FALSE);
	cp = buf;			// force line to be skipped
    }
    *cp = '\0';
    return (TRUE);
}

static void
readMetrics()
{
    char font_path[1024];
    FILE* fp = OpenAFMFile(font_path);
    if (fp == NULL) {
	fprintf(stderr, "%s: can't open font metrics file %s\n",
	    prog,font_path);
	for (int i = 0; i < NCHARS; i++)
	    CharWidth[i] = 600*pointSize/1000L;
	return;
    }
    for (int i = 0; i < NCHARS; i++)
	CharWidth[i] = 0;

    char buf[1024];
    u_int line = 0;
    do {
	if (!getAFMLine(fp, buf, sizeof (buf))) {
	    fclose(fp);
	    return;			// XXX
	}
	line++;
    } while (strncmp(buf, "StartCharMetrics", 16));
    while (getAFMLine(fp, buf, sizeof (buf)) && strcmp(buf, "EndCharMetrics")) {
	line++;
	int ix, w;
	/* read the glyph position and width */
	if (sscanf(buf, "C %d ; WX %d ;", &ix, &w) == 2) {
	    if (ix == -1)		// end of unencoded glyphs
		break;
	    if (ix < NCHARS)
		CharWidth[ix] = w*pointSize/1000L;
	} else
	    fprintf(stderr, "%s, line %u: format error", font_path, line);
    }
    fclose(fp);
}

static void
setDefaultFont()
{
    font_name = "Courier";
}

static fxBool
findFont(const char* name)
{
    fxBool ok = FALSE;
    DIR* dir = opendir(FONTDIR);
    if (dir) {
	int len = strlen(name);
	dirent* dp;
	while ((dp = readdir(dir)) != NULL) {
	    int l = strlen(dp->d_name);		// XXX should d_namlen
	    if (l < len)
		continue;
	    if (strcasecmp(name, dp->d_name) == 0) {
		ok = TRUE;
		break;
	    }
	    // check for .afm suffix
	    if (l-4 != len || strcmp(&dp->d_name[len], ".afm"))
		continue;
	    if (strncasecmp(name, dp->d_name, len) == 0) {
		ok = TRUE;
		break;
	    }
	}
	closedir(dir);
    }
    return (ok);
}

static void
usage()
{
    fprintf(stderr, "Usage: %s"
	" [-1]"
	" [-2]"
	" [-B]"
	" [-c]"
	" [-D]"
	" [-f fontname]"
	" [-m N]"
	" [-o #]"
	" [-p #]"
	" [-r]"
	" [-U]"
	" [-Ml=#,r=#,t=#,b=#]"
	" [-V #]"
	" files... >out.ps\n", prog);
    fprintf(stderr,"Default options:"
	" -f Courier"
	" -1"
	" -p 11bp"
	" -o 0"
	"\n");
    exit(1);
}

static void
fatal(const char* fmt ...)
{
    fputs("\n?", stderr);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    perror("\n?perror() says");
    exit(1);
}
