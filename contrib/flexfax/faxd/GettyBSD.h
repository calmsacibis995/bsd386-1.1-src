/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/GettyBSD.h,v 1.1.1.1 1994/01/14 23:09:44 torek Exp $
/*
 * Copyright (c) 1993 Sam Leffler
 * Copyright (c) 1993 Silicon Graphics, Inc.
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
#ifndef _BSDGETTY_
#define	_BSDGETTY_
/*
 * BSD-style getty support that
 * just invokes the getty program.
 */
#include "Getty.h"

struct utmp;

class BSDGetty : public Getty {
private:
    fxStr	argbuf;		// argv values
    const char** argv;		// getty argv array

    void setupState(const char* args);
    void setupSession(int modemFd);
    void writeWtmp(utmp* ut);
    void logout(const char* line);
public:
    BSDGetty(const fxStr& dev, const fxStr& speed, u_int timeout = 60);
    ~BSDGetty();

    static fxBool isBSDGetty();

    fxBool wait(int& status, fxBool block = FALSE);
    void hangup();
};
#endif /* _BSDGETTY_ */
