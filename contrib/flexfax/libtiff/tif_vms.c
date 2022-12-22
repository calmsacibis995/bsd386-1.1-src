#ifndef lint
static char rcsid[] = "$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/libtiff/tif_vms.c,v 1.1.1.1 1994/01/14 23:10:08 torek Exp $";
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
 * TIFF Library VMS-specific Routines.
 */
#include "tiffiop.h"

static int
_tiffReadProc(void* fd, char* buf, u_long size)
{
	return (read((int) fd, buf, size));
}

static int
_tiffWriteProc(void* fd, char* buf, u_long size)
{
	return (write((int) fd, buf, size));
}

static int
_tiffSeekProc(void* fd, long off, int whence)
{
	return (lseek((int) fd, off, whence));
}

static int
_tiffCloseProc(void* fd)
{
	return (close((int) fd));
}

#include <sys/stat.h>

static long
_tiffSizeProc(void* fd)
{
	struct stat sb;
	return (fstat((int) fd, &sb) < 0 ? 0 : sb.st_size);
}

#ifdef MMAP_SUPPORT
#include <fab.h>
#include <secdef.h>

/*
 * Table for storing information on current open sections. 
 * (Should really be a linked list)
 */
#define MAX_MAPPED 100
static int no_mapped = 0;
static struct {
	char *base;
	char *top;
	unsigned short channel;
} map_table[MAX_MAPPED];

/* 
 * This routine maps a file into a private section. Note that this 
 * method of accessing a file is by far the fastest under VMS.
 * The routine may fail (i.e. return 0) for several reasons, for
 * example:
 * - There is no more room for storing the info on sections.
 * - The process is out of open file quota, channels, ...
 * - fd does not describe an opened file.
 * - The file is already opened for write access by this process
 *   or another process
 * - There is no free "hole" in virtual memory that fits the
 *   size of the file
 */
static int
_tiffMapProc(void* fd, char** pbase, long* psize)
{
	char name[256];
	struct FAB fab;
	unsigned short channel;
	char *inadr[2], *retadr[2];
	unsigned long status;
	long size;
	
	if (no_mapped >= MAX_MAPPED)
		return(0);
	/*
	 * We cannot use a file descriptor, we
	 * must open the file once more.
	 */
	if (getname(fd, name, 1) == NULL)
		return(0);
	/* prepare the FAB for a user file open */
	fab = cc$rms_fab;
	fab.fab$v_ufo = 1;
	fab.fab$b_fac = FAB$M_GET;
	fab.fab$b_shr = FAB$M_SHRGET;
	fab.fab$l_fna = name;
	fab.fab$b_fns = strlen(name);
	status = sys$open(&fab);	/* open file & get channel number */
	if ((status&1) == 0)
		return(0);
	channel = (unsigned short)fab.fab$l_stv;
	inadr[0] = inadr[1] = (char *)0; /* just an address in P0 space */
	/*
	 * Map the blocks of the file up to
	 * the EOF block into virtual memory.
	 */
	size = _tiffSizeProc(fd);
	status = sys$crmpsc(inadr, retadr, 0, SEC$M_EXPREG, 0,0,0, channel,
		howmany(size,512), 0,0,0);
	if ((status&1) == 0){
		sys$dassgn(channel);
		return(0);
	}
	*pbase = retadr[0];		/* starting virtual address */
	/*
	 * Use the size of the file up to the
	 * EOF mark for UNIX compatibility.
	 */
	*psize = size;
	/* Record the section in the table */
	map_table[no_mapped].base = retadr[0];
	map_table[no_mapped].top = retadr[1];
	map_table[no_mapped].channel = channel;
	no_mapped++;

        return(1);
}

/*
 * This routine unmaps a section from the virtual address space of 
 * the process, but only if the base was the one returned from a
 * call to TIFFMapFileContents.
 */
static void
_tiffUnmapProc(void* fd, char* base, long size)
{
	char *inadr[2];
	int i, j;
	
	/* Find the section in the table */
	for (i = 0;i < no_mapped; i++) {
		if (map_table[i].base == base) {
			/* Unmap the section */
			inadr[0] = base;
			inadr[1] = map_table[i].top;
			sys$deltva(inadr, 0, 0);
			sys$dassgn(map_table[i].channel);
			/* Remove this section from the list */
			for (j = i+1; j < no_mapped; j++)
				map_table[j-1] = map_table[j];
			no_mapped--;
			return;
		}
	}
}
#else /* !MMAP_SUPPORT */
static int
_tiffMapProc(void* fd, char** pbase, long* psize)
{
	return (0);
}

static void
_tiffUnmapProc(void* fd, char* base, long size)
{
}
#endif /* !MMAP_SUPPORT */

/*
 * Open a TIFF file descriptor for read/writing.
 */
TIFF *
TIFFFdOpen(int fd, const char* name, const char* mode)
{
	TIFF *tif;

	tif = TIFFClientOpen(name, mode,
	    (void*) fd,
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
TIFFOpen(const char* name, const char* mode)
{
	static const char module[] = "TIFFOpen";
	int m, fd;

	m = _TIFFgetMode(mode, module);
	if (m == -1)
		return ((TIFF *)0);
        if (m&O_TRUNC){
                /*
		 * There is a bug in open in VAXC. If you use
		 * open w/ m=O_RDWR|O_CREAT|O_TRUNC the
		 * wrong thing happens.  On the other hand
		 * creat does the right thing.
                 */
                fd = creat(name, 0666,
		    "alq = 128", "deq = 64", "mbc = 32",  "fop = tef");
	} else if (m&O_RDWR) {
		fd = open(name, m, 0666,
		    "deq = 64", "mbc = 32", "fop = tef");
	} else
		fd = open(name, m, 0666, "mbc = 32");
	if (fd < 0) {
		TIFFError(module, "%s: Cannot open", name);
		return ((TIFF *)0);
	}
	return (TIFFFdOpen(fd, name, mode));
}

void *
_TIFFmalloc(size_t s)
{
	return (malloc(s));
}

void
_TIFFfree(void* p)
{
	free(p);
}

void *
_TIFFrealloc(void* p, size_t s)
{
	return (realloc(p, s));
}
