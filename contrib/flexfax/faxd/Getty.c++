/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/Getty.c++,v 1.1.1.1 1994/01/14 23:09:48 torek Exp $
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
#include "Getty.h"
#include "UUCPLock.h"

#include <stdarg.h>
#include <termios.h>
#include <osfcn.h>
#include <signal.h>
#include <syslog.h>
#include <paths.h>
#include <sys/param.h>
#include <sys/stat.h>

/*
 * FAX Server Getty Support Base Class.
 */

const fxStr Getty::getty = _PATH_GETTY;

Getty::Getty(const fxStr& l, const fxStr& s, u_int t)
    : line(l)
    , speed(s)
{
    pid = 0;
    timeout = t;
}

Getty::~Getty()
{
}

void Getty::setTimeout(u_int t)		{ timeout = t; }

static void
sigHUP(int)
{
    (void) close(0);
    (void) close(1);
    (void) close(2);
    sleep(5);
    _exit(1);
}

/*
 * Parse the getty argument string and
 * substitute runtime parameters:
 *
 *    %l	tty device name
 *    %s	current baud rate
 *
 * The resultant argv array is used to
 * exec getty below.
 */
void
Getty::setupArgv(const char* args)
{
    argbuf = args;
    u_int l;
    /*
     * Substitute escape sequences.
     */
    for (l = 0; l < argbuf.length();) {
	l = argbuf.next(l, '%');
	if (l >= argbuf.length()-1)
	    break;
	switch (argbuf[l+1]) {
	case 'l':			// %l = tty device name
	    argbuf.remove(l,2);
	    argbuf.insert(line, l);
	    l += line.length();		// avoid loops
	    break;
	case 's':			// %s = tty speed
	    argbuf.remove(l,2);
	    argbuf.insert(speed, l);
	    l += speed.length();	// avoid loops
	    break;
	case '%':			// %% = %
	    argbuf.remove(l,1);
	    break;
	}
    }
    /*
     * Crack argument string and setup argv.
     */
    argv[0] = &getty[getty.nextR(getty.length(), '/')];
    u_int nargs = 1;
    for (l = 0; l < argbuf.length() && nargs < GETTY_MAXARGS-1;) {
	l = argbuf.skip(l, " \t");
	u_int token = l;
	l = argbuf.next(l, " \t");
	if (l > token) {
	    if (l < argbuf.length())
		argbuf[l++] = '\0';		// null terminate argument
	    argv[nargs++] = &argbuf[token];
	}
    }
    argv[nargs] = NULL;
}

/*
 * Run a getty session and if successful exec the
 * getty program.  Note that this is always run in
 * the child.  We actually fork again here so that
 * we immediately reap the real getty and cleanup
 * state.  It also simplifies matters of uid management
 * since the fax server runs as effective uid other
 * than root, but we're started up with real&effective
 * uid root.
 */
void
Getty::run(int fd, const char* args)
{
    pid = fork();
    if (pid == -1)
	fatal("getty: can not fork reaper process");
    closelog();
    openlog("FaxGetty", LOG_PID, LOG_AUTH);
    if (chdir("/dev") < 0)
	fatal("chdir: %m");
    if (pid == 0) {			// child, exec getty
	signal(SIGHUP, fxSIGHANDLER(sigHUP));
	setupArgv(args);
	/*
	 * After the session is properly setup, the
	 * stdio streams will be hooked to the tty
	 * and we can discard the file descriptor.
	 */
	setupSession(fd);

	char* envp[1];
	envp[0] = NULL;
	execve((char*) getty, argv, envp);
	_exit(127);
    } else {				// parent, wait for getty/login
	close(fd);			// close so HUPCL works correctly
	int status;
	wait(status, TRUE);		// wait for getty/login work to complete
	hangup();
	_exit(status);			// pass exit status upward
    }
}

void
Getty::hangup()
{
    chown(getLine(), UUCPLock::getUUCPUid(), UUCPLock::getUUCPGid());
    chmod(getLine(), 0600);		// reset protection
}

void
Getty::error(const char* va_alist, ...)
#define	fmt va_alist
{
    va_list ap;
    va_start(ap, fmt);
    vsyslog(LOG_ERR, fmt, ap);
    va_end(ap);
}
#undef fmt

void
Getty::fatal(const char* va_alist, ...)
#define	fmt va_alist
{
    va_list ap;
    va_start(ap, fmt);
    vsyslog(LOG_ERR, fmt, ap);
    va_end(ap);
    hangup();
}
#undef fmt
