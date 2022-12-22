/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/util/StackBuffer.h,v 1.1.1.1 1994/01/14 23:10:49 torek Exp $
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
#ifndef _StackBuffer_
#define	_StackBuffer_

#include "Types.h"

// A growable buffer of characters, designed to be used on the cpu stack.
// It contains an internal buffer, and avoids allocating memory from
// freestore, unless the buffer is insufficient to the task at hand.

class fxStackBuffer {
public:
    fxStackBuffer(u_int amountToGrowBy = 0);
    fxStackBuffer(const fxStackBuffer& sb);
    ~fxStackBuffer();

    void put(char c);			// Put the character "c" into the buffer
    void put(char const* c, u_int len);	// Put bunch of bytes in the buffer
    void put(char const* c);
    void set(char c);			// Stick in char w/o extending length
    void reset();			// Reset buffer to empty
    u_int getLength() const;		// Return number of bytes in buffer

    // NB: the buffer is *NOT* null terminated, unless you put one there.
    operator char*();			// Return base of buffer
protected:
    char	buf[1000];
    char*	next;
    char*	end;
    char*	base;
    u_int	amountToGrowBy;

    void addc(char c);			// make room & add a char to the buffer
    void grow(u_int amount);		// make more room in the buffer
};

inline void fxStackBuffer::put(char c)
    { if (next < end) *next++ = c; else addc(c); }
inline void fxStackBuffer::put(char const* c)	{ put(c, strlen(c)); }
inline void fxStackBuffer::set(char c)		{ put(c); next--; }
inline void fxStackBuffer::reset()		{ next = base; }
inline fxStackBuffer::operator char*()		{ return base; }
inline u_int fxStackBuffer::getLength() const	{ return next - base; }
#endif /* _StackBuffer_ */
