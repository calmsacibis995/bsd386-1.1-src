#ifndef lint
static char rcsid[] = "$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/libtiff/tif_apple.c,v 1.1.1.1 1994/01/14 23:10:04 torek Exp $";
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
 * TIFF Library Macintosh-specific Routines.
 *
 * These routines use only the Toolbox and high level File Manager traps.
 * They make no calls to the THINK C "unix" compatibility library.  Also,
 * malloc is not used directly but it is still referenced internally by
 * in the ANSI library in rare cases.  Heap fragmentation by the malloc ring
 * buffer is therefore minimized.
 *
 * O_RDONLY and O_RDWR are treated identically here.  The tif_mode flag is
 * checked in TIFFWriteCheck().
 *
 * Create below fills in a blank creator signature and sets the file type
 * to 'TIFF'.  It is much better for the application to do this by Create'ing
 * the file first and TIFFOpen'ing it later.
 */

#include "tiffiop.h"
#include <Errors.h>
#include <Files.h>
#include <Memory.h>

#ifdef applec
#define	CtoPstr	c2pstr
#endif

static int
_tiffReadProc(void *fd, char* buf, unsigned long size)
{
	return (FSRead((short) fd, (long *) &size, buf) == noErr ? size : -1);
}

static int
_tiffWriteProc(void *fd, char* buf, unsigned long size)
{
	return (FSWrite((short) fd, (long *) &size, buf) == noErr ? size : -1);
}

static long
_tiffSeekProc(void *fd, long off, int whence)
{
	long fpos, size;

	if (GetEOF((short) fd, &size) != noErr)
		return EOF;
	(void) GetFPos((short) fd, &fpos);

	switch (whence) {
	case SEEK_CUR:
		if (off + fpos > size)
			SetEOF((short) fd, off + fpos);
		if (SetFPos((short) fd, fsFromMark, off) != noErr)
			return EOF;
		break;
	case SEEK_END:
		if (off > 0)
			SetEOF((short) fd, off + size);
		if (SetFPos((short) fd, fsFromLEOF, off) != noErr)
			return EOF;
		break;
	case SEEK_SET:
		if (off > size)
			SetEOF((short) fd, off);
		if (SetFPos((short) fd, fsFromLEOF, off - size) != noErr)
			return EOF;
		break;
	}

	return (GetFPos((short) fd, &fpos) == noErr ? fpos : EOF);
}

static int
_tiffMapProc(void *fd, char **pbase, long *psize)
{
	return (0);
}

static void
_tiffUnmapProc(void *fd, char *base, long size)
{
}

static int
_tiffCloseProc(void *fd)
{
	return (FSClose((short) fd));
}

static long
_tiffSizeProc(void *fd)
{
	long size;

	if (GetEOF((short) fd, &size) != noErr) {
		TIFFError("_tiffSizeProc", "%s: Cannot get file size");
		return (-1L);
	}
	return (size);
}

/*
 * TODO: use Multifinder temporary memory for large strip chunks.
 */
void *
_TIFFmalloc(size_t s)
{
	return (NewPtr(s));
}

void
_TIFFfree(void* p)
{
	DisposePtr(p);
}

void *
_TIFFrealloc(void* p, size_t s)
{
	Ptr n = p;

	SetPtrSize(p, s);
	if (MemError() && (n = NewPtr(s)) != NULL) {
		BlockMove(p, n, GetPtrSize(p));
		DisposePtr(p);
	}
	return (n);
}

/*
 * Open a TIFF file descriptor for read/writing.
 */
TIFF *
TIFFFdOpen(int fd, const char *name, const char *mode)
{
	TIFF *tif;

	tif = TIFFClientOpen(name, mode, (void *) fd,
	    _tiffReadProc, _tiffWriteProc, _tiffSeekProc, _tiffCloseProc,
	    _tiffSizeProc, _tiffMapProc, _tiffUnmapProc);
	if (tif)
		tif->tif_fd = fd;
	return (tif);
}

/*
 * Open a TIFF file for read/writing.
 */
TIFF *
TIFFOpen(const char *name, const char *mode)
{
	static const char module[] = "TIFFOpen";
	Str255 pname;
	FInfo finfo;
	short fref;
	OSErr err;

	strcpy((char *) pname, name);
	CtoPstr(pname);

	switch (_TIFFgetMode(mode, module)) {
	default:
		return ((TIFF *) 0);
	case O_RDWR | O_CREAT | O_TRUNC:
		if (GetFInfo(pname, 0, &finfo) == noErr)
			FSDelete(pname, 0);
		/* fall through */
	case O_RDWR | O_CREAT:
		if ((err = GetFInfo(pname, 0, &finfo)) == fnfErr) {
			if (Create(pname, 0, '    ', 'TIFF') != noErr)
				goto badCreate;
			if (FSOpen(pname, 0, &fref) != noErr)
				goto badOpen;
		} else if (err == noErr) {
			if (FSOpen(pname, 0, &fref) != noErr)
				goto badOpen;
		} else
			goto badOpen;
		break;
	case O_RDONLY:
	case O_RDWR:
		if (FSOpen(pname, 0, &fref) != noErr)
			goto badOpen;
		break;
	}
	return (TIFFFdOpen((int) fref, name, mode));
badCreate:
	TIFFError(module, "%s: Cannot create", name);
	return ((TIFF *) 0);
badOpen:
	TIFFError(module, "%s: Cannot open", name);
	return ((TIFF *) 0);
}
