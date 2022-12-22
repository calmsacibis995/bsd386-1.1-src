/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/GettySysV.h,v 1.1.1.1 1994/01/14 23:09:44 torek Exp $
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
#ifndef _SYSVGETTY_
#define	_SYSVGETTY_
/*
 * Syste V-style getty support that
 * just invokes the getty program.
 */
#include "Getty.h"

struct utmp;

class SysVGetty : public Getty {
private:
    int		exitStatus;	// login's exit status

    void setupSession(int modemFd);

    void loginAccount();
    void writeWtmp(utmp* ut);
public:
    SysVGetty(const fxStr& dev, const fxStr& speed, u_int timeout = 60);
    ~SysVGetty();

    static fxBool isSysVGetty();

    fxBool wait(int& status, fxBool block = FALSE);
    void hangup();
};
#endif /* _SYSVGETTY_ */
