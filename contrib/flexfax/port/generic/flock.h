/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/port/generic/flock.h,v 1.1.1.1 1994/01/14 23:10:14 torek Exp $
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
#if !defined(_FLOCK_) && !defined(LOCK_SH)
#define	_FLOCK_
/* XXX can't include <sys/file.h> because of conflicts */
/*
 * Flock call.
 */
#define LOCK_SH        1    /* shared lock */
#define LOCK_EX        2    /* exclusive lock */
#define LOCK_NB        4    /* don't block when locking */
#define LOCK_UN        8    /* unlock */

#ifdef __cplusplus
extern "C" int flock(int, int);
#else
extern int flock(int, int);
#endif
#endif /* !_FLOCK && !LOCK_SH */
