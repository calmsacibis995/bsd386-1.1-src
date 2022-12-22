#ifndef lint
static char rcsid[] = "$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/libtiff/tif_dirwrite.c,v 1.1.1.1 1994/01/14 23:10:05 torek Exp $";
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
 * Directory Write Support Routines.
 *
 * NB: Beware of the varargs declarations for routines in
 *     this file.  The names and types of variables has been
 *     carefully chosen to make things work with compilers that
 *     are busted in one way or another (e.g. SGI/MIPS).
 */
#include "tiffiop.h"

#if HAVE_IEEEFP
#define	TIFFCvtNativeToIEEEFloat(tif, n, fp)
#endif

#if USE_PROTOTYPES
static	int TIFFWriteNormalTag(TIFF*, TIFFDirEntry*, const TIFFFieldInfo*);
static	void TIFFSetupShortLong(TIFF *, u_short, TIFFDirEntry *, u_long);
static	int TIFFSetupShortPair(TIFF *, u_short, TIFFDirEntry *);
static	int TIFFWriteRational(TIFF *,
	    TIFFDataType, u_short, TIFFDirEntry *, float);
static	int TIFFWritePerSampleShorts(TIFF *, u_short, TIFFDirEntry *);
static	int TIFFWriteShortTable(TIFF *, u_short, TIFFDirEntry *, int, u_short **);
static	int TIFFWriteShortArray(TIFF *,
	    TIFFDataType, u_short, TIFFDirEntry *, int, u_short *);
static	int TIFFWriteLongArray(TIFF *,
	    TIFFDataType, u_short, TIFFDirEntry *, int, u_long *);
static	int TIFFWriteRationalArray(TIFF *,
	    TIFFDataType, u_short, TIFFDirEntry *, int, float *);
static	int TIFFWriteFloatArray(TIFF *,
	    TIFFDataType, u_short, TIFFDirEntry *, int, float *);
static	int TIFFWriteString(TIFF *, u_short, TIFFDirEntry *, char *);
#ifdef JPEG_SUPPORT
static	int TIFFWriteJPEGQTables(TIFF *, TIFFDirEntry *);
static	int TIFFWriteJPEGCTables(TIFF *, u_short, TIFFDirEntry *, u_char **);
#endif
#ifdef COLORIMETRY_SUPPORT
static	int TIFFWriteTransferFunction(TIFF*, TIFFDirEntry*);
#endif
static	int TIFFWriteData(TIFF *, TIFFDirEntry *, char *);
static	int TIFFLinkDirectory(TIFF *);
#else
static	int TIFFWriteNormalTag();
static	void TIFFSetupShortLong();
static	int TIFFSetupShortPair();
static	int TIFFWriteRational();
static	int TIFFWritePerSampleShorts();
static	int TIFFWriteShortTable();
static	int TIFFWriteShortArray();
static	int TIFFWriteLongArray();
static	int TIFFWriteRationalArray();
static	int TIFFWriteFloatArray();
static	int TIFFWriteString();
#ifdef JPEG_SUPPORT
static	int TIFFWriteJPEGQTables();
static	int TIFFWriteJPEGCTables();
#endif
#ifdef COLORIMETRY_SUPPORT
static	int TIFFWriteTransferFunction();
#endif
static	int TIFFWriteData();
static	int TIFFLinkDirectory();
#endif

#define	WriteRationalPair(type, tag1, v1, tag2, v2) {		\
	if (!TIFFWriteRational(tif, type, tag1, dir, v1))	\
		goto bad;					\
	if (!TIFFWriteRational(tif, type, tag2, dir+1, v2))	\
		goto bad;					\
	dir++;							\
}

/*
 * Write the contents of the current directory
 * to the specified file.  This routine doesn't
 * handle overwriting a directory with auxiliary
 * storage that's been changed.
 */
int
DECLARE1(TIFFWriteDirectory, TIFF*, tif)
{
	short dircount, tag;
	int nfields, dirsize;
	char *data;
	const TIFFFieldInfo *fip;
	TIFFDirEntry *dir;
	TIFFDirectory *td;
	u_long b, fields[FIELD_SETLONGS];

	if (tif->tif_mode == O_RDONLY)
		return (1);
	/*
	 * Clear write state so that subsequent images with
	 * different characteristics get the right buffers
	 * setup for them.
	 */
	if (tif->tif_flags & TIFF_POSTENCODE) {
		tif->tif_flags &= ~TIFF_POSTENCODE;
		if (tif->tif_postencode && !(*tif->tif_postencode)(tif)) {
			TIFFError(tif->tif_name,
			    "Error post-encoding before directory write");
			return (0);
		}
	}
	if (tif->tif_close)
		(*tif->tif_close)(tif);
	if (tif->tif_cleanup)
		(*tif->tif_cleanup)(tif);
	/*
	 * Flush any data that might have been written
	 * by the compression close+cleanup routines.
	 */
	if (tif->tif_rawcc > 0 && !TIFFFlushData1(tif)) {
		TIFFError(tif->tif_name,
		    "Error flushing data before directory write");
		return (0);
	}
	if ((tif->tif_flags & TIFF_MYBUFFER) && tif->tif_rawdata) {
		_TIFFfree(tif->tif_rawdata);
		tif->tif_rawdata = NULL;
		tif->tif_rawcc = 0;
	}
	tif->tif_flags &= ~(TIFF_BEENWRITING|TIFF_BUFFERSETUP);

	td = &tif->tif_dir;
	/*
	 * Size the directory so that we can calculate
	 * offsets for the data items that aren't kept
	 * in-place in each field.
	 */
	nfields = 0;
	for (b = 0; b <= FIELD_LAST; b++)
		if (TIFFFieldSet(tif, b))
			nfields += (b < FIELD_SUBFILETYPE ? 2 : 1);
	dirsize = nfields * sizeof (TIFFDirEntry);
	data = _TIFFmalloc(dirsize);
	if (data == NULL) {
		TIFFError(tif->tif_name,
		    "Cannot write directory, out of space");
		return (0);
	}
	/*
	 * Directory hasn't been placed yet, put
	 * it at the end of the file and link it
	 * into the existing directory structure.
	 */
	if (tif->tif_diroff == 0 && !TIFFLinkDirectory(tif))
		goto bad;
	tif->tif_dataoff =
	    tif->tif_diroff + sizeof (short) + dirsize + sizeof (long);
	if (tif->tif_dataoff & 1)
		tif->tif_dataoff++;
	(void) TIFFSeekFile(tif, tif->tif_dataoff, L_SET);
	tif->tif_curdir++;
	dir = (TIFFDirEntry *)data;
	/*
	 * Setup external form of directory
	 * entries and write data items.
	 */
	memcpy(fields, td->td_fieldsset, sizeof (fields));
/*BEGIN XXX*/
	/*
	 * Write out ExtraSamples tag only if Matteing would
	 * be set to 1 (i.e. Associated Alpha data is present).
	 */
	if (FieldSet(fields, FIELD_MATTEING) && !td->td_matteing) {	/*XXX*/
		ResetFieldBit(fields, FIELD_MATTEING);			/*XXX*/
		nfields--;						/*XXX*/
		dirsize -= sizeof (TIFFDirEntry);			/*XXX*/
	}								/*XXX*/
/*END XXX*/
	for (fip = tiffFieldInfo; fip->field_tag; fip++) {
		if (fip->field_bit == FIELD_IGNORE ||
		    !FieldSet(fields, fip->field_bit))
			continue;
		switch (fip->field_bit) {
		case FIELD_STRIPOFFSETS:
			/*
			 * We use one field bit for both strip and tile
			 * offsets, and so must be careful in selecting
			 * the appropriate field descriptor (so that tags
			 * are written in sorted order).
			 */
			tag = isTiled(tif) ?
			    TIFFTAG_TILEOFFSETS : TIFFTAG_STRIPOFFSETS;
			if (tag != fip->field_tag)
				continue;
			if (!TIFFWriteLongArray(tif, TIFF_LONG, tag, dir,
			    (int) td->td_nstrips, td->td_stripoffset))
				goto bad;
			break;
		case FIELD_STRIPBYTECOUNTS:
			/*
			 * We use one field bit for both strip and tile
			 * byte counts, and so must be careful in selecting
			 * the appropriate field descriptor (so that tags
			 * are written in sorted order).
			 */
			tag = isTiled(tif) ?
			    TIFFTAG_TILEBYTECOUNTS : TIFFTAG_STRIPBYTECOUNTS;
			if (tag != fip->field_tag)
				continue;
			if (!TIFFWriteLongArray(tif, TIFF_LONG, tag, dir,
			    (int) td->td_nstrips, td->td_stripbytecount))
				goto bad;
			break;
		case FIELD_ROWSPERSTRIP:
			TIFFSetupShortLong(tif, TIFFTAG_ROWSPERSTRIP,
			    dir, td->td_rowsperstrip);
			break;
		case FIELD_COLORMAP:
			if (!TIFFWriteShortTable(tif, TIFFTAG_COLORMAP, dir,
			    3, td->td_colormap))
				goto bad;
			break;
		case FIELD_IMAGEDIMENSIONS:
			TIFFSetupShortLong(tif, TIFFTAG_IMAGEWIDTH,
			    dir++, td->td_imagewidth);
			TIFFSetupShortLong(tif, TIFFTAG_IMAGELENGTH,
			    dir, td->td_imagelength);
			break;
		case FIELD_TILEDIMENSIONS:
			TIFFSetupShortLong(tif, TIFFTAG_TILEWIDTH,
			    dir++, td->td_tilewidth);
			TIFFSetupShortLong(tif, TIFFTAG_TILELENGTH,
			    dir, td->td_tilelength);
			break;
		case FIELD_POSITION:
			WriteRationalPair(TIFF_RATIONAL,
			    TIFFTAG_XPOSITION, td->td_xposition,
			    TIFFTAG_YPOSITION, td->td_yposition);
			break;
		case FIELD_RESOLUTION:
			WriteRationalPair(TIFF_RATIONAL,
			    TIFFTAG_XRESOLUTION, td->td_xresolution,
			    TIFFTAG_YRESOLUTION, td->td_yresolution);
			break;
		case FIELD_BITSPERSAMPLE:
		case FIELD_MINSAMPLEVALUE:
		case FIELD_MAXSAMPLEVALUE:
		case FIELD_SAMPLEFORMAT:
			if (!TIFFWritePerSampleShorts(tif, fip->field_tag, dir))
				goto bad;
			break;
		case FIELD_PAGENUMBER:
		case FIELD_HALFTONEHINTS:
#ifdef YCBCR_SUPPORT
		case FIELD_YCBCRSUBSAMPLING:
#endif
#ifdef CMYK_SUPPORT
		case FIELD_DOTRANGE:
#endif
			if (!TIFFSetupShortPair(tif, fip->field_tag, dir))
				goto bad;
			break;
#ifdef JPEG_SUPPORT
		case FIELD_JPEGQTABLES:
			if (!TIFFWriteJPEGQTables(tif, dir))
				goto bad;
			break;
		case FIELD_JPEGDCTABLES:
			if (!TIFFWriteJPEGCTables(tif,
			    TIFFTAG_JPEGDCTABLES, dir, td->td_dctab))
				goto bad;
			break;
		case FIELD_JPEGACTABLES:
			if (!TIFFWriteJPEGCTables(tif,
			    TIFFTAG_JPEGACTABLES, dir, td->td_actab))
				goto bad;
			break;
#endif
#ifdef COLORIMETRY_SUPPORT
		case FIELD_TRANSFERFUNCTION:
			if (!TIFFWriteTransferFunction(tif, dir))
				goto bad;
			break;
#endif
		default:
			if (!TIFFWriteNormalTag(tif, dir, fip))
				goto bad;
			break;
		}
		dir++;
		ResetFieldBit(fields, fip->field_bit);
	}
	/*
	 * Write directory.
	 */
	(void) TIFFSeekFile(tif, tif->tif_diroff, L_SET);
	dircount = nfields;
	if (!WriteOK(tif, &dircount, sizeof (short))) {
		TIFFError(tif->tif_name, "Error writing directory count");
		goto bad;
	}
	if (!WriteOK(tif, data, dirsize)) {
		TIFFError(tif->tif_name, "Error writing directory contents");
		goto bad;
	}
	if (!WriteOK(tif, &tif->tif_nextdiroff, sizeof (long))) {
		TIFFError(tif->tif_name, "Error writing directory link");
		goto bad;
	}
	TIFFFreeDirectory(tif);
	_TIFFfree(data);
	tif->tif_flags &= ~TIFF_DIRTYDIRECT;

	/*
	 * Reset directory-related state for subsequent
	 * directories.
	 */
	TIFFDefaultDirectory(tif);
	tif->tif_diroff = 0;
	tif->tif_curoff = 0;
	tif->tif_row = -1;
	tif->tif_curstrip = -1;
	return (1);
bad:
	_TIFFfree(data);
	return (0);
}
#undef WriteRationalPair

/*
 * Process tags that are not special cased.
 */
static int
DECLARE3(TIFFWriteNormalTag,
    TIFF*, tif, TIFFDirEntry*, dir, const TIFFFieldInfo*, fip)
{
	TIFFDirectory* td = &tif->tif_dir;
	u_short wc = (u_short) fip->field_writecount;

	dir->tdir_tag = fip->field_tag;
	dir->tdir_type = (u_short)fip->field_type;
	dir->tdir_count = wc;
#define	WRITE(x,y)	x(tif, fip->field_type, fip->field_tag, dir, wc, y)
	switch (fip->field_type) {
	case TIFF_SHORT:
	case TIFF_SSHORT:
		if (wc > 1) {
			u_short *wp;
			if (wc == (u_short) TIFF_VARIABLE) {
				_TIFFgetfield(td, fip->field_tag, &wc, &wp);
				dir->tdir_count = wc;
			} else
				_TIFFgetfield(td, fip->field_tag, &wp);
			if (!WRITE(TIFFWriteShortArray, wp))
				return (0);
		} else {
			u_short sv;
			_TIFFgetfield(td, fip->field_tag, &sv);
			dir->tdir_offset =
			    TIFFInsertData(tif, dir->tdir_type, sv);
		}
		break;
	case TIFF_LONG:
	case TIFF_SLONG:
		if (wc > 1) {
			u_long *lp;
			if (wc == (u_short) TIFF_VARIABLE) {
				_TIFFgetfield(td, fip->field_tag, &wc, &lp);
				dir->tdir_count = wc;
			} else
				_TIFFgetfield(td, fip->field_tag, &lp);
			if (!WRITE(TIFFWriteLongArray, lp))
				return (0);
		} else {
			/* XXX handle LONG->SHORT conversion */
			_TIFFgetfield(td, fip->field_tag, &dir->tdir_offset);
		}
		break;
	case TIFF_RATIONAL:
	case TIFF_SRATIONAL:
		if (wc > 1) {
			float *fp;
			if (wc == (u_short) TIFF_VARIABLE) {
				_TIFFgetfield(td, fip->field_tag, &wc, &fp);
				dir->tdir_count = wc;
			} else
				_TIFFgetfield(td, fip->field_tag, &fp);
			if (!WRITE(TIFFWriteRationalArray, fp))
				return (0);
		} else {
			float fv;
			_TIFFgetfield(td, fip->field_tag, &fv);
			if (!TIFFWriteRational(tif, fip->field_type, fip->field_tag, dir, fv))
				return (0);
		}
		break;
	case TIFF_FLOAT:
		if (wc > 1) {
			float *fp;
			if (wc == (u_short) TIFF_VARIABLE) {
				_TIFFgetfield(td, fip->field_tag, &wc, &fp);
				dir->tdir_count = wc;
			} else
				_TIFFgetfield(td, fip->field_tag, &fp);
			if (!WRITE(TIFFWriteFloatArray, fp))
				return (0);
		} else {
			float fv;
			_TIFFgetfield(td, fip->field_tag, &fv);
			TIFFCvtNativeToIEEEFloat(tif, 1, &fv);
			/* XXX assumes sizeof (long) == sizeof (float) */
			dir->tdir_offset = *(u_long *)&fv;	/* XXX */
		}
		break;
	case TIFF_ASCII: {
		char *cp;
		_TIFFgetfield(td, fip->field_tag, &cp);
		if (!TIFFWriteString(tif, fip->field_tag, dir, cp))
			return (0);
		break;
	}
	}
	return (1);
}
#undef WRITE

/*
 * Setup a directory entry with either a SHORT
 * or LONG type according to the value.
 */
static void
DECLARE4(TIFFSetupShortLong,
    TIFF*, tif, u_short, tag, TIFFDirEntry*, dir, u_long, v)
{
	dir->tdir_tag = tag;
	dir->tdir_count = 1;
	if (v > 0xffffL) {
		dir->tdir_type = (short)TIFF_LONG;
		dir->tdir_offset = v;
	} else {
		dir->tdir_type = (short)TIFF_SHORT;
		dir->tdir_offset = TIFFInsertData(tif, (int)TIFF_SHORT, v);
	}
}
#undef MakeShortDirent

/*
 * Setup a RATIONAL directory entry and
 * write the associated indirect value.
 */
static int
DECLARE5(TIFFWriteRational,
    TIFF*, tif, TIFFDataType, type, u_short, tag, TIFFDirEntry*, dir, float, v)
{
	return (TIFFWriteRationalArray(tif, type, tag, dir, 1, &v));
}

#define	NITEMS(x)	(sizeof (x) / sizeof (x[0]))
/*
 * Setup a directory entry that references a
 * samples/pixel array of SHORT values and
 * (potentially) write the associated indirect
 * values.
 */
static int
DECLARE3(TIFFWritePerSampleShorts,
    TIFF*, tif, u_short, tag, TIFFDirEntry*, dir)
{
	u_short buf[10], v;
	u_short* w = buf;
	int i, status, samples = tif->tif_dir.td_samplesperpixel;

	if (samples > NITEMS(buf))
		w = (u_short *)_TIFFmalloc(samples * sizeof (u_short));
	_TIFFgetfield(&tif->tif_dir, tag, &v);
	for (i = 0; i < samples; i++)
		w[i] = v;
	status = TIFFWriteShortArray(tif, TIFF_SHORT, tag, dir, samples, w);
	if (w != buf)
		_TIFFfree((char*)w);
	return (status);
}
#undef NITEMS

/*
 * Setup a pair of shorts that are returned by
 * value, rather than as a reference to an array.
 */
static int
DECLARE3(TIFFSetupShortPair,
    TIFF*, tif, u_short, tag, TIFFDirEntry*, dir)
{
	u_short v[2];

	_TIFFgetfield(&tif->tif_dir, tag, &v[0], &v[1]);
	return (TIFFWriteShortArray(tif, TIFF_SHORT, tag, dir, 2, v));
}

/*
 * Setup a directory entry for an NxM table of shorts,
 * where M is known to be 2**bitspersample, and write
 * the associated indirect data.
 */
static int
DECLARE5(TIFFWriteShortTable,
    TIFF*, tif, u_short, tag, TIFFDirEntry*, dir, int, n, u_short**, table)
{
	u_long off;
	int i;

	dir->tdir_tag = tag;
	dir->tdir_type = (short)TIFF_SHORT;
	/* XXX -- yech, fool TIFFWriteData */
	dir->tdir_count = 1L<<tif->tif_dir.td_bitspersample;
	off = tif->tif_dataoff;
	for (i = 0; i < n; i++)
		if (!TIFFWriteData(tif, dir, (char *)table[i]))
			return (0);
	dir->tdir_count *= n;
	dir->tdir_offset = off;
	return (1);
}

/*
 * Setup a directory entry of an ASCII string
 * and write any associated indirect value.
 */
static int
DECLARE4(TIFFWriteString,
    TIFF*, tif, u_short, tag, TIFFDirEntry*, dir, char*, cp)
{
	dir->tdir_tag = tag;
	dir->tdir_type = (short)TIFF_ASCII;
	dir->tdir_count = strlen(cp) + 1;	/* includes \0 byte */
	if (dir->tdir_count > 4) {
		if (!TIFFWriteData(tif, dir, cp))
			return (0);
	} else
		memcpy(&dir->tdir_offset, cp, dir->tdir_count);
	return (1);
}

/*
 * Setup a directory entry of an array of SHORT
 * or SSHORT and write the associated indirect values.
 */
static int
DECLARE6(TIFFWriteShortArray, TIFF*, tif,
    TIFFDataType, type, u_short, tag, TIFFDirEntry*, dir, int, n, u_short*, v)
{
	dir->tdir_tag = tag;
	dir->tdir_type = (short)type;
	dir->tdir_count = n;
	if (n <= 2) {
		if (tif->tif_header.tiff_magic == TIFF_BIGENDIAN) {
			dir->tdir_offset = (long)v[0] << 16;
			if (n == 2)
				dir->tdir_offset |= v[1] & 0xffff;
		} else {
			dir->tdir_offset = v[0] & 0xffff;
			if (n == 2)
				dir->tdir_offset |= (long)v[1] << 16;
		}
		return (1);
	} else
		return (TIFFWriteData(tif, dir, (char *)v));
}

/*
 * Setup a directory entry of an array of LONG
 * or SLONG and write the associated indirect values.
 */
static int
DECLARE6(TIFFWriteLongArray, TIFF*, tif,
    TIFFDataType, type, u_short, tag, TIFFDirEntry*, dir, int, n, u_long*, v)
{
	dir->tdir_tag = tag;
	dir->tdir_type = (short)type;
	dir->tdir_count = n;
	if (n == 1) {
		dir->tdir_offset = v[0];
		return (1);
	} else
		return (TIFFWriteData(tif, dir, (char *)v));
}

/*
 * Setup a directory entry of an array of RATIONAL
 * or SRATIONAL and write the associated indirect values.
 */
static int
DECLARE6(TIFFWriteRationalArray, TIFF*, tif,
    TIFFDataType, type, u_short, tag, TIFFDirEntry*, dir, int, n, float*, v)
{
	int i, status;
	u_long *t;

	dir->tdir_tag = tag;
	dir->tdir_type = (short)type;
	dir->tdir_count = n;
	t = (u_long *)_TIFFmalloc(2*n * sizeof (long));
	for (i = 0; i < n; i++) {
		float fv = v[i];
		int sign = 1;
		u_long den;

		if (fv < 0) {
			if (type == TIFF_RATIONAL) {
				TIFFWarning(tif->tif_name,
	"\"%s\": Information lost writing value (%g) as (unsigned) RATIONAL",
					TIFFFieldWithTag(tag)->field_name, v);
				fv = 0;
			} else
				fv = -fv, sign = -1;
		}
		den = 1L;
		if (fv > 0) {
			while (fv < 1L<<(31-3) && den < 1L<<(31-3))
				fv *= 1<<3, den *= 1L<<3;
		}
		t[2*i+0] = sign * (fv + 0.5);
		t[2*i+1] = den;
	}
	status = TIFFWriteData(tif, dir, (char *)t);
	_TIFFfree((char *)t);
	return (status);
}

static int
DECLARE6(TIFFWriteFloatArray, TIFF *, tif,
    TIFFDataType, type, u_short, tag, TIFFDirEntry *, dir, int, n, float *, v)
{
	dir->tdir_tag = tag;
	dir->tdir_type = (short)type;
	dir->tdir_count = n;
	TIFFCvtNativeToIEEEFloat(tif, n, v);
	if (n == 1) {
		dir->tdir_offset = *(u_long *)&v[0];
		return (1);
	} else
		return (TIFFWriteData(tif, dir, (char *)v));
}

#ifdef JPEG_SUPPORT
#define	NITEMS(x)	(sizeof (x) / sizeof (x[0]))
/*
 * Setup a directory entry for JPEG Quantization
 * tables and write the associated indirect values.
 */
static int
DECLARE2(TIFFWriteJPEGQTables, TIFF*, tif, TIFFDirEntry*, dir)
{
	int i, status = 0, samples = tif->tif_dir.td_samplesperpixel;
	u_long buf[10], *off = buf;
	TIFFDirEntry tdir;

	tdir.tdir_tag = TIFFTAG_JPEGQTABLES;	/* for diagnostics */
	tdir.tdir_type = (short)TIFF_BYTE;
	tdir.tdir_count = 64;
	if (samples > NITEMS(buf))
		off = (u_long*)_TIFFmalloc(samples * sizeof (u_long));
	for (i = 0; i < samples; i++) {
		if (!TIFFWriteData(tif, &tdir, (char *)tif->tif_dir.td_qtab[i]))
			goto bad;
		off[i] = tdir.tdir_offset;
	}
	status = TIFFWriteLongArray(tif, TIFF_LONG,
	    TIFFTAG_JPEGQTABLES, dir, samples, off);
bad:
	if (off != buf)
		_TIFFfree((char*)off);
	return (status);
}

/*
 * Setup a directory entry for JPEG Coefficient
 * tables and write the associated indirect values.
 */
static int
DECLARE4(TIFFWriteJPEGCTables,
    TIFF*, tif, u_short, tag, TIFFDirEntry*, dir, u_char **, tab)
{
	int status = 0, samples = tif->tif_dir.td_samplesperpixel;
	u_long buf[10], *off = buf;
	TIFFDirEntry tdir;
	int i, j, ncodes;

	tdir.tdir_tag = tag;		/* for diagnostics */
	tdir.tdir_type = (short)TIFF_BYTE;
	if (samples > NITEMS(buf))
		off = (u_long*)_TIFFmalloc(samples * sizeof (u_long));
	for (i = 0; i < samples; i++) {
		for (ncodes = 0, j = 0; j < 16; j++)
			ncodes += tab[i][j];
		tdir.tdir_count = 16+ncodes;
		if (!TIFFWriteData(tif, &tdir, (char *)tab[i]))
			goto bad;
		off[i] = tdir.tdir_offset;
	}
	status = TIFFWriteLongArray(tif, TIFF_LONG, tag, dir, samples, off);
bad:
	if (off != buf)
		_TIFFfree((char*)off);
	return (status);
}
#undef NITEMS
#endif

#ifdef COLORIMETRY_SUPPORT
static int
DECLARE2(TIFFWriteTransferFunction, TIFF*, tif, TIFFDirEntry*, dir)
{
	TIFFDirectory *td = &tif->tif_dir;
	u_long n = (1L<<td->td_bitspersample) * sizeof (u_short);
	u_short **tf = td->td_transferfunction;
	int ncols;

	/*
	 * Check if the table can be written as a single column,
	 * or if it must be written as 3 columns.  Note that we
	 * write a 3-column tag if there are 2 samples/pixel and
	 * a single column of data won't suffice--hmm.
	 */
	switch (td->td_samplesperpixel - td->td_matteing) {
	default:	if (memcmp(tf[0], tf[2], n)) { ncols = 3; break; }
	case 2:		if (memcmp(tf[0], tf[1], n)) { ncols = 3; break; }
	case 1: case 0:	ncols = 1;
	}
	return (TIFFWriteShortTable(tif,
	    TIFFTAG_TRANSFERFUNCTION, dir, ncols, tf));
}
#endif

/*
 * Write a contiguous directory item.
 */
static int
DECLARE3(TIFFWriteData, TIFF*, tif, TIFFDirEntry*, dir, char*, cp)
{
	int cc;

	dir->tdir_offset = tif->tif_dataoff;
	cc = dir->tdir_count * tiffDataWidth[dir->tdir_type];
	if (SeekOK(tif, dir->tdir_offset) &&
	    WriteOK(tif, cp, cc)) {
		tif->tif_dataoff += (cc + 1) & ~1;
		return (1);
	}
	TIFFError(tif->tif_name, "Error writing data for field \"%s\"",
	    TIFFFieldWithTag(dir->tdir_tag)->field_name);
	return (0);
}

/*
 * Link the current directory into the
 * directory chain for the file.
 */
static int
DECLARE1(TIFFLinkDirectory, TIFF*, tif)
{
	static const char module[] = "TIFFLinkDirectory";
	u_short dircount;
	long nextdir;

	tif->tif_diroff = (TIFFSeekFile(tif, 0L, L_XTND)+1) &~ 1L;
	if (tif->tif_header.tiff_diroff == 0) {
		/*
		 * First directory, overwrite header.
		 */
		tif->tif_header.tiff_diroff = tif->tif_diroff;
		(void) TIFFSeekFile(tif, 0L, L_SET);
		if (!WriteOK(tif, &tif->tif_header,
		    sizeof (tif->tif_header))) {
			TIFFError(tif->tif_name, "Error writing TIFF header");
			return (0);
		}
		return (1);
	}
	/*
	 * Not the first directory, search to the last and append.
	 */
	nextdir = tif->tif_header.tiff_diroff;
	do {
		if (!SeekOK(tif, nextdir) ||
		    !ReadOK(tif, &dircount, sizeof (dircount))) {
			TIFFError(module, "Error fetching directory count");
			return (0);
		}
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabShort(&dircount);
		TIFFSeekFile(tif, dircount * sizeof (TIFFDirEntry), L_INCR);
		if (!ReadOK(tif, &nextdir, sizeof (nextdir))) {
			TIFFError(module, "Error fetching directory link");
			return (0);
		}
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabLong((u_long *)&nextdir);
	} while (nextdir != 0);
	(void) TIFFSeekFile(tif, -sizeof (nextdir), L_INCR);
	if (!WriteOK(tif, &tif->tif_diroff, sizeof (tif->tif_diroff))) {
		TIFFError(module, "Error writing directory link");
		return (0);
	}
	return (1);
}
