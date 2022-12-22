
/***********************************************************
Copyright 1990, by Alfalfa Software Incorporated, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that Alfalfa's name not be used in
advertising or publicity pertaining to distribution of the software
without specific, written prior permission.

ALPHALPHA DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
ALPHALPHA BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

If you make any modifications, bugfixes or other changes to this software
we'd appreciate it if you could send a copy to us so we can keep things
up-to-date.  Many thanks.
				Kee Hinckley
				Alfalfa Software, Inc.
				267 Allston St., #3
				Cambridge, MA 02139  USA
				nazgul@alfalfa.com
    
******************************************************************/

/* Edit History

10/01/91   1 larryp	created
*/

#include <stdio.h>
#include <sys/types.h>
#ifdef SYSV
#include <sys/fcntl.h>
#define L_SET SEEK_SET
#define L_INCR SEEK_CUR
#endif
#include <sys/file.h>
#include <sys/stat.h>
#include "gencat.h"

/*
 * dump a binary message catalog so we can see what's in it.
 */


void usage() {
    fprintf(stderr, "Use: dumpmsg catfile msgfile\n");
}

void main(
#if ANSI_C || defined(__cplusplus)
		int argc, char *argv[])
#else
		argc, argv)
int argc;
char *argv[];
#endif
{
    int		ifd, i;
    FILE	*ofp;
    
    if (argc != 3) {
	usage();
	exit(1);
    }

    if ((ifd = open(argv[1], O_RDONLY)) < 0) {
	perror(argv[1]);
	exit(2);
    }

    if (!strcmp(argv[2], "-")) {
	ofp = stdout;
    } else {
	if ((ofp = fopen(argv[2], "w")) == NULL) {
	    perror(argv[2]);
	    exit(3);
	}
    }

    MCReadCat(ifd);

    MCDumpcat(ofp);
}
