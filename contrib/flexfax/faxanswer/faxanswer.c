/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxanswer/faxanswer.c,v 1.1.1.1 1994/01/14 23:10:57 torek Exp $
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
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>

#include "config.h"
#include "port.h"

static void
fatal(char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    if (isatty(fileno(stderr))) {
	vfprintf(stderr, fmt, ap);
	putc('\n', stderr);
    } else
	vsyslog(LOG_ERR, fmt, ap);
    va_end(ap);
    exit(-1);
}

main(int argc, char** argv)
{
    extern int optind;
    extern char* optarg;
    int fifo, c;
    char* spooldir = FAX_SPOOLDIR;
    char* how = "";
    char fifoname[256];
    char answercmd[80];

    openlog(argv[0], LOG_PID, LOG_FAX);
    while ((c = getopt(argc, argv, "h:q")) != -1)
	switch (c) {
	case 'h':
	    how = optarg;
	    break;
	case 'q':
	    spooldir = optarg;
	    break;
	case '?':
	    fatal("Bad option `%c'; usage: %s [-h how] [-q queue-dir] [modem]",
		c, argv[0]);
	    /*NOTREACHED*/
	}
    if (optind == argc-1) {
	if (argv[optind][0] == FAX_FIFO[0])
	    strcpy(fifoname, argv[optind]);
	else
	    sprintf(fifoname, "%s.%.*s", FAX_FIFO,
		sizeof (fifoname) - sizeof (FAX_FIFO), argv[optind]);
    } else
	strcpy(fifoname, FAX_FIFO);
    if (chdir(spooldir) < 0)
	fatal("%s: chdir: %s", spooldir, strerror(errno));
    fifo = open(fifoname, O_WRONLY|O_NDELAY);
    if (fifo < 0)
	fatal("%s: open: %s", fifoname, strerror(errno));
    sprintf(answercmd, "A%s", how);
    if (write(fifo, answercmd, strlen(answercmd)) != strlen(answercmd))
	fatal("FIFO write failed for answer (%s)", strerror(errno));
    (void) close(fifo);
    exit(0);
}
