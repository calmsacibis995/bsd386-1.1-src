/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/util/RegExArray.h,v 1.1.1.1 1994/01/14 23:10:49 torek Exp $
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
#ifndef _RegExArray_
#define	_RegExArray_
/*
 * Array of Regular Expressions.
 */
#include "Array.h"
#include <InterViews/regexp.h>

/*
 * Reference-counted regular expressions;
 * for use with Ptrs and Arrays.
 */
class RegEx : public Regexp {
public:
    RegEx(const char* pat);
    RegEx(const char* pat, int length);

    void inc();
    void dec();
    u_long getReferenceCount();
protected:
    u_long	referenceCount;
};
inline RegEx::RegEx(const char* pat) : Regexp(pat)
    { referenceCount = 0; };
inline RegEx::RegEx(const char* pat, int length) : Regexp(pat,length)
    { referenceCount = 0; }

inline u_long RegEx::getReferenceCount()	{ return referenceCount; }
inline void RegEx::inc()			{ referenceCount++; }
inline void RegEx::dec()
    { if (--referenceCount <= 0) delete this; }

fxDECLARE_Ptr(RegEx);
fxDECLARE_ObjArray(RegExArray, RegExPtr);
#endif /* _RegExArray_ */
