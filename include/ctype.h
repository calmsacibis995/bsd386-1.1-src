/*	BSDI $Id: ctype.h,v 1.8 1994/02/07 20:04:20 polk Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * (c) UNIX System Laboratories, Inc.  All or some portions of this file
 * are derived from material licensed to the University of California by
 * American Telephone and Telegraph Co. or UNIX System Laboratories, Inc.
 * and are reproduced herein with the permission of UNIX System Laboratories,
 * Inc.
 *
 * This code is derived from software contributed to Berkeley by
 * Paul Borman at Krystal Technologies.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ctype.h	8.1 (Berkeley) 6/6/93
 */

#ifndef	_CTYPE_H_
#define _CTYPE_H_

#include <runetype.h>

#define	_A	0x00000100L		/* Alpha */
#define	_C	0x00000200L		/* Control */
#define	_D	0x00000400L		/* Digit */
#define	_G	0x00000800L		/* Graph */
#define	_L	0x00001000L		/* Lower */
#define	_P	0x00002000L		/* Punct */
#define	_S	0x00004000L		/* Space */
#define	_U	0x00008000L		/* Upper */
#define	_X	0x00010000L		/* X digit */
#define	_B	0x00020000L		/* Blank */
#define	_R	0x00040000L		/* Print */
#define	_I	0x00080000L		/* Ideogram */
#define	_T	0x00100000L		/* Special */
#define	_Q	0x00200000L		/* Phonogram */
#define	_SWM	0xf0000000L		/* Mask to get screen width data */
#define	_SWS	28			/* Bits to shift to get width */
#define	_SW0	0x00000000L		/* 0 width character */
#define	_SW1	0x10000000L		/* 1 width character */
#define	_SW2	0x20000000L		/* 2 width character */
#define	_SW3	0x30000000L		/* 2 width character */

#include <sys/cdefs.h>

__BEGIN_DECLS
int isalnum __P((_BSD_RUNE_T_));
int isalpha __P((_BSD_RUNE_T_));
int iscntrl __P((_BSD_RUNE_T_));
int isdigit __P((_BSD_RUNE_T_));
int isgraph __P((_BSD_RUNE_T_));
int islower __P((_BSD_RUNE_T_));
int isprint __P((_BSD_RUNE_T_));
int ispunct __P((_BSD_RUNE_T_));
int isspace __P((_BSD_RUNE_T_));
int isupper __P((_BSD_RUNE_T_));
int isxdigit __P((_BSD_RUNE_T_));
__END_DECLS

#define isalnum(c)      __istype((c), (_A|_D))
#define isalpha(c)      __istype((c),     _A)
#define iscntrl(c)      __istype((c),     _C)
#define isdigit(c)      __isctype((c),    _D)	/* ANSI -- locale independent */
#define isgraph(c)      __istype((c),     _G)
#define islower(c)      __istype((c),     _L)
#define isprint(c)      __istype((c),     _R)
#define ispunct(c)      __istype((c),     _P)
#define isspace(c)      __istype((c),     _S)
#define isupper(c)      __istype((c),     _U)
#define isxdigit(c)     __isctype((c),    _X)	/* ANSI -- locale independent */

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_SOURCE)
__BEGIN_DECLS
int isascii __P((_BSD_RUNE_T_));
_BSD_RUNE_T_ toascii __P((_BSD_RUNE_T_));
int digittoint __P((_BSD_RUNE_T_));
int scrwidth __P((_BSD_RUNE_T_));
int isideogram __P((_BSD_RUNE_T_));
int isphonogram __P((_BSD_RUNE_T_));
int isspecial __P((_BSD_RUNE_T_));
int isblank __P((_BSD_RUNE_T_));
int isrune __P((_BSD_RUNE_T_));
int isnumber __P((_BSD_RUNE_T_));
int ishexnumber __P((_BSD_RUNE_T_));
__END_DECLS

#define	isascii(c)	((c & ~0x7F) == 0)
#define toascii(c)	((c) & 0x7F)
#define	digittoint(c)	__istype((c), 0xFF)
#define	scrwidth(c)	(__istype((c), _SWM) >> _SWS)
#define	isideogram(c)	__istype((c), _I)
#define	isphonogram(c)	__istype((c), _T)
#define	isspecial(c)	__istype((c), _Q)
#define isblank(c)	__istype((c), _B)
#define	isrune(c)	__istype((c),  0x0FFFFF00L)
#define	isnumber(c)	__istype((c), _D)
#define	ishexnumber(c)	__istype((c), _X)
#endif

/* See comments in <machine/ansi.h> about _BSD_RUNE_T_. */
__BEGIN_DECLS
unsigned long	___runetype __P((_BSD_RUNE_T_));
_BSD_RUNE_T_	___tolower __P((_BSD_RUNE_T_));
_BSD_RUNE_T_	___toupper __P((_BSD_RUNE_T_));
__END_DECLS

/*
 * If your compiler supports prototypes and inline functions,
 * #define _USE_CTYPE_INLINE_.  Otherwise, use the C library
 * functions.
 */
#if defined(__cplusplus) || \
    !defined(_USE_CTYPE_CLIBRARY_) && defined(__GNUC__)
#define	_USE_CTYPE_INLINE_	1
#endif

#if defined(_USE_CTYPE_INLINE_)
static __inline int
__istype(_BSD_RUNE_T_ c, unsigned long f)
{
	return((((c & _CRMASK) ? ___runetype(c) :
	    _CurrentRuneLocale->runetype[c]) & f) ? 1 : 0);
}

static __inline int
__isctype(_BSD_RUNE_T_ c, unsigned long f)
{
	return((((c & _CRMASK) ? 0 :
	    _DefaultRuneLocale.runetype[c]) & f) ? 1 : 0);
}

/* _ANSI_LIBRARY is defined by lib/libc/gen/isctype.c. */
#if !defined(_ANSI_LIBRARY)
static __inline _BSD_RUNE_T_
toupper(_BSD_RUNE_T_ c)
{
	return((c & _CRMASK) ?
	    ___toupper(c) : _CurrentRuneLocale->mapupper[c]);
}

static __inline _BSD_RUNE_T_
tolower(_BSD_RUNE_T_ c)
{
	return((c & _CRMASK) ?
	    ___tolower(c) : _CurrentRuneLocale->maplower[c]);
}
#endif /* !_ANSI_LIBRARY */

#else /* !_USE_CTYPE_INLINE_ */

__BEGIN_DECLS
int		__istype __P((_BSD_RUNE_T_, unsigned long));
int		__isctype __P((_BSD_RUNE_T_, unsigned long));
_BSD_RUNE_T_	toupper __P((_BSD_RUNE_T_));
_BSD_RUNE_T_	tolower __P((_BSD_RUNE_T_));
__END_DECLS
#endif /* _USE_CTYPE_INLINE_ */

#endif /* !_CTYPE_H_ */
