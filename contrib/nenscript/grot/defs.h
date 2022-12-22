/*
 *  Default definitions for nenscript
 *
 *   This code was written by Craig Southeren whilst under contract
 *   to Computer Sciences of Australia, Systems Engineering Division.
 *   It has been kindly released by CSA into the public domain.
 *
 *   Neither CSA or me guarantee that this source code is fit for anything,
 *   so use it at your peril. I don't even work for CSA any more, so
 *   don't bother them about it. If you have any suggestions or comments
 *   (or money, cheques, free trips =8^) !!!!! ) please contact me
 *   care of geoffw@extro.ucc.oz.au
 *

 */

/* define page size in 72 dpi units */
#ifdef A4_PAPER
#define	PAGE_HEIGHT		842L		/* 297 mm */
#define PAGE_WIDTH		595L		/* 210 mm */
#endif
#ifdef US_PAPER
#define PAGE_HEIGHT		792L		/* 11 inches */
#define PAGE_WIDTH		612L		/* 8.5 inches */
#endif

/* define margins around printable area */
#ifndef SILLY_PAGE

#define	PAGE_LEFT_MARGIN	25L
#define	PAGE_RIGHT_MARGIN	25L
#define	PAGE_TOP_MARGIN		25L
#define	PAGE_BOT_MARGIN		36L

#define PAGE_LANDSCAPE_XOFFS	0L		/* origin X translate when in landscape mode */
#define PAGE_LANDSCAPE_YOFFS	0L		/* origin Y translate when in landscape mode */

#else

#define	PAGE_LEFT_MARGIN	25L			/* margin on left size of page */
#define	PAGE_RIGHT_MARGIN	25L			/* margin on right side of page */
#define	PAGE_TOP_MARGIN		75L		    /* margin at top of page */
#define	PAGE_BOT_MARGIN		36L			/* margin at bottom of page */

#define PAGE_LANDSCAPE_XOFFS	0L		/* origin X translate when in landscape mode */
#define PAGE_LANDSCAPE_YOFFS	0L		/* origin Y translate when in landscape mode */

#endif

#define	COLUMN_SEP			36L

/* default fonts */
#define	TITLEFONT		"Courier-Bold10";
#define	NORMALFONT		"Courier10"
#define	LANDSCAPE2COLFONT	"Courier7"

/* define the scaling factor used */
#define	SCALE			100L

/* define dimensions of gaudy mode artifacts */
#define	BOX_WIDTH		72L	/* width of box for date/page */
#define	BOX_HEIGHT		36L	/* height of box for date/page */
#define	BAR_HEIGHT		18L	/* height of bar across top of page */

/* "colour" of gaudy mode artifacts */
#define	BOXGRAY			"0.7"
#define	BARGRAY			"0.95"

/* scaled versions of the dimensions above */
#define	BW			(BOX_WIDTH * SCALE)
#define	BH			(BOX_HEIGHT * SCALE)
#define	BS			(BAR_HEIGHT * SCALE)

/* fonts used for gaudy mode */
#define	GAUDYPNFONT		"Helvetica-Bold36"
#define	GAUDYDATEFONT		"Times-Bold12"
#define	GAUDYTITLEFONT		"Times-Roman14"
#define	CLASSIFICATIONFONT	"Helvetica-Bold28"

/* default printer */
#ifdef MSDOS
#define DEF_PRINTER "prn"
#else
#define	DEF_PRINTER	"QMS2200"		/* default printer */
#endif
