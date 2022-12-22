/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/util/Types.h,v 1.1.1.1 1994/01/14 23:10:50 torek Exp $
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
#ifndef _Types_
#define	_Types_

#include "string.h"
#include "assert.h"
#include "stdio.h"
#include "new.h"
#include "sys/types.h"
#include "port.h"

// Boolean type
typedef unsigned char fxBool;
#undef	TRUE
#define	TRUE	((fxBool)1)
#undef	FALSE
#define	FALSE	((fxBool)0)

// minimum of two numbers
inline int fxmin(int a, int b)		{ return (a < b) ? a : b; }
inline u_long fxmin(u_long a, u_long b)	{ return (a < b) ? a : b; }
inline u_int fxmin(u_int a, u_int b)	{ return (a < b) ? a : b; }

// maximum of two numbers
inline int fxmax(int a, int b)		{ return (a > b) ? a : b; }
inline u_long fxmax(u_long a, u_long b)	{ return (a > b) ? a : b; }
inline u_int fxmax(u_int a, u_int b)	{ return (a > b) ? a : b; }

#ifdef NDEBUG
#define fxAssert(EX,MSG)
#else
extern "C" void _fxassert(const char*, const char*, int);
#define fxAssert(EX,MSG) if (EX); else _fxassert(MSG,__FILE__,__LINE__);
#endif

//----------------------------------------------------------------------

// Use this macro at the end of a multi-line macro definition.  This
// helps eliminate the extra back slash problem.
#define __enddef__

// Some macros for namespace hacking.  These macros concatenate their
// arguments.
#ifdef	__ANSI_CPP__
#define fxIDENT(a) a
#define fxCAT(a,b) a##b
#define fxCAT2(a,b) a##b
#define fxCAT3(a,b,c) a##b##c
#define fxCAT4(a,b,c,d) a##b##c##d
#define fxCAT5(a,b,c,d,e) a##b##c##d##e
#else
#define fxIDENT(a) a
#define fxCAT(a,b) fxIDENT(a)b
#define fxCAT2(a,b) fxIDENT(a)b
#define fxCAT3(a,b,c) fxCAT2(a,b)c
#define fxCAT4(a,b,c,d) fxCAT3(a,b,c)d
#define fxCAT5(a,b,c,d,e) fxCAT4(a,b,c,d)e
#endif

//----------------------------------------------------------------------

// Workaround for g++ new(where) incompatibility (yech)

#if defined(__GNUC__) && !defined(NEW_FIXED)
#define	fxNEW(where)	new{where}
#else
#define	fxNEW(where)	new(where)
#endif

//----------------------------------------------------------------------

// Workaround for difficulties with signal handler definitions (yech)

#ifndef fxSIGHANDLER
#define	fxSIGHANDLER
#endif
#ifndef fxSIGVECHANDLER
#define	fxSIGVECHANDLER
#endif
#ifndef fxSIGACTIONHANDLER
#define	fxSIGACTIONHANDLER
#endif

#endif /* _Types_ */
