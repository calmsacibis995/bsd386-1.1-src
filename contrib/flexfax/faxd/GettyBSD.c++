/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/GettyBSD.c++,v 1.1.1.1 1994/01/14 23:09:48 torek Exp $
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

/*
 * FAX Server BSD Getty Support.
 */
#include "GettyBSD.h"

#include <stddef.h>
#include <termios.h>
#include <osfcn.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <paths.h>
#include <time.h>
#include <utmp.h>

BSDGetty::BSDGetty(const fxStr& l, const fxStr& s, u_int t) : Getty(l,s,t)
{
}

BSDGetty::~BSDGetty()
{
}

fxBool
BSDGetty::isBSDGetty()
{
    return (access((char*) getty, X_OK) == 0);
}

Getty*
OSnewGetty(const fxStr& dev, const fxStr& speed)
{
    return (BSDGetty::isBSDGetty() ? new BSDGetty(dev, speed) : NULL);
}

/*
 * ``Open'' the device and setup the initial tty state
 * so that the normal stdio routines can be used.
 */
void
BSDGetty::setupSession(int modemFd)
{
    int fd;
    /*
     * Close everything down except the modem so
     * that the remote side doesn't get hung up on.
     */
    for (fd = getdtablesize()-1; fd >= 0; fd--)
	if (fd != modemFd)
	    (void) close(fd);
    /*
     * Now make the line be the controlling tty
     * and create a new process group/session for
     * the login process that follows.
     */
    fd = open("tty", 0);		// NB: assumes we're in /dev
    if (fd >= 0) {
	(void) ioctl(fd, TIOCNOTTY, 0);
	(void) close(fd);
    }
    (void) setsid();
    fd = open(getLine(), O_RDWR|O_NONBLOCK);
    if (fd != STDIN_FILENO)
	fatal("Can not setup \"%s\" as stdin", getLine());
    if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) &~ O_NONBLOCK))
	fatal("Can not reset O_NONBLOCK: %m");
    close(modemFd);			// done with this, pitch it

#ifdef TIOCSCTTY
    if (ioctl(fd, TIOCSCTTY, 0))
	fatal("Cannot set controlling tty: %m");
#endif
#ifdef TIOCGETDFLT
    /*
     * FaxServer::setBaudRate forces CLOCAL on for BSDi.  However
     * getty resets the terminal to the system default, if one exists,
     * prior to execing login, but preserves CLOCAL, if it is on.
     * Thus, if a system default setting exists for the tty, we
     * set the CLOCAL state from it; otherwise, we leave CLOCAL on.
     */
    struct termios term, dflt;
    if (ioctl(fd, TIOCGETDFLT, &dflt) == 0 && (dflt.c_cflag & CLOCAL) == 0) {
	(void) tcgetattr(fd, &term);
	term.c_cflag &= ~CLOCAL;
	(void) tcsetattr(fd, TCSAFLUSH, &term);
    }
#else
    /*
     * Turn off CLOCAL so that SIGHUP is sent on modem disconnect.
     */
    struct termios term;
    if (tcgetattr(fd, &term) == 0) {
	term.c_cflag &= ~CLOCAL;
	(void) tcsetattr(fd, TCSAFLUSH, &term);
    }
#endif
    /*
     * Setup descriptors for stdout, and stderr.
     * Establish the initial line termio settings and set
     * protection on the device file.
     */
    struct stat sb;
    (void) stat(getLine(), &sb);
    (void) chown(getLine(), 0, sb.st_gid);
    (void) chmod(getLine(), 0622);
    if (dup2(fd, STDOUT_FILENO) < 0)
	fatal("Unable to dup stdin to stdout: %m");
    if (dup2(fd, STDERR_FILENO) < 0)
	fatal("Unable to dup stdin to stderr: %m");
}

#ifdef __bsdi__
extern "C" int logout(const char*);
extern "C" int logwtmp(const char*, const char*, const char*);
#endif

#define	lineEQ(a,b)	(strncmp(a,b,sizeof(a)) == 0)

void
BSDGetty::writeWtmp(utmp* ut)
{
#ifndef __bsdi__
    int wfd = open(_PATH_WTMP, O_WRONLY|O_APPEND);
    if (wfd >= 0) {
	struct stat buf;
	if (fstat(wfd, &buf) == 0) {
	    memset(ut->ut_name, 0, sizeof (ut->ut_name));
	    memset(ut->ut_host, 0, sizeof (ut->ut_host));
	    ut->ut_time = time(0);
	    if (write(wfd, (char *)ut, sizeof (*ut)) != sizeof (*ut))
		(void) ftruncate(wfd, buf.st_size);
	}
	(void) close(wfd);
    }
#else
    logwtmp(ut->ut_line, "", "");
#endif
}

void
BSDGetty::logout(const char* line)
{
#ifndef __bsdi__
    int ufd = open(_PATH_UTMP, O_RDWR);
    if (ufd >= 0) {
	struct utmp ut;
	while (read(ufd, (char *)&ut, sizeof (ut)) == sizeof (ut))
	    if (ut.ut_name[0] && lineEQ(ut.ut_line, line)) {
		memset(ut.ut_name, 0, sizeof (ut.ut_name));
		memset(ut.ut_host, 0, sizeof (ut.ut_host));
		ut.ut_time = time(0);
		(void) lseek(ufd, -(long) sizeof (ut), SEEK_CUR);
		(void) write(ufd, (char *)&ut, sizeof (ut));
	    }
	(void) close(ufd);
    }
#else
    ::logout(line);
#endif
}

void
BSDGetty::hangup()
{
    // at this point we're root and we can reset state
    int ufd = open(_PATH_UTMP, O_RDONLY);
    if (ufd >= 0) {
	struct utmp ut;
	while (read(ufd, (char *)&ut, sizeof (ut)) == sizeof (ut))
	    if (ut.ut_name[0] && lineEQ(ut.ut_line, getLine())) {
		writeWtmp(&ut);
		break;
	    }
	(void) close(ufd);
    }
    logout(getLine());
    Getty::hangup();
}

fxBool
BSDGetty::wait(int& status, fxBool block)
{
    return (waitpid(getPID(), &status, block ? 0 : WNOHANG) == getPID());
}
