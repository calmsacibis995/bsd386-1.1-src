/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxinfo/faxinfo.c,v 1.1.1.1 1994/01/14 23:10:57 torek Exp $
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
#include "tiffio.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include "PageSize.h"

static int
isFAXImage(TIFF* tif)
{
    u_short w;
    if (TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &w) && w != 1)
	return (0);
    if (TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &w) && w != 1)
	return (0);
    if (!TIFFGetField(tif, TIFFTAG_COMPRESSION, &w) ||
      (w != COMPRESSION_CCITTFAX3 && w != COMPRESSION_CCITTFAX4))
	return (0);
    if (!TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &w) ||
      (w != PHOTOMETRIC_MINISWHITE && w != PHOTOMETRIC_MINISBLACK))
	return (0);
    return (1);
}

static void
sanitize(char* dst, const char* src, u_int maxlen)
{
    u_int i;
    for (i = 0; i < maxlen-1 && src[i] != '\0'; i++)
	dst[i] = (isascii(src[i]) && isprint(src[i]) ? src[i] : '?');
    dst[i] = '\0';
}

main(int argc, char** argv)
{
    char* cp;
    int npages, ok;
    struct stat sb;
    TIFF* tif;
    float vres, w, h;
    long pageWidth, pageLength;
    char sender[80];
    char date[80];
    struct pageSizeInfo* info;

    if (argc != 2) {
	fprintf(stderr, "usage: %s file.tif\n", argv[0]);
	exit(-1);
    }
    printf("%s:\n", argv[1]);
    TIFFSetErrorHandler(NULL);
    TIFFSetWarningHandler(NULL);
    tif = TIFFOpen(argv[1], "r");
    if (tif == NULL) {
	printf("Could not open (%s).\n", strerror(errno));
	exit(0);
    }
    ok = isFAXImage(tif);
    if (!ok) {
	printf("Does not look like a facsimile?\n");
	exit(0);
    }
    if (TIFFGetField(tif, TIFFTAG_IMAGEDESCRIPTION, &cp))
	sanitize(sender, cp, sizeof (sender));
    else
	strcpy(sender, "<unknown>");
    printf("%11s %s\n", "Sender:", sender);
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &pageWidth);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &pageLength);
    if (TIFFGetField(tif, TIFFTAG_YRESOLUTION, &vres)) {
	short resunit = RESUNIT_NONE;
	TIFFGetField(tif, TIFFTAG_RESOLUTIONUNIT, &resunit);
	if (resunit == RESUNIT_CENTIMETER)
	    vres *= 25.4;
    } else
	vres = 98;
    npages = 0;
    do {
	npages++;
    } while (TIFFReadDirectory(tif));
    printf("%11s %u\n", "Pages:", npages);
    if (vres == 98)
	printf("%11s Normal\n", "Quality:");
    else if (vres == 196)
	printf("%11s Fine\n", "Quality:");
    else
	printf("%11s %.2f lines/inch\n", "Quality:", vres);
    h = pageLength / (vres < 100 ? 3.85 : 7.7);
    w = (pageWidth / 204.) * 25.4;
    info = closestPageSize(w, h);
    if (info)
	printf("%11s %s\n", "Page:", getPageName(info));
    else
	printf("%11s %lu by %lu\n", "Page:", pageWidth, pageLength);
    delPageSize(info);
    if (!TIFFGetField(tif, TIFFTAG_DATETIME, &cp)) {
	struct stat sb;
	fstat(TIFFFileno(tif), &sb);
	strftime(date, sizeof (date),
	    "%Y:%m:%d %H:%M:%S %Z", localtime(&sb.st_mtime));
    } else
	sanitize(date, cp, sizeof (date));
    printf("%11s %s\n", "Received:", date);
    exit(0);
}
