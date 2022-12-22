/* $Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/libtiff/tiffcomp.h,v 1.1.1.1 1994/01/14 23:10:03 torek Exp $ */

/*
 * Copyright (c) 1990, 1991, 1992 Sam Leffler
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

#ifndef _COMPAT_
#define	_COMPAT_
/*
 * This file contains a hodgepodge of definitions and
 * declarations that are needed to provide compatibility
 * between the native system and the base implementation
 * that the library assumes.
 *
 * NB: This file is a mess.
 */

/*
 * If nothing has been specified about the use of function prototypes,
 * deduce it's use from thje compilation environment.  Note that if
 * we decide protototypes should be used, we also assume that const is
 * supported.  If this is not true, then the user must force the
 * definitions in the Makefile or similar.
 */
#if !defined(USE_PROTOTYPES)
#if defined(__STDC__) || defined(__EXTENDED__) || defined(c_plusplus) || defined(__cplusplus)
#define	USE_PROTOTYPES	1
#define	USE_CONST	1
#endif /* __STDC__ || __EXTENDED__ || c_plusplus || __cplusplus */
#endif /* !defined(USE_PROTOTYPES) */

/*
 * If nothing has been specified about the
 * use of const, define it to be void.
 */
#if !USE_CONST
#if !defined(const)
#define	const
#endif
#endif

/*
 * Setup basic type definitions and function declaratations.
 */
#ifdef THINK_C
#include <unix.h>
#include <math.h>
#endif
#if USE_PROTOTYPES
#include <stdio.h>
#endif
#ifdef applec
#include <types.h>
#else
#ifndef THINK_C
#include <sys/types.h>
#endif
#endif
#ifdef VMS
#include <file.h>
#include <unixio.h>
#else
#include <fcntl.h>
#endif
#if defined(THINK_C) || defined(applec)
#include <stdlib.h>
typedef long off_t;
#define	BSDTYPES
#endif

/*
 * Workarounds for BSD lseek definitions.
 */
#if defined(SYSV) || defined(VMS)
#if defined(SYSV)
#include <unistd.h>
#endif
#define	L_SET	SEEK_SET
#define	L_INCR	SEEK_CUR
#define	L_XTND	SEEK_END
#endif /* defined(SYSV) || defined(VMS) */
#ifndef L_SET
#define L_SET	0
#define L_INCR	1
#define L_XTND	2
#endif

/*
 * The library uses memset, memcpy, and memcmp.
 * ANSI C and System V define these in string.h.
 */
#include <string.h>

/*
 * The BSD typedefs are used throughout the library.
 * If your system doesn't have them in <sys/types.h>,
 * then define BSDTYPES in your Makefile.
 */
#ifdef BSDTYPES
typedef	unsigned char u_char;
typedef	unsigned short u_short;
typedef	unsigned int u_int;
typedef	unsigned long u_long;
#endif

/*
 * Default Read/Seek/Write definitions.  These aren't really
 * compatibility related and should probably go in tiffiop.h.
 */
#ifndef ReadOK
#define	ReadOK(tif, buf, size) \
	(TIFFReadFile(tif, (char *)buf, size) == size)
#endif
#ifndef SeekOK
#define	SeekOK(tif, off) \
	(TIFFSeekFile(tif, (long)off, L_SET) == (long)off)
#endif
#ifndef WriteOK
#define	WriteOK(tif, buf, size) \
	(TIFFWriteFile(tif, (char *)buf, size) == size)
#endif

/*
 * dblparam_t is the type that a double precision
 * floating point value will have on the parameter
 * stack (when coerced by the compiler).
 */
#ifdef applec
typedef extended dblparam_t;
#else
typedef double dblparam_t;
#endif

/*
 * Varargs parameter list handling...YECH!!!!
 */
#if defined(__STDC__) && !defined(USE_VARARGS)
#define	USE_VARARGS	0
#endif

#if defined(USE_VARARGS)
#if USE_VARARGS
#include <varargs.h>
#define	VA_START(ap, parmN)	va_start(ap)
#else
#include <stdarg.h>
#define	VA_START(ap, parmN)	va_start(ap, parmN)
#endif
#endif /* defined(USE_VARARGS) */

/*
 * Macros for declaring functions with and without
 * prototypes--according to the definition of USE_PROTOTYPES.
 */
#if USE_PROTOTYPES
#define	DECLARE1(f,t1,a1)		f(t1 a1)
#define	DECLARE2(f,t1,a1,t2,a2)		f(t1 a1, t2 a2)
#define	DECLARE3(f,t1,a1,t2,a2,t3,a3)	f(t1 a1, t2 a2, t3 a3)
#define	DECLARE4(f,t1,a1,t2,a2,t3,a3,t4,a4)\
	f(t1 a1, t2 a2, t3 a3, t4 a4)
#define	DECLARE5(f,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5)\
	f(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5)
#define	DECLARE6(f,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6)\
	f(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6)
#define	DECLARE1V(f,t1,a1)		f(t1 a1 ...)
#define	DECLARE2V(f,t1,a1,t2,a2)	f(t1 a1, t2 a2, ...)
#define	DECLARE3V(f,t1,a1,t2,a2,t3,a3)	f(t1 a1, t2 a2, t3 a3, ...)
#else
#define	DECLARE1(f,t1,a1)		f(a1) t1 a1;
#define	DECLARE2(f,t1,a1,t2,a2)		f(a1,a2) t1 a1; t2 a2;
#define	DECLARE3(f,t1,a1,t2,a2,t3,a3)	f(a1, a2, a3) t1 a1; t2 a2; t3 a3;
#define	DECLARE4(f,t1,a1,t2,a2,t3,a3,t4,a4) \
	f(a1, a2, a3, a4) t1 a1; t2 a2; t3 a3; t4 a4;
#define	DECLARE5(f,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5)\
	f(a1, a2, a3, a4, a5) t1 a1; t2 a2; t3 a3; t4 a4; t5 a5;
#define	DECLARE6(f,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6)\
	f(a1, a2, a3, a4, a5, a6) t1 a1; t2 a2; t3 a3; t4 a4; t5 a5; t6 a6;
#if USE_VARARGS
#define	DECLARE1V(f,t1,a1) \
	f(a1, va_alist) t1 a1; va_dcl
#define	DECLARE2V(f,t1,a1,t2,a2) \
	f(a1, a2, va_alist) t1 a1; t2 a2; va_dcl
#define	DECLARE3V(f,t1,a1,t2,a2,t3,a3) \
	f(a1, a2, a3, va_alist) t1 a1; t2 a2; t3 a3; va_dcl
#else
"Help, I don't know how to handle this case: !USE_PROTOTYPES and !USE_VARARGS?"
#endif
#endif
#endif /* _COMPAT_ */
