/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/UUCPLock.c++,v 1.1.1.1 1994/01/14 23:09:49 torek Exp $
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
#include <stdlib.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <pwd.h>
#ifdef svr4
extern "C" {
#include <sys/mkdev.h>
}
#endif

#include "UUCPLock.h"
#include "config.h"

/*
 * UUCP Device Locking Support.
 */

extern	void fxFatal(const char* va_alist ...);

UUCPLock::UUCPLock(const char* device)
{
    setupIDs();
    char pathname[1024];
#ifdef svr4
    struct stat sb;
    (void) stat(device, &sb);
    sprintf(pathname, "%s/%s%03d.%03d.%03d", UUCP_LOCKDIR, UUCP_LOCKPREFIX,
	major(sb.st_dev), major(sb.st_rdev), minor(sb.st_rdev));
#else
    const char* cp = strrchr(device, '/');
    sprintf(pathname, "%s/%s%s", UUCP_LOCKDIR, UUCP_LOCKPREFIX,
	cp ? cp+1 : device);
#endif
    file = pathname;
    locked = FALSE;
}

UUCPLock::~UUCPLock()
{
    unlock();
}

uid_t UUCPLock::UUCPuid = (uid_t) -1;
gid_t UUCPLock::UUCPgid = (gid_t) -1;

void
UUCPLock::setupIDs()
{
    if (UUCPuid == (uid_t) -1) {
	passwd *pwd = getpwnam("uucp");
	if (!pwd)
	    fxFatal("Can not deduce identity of UUCP");
	UUCPuid = pwd->pw_uid;
	UUCPgid = pwd->pw_gid;
	endpwent();			// paranoia
    }
}
uid_t UUCPLock::getUUCPUid() { setupIDs(); return UUCPuid; }
gid_t UUCPLock::getUUCPGid() { setupIDs(); return UUCPgid; }

/*
 * Create a lock file.
 */
fxBool
UUCPLock::create()
{	
    /*
     * We create a separate file and link it to
     * the destination to avoid a race condition.
     */
    fxStr tmpS(UUCP_LOCKDIR);
    tmpS.append("/TM.faxXXXXXX");
    char* tmp = tmpS;			// NB: for systems w/o prototypes
    int fd = mkstemp(tmp);
    if (fd >= 0) {
	(void) writeData(fd);
#if HAS_FCHMOD
	(void) fchmod(fd, UUCP_LOCKMODE);
#else
	(void) chmod(tmp, UUCP_LOCKMODE);
#endif
#if HAS_FCHOWN
	(void) fchown(fd, UUCPuid, UUCPgid);
#else
	(void) chown(tmp, UUCPuid, UUCPgid);
#endif
	(void) close(fd);

	locked = (link(tmp, (char*) file) == 0);
	(void) unlink(tmp);
    }
    return (locked);
}

/*
 * Check if the lock file is
 * newer than the specified age.
 */
fxBool
UUCPLock::isNewer(time_t age)
{
    struct stat sb;
    if (stat((char*) file, &sb) != 0)
	return (FALSE);
    return ((time(0) - sb.st_ctime) < age);
}

/*
 * Create a lock file.  If one already exists, the create
 * time is checked for older than the age time (atime).
 * If it is older, an attempt is made to unlink it and
 * create a new one.
 */
fxBool
UUCPLock::lock()
{
    if (locked)
	return (FALSE);
    uid_t ouid = geteuid();
    seteuid(0);				// need to be root
    fxBool ok = create();
    if (!ok)
	ok = check() && create();
    seteuid(ouid);
    return (ok);
}

/*
 * Unlock the device.
 */
void
UUCPLock::unlock()
{
    if (locked) {
	uid_t ouid = geteuid();
	seteuid(0);			// need to be root
	(void) unlink((char*) file);
	seteuid(ouid);
	locked = FALSE;
    }
}

/*
 * Check if the owning process exists.
 */
fxBool
UUCPLock::ownerExists(int fd)
{
    pid_t pid;
    return readData(fd, pid) && (kill(pid, 0) == 0 || errno != ESRCH);
}

/*
 * Check to see if the lock exists and is still active.
 * Locks are automatically expired after UUCP_LCKTIMEOUT seconds,
 * or if the process owner is no longer around.
 */
fxBool
UUCPLock::check()
{
    int fd = open((char*) file, O_RDONLY);
    if (fd != -1) {
#if UUCP_LCKTIMEOUT > 0
	if (isNewer(UUCP_LCKTIMEOUT) && ownerExists(fd)) {
	    close(fd);
	    return (FALSE);
	}
	close(fd);
	return (unlink((char*) file) == 0);
#else
	close(fd);
	return (FALSE);
#endif
    }
    return (TRUE);
}

/*
 * ASCII lock file interface.
 */
AsciiUUCPLock::AsciiUUCPLock(const char* device) : UUCPLock(device),
    data(UUCP_PIDDIGITS+2)
{
    sprintf((char*) data, "%*d\n", UUCP_PIDDIGITS, getpid());
}

AsciiUUCPLock::~AsciiUUCPLock()
{
}

fxBool
AsciiUUCPLock::writeData(int fd)
{
    return write(fd, (char*) data, UUCP_PIDDIGITS+1) == (UUCP_PIDDIGITS+1);
}

fxBool
AsciiUUCPLock::readData(int fd, pid_t& pid)
{
    char buf[UUCP_PIDDIGITS+1];
    if (read(fd, buf, UUCP_PIDDIGITS) == UUCP_PIDDIGITS) {
	buf[UUCP_PIDDIGITS] = '\0';
	pid = atoi(buf);
	return (TRUE);
    } else
	return (FALSE);
}

/*
 * Binary lock file interface.
 */

BinaryUUCPLock::BinaryUUCPLock(const char* device) : UUCPLock(device)
{
    data = getpid();		// binary pid of lock holder
}

BinaryUUCPLock::~BinaryUUCPLock()
{
}

fxBool
BinaryUUCPLock::writeData(int fd)
{
    return write(fd, &data, sizeof (data)) == sizeof (data);
}

fxBool
BinaryUUCPLock::readData(int fd, pid_t& pid)
{
    int data;
    if (read(fd, &data, sizeof (data)) == sizeof (data)) {
	pid = data;
	return (TRUE);
    } else
	return (FALSE);
}

UUCPLock*
OSnewUUCPLock(const char* device)
{
#if UUCP_LOCKTYPE == 1
    return new BinaryUUCPLock(device);
#else
    return new AsciiUUCPLock(device);
#endif
}
