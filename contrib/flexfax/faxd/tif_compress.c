#ifndef lint
static char rcsid[] = "$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/tif_compress.c,v 1.1.1.1 1994/01/14 23:09:49 torek Exp $";
#endif

/*
 * Copyright (c) 1988, 1989, 1990, 1991, 1992 Sam Leffler
 * Copyright (c) 1991, 1992 Silicon Graphics, Inc.
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
 * TIFF Library
 *
 * Compression Scheme Configuration Support.
 */
#include "tiffiop.h"

typedef struct {
	char*	name;
	int	scheme;
	TIFFVoidMethod	init;
} cscheme_t;
static const cscheme_t CompressionSchemes[] = {
    { "Null",		COMPRESSION_NONE,	TIFFInitDumpMode },
    { "LZW",		COMPRESSION_LZW,	TIFFInitDumpMode },
    { "PackBits",	COMPRESSION_PACKBITS,	TIFFInitDumpMode },
    { "ThunderScan",	COMPRESSION_THUNDERSCAN,TIFFInitDumpMode },
    { "NeXT",		COMPRESSION_NEXT,	TIFFInitDumpMode },
    { "JPEG",		COMPRESSION_JPEG,	TIFFInitDumpMode },
    { "CCITT RLE",	COMPRESSION_CCITTRLE,	TIFFInitDumpMode },
    { "CCITT RLE/W",	COMPRESSION_CCITTRLEW,	TIFFInitDumpMode },
    { "CCITT Group3",	COMPRESSION_CCITTFAX3,	TIFFInitDumpMode },
    { "CCITT Group4",	COMPRESSION_CCITTFAX4,	TIFFInitDumpMode },
};
#define	NSCHEMES (sizeof (CompressionSchemes) / sizeof (CompressionSchemes[0]))

static const cscheme_t *
DECLARE1(findScheme, int, scheme)
{
	register const cscheme_t *c;

	for (c = CompressionSchemes; c < &CompressionSchemes[NSCHEMES]; c++)
		if (c->scheme == scheme)
			return (c);
	return ((const cscheme_t *)0);
}

static int
DECLARE2(TIFFNoEncode, TIFF*, tif, char*, method)
{
	const cscheme_t *c = findScheme(tif->tif_dir.td_compression);
	TIFFError(tif->tif_name,
	    "%s %s encoding is not implemented", c->name, method);
	return (-1);
}

int
DECLARE4(TIFFNoRowEncode, TIFF*, tif, u_char*, pp, u_long, cc, u_int, s)
{
	return (TIFFNoEncode(tif, "scanline"));
}

int
DECLARE4(TIFFNoStripEncode, TIFF*, tif, u_char*, pp, u_long, cc, u_int, s)
{
	return (TIFFNoEncode(tif, "strip"));
}

int
DECLARE4(TIFFNoTileEncode, TIFF*, tif, u_char*, pp, u_long, cc, u_int, s)
{
	return (TIFFNoEncode(tif, "tile"));
}

static int
DECLARE2(TIFFNoDecode, TIFF*, tif, char*, method)
{
	const cscheme_t *c = findScheme(tif->tif_dir.td_compression);
	TIFFError(tif->tif_name,
	    "%s %s decoding is not implemented", c->name, method);
	return (-1);
}

int
DECLARE4(TIFFNoRowDecode, TIFF*, tif, u_char*, pp, u_long, cc, u_int, s)
{
	return (TIFFNoDecode(tif, "scanline"));
}

int
DECLARE4(TIFFNoStripDecode, TIFF*, tif, u_char*, pp, u_long, cc, u_int, s)
{
	return (TIFFNoDecode(tif, "strip"));
}

int
DECLARE4(TIFFNoTileDecode, TIFF*, tif, u_char*, pp, u_long, cc, u_int, s)
{
	return (TIFFNoDecode(tif, "tile"));
}

int
DECLARE2(TIFFSetCompressionScheme, TIFF*, tif, int, scheme)
{
	const cscheme_t *c = findScheme(scheme);

	if (!c) {
		TIFFError(tif->tif_name,
		    "Unknown data compression algorithm %u (0x%x)",
		    scheme, scheme);
		return (0);
	}
	tif->tif_predecode = NULL;
	tif->tif_decoderow = TIFFNoRowDecode;
	tif->tif_decodestrip = TIFFNoStripDecode;
	tif->tif_decodetile = TIFFNoTileDecode;
	tif->tif_preencode = NULL;
	tif->tif_postencode = NULL;
	tif->tif_encoderow = TIFFNoRowEncode;
	tif->tif_encodestrip = TIFFNoStripEncode;
	tif->tif_encodetile = TIFFNoTileEncode;
	tif->tif_close = NULL;
	tif->tif_seek = NULL;
	tif->tif_cleanup = NULL;
	tif->tif_flags &= ~TIFF_NOBITREV;
	tif->tif_options = 0;
	return ((*c->init)(tif));
}
