#ifndef lint
static char rcsid[] = "$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/libtiff/tif_dumpmode.c,v 1.1.1.1 1994/01/14 23:10:05 torek Exp $";
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
 * TIFF Library.
 *
 * "Null" Compression Algorithm Support.
 */
#include "tiffiop.h"
#include <assert.h>
#include <stdio.h>

/*
 * Encode a hunk of pixels.
 */
static int
DECLARE4(DumpModeEncode, TIFF*, tif, u_char*, pp, u_long, cc, u_int, s)
{
	/*
	 * This may be overzealous, but avoids having to
	 * worry about byte alignment for the (potential)
	 * byte-swapping work below.
	 */
	if (tif->tif_rawcc + cc > tif->tif_rawdatasize)
		if (!TIFFFlushData1(tif))
			return (-1);
	while (cc > 0) {
		int n;
		if ((n = cc) > tif->tif_rawdatasize)
			n = tif->tif_rawdatasize;
		memcpy(tif->tif_rawcp, pp, n);
		if (tif->tif_flags & TIFF_SWAB) {
			switch (tif->tif_dir.td_bitspersample) {
			case 16:
				assert((n & 3) == 0);
				TIFFSwabArrayOfShort((u_short *)tif->tif_rawcp,
				    n/2);
				break;
			case 32:
				assert((n & 15) == 0);
				TIFFSwabArrayOfLong((u_long *)tif->tif_rawcp,
				    n/4);
				break;
			}
		}
		tif->tif_rawcp += n;
		tif->tif_rawcc += n;
		pp += n;
		cc -= n;
		if (tif->tif_rawcc >= tif->tif_rawdatasize &&
		    !TIFFFlushData1(tif))
			return (-1);
	}
	return (1);
}

/*
 * Decode a hunk of pixels.
 */
static int
DECLARE4(DumpModeDecode, TIFF*, tif, u_char*, buf, u_long, cc, u_int, s)
{
	if (tif->tif_rawcc < cc) {
		TIFFError(tif->tif_name,
		    "DumpModeDecode: Not enough data for scanline %d",
		    tif->tif_row);
		return (0);
	}
	/*
	 * Avoid copy if client has setup raw
	 * data buffer to avoid extra copy.
	 */
	if (tif->tif_rawcp != (char*) buf)
		memcpy(buf, tif->tif_rawcp, cc);
	if (tif->tif_flags & TIFF_SWAB) {
		switch (tif->tif_dir.td_bitspersample) {
		case 16:
			assert((cc & 3) == 0);
			TIFFSwabArrayOfShort((u_short *)buf, cc/2);
			break;
		case 32:
			assert((cc & 15) == 0);
			TIFFSwabArrayOfLong((u_long *)buf, cc/4);
			break;
		}
	}
	tif->tif_rawcp += cc;
	tif->tif_rawcc -= cc;
	return (1);
}

/*
 * Seek forwards nrows in the current strip.
 */
static int
DECLARE2(DumpModeSeek, TIFF*, tif, u_long, nrows)
{
	tif->tif_rawcp += nrows * tif->tif_scanlinesize;
	tif->tif_rawcc -= nrows * tif->tif_scanlinesize;
	return (1);
}

/*
 * Initialize dump mode.
 */
int
DECLARE1(TIFFInitDumpMode, TIFF*, tif)
{
	tif->tif_decoderow = DumpModeDecode;
	tif->tif_decodestrip = DumpModeDecode;
	tif->tif_decodetile = DumpModeDecode;
	tif->tif_encoderow = DumpModeEncode;
	tif->tif_encodestrip = DumpModeEncode;
	tif->tif_encodetile = DumpModeEncode;
	tif->tif_seek = DumpModeSeek;
	return (1);
}
