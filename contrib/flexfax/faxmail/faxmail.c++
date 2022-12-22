/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxmail/faxmail.c++,v 1.1.1.1 1994/01/14 23:09:50 torek Exp $
/*
 * Copyright (c) 1990, 1991, 1992, 1993 Sam Leffler
 * Copyright (c) 1991, 1992, 1993 Silicon Graphics, Inc.
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
#include "Str.h"
#include "StackBuffer.h"
#include "PageSize.h"

#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#include <pwd.h>
#include <osfcn.h>
#include <sys/fcntl.h>

extern void fxFatal(const char* fmt, ...);

static	const NCHARS = 256;		// chars per font

struct Font {
    fxStr	name;			// font name
    fxStr	showproc;		// PostScript show operator
    float	widths[NCHARS];		// width table

    static const fxStr fontDir;

    Font();

    void show(FILE*, const char*, int len);
    void show(FILE*, const fxStr&);
    float strwidth(const char*);

    void setDefaultMetrics(float pointsize);
    FILE* openFile(fxStr& pathname);
    fxBool readMetrics(float pointsize);
    fxBool getAFMLine(FILE* fp, char* buf, int bsize);
};
const fxStr Font::fontDir = FONTDIR;

class faxMailApp {
private:
    fxStr	appName;		// for error messages
    long	pagefactor;		// n-up factor
    int		row, col;		// row+column for n-up
    int		pageno;			// page number

    fxBool	lastHeaderShown;	// header is shown
    fxBool	doBodyFont;		// look for body font in mail headers
    fxBool	wrapLines;		// wrap/truncate long lines
    fxBool	startNewPage;		// emit new page prologue

    char	buffer[2048];		// input buffer (mail)
    fxStr	output;			// output buffer (postscript)

    float	pageWidth, pageHeight;	// physical page dimensions
    float	pagewidth, pageheight;	// imaged page dimensions
    float	xMargin, yMargin;	// horizontal+vertical margins
    float	ymax, ymin;		// max/min vertical spots for text
    float	xmax;			// max horizontal spot for text
    float	xpos, ypos;		// current x-y spot on logical page
    float	vh;			// vertical height of a line
    float	tabWidth;		// width in points of tab
    float	headerStop;		// tab stop for headers

    float	pointsize;		// font size
    Font	italic;			// italic font info
    Font	bold;			// bold font info
    Font	body;			// body font info
    fxStr	defBodyFont;		// fallback font to use for body
    fxStr	bodyFont;		// body font name
    fxStr	bodyFontFile;		// file holding body font

    static const char* headers[];

    void formatMail(FILE*);
    void flushHeaderLine(FILE*, const fxStr&);
    void calculateHeaderStop();
    fxBool okToShowHeader(const char* tag);

    void defineFont(FILE* fd, const char* psname, const char* fname);
    void startLogicalPage(FILE*);
    void endLogicalPage(FILE*);
    void tab(FILE*);
    void hmove(FILE*, float pos);
    void lineBreak(FILE*);
    void flushOutput(FILE*);
    void showWrapItalic(FILE* fd, const char* cp);
    void setupPageSize(const char* name);

    void lookForBodyFont(const char* fromLine);
    fxBool loadBodyFont(FILE*, const char* fileName, const char* fontName);

    void usage();
    void printError(const char* va_alist ...);
    void fontWarn(const fxStr& name);
public:
    faxMailApp();
    ~faxMailApp();

    void initialize(int argc, char** argv);
    void open();
};

const char* faxMailApp::headers[] = {
    "To",
    "Subject",
    "From",
    "Date",
    "Cc",
    "Summary",
    "Keywords",
};
#define	NHEADERS \
    (sizeof (faxMailApp::headers) / sizeof (faxMailApp::headers[0]))

faxMailApp::faxMailApp()
{
    bold.name = "Helvetica-Bold";
    italic.name = "Helvetica-Oblique";
    defBodyFont = "Courier-Bold";
    body.name = defBodyFont;
    doBodyFont = TRUE;
    wrapLines = TRUE;
    pagefactor = 1;
    pointsize = 12.;			// medium size
    xMargin = .35;			// .35" works ok with 12 point font
    yMargin = .5;			// .5" top+bottom margins
    pageno = 1;				// 1 input page per output page
}

faxMailApp::~faxMailApp()
{
}

void
faxMailApp::initialize(int argc, char** argv)
{
    extern int optind;
    extern char* optarg;
    int c;

    appName = argv[0];
    u_int l = appName.length();
    appName = appName.tokenR(l, '/');

    setupPageSize("default");
    while ((c = getopt(argc, argv, "b:H:i:f:F:p:s:W:x:y:1234nw")) != -1)
	switch (c) {
	case 'b':			// bold font for headers
	    bold.name = optarg;
	    break;
	case 'H':			// page height
	    pageHeight = atof(optarg);
	    break;
	case 'i':			// italic font for headers
	    italic.name = optarg;
	    break;
	case 'f':			// default font for text body
	    bodyFont = optarg;
	    break;
	case 'F':			// special body font
	    bodyFontFile = optarg;
	    doBodyFont = FALSE;
	    break;
	case 'n':			// no special body font
	    doBodyFont = FALSE;
	    break;
	case 'p':			// point size
	    pointsize = atof(optarg);
	    break;
	case 's':			// page size
	    setupPageSize(optarg);
	    break;
	case 'W':			// page width
	    pageWidth = atof(optarg);
	    break;
	case 'x':			// horizontal margins
	    xMargin = atof(optarg);
	    break;
	case 'y':			// vertical margins
	    yMargin = atof(optarg);
	    break;
	case '1': case '2': case '3': case '4':
	    pagefactor = c - '0';
	    break;
	case 'w':
	    wrapLines = FALSE;
	    break;
	case '?':
	    usage();
	    /*NOTREACHED*/
	}

    pagewidth = pageWidth - 2*xMargin;
    pageheight = pageHeight - 2*yMargin;

    // vertical height of a line XXX should use descender info
    vh = (pageHeight+.018*pageHeight)/(pageHeight-.018*pageHeight)*pointsize;
    xmax = 72 * pagewidth;	// max horizontal place
    ymax = 72 * pageheight;	// max vertical location
    ymin = 0;			// lowest imageable point on page

    if (!italic.readMetrics(pointsize))
	fontWarn(italic.name);
    if (!bold.readMetrics(pointsize))
	fontWarn(bold.name);
    calculateHeaderStop();

    if (bodyFontFile != "") {
	u_int l = bodyFontFile.length();
	bodyFont = bodyFontFile.tokenR(l, '/');		// last component
	l = bodyFont.nextR(bodyFont.length(), '.');	// strip any suffix
	if (l > 0)
	    bodyFont.resize(l-1);
    }
}

void
faxMailApp::setupPageSize(const char* name)
{
    PageSizeInfo* info = PageSizeInfo::getPageSizeByName(name);
    if (!info)
	fxFatal("Unknown page size \"%s\"", name);
    pageWidth = info->width() / 25.4;
    pageHeight = info->height() / 25.4;
    delete info;
}

static const char* prolog =
"/inch {72 mul} def\n"
"/pw %.2f inch def\n"
"/ph %.2f inch def\n"
"/nl {"
    "/vpos vpos %.2f sub def\n"
    "0 vpos moveto\n"
"} bind def\n"
"/hm {"
    "vpos moveto\n"
"} bind def\n"
"/pagetop {"
    "/vpos %.2f def\n"
    "0 vpos moveto\n"
"} bind def\n"
"/outline {\n"
    ".1 setlinewidth\n"
    "0.8 setgray\n"
    "gsave\n"
    ".1 inch -.1 inch translate\n"
    "newpath\n"
    "0 0 moveto\n"
    "0 ph lineto\n"
    "pw ph lineto\n"
    "pw 0 lineto\n"
    "closepath\n"
    "fill\n"
    "grestore\n"
    "1.0 setgray\n"
    "newpath\n"
    "0 0 moveto\n"
    "0 ph lineto\n"
    "pw ph lineto\n"
    "pw 0 lineto\n"
    "closepath\n"
    "fill\n"
    "0.0 setgray\n"
    "newpath\n"
    "0 0 moveto\n"
    "0 ph lineto\n"
    "pw ph lineto\n"
    "pw 0 lineto\n"
    "closepath\n"
    "stroke\n"
"} bind def\n"
"/ishow {ifont setfont show} bind def\n"
"/bshow {bfont setfont show} bind def\n"
"/rshow {bodyfont setfont show} bind def\n"
;

void
faxMailApp::open()
{
    printf("%%!PS-Adobe-3.0\n");
    printf("%%%%Creator: faxmail\n");
    printf("%%%%Title: E-Mail\n");
    time_t t = time(0);
    printf("%%%%CreationDate: %s", ctime(&t));
    printf("%%%%Origin: 0 0\n");
    printf("%%%%BoundingBox: 0 0 %.0f %.0f\n", pageHeight*72, pageWidth*72);
    printf("%%%%Pages: (atend)\n");
    printf("%%%%PageOrder: Ascend\n");
    printf("%%%%EndComments\n");
    printf("%%%%BeginProlog\n");
    printf(prolog, pageWidth, pageHeight, vh, ymax);
    printf("%%%%EndProlog\n");
    printf("%%%%BeginSetup\n");
    defineFont(stdout, "ifont", italic.name);	italic.showproc = "ishow";
    defineFont(stdout, "bfont", bold.name);	bold.showproc = "bshow";
    if (bodyFontFile == "" || !loadBodyFont(stdout, bodyFontFile, bodyFont))
	defineFont(stdout, "bodyfont", defBodyFont);
    body.showproc = "rshow";
    printf("%%%%EndSetup\n");
    formatMail(stdout);
    printf("%%%%Trailer\n");
    printf("%%%%Pages: %d\n", pageno);
    printf("%%%%EOF\n");
}

fxBool
faxMailApp::loadBodyFont(FILE* fd, const char* fileName, const char* fontName)
{
    int f = ::open(fileName, O_RDONLY);
    if (f >= 0) {
	char buf[16*1024];
	int n;
	while ((n = read(f, buf, sizeof (buf))) > 0)
	    fwrite(buf, n, 1, fd);
	::close(f);
	defineFont(fd, "bodyfont", fontName);
	return (TRUE);
    } else
	return (FALSE);
}

void
faxMailApp::formatMail(FILE* fd)
{
    startLogicalPage(fd);
    int cc = 0;
    char* bp = buffer;
    int lastc = 0;
    fxStr line;
    for (;;) {
	if (cc <= 0) {
	    cc = fread(buffer, 1, sizeof (buffer), stdin);
	    if (cc == 0)		// EOF
		break;
	    bp = buffer;
	}
	int c = *bp++; cc--;
	if (c == '\n') {
	    if (lastc == '\n')
		break;
	    flushHeaderLine(fd, line);
	    line.resize(0);
	} else
	    line.append(c);
	lastc = c;
    }
    lineBreak(fd);
    if (!body.readMetrics(pointsize))
	fontWarn(body.name);
    tabWidth = 8 * body.widths[' '];
    for (;;) {
	if (cc <= 0) {
	    cc = fread(buffer, 1, sizeof (buffer), stdin);
	    if (cc == 0)		// EOF
		break;
	    bp = buffer;
	}
	int c = *bp++; cc--;
	switch (c) {
	case '\t':
	    tab(fd);
	    break;
	case '\f':
	    endLogicalPage(fd);		// flush current page
	    startLogicalPage(fd);	// ... and being a new one
	    break;
	case '\n':
	    lineBreak(fd);
	    break;
	default:
	    if (c >= NCHARS)		// only accept 7-bit ascii
		break;
	    if (startNewPage)
		startLogicalPage(fd);
	    if (xpos + body.widths[c] > xmax) {
		if (!wrapLines)		// toss character
		    break;
		lineBreak(fd);
	    }
	    xpos += body.widths[c];
	    output.append(c);
	    break;
	}
    }
    endLogicalPage(fd);
}

void
faxMailApp::flushHeaderLine(FILE* fd, const fxStr& line)
{
    /*
     * Collect field name in a canonical format.
     * If the line begins with whitespace, then
     * it's the continuation of a previous header.
     */ 
    if (line.length() > 0 && !isspace(line[0])) { 
	u_int l = 0;
	fxStr field(line.token(l, ':'));
	if (field == "" || l >= line.length())
	    return;
	if (lastHeaderShown = okToShowHeader(field)) {
	    /*
	     * Embolden field name and italicize field value.
	     */
	    fxStr value(line.tail(line.length() - l));
	    bold.show(fd, field | ":");
	    hmove(fd, headerStop);
	    showWrapItalic(fd, value);
	    lineBreak(fd);
	    if (field == "from" && doBodyFont)
		lookForBodyFont(value);
	}
    } else if (lastHeaderShown) {
	hmove(fd, headerStop);			// line up with above header
	showWrapItalic(fd, line);
	lineBreak(fd);
    }
}

void
faxMailApp::showWrapItalic(FILE* fd, const char* cp)
{
    while (isspace(*cp))			// skip white space
	cp++;
    float x = xpos;
    const char* tp = cp;
    for (; *tp != '\0'; tp++) {
	float w = italic.widths[*tp];
	if (x + w > xmax) {
	    italic.show(fd, cp, tp-cp);
	    lineBreak(fd);
	    hmove(fd, headerStop);
	    cp = tp, x = xpos;
	}
	x += w;
    }
    if (tp > cp)
	italic.show(fd, cp, tp-cp);
}

void
faxMailApp::startLogicalPage(FILE* fd)
{
    if (row == 0 && col == 0) {
	fprintf(fd, "%%%%Page: \"%d\" %d\n", pageno, pageno);
	pageno++;
    }
    ypos = ymax;
    xpos = 0;				// translate offsets page
    fprintf(fd, "gsave\n");
    if (pagefactor > 1) {
	fprintf(fd, "%g inch %g inch translate\n",
	    xMargin + col * pagewidth / (float) pagefactor,
	    yMargin + (pagefactor - row - 1) * pageheight / (float) pagefactor);
	fprintf(fd, "%g dup scale\n", 1./pagefactor);
    } else
	fprintf(fd, "%.2f inch %.2f inch translate\n", xMargin, yMargin);
    if (pagefactor > 1) 
	fprintf(fd, "outline\n");
    fprintf(fd, "pagetop\n");		// top of page
    startNewPage = FALSE;
}

void
faxMailApp::endLogicalPage(FILE* fd)
{
    flushOutput(fd);
    fprintf(fd, "grestore showpage\n");
    if (++row == pagefactor) {
	row = 0;
	if (++col == pagefactor)
	    col = 0;
    }
    startNewPage = TRUE;
}

void
faxMailApp::lineBreak(FILE* fd)
{
    flushOutput(fd);
    fprintf(fd, "nl\n");
    ypos -= vh;
    xpos = 0;
    if (ypos < ymin)
	endLogicalPage(fd);
}

void
faxMailApp::tab(FILE* fd)
{
    flushOutput(fd);
    if (xpos)
	xpos += fmod(xpos, tabWidth);
    else
	xpos = tabWidth;
    hmove(fd, xpos);
}

void
faxMailApp::hmove(FILE* fd, float pos)
{
    flushOutput(fd);
    xpos = pos;
    fprintf(fd, "%.2f hm\n", xpos);
}

void
faxMailApp::flushOutput(FILE* fd)
{
    if (startNewPage)
	startLogicalPage(fd);
    if (output.length() > 0) {
	body.show(fd, output, output.length());
	output.resize(0);
    }
}

void
faxMailApp::defineFont(FILE* fd, const char* psname, const char* fname)
{
    fprintf(fd, "/%s /%s findfont %.1f scalefont def\n",
	psname, fname, pointsize);
}

fxBool
faxMailApp::okToShowHeader(const char* tag)
{
    for (u_int i = 0; i < NHEADERS; i++)
	if (strcasecmp(headers[i], tag) == 0)
	    return (TRUE);
    return (FALSE);
}

void
faxMailApp::calculateHeaderStop()
{
    headerStop = 0;
    for (u_int i = 0; i < NHEADERS; i++) {
	float w = bold.strwidth(headers[i]);
	if (w > headerStop)
	    headerStop = w;
    }
    headerStop += bold.widths[':'];
    float boldTab = 8 * bold.widths[' '];
    headerStop += fmod(headerStop, boldTab);
}

void
faxMailApp::lookForBodyFont(const char* sender)
{
    for (const char* cp = sender; isspace(*cp); cp++)
	;
    fxStr name(cp);
    int l = name.next(0, '<');
    if (l != name.length()) {	// Joe Schmo <joe@foobar>
	name.remove(0, l);
	name.resize(name.next(0, '>'));
    } else			// joe@foobar (Joe Schmo)
	name.resize(name.next(0, ' '));
    if (name.length() == 0)
	return;
    // remove @host
    name.resize(name.next(0, '@'));
    // now strip leading host!host!...
    while ((l = name.next(0, '!')) != name.length())
	name.remove(0, l+1);
    // got user account name, look for font in home directory!
    struct passwd* pwd = getpwnam(name);
    if (pwd) {
	bodyFontFile = fxStr(pwd->pw_dir) | "/" | ".faxfont.ps";
	bodyFont = name;
    }
}

void
faxMailApp::usage()
{
    fxFatal("usage: %s"
	" [-p pointsize]"
	" [-1234]"
	, (char*) appName);
}

void
faxMailApp::fontWarn(const fxStr& name)
{
    printError(
	"Warning, can not open metrics for %s, using constant width instead.\n",
	(char*) name);
}

void
faxMailApp::printError(const char* va_alist ...)
#define	fmt va_alist
{
    va_list ap;
    va_start(ap, va_alist);
    fprintf(stderr, "%s: ", (char*) appName);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, ".\n");
}
#undef fmt

Font::Font()
{
    setDefaultMetrics(10.);		// XXX insure something is defined
}

void
Font::show(FILE* fd, const char* val, int len)
{
    if (len > 0) {
	fprintf(fd, "(");
	do {
	    if (*val == '(' || *val == ')' || *val == '\\')
		putc('\\', fd);
	    putc(*val++, fd);
	} while (--len);
	fprintf(fd, ") %s\n", (char*) showproc);
    }
}

void
Font::show(FILE* fd, const fxStr& s)
{
    show(fd, s, s.length());
}

float
Font::strwidth(const char* cp)
{
    float w = 0;
    while (*cp)
	w += widths[*cp++];
    return w;
}

fxBool
Font::getAFMLine(FILE* fp, char* buf, int bsize)
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

FILE*
Font::openFile(fxStr& pathname)
{
    FILE* fd;
    pathname = fontDir | "/" | name | ".afm";
    fd = fopen(pathname, "r");
    if (fd != NULL || errno != ENOENT)
	return (fd);
    pathname.resize(pathname.length()-4);
    return (fopen(pathname, "r"));
}

fxBool
Font::readMetrics(float pointsize)
{
    fxStr file;
    FILE *fp = openFile(file);
    if (fp == NULL) {
	setDefaultMetrics(pointsize);	// NB: use fixed width metrics
	return (FALSE);
    }
    for (int i = 0; i < NCHARS; i++)
	widths[i] = 0;

    char buf[1024];
    u_int line = 0;
    do {
	if (!getAFMLine(fp, buf, sizeof (buf))) {
	    fclose(fp);
	    return (TRUE);			// XXX
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
		widths[ix] = (w/1000.) * pointsize;
	} else
	    fprintf(stderr, "%s, line %u: format error", (char*) file, line);
    }
    fclose(fp);
    return (TRUE);
}

void
Font::setDefaultMetrics(float pointsize)
{
    for (int i = 0; i < NCHARS; i++)
	widths[i] = .625 * pointsize;
}

int
main(int argc, char** argv)
{
    faxMailApp app;
    app.initialize(argc, argv);
    app.open();
    return 0;
}
