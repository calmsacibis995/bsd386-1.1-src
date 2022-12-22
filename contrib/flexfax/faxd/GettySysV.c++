/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/GettySysV.c++,v 1.1.1.1 1994/01/14 23:09:48 torek Exp $
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
#include <limits.h>
#include <stddef.h>
#include <termios.h>
#include <osfcn.h>
#include <paths.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <utmp.h>

#include "GettySysV.h"

#ifndef _PATH_WTMP
#define	_PATH_WTMP	WTMP_FILE
#endif

/*
 * FAX Server System V Getty Support.
 */

SysVGetty::SysVGetty(const fxStr& l, const fxStr& s, u_int t) : Getty(l,s,t)
{
}

SysVGetty::~SysVGetty()
{
}

fxBool
SysVGetty::isSysVGetty()
{
    return (access((char*) getty, X_OK) == 0);
}

Getty*
OSnewGetty(const fxStr& dev, const fxStr& speed)
{
    return (SysVGetty::isSysVGetty() ? new SysVGetty(dev, speed) : NULL);
}

/*
 * ``Open'' the device and setup the initial tty state
 * so that the normal stdio routines can be used.
 */
void
SysVGetty::setupSession(int modemFd)
{
    int fd;
    /*
     * Close everything down except the modem so
     * that the remote side doesn't get hung up on.
     */
    for (fd = _POSIX_OPEN_MAX-1; fd >= 0; fd--)
	if (fd != modemFd)
	    (void) close(fd);
    fclose(stdin);
    /*
     * Now make the line be the controlling tty
     * and create a new process group/session for
     * the login process that follows.
     */
    fd = open("tty", 0);		// NB: assumes we're in /dev
    if (fd >= 0) {
#ifdef TIOCNOTTY
	(void) ioctl(fd, TIOCNOTTY, 0);
#else
	(void) setpgrp();
#endif
	(void) close(fd);
    }
    (void) setsid();
    fd = open(getLine(), O_RDWR|O_NONBLOCK);
    if (fd != STDIN_FILENO)
	fatal("Can not setup \"%s\" as stdin", getLine());
    if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) &~ O_NONBLOCK))
	fatal("Can not reset O_NONBLOCK: %m");
    close(modemFd);			// done with this, pitch it

    /*
     * Turn off CLOCAL so that SIGHUP is sent on modem disconnect.
     */
    struct termios term;
    if (tcgetattr(fd, &term) == 0) {
	term.c_cflag &= ~CLOCAL;
	(void) tcsetattr(fd, TCSAFLUSH, &term);
    }
    /*
     * Setup descriptors for stdout, and stderr.
     * Establish the initial line termio settings and set
     * protection on the device file.  Finally, update the
     * utmp and wtmp files to reflect the login attempt
     * (login will abort if the utmp entry is not present).
     */
    struct stat sb;
    (void) stat(getLine(), &sb);
    (void) chown(getLine(), 0, sb.st_gid);
    (void) chmod(getLine(), 0622);
    if (dup2(fd, STDOUT_FILENO) < 0)
	fatal("Unable to dup stdin to stdout: %m");
    if (dup2(fd, STDERR_FILENO) < 0)
	fatal("Unable to dup stdin to stderr: %m");

    loginAccount();
}

void
SysVGetty::writeWtmp(utmp* ut)
{
    // append record of login to wtmp file
#ifndef svr4
    int fd = open(_PATH_WTMP, O_WRONLY|O_APPEND);
    if (fd >= 0) {
	write(fd, (char *)ut, sizeof (*ut));
	close(fd);
    }
#else
    updwtmpx(WTMPX_FILE, ut);
#endif
}

/*
 * Record the login session.
 */
void
SysVGetty::loginAccount()
{
    static utmp ut;			// zero unset fields
    ut.ut_pid = getpid();
    ut.ut_type = LOGIN_PROCESS;
#ifndef __linux__
    ut.ut_exit.e_exit = 0;
    ut.ut_exit.e_termination = 0;
#endif
    ut.ut_time = time(0);
    // mark utmp entry as a login
    strncpy(ut.ut_user, "LOGIN", sizeof (ut.ut_user));
    /*
     * For SVR4 systems, use the trailing component of
     * the pathname to avoid problems where truncation
     * results in non-unique identifiers.
     */
    fxStr id(getLine());
    if (id.length() > sizeof (ut.ut_id))
	id.remove(0, id.length() - sizeof (ut.ut_id));
    strncpy(ut.ut_id, (char*) id, sizeof (ut.ut_id));
    strncpy(ut.ut_line, getLine(), sizeof (ut.ut_line));
    setutent();
    pututline(&ut);
    endutent();
    writeWtmp(&ut);
}

/*
 * Record the termination of login&co and
 * reset the state of the tty device.  Note
 * that this is called in the parent and
 * that we're entered with effective uid set
 * to the fax user and real uid of root.  Thus
 * we have to play games with uids in order
 * to write the utmp&wtmp entries, etc.
 */
void
SysVGetty::hangup()
{
    // at this point we're root and we can reset state
    struct utmp* ut;
    setutent();
    while ((ut = getutent()) != NULL) { 
	if (strncmp(ut->ut_line, getLine(), sizeof (ut->ut_line)) != 0)
	    continue;
	strncpy(ut->ut_user, "DEAD", sizeof (ut->ut_user));
	ut->ut_type = DEAD_PROCESS;
#ifndef __linux__
	ut->ut_exit.e_exit = (exitStatus >> 8) & 0xff;		// XXX
	ut->ut_exit.e_termination = exitStatus & 0xff;		// XXX
#endif
	ut->ut_time = time(0);
	pututline(ut);
	writeWtmp(ut);
	break;
    }
    endutent();
    Getty::hangup();
}

fxBool
SysVGetty::wait(int& status, fxBool block)
{
    if (waitpid(getPID(), &exitStatus, block ? 0 : WNOHANG) == getPID()) {
	status = exitStatus;
	return (TRUE);
    } else
	return (FALSE);
}
