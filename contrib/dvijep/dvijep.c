/* -*-C-*- dvijep.c */
/*-->dvijep*/
/**********************************************************************/
/******************************* dvijep *******************************/
/**********************************************************************/

#include "dvihead.h"

/***********************************************************************
***********************************************************************/

/**********************************************************************/
/************************  Device Definitions  ************************/
/**********************************************************************/

/* All output-device-specific definitions go here.  This section must
be changed when modifying a dvi driver for use on a new device */

#undef HPJETPLUS
#define  HPJETPLUS   1			/* conditional compilation flag */

#define VERSION_NO	"2.10"		/* DVI driver version number */

#define  DEVICE_ID	"Hewlett-Packard LaserJet Plus laser printer"
				/* this string is printed at runtime */

#define OUTFILE_EXT	"jep"

#define  BYTE_SIZE	  8		/* output file byte size */

#undef STDRES
#define STDRES  1			/* to get standard font resolution */

#define  MAXLINE	4096		/* maximum input file line size */
					/* it is needed only in special() */

#define  MAXPAGEFONTS	16		/* silly limit on fonts/page */

#define  XDPI		300		/* horizontal dots/inch */
#define  XPSIZE		8		/* horizontal paper size in inches */
long XSIZE;

#define  YDPI		300		/* vertical dots/inch */
#define  YPSIZE		11		/* vertical paper size in inches */
long YSIZE;

#define XORIGIN		30		/* measured pixel coordinates of */
#define YORIGIN		-15		/* page origin (should be at (0,0) */
#define XORIGIN_LANDSCAPE	60	/* Same but for landscape mode */
#define YORIGIN_LANDSCAPE	147	/*   ;;        ;;       ;;     */

/***********************************************************************
Define some  useful  shorthand macros  for  all required  LaserJet  Plus
commands.  With the exception of the CREATEFONT and DOWNLOADCHAR macros,
which require extra data,  NO escape sequences  appear anywhere else  in
the text.

The LaserJet Plus allows coordinates in  dots as well as the  decipoints
supported by the regular LaserJet.  Dot coordinates are more convenient,
so we use them exclusively.  The Plus puts the page origin in the upper-
left corner, while we have it in the lower-left corner, so MOVEY adjusts
the coordinate accordingly.

With 7-bit fonts, only characters 33..127 are printable, and with  8-bit
fonts, 33..127 and 160..255 are printable.  Since TeX assumes characters
0..127 are available, we  remap character numbers  in 0..32 to  160..192
and use 8-bit fonts.  The macros conceal the remapping.

Whenever possible,  escape  sequences  are  combined  and  0  parameters
omitted, in order to compress the output.
***********************************************************************/

/* Begin raster graphics .. {transfer raster graphics} .. end raster graphics */
#define BEGIN_RASTER_GRAPHICS	OUTS("\033*t300R\033*r1A")

/* CREATE_FONT must be followed by a 26-byte font header (see readfont) */
#define CREATE_FONT OUTS("\033\051s26W")

#define DELETE_ALL_FONTS OUTS("\033*cF")

/* DOWNLOAD_CHARACTER is not a complete specification---it must be followed by
two-byte values of xoff, yoff, width, height, deltax */
#define DOWNLOAD_CHARACTER(nbytes) OUTF("\033\050s%dW",(nbytes)+16)

/* End raster graphics */
#define END_RASTER_GRAPHICS	OUTS("\033*rB")

/* Map characters (0..32,33..127) to (160..192,33..127) */
#define MAP_CHAR(c) (((c) > 32) ? (c) : ((c) + 160))

/* Move absolute to (x,y), page origin in lower left corner */
#define MOVETO(x,y) OUTF2("\033*p%dx%dY",(x),YSIZE-(y))

/* Move absolute to (x,current y), page origin in lower left corner */
#define MOVEX(x) OUTF("\033*p%dX",(x))

/* Move absolute to (current x,y), page origin in lower left corner */
#define MOVEY(y) OUTF("\033*p%dY",YSIZE-(y))

/* Output TeX character number with mapping to HP LaserJet Plus number */
#define OUT_CHAR(c) OUTC(MAP_CHAR(c))

/* Output a 16-bit binary number in two bytes */
#define OUT16(n) {OUTC((n)>>8); OUTC((n));}

/* Eject a page from the printer--this is the last command on each page */
#define PAGEEJECT OUTC('\f')

/* Reset printer at job start and end.

<ESC>E		Global reset to clear everything but permanent fonts,
		which  we do not use.
<ESC>&l0E	Reset top margin to 0.
<ESC>&l0F	Reset text length to 0.
<ESC>&l0L	Disable perforation skip.
<ESC>&l0O	Select portrait orientation.  The HPLJ+ printer
		remembers the orientation across global resets (that
		was a surprise), so if landscape printing is being
		done, we must reset to portrait mode to avoid
		incorrect orientation of the next job.
<ESC>&l0P	Reset page length to 0.
<ESC>&a0L	Reset left margin to 0.
<ESC>&a0M	Reset right margin to 0.

The HPLJ PCL allows zero argument values to be omitted, and repeated
sequences <ESC>&<letter><arg><upper-case-letter> to be compressed by
omission of the <ESC>&<letter> on subsequent sequences, provided the
terminal letter is changed to lower case.

Without the margin resets, default margins are supplied which lie
inside page.  Unfortunately, these do not permit negative values, so
we have to further correct the origin with (XORIGIN, YORIGIN) defined
above; these bias (lmargin, tmargin) set in dviinit().  */
#if 0
#define RESET_PRINTER OUTS("\033E\033&lE\033&aL")
#else
#define RESET_PRINTER OUTS("\033&lefloP\033&alM")
#endif

/* Move relative by (delx,dely) from current point */
#define RMOVETO(delx,dely) {\
if ((delx) == 0)\
	RMOVEY(dely)\
else if ((dely) == 0)\
	RMOVEX(delx)\
else\
	OUTF2("\033*p%c%dx%c%dY",\
		((delx) > 0 ? '+' : '-'),ABS(delx),\
		((dely) < 0 ? '+' : '-'),ABS(dely));\
}

/* Move relative to (delx+currentx,currenty), page origin in lower  left
corner */
#define RMOVEX(delx) {\
if ((delx) > 0)\
	OUTF("\033*p+%dX",(delx));\
else if ((delx) < 0)\
	OUTF("\033*p%dX",(delx));\
}

/* Move relative to (currentx,dely+currenty), page origin in lower  left
corner */
#define RMOVEY(dely) {\
if ((dely) > 0)\
	OUTF("\033*p%dY",-(dely));\
else if ((dely) < 0)\
	OUTF("\033*p+%dY",-(dely));\
}

/* Round a POSITIVE floating-point value to the nearest integer */
#define ROUND(x) ((int)(x + 0.5))

/* Output a new rule at TeX position (x,y).  The device coordinates will
be changed on completion.  The rule origin is the TeX convention of  the
lower-left corner, while the LaserJet  Plus uses the upper-left  corner.
*/
#define RULE(x,y,width,height) {\
	MOVETO(x,(y)+height-1);\
	OUTF2("\033*c%da%dbP",width,height);\
}

/* Set rule of  same size as  previous one at  TeX position (x,y).   The
device coordinates will be changed on completion. */
#define RULE2(x,y) {\
	MOVETO(x,(y)+rule_height-1);\
	OUTS("\033*cP");\
}

/* Set the current font and character in preparation for a DOWNLOAD_CHARACTER */
#define SET_CHAR_CODE(fontnumber,ch) {\
if (fontnumber)\
	OUTF2("\033*c%dd%dE",fontnumber,MAP_CHAR(ch));\
else\
	OUTF("\033*cd%dE",MAP_CHAR(ch));\
}

/* Set the number of copies of the current page (issue just before
PAGEEJECT) */
#define SETCOPIES(n) OUTF("\033&l%dX",MAX(1,MIN(n,99)))

/* Set the font number in preparation for a CREATE_FONT */
#define SET_FONT_ID(fontnumber) OUTF("\033*c%dd4F",(fontnumber));

#define TRANSFER_RASTER_GRAPHICS(nbytes) OUTF("\033*b%dW",(nbytes));

#define UNDEFINED_FONT 65535		/* fonts are numbered 0,1,2,... */

/* In order to handle the MAXPAGEFONTS restriction, we keep a set of the
fonts in  use, accessing  them with  these two  functions.  If  a  later
printer model increases MAXFONTS beyond 32, we can transparently  change
to another  representation,  but  for  now,  we  use  an  efficient  bit
set implementation. */

UNSIGN32 page_members;			/* the bit set for current page */
#define ADD_MEMBER(set,member) {set |= (1 << (member));}
#define IS_MEMBER(set,member) (set & (1 << (member)))
#define EMPTY_SET(set) {set = 0L;}

#if    ANSI_PROTOTYPES
static void	loadrotatedchar(register int c,BOOLEAN do_header);
#else
static void	loadrotatedchar();
#endif


#include "main.h"
#include "abortrun.h"
#include "actfact.h"
#include "alldone.h"

/*-->bopact*/
/**********************************************************************/
/******************************* bopact *******************************/
/**********************************************************************/

void
bopact()			/* beginning of page action */
{
    page_fonts = 0;		/* no fonts used on this page yet */
    EMPTY_SET(page_members);

    rule_height = -1;		/* reset last rule parameters */
    rule_width = -1;

    str_ycp = -1;		/* reset string y coordinate */
    if ( landscape )
	OUTS("\033&l1O");	/* select landscape page */
    else
	OUTS("\033&lO");	/* select portrait page */
}

#include "chargf.h"
#include "charpk.h"
#include "charpxl.h"
#include "clrrow.h"
#include "dbgopen.h"

/*-->devinit*/
/**********************************************************************/
/****************************** devinit *******************************/
/**********************************************************************/

void
devinit(argc,argv)		/* initialize device */
int argc;
char *argv[];
{
    RESET_PRINTER;		/* printer reset */
    DELETE_ALL_FONTS;		/* delete all downloaded fonts */
    font_count = 0;		/* no font numbers are assigned yet */
    font_switched = TRUE;
    if ( landscape )
    {
        XSIZE = ((long)XDPI * 11L);
        YSIZE = ((long)YDPI * 8L);
    }
    else
    {
        XSIZE = ((long)XDPI * 8L);
        YSIZE = ((long)YDPI * 11L);
    }
}

/*-->devterm*/
/**********************************************************************/
/****************************** devterm *******************************/
/**********************************************************************/

void
devterm()			/* terminate device */
{
    DELETE_ALL_FONTS;		/* delete all downloaded fonts */
    RESET_PRINTER;		/* printer reset */
}

#include "dvifile.h"
#include "dviinit.h"
#include "dviterm.h"

/*-->eopact*/
/**********************************************************************/
/******************************* eopact *******************************/
/**********************************************************************/

void
eopact()			/* end of page action */
{
    if (copies > 1)
        SETCOPIES(copies);
    PAGEEJECT;
}

#include "f20open.h"
#include "fatal.h"

/*-->fillrect*/
/**********************************************************************/
/****************************** fillrect ******************************/
/**********************************************************************/

void
fillrect(x,y,width,height)
COORDINATE x,y,width,height;		/* lower left corner, size */

/***********************************************************************
With the  page origin  (0,0) at  the lower-left  corner, draw  a  filled
rectangle at (x,y).

For most  TeX  uses, rules  are  uncommon, and  little  optimization  is
possible.  However, for the LaTeX Bezier option, curves are simulated by
many small rules (typically  2 x 2)  separated by positioning  commands.
By remembering  the size  of the  last rule  set, we  can test  for  the
occurrence of repeated rules of the same size, and reduce the output  by
omitting the rule  sizes.  The  last rule  parameters are  reset by  the
begin-page action in prtpage(), so they do not carry across pages.

It is not possible to use relative, instead of absolute, moves in  these
sequences, without stacking rules for the whole page, because each  rule
is separated in  the DVI file  by push, pop,  and positioning  commands,
making  for  an  uncertain  correspondence  between  internal  (xcp,ycp)
pixel page coordinates and external device coordinates.

The last string y coordinate, str_ycp,  must be reset here to force  any
later setstring() to reissue new absolute positioning commands.
***********************************************************************/

{
    str_ycp = -1;		/* invalidate string y coordinate */

    if ((height != rule_height) || (width != rule_width))
    {
	RULE(x,y,width,height);
	rule_width = width;	/* save current rule parameters */
	rule_height = height;
    }
    else			/* same size rule again */
	RULE2(x,y);
}

#include "findpost.h"
#include "fixpos.h"
#include "fontfile.h"
#include "fontsub.h"
#include "getbytes.h"
#include "getfntdf.h"
#include "getpgtab.h"
#include "inch.h"
#include "initglob.h"

/*-->loadbmap*/
/**********************************************************************/
/****************************** loadbmap ******************************/
/**********************************************************************/

void
loadbmap(c)			/* load big character as raster bitmap */
register BYTE c;
{
    register struct char_entry *tcharptr;  /* temporary char_entry pointer */
    void (*charyy)();		/* subterfuge to get around PCC-20 bug */

    if (fontptr != pfontptr)
	(void)openfont(fontptr->n);
    if (fontfp == (FILE *)NULL)	/* do nothing if no font file */
	return;

    tcharptr = &(fontptr->ch[c]);

    moveto(hh - tcharptr->xoffp,YSIZE-vv+tcharptr->yoffp);

    if (landscape)
    {				/* landscape bitmap origin is on top-right */
	MOVETO(xcp + tcharptr->wp,ycp);	/* so adjust horizontal position */
	BEGIN_RASTER_GRAPHICS;
	loadrotatedchar(c,TRUE);
    }
    else
    {
	MOVETO(xcp,ycp);
	BEGIN_RASTER_GRAPHICS;

	/* Bug fix: PCC-20 otherwise jumps to charxx instead of *charxx */
	charyy = fontptr->charxx;
	(void)(*charyy)(c,outraster);	/* output rasters */
    }

    END_RASTER_GRAPHICS;
    str_ycp = -1;		/* invalidate string y coordinate */
}

/*-->loadchar*/
/**********************************************************************/
/****************************** loadchar ******************************/
/**********************************************************************/

void
loadchar(c)
register BYTE c;
{
    UNSIGN16 bytes_per_row;	/* number of raster bytes to copy */
    void (*charyy)();		/* subterfuge to get around PCC-20 bug */
    INT16 k;			/* loop index */
    INT16 m;			/* loop index */
    COORDINATE nbytes;		/* number of bytes per row */
    UNSIGN32 nwords;		/* how many 32-bit words we need */
    INT16 ntemp;
    register struct char_entry *tcharptr; /* temporary char_entry pointer */
    float temp;
    register BYTE the_byte;	/* unpacked raster byte */

    UNSIGN16 words_per_row;	/* number of raster words per row */

    if ((c < FIRSTPXLCHAR) || (LASTPXLCHAR < c)) /* check character range */
	return;

    tcharptr = &(fontptr->ch[c]);

    if (!VISIBLE(tcharptr))
	return;				/* do nothing for invisible fonts */

    if (fontptr != pfontptr)
	openfont(fontptr->n);

    if (fontfp == (FILE *)NULL)		/* do nothing if no font file */
	return;

    (void)clearerr(plotfp);

    tcharptr->isloaded = TRUE;

    SET_CHAR_CODE(fontptr->font_number,c);

    /* Number of bytes in bitmap, possibly reduced because of LaserJet Plus */
    /* limits.  Large characters really need to be handled as bitmaps */
    /* instead of as downloaded characters. */

    if ( landscape )
    {
	nbytes = ((tcharptr->hp) + 7) >> 3;		/* wp div 8 */
	DOWNLOAD_CHARACTER(MIN(16,nbytes)*MIN(255,(tcharptr->wp)));
    }
    else
    {
	nbytes = ((tcharptr->wp) + 7) >> 3;		/* wp div 8 */
	DOWNLOAD_CHARACTER(MIN(16,nbytes)*MIN(255,(tcharptr->hp)));
    }

    OUTC(4);			/* 4 0 14 1 is fixed sequence */
    OUTC(0);

    OUTC(14);
    OUTC(1);

    if ( landscape )
	OUTC(1);		/* orientation = 1 ==> landscape */
    else
	OUTC(0);		/* orientation = 0 ==> portrait */

    OUTC(0);

    /* Apologies for the temporary variables; the Sun OS 3.2 cc could
    not compile the original expressions. */
    ntemp = landscape ? tcharptr->yoffp : tcharptr->xoffp;
    ntemp = MIN(-(ntemp),127);
    ntemp = MAX(-128,ntemp);
    OUT16(ntemp);

    ntemp = landscape ? (tcharptr->wp - tcharptr->xoffp - 1) : tcharptr->yoffp;
    ntemp = MIN(ntemp,127);
    ntemp = MAX(-128,ntemp);
    OUT16(ntemp);

    if (landscape)
	ntemp = MIN(255,tcharptr->hp);
    else
	ntemp = MIN(128,tcharptr->wp);
    OUT16(ntemp);

    if (landscape)
	ntemp = MIN(128,tcharptr->wp);
    else
	ntemp = MIN(255,tcharptr->hp);
    OUT16(ntemp);

    temp = (float)(tcharptr->tfmw);
    temp = 4.0*temp*conv;
    ntemp = ROUND(temp);
    OUT16(ntemp);			/* delta x to nearest 1/4 dot */

    if ( landscape )
	loadrotatedchar(c,FALSE);
    else
    {
	/* Bug fix: PCC-20 otherwise jumps to charxx instead of *charxx */
	charyy = fontptr->charxx;
	(void)(*charyy)(c,outrow);		/* output rasters */
    }

    if (DISKFULL(plotfp))
	(void)fatal("loadchar():  Output error -- disk storage probably full");
}

/*-->loadrotatedchar*/
/**********************************************************************/
/************************** loadrotatedchar ***************************/
/**********************************************************************/

static void
loadrotatedchar(c,do_header)
register int		c;
BOOLEAN			do_header; /* TRUE if TRANSFER_RASTER_GRAPHICS needed */
{
    UNSIGN16 bytes_per_row;	/* number of raster bytes to copy */
    void			(*charyy)();
    INT16 k;			/* loop index */
    INT16 m;			/* loop index */
    UNSIGN32 nwords;		/* how many 32-bit words we need */
    register struct char_entry *tcharptr; /* temporary char_entry pointer */
    register BYTE the_byte;	/* unpacked raster byte */
    UNSIGN16 words_per_row;	/* number of raster words per row */

    tcharptr = &(fontptr->ch[c]);

    nwords = (UNSIGN32)(((tcharptr->wp+31) >> 5) * (tcharptr->hp));
    tcharptr->rasters = (UNSIGN32*)MALLOC((unsigned)(nwords*sizeof(UNSIGN32)));
    if (tcharptr->rasters == (UNSIGN32*)NULL)
    {
	(void)sprintf(message,"loadchar():  Could not allocate %ld words of \
raster space--used %ld words so far",
		  (long)nwords,(long)cache_size);
	(void)fatal(message);
    }
    tcharptr->refcount = 0;		/* clear reference count */
    cache_size += (INT32)nwords;	/* update cache size record */

    /* Bug fix: PCC-20 otherwise jumps to charxx instead of *charxx */
    charyy = fontptr->charxx;
    (void)(*charyy)(c,saverow);		/* fetch rows into tcharptr->rasters */

    words_per_row = (UNSIGN16)(tcharptr->wp + 31) >> 5;
    bytes_per_row = (UNSIGN16)((tcharptr->hp) + 7) >> 3; /* wp div 8 */
    if (!do_header)
	bytes_per_row = MIN(16,bytes_per_row); /* limit chars to 128 bits/row */
    for (k = tcharptr->wp; --k >= 0;)
    {
	if (do_header)
	    TRANSFER_RASTER_GRAPHICS(bytes_per_row);
	the_byte = 0;
	for (m = 0; m < tcharptr->hp; ++m)
	{
	    the_byte <<= 1;
	    the_byte |= ((tcharptr->rasters + m*words_per_row)[k / 32] >>
			 (31 - (k % 32))) & 01;
	    if ((m % 8) == 7)
	    {
		OUTC(the_byte);
		the_byte = 0;
	    }
	}
	if (m % 8)
	{
	    the_byte <<= 8 - (m % 8);
	    OUTC(the_byte);
	}
    }
}

/*-->makefont*/
/**********************************************************************/
/***************************** makefont *******************************/
/**********************************************************************/

void
makefont()
{
    register UNSIGN16 the_char;	/* loop index */
    INT16 j;			/* loop index */
				/* parameters of largest cell */
    COORDINATE cell_above;	/* dots above baseline */
    COORDINATE cell_below;	/* dots below baseline */
    COORDINATE char_below;	/* dots below baseline */
    COORDINATE cell_height;	/* total height */
    COORDINATE cell_left;	/* dots left of character cell */
    COORDINATE cell_width;	/* total width */
    COORDINATE cell_baseline;	/* in 0..(cell_height-1) */

    /*******************************************************************
    Each LaserJet  Plus  font  must  be  assigned  a  unique  number  in
    0..32767, and only 32 fonts  can be active at  one time.  We keep  a
    table of font pointers for this purpose and store the  corresponding
    table index in font_number.  Each time a new font is encountered, we
    increment font_count and  store it in  that font's font_number.  TeX
    produces a unique font number as well, and TeX82 only uses values in
    the range 0..255.  The DVI file supports 32-bit font numbers, so  it
    is better to generate  our own, because  someday other programs  may
    produce DVI files too.

    If fonts are freed dynamically, the table entry must be  invalidated
    and a command sent to the printer to delete that font.  At  present,
    font freeing happens only in dviterm() at the close of processing of
    a single DVI file.  A subsequent  DVI file processed will result  in
    the invocation of devinit() which resets font_count to 0.  It is not
    possible to  avoid  downloading  some fonts  for  subsequent  files,
    because we have no control over what order they are printed in,  and
    each output file must therefore be regarded as independent.

    In order to  deal with  the limit of  MAXFONTS active  fonts in  the
    LaserJet Plus, we  have two choices.   The first and  best and  most
    difficult, is to  entirely discard  the matching of  TeX fonts  with
    LaserJet Plus fonts---new characters would simply be assigned to the
    next available entry in the current open font.  The second choice is
    less satisfactory---when  MAXFONTS fonts  have been  downloaded,  we
    delete the last  of them and  reset the isloaded  flags for all  the
    characters  in  that  font.   The  Plus  also  allows  deletion   of
    individual characters,  so  in  the  first  case,  we  could  delete
    little-used characters  to  make room  in  the font  tables  on  the
    device; we do not use that approach here.  One problem is that  font
    metrics must be assigned  which match the  largest character in  the
    font, so  individual  character deletion  and  later re-use  of  the
    vacated slot is not  in general possible.  A  second problem is  the
    bizarre behavior  of  the  LaserJet  Plus  in  response  to  a  font
    deletion--the current page  is printed, and  the all text  following
    the delete font command will appear on the next page.

    There is a  silly restriction  that only MAXPAGEFONTS  fonts can  be
    used on a  single page,  where MAXPAGEFONTS <  MAXFONTS.  We  handle
    this by keeping a count, page_fonts,  of the number of fonts in  use
    on the  current  page,  and  whenever  a  font  is  requested  whose
    font_number  (0,1,...)   reaches   or  exceeds  MAXPAGEFONTS,   then
    setchar() and setstr()  will not  download the character  as a  font
    character, but instead will set it  as a raster bitmap, in the  same
    fashion that large characters are handled.

    We handle the font deletion misfeature  by just not using it.  If  a
    document requires more than MAXFONTS fonts, then the characters  for
    those additional  fonts  are  sent as  raster  bitmaps,  instead  of
    downloaded characters.

    These two actions completely  remove any limitation  on the size  or
    number of fonts in the LaserJet Plus.

    As an optimization  to reduce  the effort of  font table  searching,
    each time a font is requested which has already been loaded, we move
    that font to the front of font_table[]; the least popular fonts will
    therefore be the  ones that  get deleted.  The  least recently  used
    priority mechanism is  widely used in  operating systems for  buffer
    management; the overhead of  list rearrangement here is  acceptable,
    because font changing  occurs relatively  infrequently, compared  to
    character setting.

	<------wp------>
    ^	................
    |	................
    |	................
    |	................
    |	................
    |	................
    |	................
    hp  ................
    |	................
    |	................
    |	................
    |	.....o..........       <-- character reference point (xcp,ycp) at "o"
    |	................ ^
    |	................ |
    |	................ |--- (hp - yoffp - 1)
    |	................ |
    v	+............... v     <-- (xcorner,ycorner) at "+"
	<--->
	  |
	  |
	 xoffp (negative if (xcp,ycp) left of bitmap)


    The LaserJet Plus  font mechanism requires  the declaration of  cell
    height, width, and baseline in the CreateFont command.   Experiments
    show that  these must  be large  enough to  incorporate the  maximum
    extent above and below the baseline  of any characters in the  font.
    The metric correspondences are as follows:

    -----			-------------
    TeX				LaserJet Plus
    -----			-------------
    xoffp			-(left offset)
    yoffp			top offset
    max(yoffp) + 1		cell baseline
    max(wp)			cell width
    max(yoffp) +
    max(hp-yoff,yoff-hp) + 1	cell height

    *******************************************************************/

    if (font_count >= MAXFONTS)	/* then no more fonts can be downloaded */
    {
	/* set up so BIGCHAR() recognizes the font as not downloadable
	and sends bitmaps instead */
	fontptr->font_number = font_count;
	for (the_char = FIRSTPXLCHAR; the_char <= LASTPXLCHAR; ++the_char)
	{
	    fontptr->ch[the_char].isloaded = FALSE;
	    fontptr->ch[the_char].istoobig = TRUE;
	}
	return;
    }
    for (j = 0; j < (INT16)font_count; ++j)
	if (font_table[j] == fontptr)	/* then font already known */
	{
	    for (; j > 0; --j)		/* LRU: move find to front */
	        font_table[j] = font_table[j-1];
	    font_table[0] = fontptr;
	    return;			/* nothing more to do */
	}

    if (j >= (INT16)font_count)		/* new font */
    {
	fontptr->font_number = font_count;
	font_table[font_count] = fontptr;
	font_count++;			/* count the next font */

	cell_above = 0;
	cell_below = 0;
	cell_left = 0;
	cell_width = 0;

	for (the_char = FIRSTPXLCHAR; the_char <= LASTPXLCHAR; ++the_char)
	{
	    char_below = fontptr->ch[the_char].yoffp -
	        fontptr->ch[the_char].hp;
	    char_below = ABS(char_below);
	    cell_height = 1 + fontptr->ch[the_char].yoffp + char_below;
	    fontptr->ch[the_char].isloaded = FALSE;
	    fontptr->ch[the_char].istoobig = (BOOLEAN)(
	        !IN(-128,-fontptr->ch[the_char].xoffp,127) ||
		!IN(-128, fontptr->ch[the_char].yoffp,127) ||
		!IN(1,fontptr->ch[the_char].wp,255) ||
		!IN(1,cell_height,255));
	    cell_above = MAX(cell_above,fontptr->ch[the_char].yoffp);
	    cell_below = MAX(cell_below,char_below);
	    cell_left = MAX(cell_left,-fontptr->ch[the_char].xoffp);
	    cell_width = MAX(cell_width,fontptr->ch[the_char].wp);
	}
	cell_baseline = cell_above + 1;
	cell_height = cell_above + cell_below + 1;

	if ((cell_width > 255) || (cell_height > 255))
	{    /* don't worry, big characters are handled specially later */
	    cell_height = MIN(cell_height,255);
	    cell_width = MIN(cell_width,255);
	}
	cell_width = MAX(1,cell_width);

	SET_FONT_ID(fontptr->font_number);

	CREATE_FONT;

	OUTC(0);
	OUTC(26);		/* descriptor length */

	OUTC(0);		/* always 0 */
	OUTC(1);		/* 8-bit font (for more than 95 characters) */

	OUT16(0);		/* always 0 */

	OUT16(cell_baseline);	/* baseline position (in 0..cell_height-1)*/

	OUT16(cell_width);	/* cell width */

	OUT16(cell_height);	/* cell height */

	if ( landscape )
	    OUTC(1);		/* orientation = 1 ==> landscape */
	else
	    OUTC(0);		/* orientation = 0 ==> portrait */

	OUTC(1);		/* spacing = 1 ==> proportional */

	OUT16(0);		/* symbol set (arbitrary) */

	OUT16(2);		/* pitch (arbitrary in 2..1260) */

	OUT16(cell_height);	/* font height (arbitrary in 0..10922) */

	OUT16(0);		/* always 0 */

	OUTC(0);		/* always 0 */
	OUTC(0);		/* style = 0 ==> upright */

	OUTC(0);		/* stroke weight = 0 ==> normal */
	OUTC(0);		/* typeface = 0 (arbitrary) */
    }
}

#include "movedown.h"
#include "moveover.h"
#include "moveto.h"

/*-->newfont*/
/**********************************************************************/
/****************************** newfont *******************************/
/**********************************************************************/

void
newfont()
{
    /*******************************************************************
    Creating fonts immediately  (usually at postamble  read time, before
    any pages have been  printed), would exhaust  the LaserJet Plus font
    supply in a  document with more than  32 fonts, requiring many fonts
    to be sent as bitmaps, instead of as downloaded characters.  If only
    a few pages were  selected for  printing,  it  is unlikely that more
    than 32 would   actually  be needed.   With  character bitmaps,  the
    output file is much larger than when downloaded characters are used,
    and there is a likelihood of raising printer error code 21 (page too
    complex to process), at least on the LaserJet Plus, which has rather
    limited internal memory (about 512Kb).  The HPLJ  Series II can have
    up to  4.5Mb  of memory, and possibly   would   eliminate this error
    condition.

    Instead, we simply mark the font as currently undefined.  When it is
    first referenced, setfont() will call  makefont() to actually create
    it in the printer.
    *******************************************************************/

    fontptr->font_number = UNDEFINED_FONT;
}

#include "nosignex.h"
#include "openfont.h"
#include "option.h"

/*-->outraster*/
/**********************************************************************/
/***************************** outraster ******************************/
/**********************************************************************/

void
outraster(c,yoff)/* print single raster line in bitmap graphics mode */
BYTE c;		/* current character value */
UNSIGN16 yoff;	/* offset from top row (0,1,...,hp-1)  */
{
    UNSIGN16 bytes_per_row;	/* number of raster bytes to copy */
    register UNSIGN16 k;	/* loop index */
    register UNSIGN32 *p;	/* pointer into img_row[] */
    struct char_entry *tcharptr;/* temporary char_entry pointer */
    register BYTE the_byte;	/* unpacked raster byte */

    tcharptr = &(fontptr->ch[c]);/* assume check for valid c has been done */
    bytes_per_row = (UNSIGN16)((tcharptr->wp) + 7) >> 3; /* wp div 8 */
    p = img_row;		/* we step pointer p along img_row[] */

    TRANSFER_RASTER_GRAPHICS(bytes_per_row);

    for (k = bytes_per_row; k > 0; ++p)
    {
	the_byte = (BYTE)((*p) >> 24);
	OUTC(the_byte);
	if ((--k) <= 0)
	    break;

	the_byte = (BYTE)((*p) >> 16);
	OUTC(the_byte);
	if ((--k) <= 0)
	    break;

	the_byte = (BYTE)((*p) >> 8);
	OUTC(the_byte);
	if ((--k) <= 0)
	    break;

	the_byte = (BYTE)(*p);
	OUTC(the_byte);
	if ((--k) <= 0)
	    break;
    }
}

/*-->outrow*/
/**********************************************************************/
/******************************* outrow *******************************/
/**********************************************************************/

void
outrow(c,yoff)	/* copy img_row[] into rasters[] if allocated, else no-op */
BYTE c;		/* current character value */
UNSIGN16 yoff;	/* offset from top row (0,1,...,hp-1)  */
{
    UNSIGN16 bytes_per_row;	/* number of raster bytes to copy */
    register UNSIGN16 k;	/* loop index */
    register UNSIGN32 *p;	/* pointer into img_row[] */
    struct char_entry *tcharptr;/* temporary char_entry pointer */
    register BYTE the_byte;	/* unpacked raster byte */

    if (yoff > 255)	/* LaserJet Plus cannot handle big characters */
	return;

    tcharptr = &(fontptr->ch[c]);/* assume check for valid c has been done */
    bytes_per_row = (UNSIGN16)((tcharptr->wp) + 7) >> 3; /* wp div 8 */
    bytes_per_row = MIN(16,bytes_per_row);	/* limited to 128 bits/row */
    p = img_row;		/* we step pointer p along img_row[] */

    for (k = bytes_per_row; k > 0; ++p)
    {
	the_byte = (BYTE)((*p) >> 24);
	OUTC(the_byte);
	if ((--k) <= 0)
	    break;

	the_byte = (BYTE)((*p) >> 16);
	OUTC(the_byte);
	if ((--k) <= 0)
	    break;

	the_byte = (BYTE)((*p) >> 8);
	OUTC(the_byte);
	if ((--k) <= 0)
	    break;

	the_byte = (BYTE)(*p);
	OUTC(the_byte);
	if ((--k) <= 0)
	    break;
    }
}

#include "prtpage.h"
#include "readfont.h"
#include "readgf.h"
#include "readpk.h"
#include "readpost.h"
#include "readpxl.h"
#include "reldfont.h"
#include "rulepxl.h"

/*-->saverow*/
/**********************************************************************/
/****************************** saverow *******************************/
/**********************************************************************/

void
saverow(c,yoff)	/* copy img_row[] into rasters[] if allocated, else no-op */
BYTE c;		/* current character value */
UNSIGN16 yoff;	/* offset from top row (0,1,...,hp-1) */
{
    register UNSIGN16 k;	/* loop index */
    register UNSIGN32 *p;	/* pointer into img_row[] */
    register UNSIGN32 *q;	/* pointer into rasters[] */
    register struct char_entry *tcharptr; /* temporary char_entry pointer */
    UNSIGN16 words_per_row;	/* number of raster words to copy */

    tcharptr = &(fontptr->ch[c]);
    if (tcharptr->rasters != (UNSIGN32*)NULL)
    {
	words_per_row = (UNSIGN16)(tcharptr->wp + 31) >> 5;
	p = tcharptr->rasters + yoff*words_per_row;
	q = img_row;
	for (k = words_per_row; k; --k)	/* copy img_row[] into rasters[] */
	    *p++ = *q++;
    }
}

/*-->setchar*/
/**********************************************************************/
/****************************** setchar *******************************/
/**********************************************************************/

void
setchar(c, update_h)
register BYTE c;
register BOOLEAN update_h;
{
    register struct char_entry *tcharptr;  /* temporary char_entry pointer */

    /* BIGCHAR() and ONPAGE() are used here and in setstr().  BIGCHAR()
    is TRUE if the character is too big to send as a downloaded font, or
    if more than MAXPAGEFONTS have been used on this page, and the
    required font's font_number is not in the page_members set, or if
    more the font is not in the first MAXFONTS fonts. */

#define BIGCHAR(t) (((t->wp > (COORDINATE)size_limit) ||\
(t->hp > (COORDINATE)size_limit)) || (fontptr->font_number >= MAXFONTS) ||\
(t->istoobig) ||\
((page_fonts > MAXPAGEFONTS) && !IS_MEMBER(page_members,fontptr->font_number)))

#define ONPAGE(t) (((hh - t->xoffp + t->pxlw) <= XSIZE) \
    && (hh >= 0)\
    && (vv <= YSIZE)\
    && (vv >= 0))

    if (DBGOPT(DBG_SET_TEXT))
    {
	(void)fprintf(stderr,"setchar('");
	if (isprint(c))
	    (void)putc(c,stderr);
	else
	    (void)fprintf(stderr,"\\%03o",(int)c);
	(void)fprintf(stderr,"'<%d>) (hh,vv) = (%ld,%ld) font name <%s>",
	    (int)c, (long)hh, (long)vv, fontptr->n);
	NEWLINE(stderr);
    }

    tcharptr = &(fontptr->ch[c]);

    moveto(hh,(COORDINATE)(YSIZE-vv));
    if (ONPAGE(tcharptr))
    {				/* character fits entirely on page */
	if (font_switched)
	{
	    (void)setfont();
	    font_switched = FALSE;
	}

	if (VISIBLE(tcharptr))
	{
	    if (BIGCHAR(tcharptr))
		loadbmap(c);
	    else
	    {
		if (!tcharptr->isloaded)
 		    loadchar(c);

		if (ycp != str_ycp)
		{
		    MOVETO(xcp,ycp);
		    str_ycp = ycp;
		}
		else
	            MOVEX(xcp);

		OUT_CHAR(c);
	    }
	}
    }
    else if (DBGOPT(DBG_OFF_PAGE) && !quiet)
    {				/* character is off page -- discard it */
	(void)fprintf(stderr,
	    "setchar(): Char %c [10#%3d 8#%03o 16#%02x] off page.",
	    isprint(c) ? c : '?',c,c,c);
	NEWLINE(stderr);
    }

    if (update_h)
    {
	h += (INT32)tcharptr->tfmw;
	hh += (COORDINATE)tcharptr->pxlw;
	hh = fixpos(hh-lmargin,h,conv) + lmargin;
    }
}

#include "setfntnm.h"

/*-->setfont*/
/**********************************************************************/
/****************************** setfont *******************************/
/**********************************************************************/

void
setfont()
{
    if (fontptr->font_number == UNDEFINED_FONT)
	(void)makefont();
    if (!IS_MEMBER(page_members,fontptr->font_number))
	page_fonts++;
    if (page_fonts <= MAXPAGEFONTS)
	ADD_MEMBER(page_members,fontptr->font_number);
    if (!((fontptr->font_number >= MAXFONTS) || ((page_fonts > MAXPAGEFONTS) &&
	   !IS_MEMBER(page_members,fontptr->font_number))))
    {
	if (fontptr->font_number)
	    OUTF("\033\050%dX",fontptr->font_number);
	else
	    OUTS("\033\050X");
    }
}

#include "setrule.h"

/*-->setstr*/
/**********************************************************************/
/******************************* setstr *******************************/
/**********************************************************************/

void
setstr(c)
register BYTE c;
{
    register struct char_entry *tcharptr;  /* temporary char_entry pointer */
    register BOOLEAN inside;
    INT32 h0,v0;		/* (h,v) at entry */
    COORDINATE hh0,vv0;		/* (hh,vv) at entry */
    COORDINATE hh_last;		/* hh before call to fixpos() */
    register UNSIGN16 k;	/* loop index */
    UNSIGN16 nstr;		/* number of characters in str[] */
    BYTE str[MAXSTR+1];		/* string accumulator */
    BOOLEAN truncated;		/* off-page string truncation flag */

    /*******************************************************************
    Set a sequence of characters in SETC_000 .. SETC_127 with a  minimal
    number of LaserJet Plus commands.  These sequences tend to occur  in
    long clumps  in  a DVI  file,  and setting  them  together  whenever
    possible substantially decreases  the overhead and  the size of  the
    output file.  A sequence can be set as a single string if

	* TeX and LaserJet Plus coordinates of each character agree (may
	  not be true, since device coordinates are in multiples of  1/4
	  pixel; violation  of  this  requirement  can  be  detected  if
	  fixpos() changes hh, or if ycp != str_ycp), AND

	* each character is in the same font (this will always be true
	  in a sequence from a DVI file), AND

	* each character fits within the page boundaries, AND

	* each character definition is already loaded, AND

	* each character is from a visible font, AND

	* each  character bitmap  extent is smaller  than the size_limit
	  (which is  used to  enable discarding  large characters  after
	  each use in order  to conserve virtual  memory storage on  the
	  output device).

    Whenever any of these conditions  does not hold, any string  already
    output is terminated, and a new one begun.

    Two output optimizations are implemented here.  First, up to  MAXSTR
    (in practice more  than enough) characters  are collected in  str[],
    and any  that  require downloading  are  handled.  Then  the  entire
    string is set at once, subject to the above limitations.  Second, by
    recording the vertical page coordinate, ycp, in the global  variable
    str_ycp (reset  in  prtpage()  at begin-page  processing),  it  is
    possible to avoid  outputting y coordinates  unnecessarily, since  a
    single line of  text will  generally result  in many  calls to  this
    function.
    *******************************************************************/

#define BEGINSTRING {inside = TRUE;\
    if (ycp != str_ycp)\
    {\
	MOVETO(xcp,ycp);\
	str_ycp = ycp;\
    }\
    else\
	MOVEX(xcp);}

#define ENDSTRING {inside = FALSE;}

#define OFF_PAGE (-1)	/* off-page coordinate value */

    if (font_switched)	/* output new font selection */
    {
	(void)setfont();
	font_switched = FALSE;
    }

    inside = FALSE;
    truncated = FALSE;

    hh0 = hh;
    vv0 = vv;
    h0 = h;
    v0 = v;
    nstr = 0;
    while ((SETC_000 <= c) && (c <= SETC_127) && (nstr < MAXSTR))
    {			/* loop over character sequence */
	tcharptr = &(fontptr->ch[c]);

	moveto(hh,YSIZE-vv);

	if (ONPAGE(tcharptr) && VISIBLE(tcharptr))
	{		/* character fits entirely on page and is visible */
	    if ((!tcharptr->isloaded) && (!BIGCHAR(tcharptr)))
		    loadchar(c);
	}
	/* update horizontal positions in TFM and pixel units */
	h += (INT32)tcharptr->tfmw;
	hh += (COORDINATE)tcharptr->pxlw;

	str[nstr++] = c;		/* save string character */

	c = (BYTE)nosignex(dvifp,(BYTE)1);
    }

    /* put back character which terminated the loop */
    (void)UNGETC((int)(c),dvifp);

    hh = hh0;			/* restore coordinates at entry */
    vv = vv0;
    h = h0;
    v = v0;

    if (DBGOPT(DBG_SET_TEXT))
    {
	(void)fprintf(stderr,"setstr(\"");
	for (k = 0; k < nstr; ++k)
	{
	    c = str[k];
	    if (isprint(c))
	        (void)putc(c,stderr);
	    else
	        (void)fprintf(stderr,"\\%03o",(int)c);
	}
	(void)fprintf(stderr,"\") (hh,vv) = (%ld,%ld) font name <%s>",
	    (long)hh, (long)vv, fontptr->n);
	NEWLINE(stderr);
    }

    for (k = 0; k < nstr; ++k)
    {			/* loop over character sequence */
	c = str[k];
	tcharptr = &(fontptr->ch[c]);
	moveto(hh,YSIZE-vv);

	if (ONPAGE(tcharptr) && VISIBLE(tcharptr))
	{		/* character fits entirely on page and is visible */
	    /* We must check first for a big character.  The character */
	    /* may have been downloaded on a previous page, but if it is */
	    /* not in the page_members font set, we must send it again */
	    /* as a bitmap instead. */
	    if (BIGCHAR(tcharptr))
	    {
		if (inside)
		    ENDSTRING;		/* finish any open string */
		loadbmap(c);
	    }
	    else			/* character already downloaded */
	    {
		if (!inside)
		    BEGINSTRING;
		OUT_CHAR(c);
	    }
	}
	else		/* character does not fit on page -- output */
	{		/* current string and discard the character */
	    truncated = TRUE;
	    if (inside)
		ENDSTRING;
	}
	/* update horizontal positions in TFM and pixel units */
	h += (INT32)tcharptr->tfmw;
	hh += (COORDINATE)tcharptr->pxlw;
	hh_last = hh;
	hh = fixpos(hh-lmargin,h,conv) + lmargin;
	if (DBGOPT(DBG_POS_CHAR))
	{
	    (void)fprintf(stderr,
		"[%03o] xcp = %d\tycp = %d\thh = %d\thh_last = %d\n",
		c,xcp,ycp,hh,hh_last);
	}

	/* If fixpos() changed position, we need new string next time */
	/* around.   Actually, since the LaserJet Plus stores character */
	/* widths in units of 1/4 dot, we could compute coordinates at */
	/* four times the precision, but for now, we start a new string */
	/* each time we have one or more dots of error. */

	if ((hh != hh_last) && inside)
	    ENDSTRING;
    }
    if (truncated && DBGOPT(DBG_OFF_PAGE) && !quiet)
    {
	(void)fprintf(stderr,"setstr(): Text [");
	for (k = 0; k < nstr; ++k)
	    (void)fprintf(stderr,isprint(str[k]) ? "%c" : "\\%03o",str[k]);
	(void)fprintf(stderr,"] truncated at page boundaries.");
	NEWLINE(stderr);
    }
    if (inside)		/* finish last string */
	ENDSTRING;
}

#include "signex.h"
#include "skgfspec.h"
#include "skipfont.h"
#include "skpkspec.h"
#include "special.h"
#include "strchr.h"
#include "strcm2.h"
#include "strid2.h"
#include "strrchr.h"
#include "tctos.h"
#include "usage.h"
#include "warning.h"
