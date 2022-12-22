#ifndef lint
static char rcsid[] = "$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/libtiff/tif_unix.c,v 1.1.1.1 1994/01/14 23:10:08 torek Exp $";
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
 * TIFF Library UNIX-specific Routines.
 */
#include "tiffiop.h"

static int
DECLARE3(_tiffReadProc, void*, fd, char*, buf, u_long, size)
{
	return (read((int) fd, buf, size));
}

static int
DECLARE3(_tiffWriteProc, void*, fd, char*, buf, u_long, size)
{
	return (write((int) fd, buf, size));
}

static long
DECLARE3(_tiffSeekProc, void*, fd, long, off, int, whence)
{
	return (lseek((int) fd, off, whence));
}

static int
DECLARE1(_tiffCloseProc, void*, fd)
{
	return (close((int) fd));
}

#include <sys/stat.h>

static long
DECLARE1(_tiffSizeProc, void*, fd)
{
	struct stat sb;
	return (fstat((int) fd, &sb) < 0 ? 0 : sb.st_size);
}

#ifdef MMAP_SUPPORT
#include <sys/mman.h>

static int
DECLARE3(_tiffMapProc, void*, fd, char**, pbase, long*, psize)
{
	long size = _tiffSizeProc(fd);
	if (size != -1) {
		*pbase = (char*)
		    mmap(0, size, PROT_READ, MAP_SHARED, (int) fd, 0);
		if (*pbase != (char *)-1) {
			*psize = size;
			return (1);
		}
	}
	return (0);
}

static void
DECLARE3(_tiffUnmapProc, void*, fd, char*, base, long, size)
{
	(void) munmap(base, size);
}
#else /* !MMAP_SUPPORT */
static int
DECLARE3(_tiffMapProc, void*, fd, char**, pbase, long*, psize)
{
	return (0);
}

static void
DECLARE3(_tiffUnmapProc, void*, fd, char*, base, long, size)
{
}
#endif /* !MMAP_SUPPORT */

/*
 * Open a TIFF file descriptor for read/writing.
 */
TIFF*
DECLARE3(TIFFFdOpen, int, fd, const char*, name, const char*, mode)
{
	TIFF *tif;

	tif = TIFFClientOpen(name, mode,
	    (void*) fd,
	    _tiffReadProc, _tiffWriteProc,
	    _tiffSeekProc, _tiffCloseProc, _tiffSizeProc,
	    _tiffMapProc, _tiffUnmapProc);
	if (tif)
		tif->tif_fd = fd;
	return (tif);
}

/*
 * Open a TIFF file for read/writing.
 */
TIFF*
DECLARE2(TIFFOpen, const char*, name, const char*, mode)
{
	static const char module[] = "TIFFOpen";
	int m, fd;

	m = _TIFFgetMode(mode, module);
	if (m == -1)
		return ((TIFF *)0);
	fd = open(name, m, 0666);
	if (fd < 0) {
		TIFFError(module, "%s: Cannot open", name);
		return ((TIFF *)0);
	}
	return (TIFFFdOpen(fd, name, mode));
}

#if defined(__MACH__) || defined(THINK_C)
extern	void *malloc(size_t size);
extern	void *realloc(void *ptr, size_t size);
#else /* !__MACH__ && !THINK_C */
#if defined(_IBMR2)
#include <stdlib.h>
#else /* !_IBMR2 */
extern	char *malloc();
extern	char *realloc();
#endif /* _IBMR2 */
#endif /* !__MACH__ */

void *
DECLARE1(_TIFFmalloc, size_t, s)
{
	return (malloc(s));
}

void
DECLARE1(_TIFFfree, void*, p)
{
	free(p);
}

void *
DECLARE2(_TIFFrealloc, void*, p, size_t, s)
{
	return (realloc(p, s));
}
