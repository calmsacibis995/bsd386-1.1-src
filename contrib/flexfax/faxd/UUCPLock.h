/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/UUCPLock.h,v 1.1.1.1 1994/01/14 23:09:45 torek Exp $
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
#ifndef _UUCPLOCK_
#define	_UUCPLOCK_
/*
 * UUCP Locking Support.
 */
#include <unistd.h>
#include "Str.h"

class UUCPLock {
private:
    fxStr	file;			// lock file pathname
    fxBool	locked;			// is lock currently held

    static uid_t UUCPuid;
    static gid_t UUCPgid;
    static void setupIDs();

    fxBool	create();		// create lock file
    fxBool	isNewer(time_t age);	// is lock file newer than age
    fxBool	ownerExists(int fd);	// does owning process exist
protected:
    UUCPLock(const char* device);	// must use derived class

    virtual fxBool writeData(int fd) = 0;
    virtual fxBool readData(int fd, pid_t& pid) = 0;
public:
    virtual ~UUCPLock();

    static uid_t getUUCPUid();
    static gid_t getUUCPGid();

    fxBool	lock();			// lock device
    void	unlock();		// unlock device
    fxBool	check();		// check if device is locked
};

/*
 * Lock files with ascii contents (System V style).
 */
class AsciiUUCPLock : public UUCPLock {
private:
    fxStr	data;			// data to write record in lock file

    fxBool writeData(int fd);
    fxBool readData(int fd, pid_t& pid);
public:
    AsciiUUCPLock(const char* device);
    ~AsciiUUCPLock();
};

/*
 * Lock files with binary contents (BSD style).
 */
class BinaryUUCPLock : public UUCPLock {
private:
    int		data;			// data to write record in lock file

    fxBool writeData(int fd);
    fxBool readData(int fd, pid_t& pid);
public:
    BinaryUUCPLock(const char* device);
    ~BinaryUUCPLock();
};

extern UUCPLock* OSnewUUCPLock(const char* device);
#endif /* _UUCPLOCK_ */
