/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/HDLCFrame.h,v 1.1.1.1 1994/01/14 23:09:45 torek Exp $
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
#ifndef _HDLCFRAME_
#define	_HDLCFRAME_
/*
 * Raw HDLC Frame Interface.
 */
#include "Types.h"

class HDLCFrame {
public:
    HDLCFrame(u_int frameOverhead);
    HDLCFrame(const HDLCFrame& fr);
    ~HDLCFrame();

    void put(u_char c);			// Put the character "c" into the buffer
    void put(const u_char* c, u_int len);// Put bunch of bytes in the buffer
    void reset();			// Reset buffer to empty
    u_int getLength() const;		// Return number of bytes in buffer

    operator u_char*();			// Return base of buffer
    u_char& operator[](u_int i) const;	// NB: no bounds checking

    fxBool moreFrames() const;		// more frames follow
    fxBool isOK() const;		// frame has good FCS
    void setOK(fxBool b);		// set frame FCS indicator
    u_int getFCF() const;		// FCF in frame
    const u_char* getFrameData() const;	// data following FCF
    u_int getFrameDataLength() const;	// length of data - (FCF+FCS)
    u_int getDataWord() const;		// first 0-4 bytes of data
    u_int getDIS() const;		// DIS/DCS data
    u_int getXINFO() const;		// extended info after DIS/DCS
protected:
    u_char	buf[2048];		// large enough for TCF at 9600 baud
    u_char*	next;
    u_char*	end;
    u_char*	base;
    u_short	amountToGrowBy;
    u_short	frameOverhead;		// # bytes for leader & FCS
    fxBool	ok;			// FCS correct

    void addc(u_char c);		// make room & add a char to the buffer
    void grow(u_int amount);		// make more room in the buffer
};

inline void HDLCFrame::put(u_char c)
    { if (next < end) *next++ = c; else addc(c); }
inline void HDLCFrame::reset()			    { next = base; ok = FALSE; }
inline u_int HDLCFrame::getLength() const	    { return next - base; }

inline HDLCFrame::operator u_char*()		    { return base; }
inline u_char& HDLCFrame::operator[](u_int i) const { return base[i]; }

inline fxBool HDLCFrame::moreFrames() const
    { return ((*this)[1]&0x08) == 0; }
inline fxBool HDLCFrame::isOK() const		     { return ok; }
inline void HDLCFrame::setOK(fxBool b)		     { ok = b; }
inline u_int HDLCFrame::getFCF() const		     { return (*this)[2]; }
inline const u_char* HDLCFrame::getFrameData() const { return &((*this)[3]); }
inline u_int HDLCFrame::getFrameDataLength() const
    { return getLength() - frameOverhead; }
#endif /* _HDLCFRAME_ */
