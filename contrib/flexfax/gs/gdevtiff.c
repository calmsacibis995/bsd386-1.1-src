/* $Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/gs/gdevtiff.c,v 1.1.1.1 1994/01/14 23:09:57 torek Exp $ */

/*
 * Copyright (c) 1992, 1993 Sam Leffler
 * Copyright (c) 1992, 1993 Silicon Graphics, Inc.
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
/* gdevtiff.c */

#include "gdevprn.h"
#include "gdevdfg3.h"
#include "gdevtiff.h"

#ifdef __PROTOTYPES__
#	define PROTO_ARGS(X)	X
#else
#	define PROTO_ARGS(X)	()
#endif

/*
 * TIFF fax output driver.
 */
typedef struct {
    FILE*	fp;
    long	prevdir;	/* file offset of previous directory offset */
    long	diroff;		/* file offset of next write */
    int		bigendian;	/* 1 if machine is big-endian, 0 otherwise */
    unsigned long iwidth;	/* width of image data in pixels */
    int		fax_byte;
    int		fax_weight;
} TIFFOUT;

private void	faxout_open_fp PROTO_ARGS((FILE *, TIFFOUT*));
private int	faxout_begin_page PROTO_ARGS((TIFFOUT*, gx_device_printer*));
private int	faxout_eolcode PROTO_ARGS((TIFFOUT *));
private int	faxout_end_page PROTO_ARGS((TIFFOUT *));
private void	tofax PROTO_ARGS((TIFFOUT*, unsigned char*));

private void putwhitespan();
private void putblackspan();
private void putcode();
private void puteol();
private void putbit();
private void flushbits();

/*
 * Redefine the device descriptor.
 */
struct gx_device_tiff_s {
    gx_device_common;
    gx_prn_device_common;
    TIFFOUT	fax;
};
typedef struct gx_device_tiff_s gx_device_tiff;

/* The device descriptor */
#define X_DPI 204
#define Y_DPI 196
#define LINE_SIZE ((X_DPI * 101 / 10 + 7) / 8)	/* bytes per line */

private dev_proc_open_device(tiff_prn_open);
private dev_proc_print_page(tiff_print_page);
private dev_proc_close_device(tiff_prn_close);

gx_device_procs tiff_std_procs =
    prn_procs(tiff_prn_open, gdev_prn_output_page, tiff_prn_close);

gx_device_tiff far_data gs_tiffg3_device =
{   prn_device_std_body(
	gx_device_tiff,
	tiff_std_procs,
	"tiffg3",
	85,			/* width_10ths, 8.5" */
	110,			/* height_10ths, 11" */
	X_DPI, Y_DPI,
	0,0,0,0,		/* margins */
	1,
	tiff_print_page
    )
};


static struct pageinfo {
    short w, h;			/* page width and height in 10ths */
    unsigned long iw;		/* image width */
} pageinfo[] = {
#define	PAPER_SIZE_LETTER	0
    { 85,  110, 1728 },
#define	PAPER_SIZE_A4		1
    { 85,  116, 1728 },
#define	PAPER_SIZE_B4		2
    { 101,  143, 2048 }
};
#define	NPAGEINFO (sizeof (pageinfo) / sizeof (pageinfo[0]))

/* Get the paper size code, based on width and height. */
static int
papersize(gx_device *dev)
{
    return
      (dev->height / dev->y_pixels_per_inch >= 11.8 ? PAPER_SIZE_B4 :
       dev->height / dev->y_pixels_per_inch >= 11.1 ? PAPER_SIZE_A4 :
       PAPER_SIZE_LETTER);
}

/*
 * Driver entry points.
 */

/*
 * Setup device according to output page.
 */
private int
tiff_prn_open(gx_device *pdev)
{
    struct pageinfo* pi = &pageinfo[papersize(pdev)];
    int	rc;

    pdev->width = (int)((pi->w * pdev->x_pixels_per_inch) / 10);
    pdev->height = (int)((pi->h * pdev->y_pixels_per_inch) / 10);
    rc = gdev_prn_open(pdev);
    if (rc == 0) {
	gx_device_tiff* ddev = (gx_device_tiff*) pdev;
	faxout_open_fp(ddev->file, &ddev->fax);
	ddev->fax.iwidth = pi->iw;
    }
    return (rc);
}

private int
tiff_print_page(gx_device_printer *pdev, FILE *prn_stream)
{
    gx_device_tiff* ddev = (gx_device_tiff*) pdev;
    unsigned char data[LINE_SIZE + 4];
    int	lnum, line_size;
    TIFFOUT* fax = &ddev->fax;

    /* For some odd reason, the file isn't open until now */
    fax->fp = prn_stream;	
    faxout_begin_page(fax, pdev);
    line_size = gdev_mem_bytes_per_scan_line((gx_device*)pdev);
    for (lnum = 0; lnum < pdev->height; lnum++) {
	gdev_prn_copy_scan_lines(pdev, lnum, (byte *)data, line_size);
	tofax(fax, data);
    }
    faxout_end_page(fax);
    return (0);
}

private int
tiff_prn_close(gx_device *pdev)
{
    gx_device_tiff* ddev = (gx_device_tiff*) pdev;
    TIFFOUT* fax = &ddev->fax;

    if (fax->fp)
	fflush(fax->fp);
    return (gdev_prn_close(pdev));
}

/*
 * Internal routines.
 */
private void
faxout_open_fp(FILE *fp, register TIFFOUT* faxp)
{
    faxp->fp = fp;
    faxp->diroff = 0L;
    faxp->prevdir = 0L;
    faxp->bigendian = arch_is_big_endian;
    faxp->fax_byte = 0;
    faxp->fax_weight = 0x01;
}

/* NB: this array is sorted by tag number (assumed below) */
typedef struct {
    TIFFDirEntry	subfiletype;
    TIFFDirEntry	imagewidth;
    TIFFDirEntry	imagelength;
    TIFFDirEntry	bitspersample;
    TIFFDirEntry	compression;
    TIFFDirEntry	photometric;
    TIFFDirEntry	fillorder;
#ifdef notdef
    TIFFDirEntry	documentname;
#endif
    TIFFDirEntry	stripoffsets;
    TIFFDirEntry	orientation;
    TIFFDirEntry	samplesperpixel;
    TIFFDirEntry	rowsperstrip;
    TIFFDirEntry	stripbytecounts;
    TIFFDirEntry	xresolution;
    TIFFDirEntry	yresolution;
    TIFFDirEntry	planarconfig;
    TIFFDirEntry	group3options;
    TIFFDirEntry	resolutionunit;
#ifdef notdef
    TIFFDirEntry	software;
#endif
    TIFFDirEntry	cleanfaxdata;
    unsigned long	diroff;			/* offset to next directory */
    unsigned long	xresValue[2];		/* xresolution indirect value */
    unsigned long	yresValue[2];		/* yresolution indirect value */
} TIFFDirectory;
private TIFFDirectory dirTemplate = {
    { TIFFTAG_SUBFILETYPE,	TIFF_LONG,  1, FILETYPE_PAGE },
    { TIFFTAG_IMAGEWIDTH,	TIFF_LONG,  1 },
    { TIFFTAG_IMAGELENGTH,	TIFF_LONG,  1 },
    { TIFFTAG_BITSPERSAMPLE,	TIFF_SHORT, 1, 1 },
    { TIFFTAG_COMPRESSION,	TIFF_SHORT, 1, COMPRESSION_CCITTFAX3 },
    { TIFFTAG_PHOTOMETRIC,	TIFF_SHORT, 1, PHOTOMETRIC_MINISWHITE },
    { TIFFTAG_FILLORDER,	TIFF_SHORT, 1, FILLORDER_LSB2MSB },
#ifdef notdef
    { TIFFTAG_DOCUMENTNAME,	TIFF_ASCII, 1 },
#endif
    { TIFFTAG_STRIPOFFSETS,	TIFF_LONG,  1 },
    { TIFFTAG_ORIENTATION,	TIFF_SHORT, 1, ORIENTATION_TOPLEFT },
    { TIFFTAG_SAMPLESPERPIXEL,	TIFF_SHORT, 1, 1 },
    { TIFFTAG_ROWSPERSTRIP,	TIFF_LONG,  1, -1L },
    { TIFFTAG_STRIPBYTECOUNTS,	TIFF_LONG,  1, 1 },
    { TIFFTAG_XRESOLUTION,	TIFF_RATIONAL, 1 },
    { TIFFTAG_YRESOLUTION,	TIFF_RATIONAL, 1 },
    { TIFFTAG_PLANARCONFIG,	TIFF_SHORT, 1, PLANARCONFIG_CONTIG },
    { TIFFTAG_GROUP3OPTIONS,	TIFF_LONG,  1 },
    { TIFFTAG_RESOLUTIONUNIT,	TIFF_SHORT, 1, RESUNIT_INCH },
#ifdef notdef
    { TIFFTAG_SOFTWARE,		TIFF_ASCII, 1 },
#endif
    { TIFFTAG_CLEANFAXDATA,	TIFF_SHORT, 1, CLEANFAXDATA_CLEAN },
    0, { 0, 1 }, { 0, 1 },
};
#define	OFFSET(x)	((unsigned)&(((TIFFDirectory*)0)->x))
#define	NTAGS		(OFFSET(diroff) / sizeof (TIFFDirEntry))

/* correct tag values on bigendian machines */
private void
faxout_fixuptags(TIFFDirEntry* dp, int n)
{
    while (n-- > 0) {
	if (dp->tdir_type == TIFF_SHORT || dp->tdir_type == TIFF_SSHORT)
	    dp->tdir_offset <<= 16;
	else if (dp->tdir_type == TIFF_BYTE || dp->tdir_type == TIFF_SBYTE)
	    dp->tdir_offset <<= 24;
	dp++;
    }
}

private int
faxout_begin_page(TIFFOUT *faxp, gx_device_printer* pdev)
{
    gx_device_tiff* ddev = (gx_device_tiff*) pdev;
    short dircount;
    TIFFDirectory dir;

    /*
     * If Ghostscript was invoked with -sOutputFile=file.%d, then
     * it is expected to generate a new file for each page.  This
     * makes no sense for TIFF, but in the interest of compatibility
     * we do the right thing here.
     */
    if (gdev_prn_file_is_new(pdev))
        faxp->diroff = 0;
    /*
     * Writing the header is delayed to here because the
     * FILE* is not setup when faxout_open is called.
     */
    if (faxp->diroff == 0) {
	TIFFHeader h;
	h.tiff_magic = (faxp->bigendian ? TIFF_BIGENDIAN : TIFF_LITTLEENDIAN);
	h.tiff_version = TIFF_VERSION;
	h.tiff_diroff = sizeof (TIFFHeader);
	fwrite((char*) &h, sizeof (h), 1, faxp->fp);
	faxp->diroff = sizeof (TIFFHeader);	/* where next directory goes */
    } else {
	/* patch pointer to this directory */
	fseek(faxp->fp, faxp->prevdir, 0);
	fwrite((char*)&faxp->diroff, sizeof (faxp->diroff), 1, faxp->fp);
    }
    fseek(faxp->fp, faxp->diroff, 0);
    /* write count of tags in directory */
    dircount = NTAGS;
    fwrite((char*)&dircount, sizeof (dircount), 1, faxp->fp);
    faxp->diroff += sizeof (dircount);

    /* fill in directory tags and write them */
    memcpy(&dir, &dirTemplate, sizeof (dirTemplate));
    dir.imagewidth.tdir_offset = pageinfo[papersize((gx_device*) pdev)].iw;
    dir.imagelength.tdir_offset = pdev->height;
    dir.stripoffsets.tdir_offset = faxp->diroff + sizeof (TIFFDirectory);
    dir.xresolution.tdir_offset = faxp->diroff + OFFSET(xresValue);
    dir.yresolution.tdir_offset = faxp->diroff + OFFSET(yresValue);
    dir.group3options.tdir_offset = 0;		/* XXX */
    dir.xresValue[0] = ddev->x_pixels_per_inch;
    dir.yresValue[0] = ddev->y_pixels_per_inch;
    if (faxp->bigendian)
	faxout_fixuptags(&dir.subfiletype, NTAGS);
    fwrite((char*)&dir, sizeof (dir), 1, faxp->fp);

    puteol(faxp);
    return (0);
}

private int
faxout_end_page(TIFFOUT *faxp)
{
    long diroff, cc;

    flushbits(faxp);
    diroff = faxp->diroff;
    faxp->prevdir = faxp->diroff + OFFSET(diroff);
    faxp->diroff = ftell(faxp->fp);
    /* patch strip byte counts value */
    cc = faxp->diroff - (diroff + sizeof (TIFFDirectory));
    fseek(faxp->fp, diroff + OFFSET(stripbytecounts.tdir_offset), 0);
    fwrite(&cc, sizeof (cc), 1, faxp->fp);
    return (0);
}

private const byte far_data b_run_tbl[8][256] = {
  {	/* START BIT 0 */
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 
  },
  {	/* START BIT 1 */
    0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 
    0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 
    0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 
    0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 
    0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 
    0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 
    0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 
    0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 
    0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 
    0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 
    0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 
    0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 
    0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 
    0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 
    0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 
    0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 
  },
  {	/* START BIT 2 */
    0, 0, 0, 0, 1, 1, 2, 3, 0, 0, 0, 0, 1, 1, 2, 3, 
    0, 0, 0, 0, 1, 1, 2, 3, 0, 0, 0, 0, 1, 1, 2, 3, 
    0, 0, 0, 0, 1, 1, 2, 3, 0, 0, 0, 0, 1, 1, 2, 3, 
    0, 0, 0, 0, 1, 1, 2, 3, 0, 0, 0, 0, 1, 1, 2, 3, 
    0, 0, 0, 0, 1, 1, 2, 3, 0, 0, 0, 0, 1, 1, 2, 3, 
    0, 0, 0, 0, 1, 1, 2, 3, 0, 0, 0, 0, 1, 1, 2, 3, 
    0, 0, 0, 0, 1, 1, 2, 3, 0, 0, 0, 0, 1, 1, 2, 3, 
    0, 0, 0, 0, 1, 1, 2, 3, 0, 0, 0, 0, 1, 1, 2, 3, 
    0, 0, 0, 0, 1, 1, 2, 3, 0, 0, 0, 0, 1, 1, 2, 3, 
    0, 0, 0, 0, 1, 1, 2, 3, 0, 0, 0, 0, 1, 1, 2, 3, 
    0, 0, 0, 0, 1, 1, 2, 3, 0, 0, 0, 0, 1, 1, 2, 3, 
    0, 0, 0, 0, 1, 1, 2, 3, 0, 0, 0, 0, 1, 1, 2, 3, 
    0, 0, 0, 0, 1, 1, 2, 3, 0, 0, 0, 0, 1, 1, 2, 3, 
    0, 0, 0, 0, 1, 1, 2, 3, 0, 0, 0, 0, 1, 1, 2, 3, 
    0, 0, 0, 0, 1, 1, 2, 3, 0, 0, 0, 0, 1, 1, 2, 3, 
    0, 0, 0, 0, 1, 1, 2, 3, 0, 0, 0, 0, 1, 1, 2, 3, 
  },
  {	/* START BIT 3 */
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 4, 
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 4, 
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 4, 
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 4, 
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 4, 
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 4, 
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 4, 
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 4, 
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 4, 
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 4, 
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 4, 
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 4, 
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 4, 
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 4, 
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 4, 
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 4, 
  },
  {	/* START BIT 4 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 5, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 5, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 5, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 5, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 5, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 5, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 5, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 5, 
  },
  {	/* START BIT 5 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 5, 6, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 5, 6, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 5, 6, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 5, 6, 
  },
  {	/* START BIT 6 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
    3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 6, 7, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
    3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 6, 7, 
  },
  {	/* START BIT 7 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 
    4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 7, 8, 
  },
};

private const byte far_data w_run_tbl[8][256] = {
  {	/* START BIT 0 */
    1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 
    1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 
    1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 
    1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 
    1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 
    1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 
    1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 
    1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 
    1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 
    1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 
    1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 
    1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 
    1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 
    1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 
    1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 
    1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 
  },
  {	/* START BIT 1 */
    2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 
    2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 
    2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 
    2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 
    2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 
    2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 
    2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 
    2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 
    2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 
    2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 
    2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 
    2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 
    2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 
    2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 
    2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 
    2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 2, 1, 0, 0, 
  },
  {	/* START BIT 2 */
    3, 2, 1, 1, 0, 0, 0, 0, 3, 2, 1, 1, 0, 0, 0, 0, 
    3, 2, 1, 1, 0, 0, 0, 0, 3, 2, 1, 1, 0, 0, 0, 0, 
    3, 2, 1, 1, 0, 0, 0, 0, 3, 2, 1, 1, 0, 0, 0, 0, 
    3, 2, 1, 1, 0, 0, 0, 0, 3, 2, 1, 1, 0, 0, 0, 0, 
    3, 2, 1, 1, 0, 0, 0, 0, 3, 2, 1, 1, 0, 0, 0, 0, 
    3, 2, 1, 1, 0, 0, 0, 0, 3, 2, 1, 1, 0, 0, 0, 0, 
    3, 2, 1, 1, 0, 0, 0, 0, 3, 2, 1, 1, 0, 0, 0, 0, 
    3, 2, 1, 1, 0, 0, 0, 0, 3, 2, 1, 1, 0, 0, 0, 0, 
    3, 2, 1, 1, 0, 0, 0, 0, 3, 2, 1, 1, 0, 0, 0, 0, 
    3, 2, 1, 1, 0, 0, 0, 0, 3, 2, 1, 1, 0, 0, 0, 0, 
    3, 2, 1, 1, 0, 0, 0, 0, 3, 2, 1, 1, 0, 0, 0, 0, 
    3, 2, 1, 1, 0, 0, 0, 0, 3, 2, 1, 1, 0, 0, 0, 0, 
    3, 2, 1, 1, 0, 0, 0, 0, 3, 2, 1, 1, 0, 0, 0, 0, 
    3, 2, 1, 1, 0, 0, 0, 0, 3, 2, 1, 1, 0, 0, 0, 0, 
    3, 2, 1, 1, 0, 0, 0, 0, 3, 2, 1, 1, 0, 0, 0, 0, 
    3, 2, 1, 1, 0, 0, 0, 0, 3, 2, 1, 1, 0, 0, 0, 0, 
  },
  {	/* START BIT 3 */
    4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
    4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
    4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
    4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
    4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
    4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
    4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
    4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
    4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
    4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
    4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
    4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
    4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
    4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
    4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
    4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
  },
  {	/* START BIT 4 */
    5, 4, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    5, 4, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    5, 4, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    5, 4, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    5, 4, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    5, 4, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    5, 4, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    5, 4, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  },
  {	/* START BIT 5 */
    6, 5, 4, 4, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    6, 5, 4, 4, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    6, 5, 4, 4, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    6, 5, 4, 4, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  },
  {	/* START BIT 6 */
    7, 6, 5, 5, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    7, 6, 5, 5, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  },
  {	/* START BIT 7 */
    8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  },
};

/*
 * Macros to use tables in findrun.c
 * Input is *p, bit
 * Output is rl, *p, bit
 */
#define	find_black_run()					\
    for (rl = 0;;) {						\
	if (run = b_run_tbl[bit][*p]) {				\
	    rl += run;						\
	    bit -= run;						\
	    if (bit < 0 && ++p < ep) { bit=7; continue;}	\
	}							\
	break;							\
    }
#define	find_white_run()					\
    for (rl = 0;;) {						\
	if (run = w_run_tbl[bit][*p]) {				\
	    rl += run;						\
	    bit -= run;						\
	    if (bit < 0 && ++p < ep) { bit=7; continue;}	\
	}							\
	break;							\
    }

private void
tofax(TIFFOUT *faxp, unsigned char *p)
{
    unsigned char	*ep;
    register int	bit;
    register int	run;
    register int	rl;

    ep = p + faxp->iwidth/8;
    bit = 7;
    for (;;) {
	find_white_run();
	putwhitespan(faxp, rl);
	if (p >= ep) break;

	find_black_run();
	putblackspan(faxp, rl);
	if (p >= ep) break;
    }
    puteol(faxp);
}


/*************************************************************************
**
** Copyright (C) 1989 by Paul Haeberli <paul@manray.sgi.com>.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
 *************************************************************************/

private void
putwhitespan(register TIFFOUT *faxp, int c)
{
    register int tpos;
    register tableentry* te;

    if(c>=64) {
	tpos = (c/64)-1;
	te = tpos < 27 ? mwtable+tpos : extable+tpos-27;
	c -= te->count;
	putcode(faxp, te);
    }
    tpos = c;
    te = twtable+tpos;
    putcode(faxp, te);
}

private void
putblackspan(register TIFFOUT *faxp, int c)
{
    register int tpos;
    register tableentry* te;

    if(c>=64) {
	tpos = (c/64)-1;
	te = tpos < 27 ? mbtable+tpos : extable+tpos-27;
	c -= te->count;
	putcode(faxp, te);
    }
    tpos = c;
    te = tbtable+tpos;
    putcode(faxp, te);
}

private void
putcode(register TIFFOUT *faxp, tableentry *te)
{
    register unsigned int mask;
    register int code;

    mask = 1<<(te->length-1);
    code = te->code;
    while(mask) {
 	if(code&mask)
	    putbit(faxp, 1);
	else
	    putbit(faxp, 0);
	mask >>= 1;
    }

}

private void
puteol(register TIFFOUT *faxp)
{
    register int i;

    for(i=0; i<11; ++i)
	putbit(faxp, 0);
    putbit(faxp, 1);
}

private void
putbit(register TIFFOUT *faxp, int d)
{
    if(d) 
	faxp->fax_byte = faxp->fax_byte|faxp->fax_weight;
    faxp->fax_weight = faxp->fax_weight<<1;
    if((faxp->fax_weight&0xff) == 0) {
	putc(faxp->fax_byte, faxp->fp);
	faxp->fax_byte = 0;
	faxp->fax_weight = 0x01;
    }
}

private void
flushbits(register TIFFOUT *faxp)
{
    if (faxp->fax_weight != 0x01) {
	putc(faxp->fax_byte, faxp->fp);
	faxp->fax_byte = 0;
	faxp->fax_weight = 0x01;
    }
}
