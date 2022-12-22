#ifndef lint
static char rcsid[] = "$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/libtiff/tif_read.c,v 1.1.1.1 1994/01/14 23:10:07 torek Exp $";
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
 * Scanline-oriented Read Support
 */
#include "tiffiop.h"

#if USE_PROTOTYPES
static	int TIFFFillStrip(TIFF *, u_int);
static	int TIFFFillTile(TIFF *, u_int);
static	int TIFFStartStrip(TIFF *, u_int);
static	int TIFFStartTile(TIFF *, u_int);
static	int TIFFCheckRead(TIFF *, int);
#else
static	int TIFFFillStrip();
static	int TIFFFillTile();
static	int TIFFStartStrip();
static	int TIFFStartTile();
static	int TIFFCheckRead();
#endif

/*
 * Seek to a random row+sample in a file.
 */
static int
DECLARE3(TIFFSeek, TIFF*, tif, u_int, row, u_int, sample)
{
	register TIFFDirectory *td = &tif->tif_dir;
	int strip;

	if (row >= td->td_imagelength) {	/* out of range */
		TIFFError(tif->tif_name, "%d: Row out of range, max %d",
		    row, td->td_imagelength);
		return (0);
	}
	if (td->td_planarconfig == PLANARCONFIG_SEPARATE) {
		if (sample >= td->td_samplesperpixel) {
			TIFFError(tif->tif_name,
			    "%d: Sample out of range, max %d",
			    sample, td->td_samplesperpixel);
			return (0);
		}
		strip = sample*td->td_stripsperimage + row/td->td_rowsperstrip;
	} else
		strip = row / td->td_rowsperstrip;
	if (strip != tif->tif_curstrip) { 	/* different strip, refill */
		if (!TIFFFillStrip(tif, strip))
			return (0);
	} else if (row < tif->tif_row) {
		/*
		 * Moving backwards within the same strip: backup
		 * to the start and then decode forward (below).
		 *
		 * NB: If you're planning on lots of random access within a
		 * strip, it's better to just read and decode the entire
		 * strip, and then access the decoded data in a random fashion.
		 */
		if (!TIFFStartStrip(tif, strip))
			return (0);
	}
	if (row != tif->tif_row) {
		if (tif->tif_seek) {
			/*
			 * Seek forward to the desired row.
			 */
			if (!(*tif->tif_seek)(tif, row - tif->tif_row))
				return (0);
			tif->tif_row = row;
		} else {
			TIFFError(tif->tif_name,
		    "Compression algorithm does not support random access");
			return (0);
		}
	}
	return (1);
}

int
/*VARARGS3*/
DECLARE4(TIFFReadScanline, TIFF*, tif, u_char*, buf, u_int, row, u_int, sample)
{
	int e;

	if (!TIFFCheckRead(tif, 0))
		return (-1);
	if (e = TIFFSeek(tif, row, sample)) {
		/*
		 * Decompress desired row into user buffer.
		 */
		e = (*tif->tif_decoderow)
		    (tif, buf, tif->tif_scanlinesize, sample);
		tif->tif_row++;
	}
	return (e ? 1 : -1);
}

/*
 * Read a strip of data and decompress the specified
 * amount into the user-supplied buffer.
 */
int
DECLARE4(TIFFReadEncodedStrip,
    TIFF*, tif, u_int, strip, u_char*, buf, u_long, size)
{
	TIFFDirectory *td = &tif->tif_dir;
	u_long nrows, stripsize;

	if (!TIFFCheckRead(tif, 0))
		return (-1);
	if (strip >= td->td_nstrips) {
		TIFFError(tif->tif_name, "%d: Strip out of range, max %d",
		    strip, td->td_nstrips);
		return (-1);
	}
	/*
	 * Calculate the strip size according to the number of
	 * rows in the strip (check for truncated last strip).
	 */
	if (strip != td->td_nstrips-1 ||
	    (nrows = td->td_imagelength % td->td_rowsperstrip) == 0)
		nrows = td->td_rowsperstrip;
	stripsize = TIFFVStripSize(tif, nrows);
	if (size == (u_long)-1)
		size = stripsize;
	else if (size > stripsize)
		size = stripsize;
	return (TIFFFillStrip(tif, strip) && 
    (*tif->tif_decodestrip)(tif, buf, size, strip / td->td_stripsperimage) ?
	    size : -1);
}

static int
DECLARE5(TIFFReadRawStrip1,
    TIFF*, tif, u_int, strip, u_char*, buf, u_long, size, const char*, module)
{
	TIFFDirectory *td = &tif->tif_dir;

	if (!isMapped(tif)) {
		if (!SeekOK(tif, td->td_stripoffset[strip])) {
			TIFFError(module,
			    "%s: Seek error at scanline %d, strip %d",
			    tif->tif_name, tif->tif_row, strip);
			return (-1);
		}
		if (!ReadOK(tif, buf, size)) {
			TIFFError(module, "%s: Read error at scanline %d",
			    tif->tif_name, tif->tif_row);
			return (-1);
		}
	} else {
		if (td->td_stripoffset[strip] + size > tif->tif_size) {
			TIFFError(module,
			    "%s: Seek error at scanline %d, strip %d",
			    tif->tif_name, tif->tif_row, strip);
			return (-1);
		}
		memcpy(buf, tif->tif_base + td->td_stripoffset[strip], size);
	}
	return (size);
}

/*
 * Read a strip of data from the file.
 */
int
DECLARE4(TIFFReadRawStrip,
    TIFF*, tif, u_int, strip, u_char*, buf, u_long, size)
{
	static const char module[] = "TIFFReadRawStrip";
	TIFFDirectory *td = &tif->tif_dir;
	u_long bytecount;

	if (!TIFFCheckRead(tif, 0))
		return (-1);
	if (strip >= td->td_nstrips) {
		TIFFError(tif->tif_name, "%d: Strip out of range, max %d",
		    strip, td->td_nstrips);
		return (-1);
	}
	bytecount = td->td_stripbytecount[strip];
	if (size != (u_int)-1 && size < bytecount)
		bytecount = size;
	return (TIFFReadRawStrip1(tif, strip, buf, bytecount, module));
}

/*
 * Read the specified strip and setup for decoding. 
 * The data buffer is expanded, as necessary, to
 * hold the strip's data.
 */
static
DECLARE2(TIFFFillStrip, TIFF*, tif, u_int, strip)
{
	static const char module[] = "TIFFFillStrip";
	TIFFDirectory *td = &tif->tif_dir;
	u_long bytecount;

	bytecount = td->td_stripbytecount[strip];
	if (isMapped(tif) &&
	    (td->td_fillorder == tif->tif_fillorder || (tif->tif_flags & TIFF_NOBITREV))) {
		/*
		 * The image is mapped into memory and we either don't
		 * need to flip bits or the compression routine is going
		 * to handle this operation itself.  In this case, avoid
		 * copying the raw data and instead just reference the
		 * data from the memory mapped file image.  This assumes
		 * that the decompression routines do not modify the
		 * contents of the raw data buffer (if they try to,
		 * the application will get a fault since the file is
		 * mapped read-only).
		 */
		if ((tif->tif_flags & TIFF_MYBUFFER) && tif->tif_rawdata)
			_TIFFfree(tif->tif_rawdata);
		tif->tif_flags &= ~TIFF_MYBUFFER;
		if (td->td_stripoffset[strip] + bytecount > tif->tif_size) {
			/*
			 * This error message might seem strange, but it's
			 * what would happen if a read were done instead.
			 */
			TIFFError(module, "%s: Read error on strip %d",
			    tif->tif_name, strip);
			tif->tif_curstrip = -1;		/* unknown state */
			return (0);
		}
		tif->tif_rawdatasize = bytecount;
		tif->tif_rawdata = tif->tif_base + td->td_stripoffset[strip];
	} else {
		/*
		 * Expand raw data buffer, if needed, to
		 * hold data strip coming from file
		 * (perhaps should set upper bound on
		 *  the size of a buffer we'll use?).
		 */
		if (bytecount > tif->tif_rawdatasize) {
			tif->tif_curstrip = -1;		/* unknown state */
			if ((tif->tif_flags & TIFF_MYBUFFER) == 0) {
				TIFFError(module,
				"%s: Data buffer too small to hold strip %d",
				    tif->tif_name, strip);
				return (0);
			}
			if (!TIFFReadBufferSetup(tif, 0,
			    roundup(bytecount, 1024)))
				return (0);
		}
		if (TIFFReadRawStrip1(tif, strip, (u_char *)tif->tif_rawdata,
		    bytecount, module) != bytecount)
			return (0);
		if (td->td_fillorder != tif->tif_fillorder &&
		    (tif->tif_flags & TIFF_NOBITREV) == 0)
			TIFFReverseBits((u_char *)tif->tif_rawdata, bytecount);
	}
	return (TIFFStartStrip(tif, strip));
}

/*
 * Tile-oriented Read Support
 * Contributed by Nancy Cam (Silicon Graphics).
 */

/*
 * Read and decompress a tile of data.  The
 * tile is selected by the (x,y,z,s) coordinates.
 */
int
DECLARE6(TIFFReadTile,
    TIFF*, tif, u_char*, buf, u_long, x, u_long, y, u_long, z, u_int, s)
{
	u_int tile;

	if (!TIFFCheckRead(tif, 1) || !TIFFCheckTile(tif, x, y, z, s))
		return (-1);
	tile = TIFFComputeTile(tif, x, y, z, s);
	if (tile >= tif->tif_dir.td_nstrips) {
		TIFFError(tif->tif_name, "%d: Tile out of range, max %d",
		    tile, tif->tif_dir.td_nstrips);
		return (-1);
	}
	return (TIFFFillTile(tif, tile) &&
	    (*tif->tif_decodetile)(tif, buf, tif->tif_tilesize, s) ?
	    tif->tif_tilesize : -1);
}

/*
 * Read a tile of data and decompress the specified
 * amount into the user-supplied buffer.
 */
int
DECLARE4(TIFFReadEncodedTile,
    TIFF*, tif, u_int, tile, u_char*, buf, u_long, size)
{
	TIFFDirectory *td = &tif->tif_dir;
	u_long tilesize = tif->tif_tilesize;

	if (!TIFFCheckRead(tif, 1))
		return (-1);
	if (tile >= td->td_nstrips) {
		TIFFError(tif->tif_name, "%d: Tile out of range, max %d",
		    tile, td->td_nstrips);
		return (-1);
	}
	if (size == (u_int)-1)
		size = tilesize;
	else if (size > tilesize )
		size = tilesize;
	return (TIFFFillTile(tif, tile) && 
	    (*tif->tif_decodetile)(tif, buf, size, tile/td->td_stripsperimage) ?
	    size : -1);
}

static int
DECLARE5(TIFFReadRawTile1,
	TIFF*, tif, u_int, tile, u_char*, buf, u_long, size,
	const char*, module)
{
	TIFFDirectory *td = &tif->tif_dir;

	if (!isMapped(tif)) {
		if (!SeekOK(tif, td->td_stripoffset[tile])) {
			TIFFError(module,
			    "%s: Seek error at row %d, col %d, tile %d",
			    tif->tif_name, tif->tif_row, tif->tif_col, tile);
			return (-1);
		}
		if (!ReadOK(tif, buf, size)) {
			TIFFError(module, "%s: Read error at row %d, col %d",
			    tif->tif_name, tif->tif_row, tif->tif_col);
			return (-1);
		}
	} else {
		if (td->td_stripoffset[tile] + size > tif->tif_size) {
			TIFFError(module,
			    "%s: Seek error at row %d, col %d, tile %d",
			    tif->tif_name, tif->tif_row, tif->tif_col, tile);
			return (-1);
		}
		memcpy(buf, tif->tif_base + td->td_stripoffset[tile], size);
	}
	return (size);
}

/*
 * Read a tile of data from the file.
 */
int
DECLARE4(TIFFReadRawTile, TIFF*, tif, u_int, tile, u_char*, buf, u_long, size)
{
	static const char module[] = "TIFFReadRawTile";
	TIFFDirectory *td = &tif->tif_dir;
	u_long bytecount;

	if (!TIFFCheckRead(tif, 1))
		return (-1);
	if (tile >= td->td_nstrips) {
		TIFFError(tif->tif_name, "%d: Tile out of range, max %d",
		    tile, td->td_nstrips);
		return (-1);
	}
	bytecount = td->td_stripbytecount[tile];
	if (size != (u_int)-1 && size < bytecount)
		bytecount = size;
	return (TIFFReadRawTile1(tif, tile, buf, bytecount, module));
}

/*
 * Read the specified tile and setup for decoding. 
 * The data buffer is expanded, as necessary, to
 * hold the tile's data.
 */
static
DECLARE2(TIFFFillTile, TIFF*, tif, u_int, tile)
{
	static const char module[] = "TIFFFillTile";
	TIFFDirectory *td = &tif->tif_dir;
	u_long bytecount;

	bytecount = td->td_stripbytecount[tile];
	if (isMapped(tif) &&
	    (td->td_fillorder == tif->tif_fillorder || (tif->tif_flags & TIFF_NOBITREV))) {
		/*
		 * The image is mapped into memory and we either don't
		 * need to flip bits or the compression routine is going
		 * to handle this operation itself.  In this case, avoid
		 * copying the raw data and instead just reference the
		 * data from the memory mapped file image.  This assumes
		 * that the decompression routines do not modify the
		 * contents of the raw data buffer (if they try to,
		 * the application will get a fault since the file is
		 * mapped read-only).
		 */
		if ((tif->tif_flags & TIFF_MYBUFFER) && tif->tif_rawdata)
			_TIFFfree(tif->tif_rawdata);
		tif->tif_flags &= ~TIFF_MYBUFFER;
		if (td->td_stripoffset[tile] + bytecount > tif->tif_size) {
			tif->tif_curtile = -1;		/* unknown state */
			return (0);
		}
		tif->tif_rawdatasize = bytecount;
		tif->tif_rawdata = tif->tif_base + td->td_stripoffset[tile];
	} else {
		/*
		 * Expand raw data buffer, if needed, to
		 * hold data tile coming from file
		 * (perhaps should set upper bound on
		 *  the size of a buffer we'll use?).
		 */
		if (bytecount > tif->tif_rawdatasize) {
			tif->tif_curtile = -1;		/* unknown state */
			if ((tif->tif_flags & TIFF_MYBUFFER) == 0) {
				TIFFError(module,
				"%s: Data buffer too small to hold tile %d",
				    tif->tif_name, tile);
				return (0);
			}
			if (!TIFFReadBufferSetup(tif, 0,
			    roundup(bytecount, 1024)))
				return (0);
		}
		if (TIFFReadRawTile1(tif, tile, (u_char *)tif->tif_rawdata,
		    bytecount, module) != bytecount)
			return (0);
		if (td->td_fillorder != tif->tif_fillorder &&
		    (tif->tif_flags & TIFF_NOBITREV) == 0)
			TIFFReverseBits((u_char *)tif->tif_rawdata, bytecount);
	}
	return (TIFFStartTile(tif, tile));
}

/*
 * Setup the raw data buffer in preparation for
 * reading a strip of raw data.  If the buffer
 * is specified as zero, then a buffer of appropriate
 * size is allocated by the library.  Otherwise,
 * the client must guarantee that the buffer is
 * large enough to hold any individual strip of
 * raw data.
 */
int
DECLARE3(TIFFReadBufferSetup, TIFF*, tif, char*, bp, u_long, size)
{
	static const char module[] = "TIFFReadBufferSetup";

	if (tif->tif_rawdata) {
		if (tif->tif_flags & TIFF_MYBUFFER)
			_TIFFfree(tif->tif_rawdata);
		tif->tif_rawdata = NULL;
	}
	if (bp) {
		tif->tif_rawdatasize = size;
		tif->tif_rawdata = bp;
		tif->tif_flags &= ~TIFF_MYBUFFER;
	} else {
		tif->tif_rawdatasize = roundup(size, 1024);
		tif->tif_rawdata = _TIFFmalloc(tif->tif_rawdatasize);
		tif->tif_flags |= TIFF_MYBUFFER;
	}
	if (tif->tif_rawdata == NULL) {
		TIFFError(module,
		    "%s: No space for data buffer at scanline %d",
		    tif->tif_name, tif->tif_row);
		tif->tif_rawdatasize = 0;
		return (0);
	}
	return (1);
}

/*
 * Set state to appear as if a
 * strip has just been read in.
 */
static int
DECLARE2(TIFFStartStrip, TIFF*, tif, u_int, strip)
{
	TIFFDirectory *td = &tif->tif_dir;

	tif->tif_curstrip = strip;
	tif->tif_row = (strip % td->td_stripsperimage) * td->td_rowsperstrip;
	tif->tif_rawcp = tif->tif_rawdata;
	tif->tif_rawcc = td->td_stripbytecount[strip];
	return (tif->tif_predecode == NULL || (*tif->tif_predecode)(tif));
}

/*
 * Set state to appear as if a
 * tile has just been read in.
 */
static int
DECLARE2(TIFFStartTile, TIFF*, tif, u_int, tile)
{
	TIFFDirectory *td = &tif->tif_dir;

	tif->tif_curtile = tile;
	tif->tif_row =
	    (tile % howmany(td->td_imagewidth, td->td_tilewidth)) *
		td->td_tilelength;
	tif->tif_col =
	    (tile % howmany(td->td_imagelength, td->td_tilelength)) *
		td->td_tilewidth;
	tif->tif_rawcp = tif->tif_rawdata;
	tif->tif_rawcc = td->td_stripbytecount[tile];
	return (tif->tif_predecode == NULL || (*tif->tif_predecode)(tif));
}

static int
DECLARE2(TIFFCheckRead, TIFF*, tif, int, tiles)
{
	if (tif->tif_mode == O_WRONLY) {
		TIFFError(tif->tif_name, "File not open for reading");
		return (0);
	}
	if (tiles ^ isTiled(tif)) {
		TIFFError(tif->tif_name, tiles ?
		    "Can not read tiles from a stripped image" :
		    "Can not read scanlines from a tiled image");
		return (0);
	}
	return (1);
}
