/*
 * texi2roff.c - texi2roff mainline
 * 		Release 1.0a 	August 1988
 *		Release 2.0	January 1990
 *
 * Copyright 1988, 1989, 1990  Beverly A.Erlebacher
 * erlebach@cs.toronto.edu    ...uunet!utai!erlebach
 *
 */

#include <stdio.h>
#ifndef __TURBOC__
#include <sys/types.h>
#endif
#include <sys/stat.h>
#include "texi2roff.h"

char *progname;
int transparent = NO;	/* for -t flag */

/*
 * main - parse arguments, handle options
 *	- initialize tables and other strings
 * 	- open files and pass them to process().
 */
main(argc, argv)
int argc;
char *argv[];
{
    int c, errflg = 0;
    FILE *in;
    char *inname;
    int macropkg = NONE;	/* user's choice of MS, ME or MM */
    int showInfo = NO; 		/* -I : display Info file material?*/
    int makeindex = NO; 	/* -i : to emit index macros? */

    extern int optind;
    extern char *optarg;
    extern int process();
    extern void initialize();

    progname = argv[0];

    while ((c = getopt(argc, argv, "itIm:--")) != EOF)
	switch (c) {
  	case 'i':
  	    makeindex  = YES;
  	    break;
  	case 't':
  	    transparent = YES;
  	    break;
	case 'I':
	    showInfo = YES;
	    break;
	case 'm':
	    if (macropkg != NONE) {
		errflg++;
	    } else {
		switch ( (char) *optarg) {
		case 's':
		    macropkg = MS;
		    break;
		case 'm':
		    macropkg = MM;
		    break;
		case 'e':
		    macropkg = ME;
		    break;
		default:
		    errflg++;
		    break;
		}
	    }
	    break;
	case '?':
	    errflg++;
	    break;
	}

    if (macropkg == NONE) {
	errflg++;
	}
    if (errflg) {
	(void) fprintf(stderr,
	    "Usage: %s [ -me -mm -ms ] [ -iIt ] [ file ... ]\n", progname);
	exit(1);
    }

    (void) initialize(macropkg, showInfo, makeindex);

    if (optind >= argc) {
	errflg += process(stdin, "stdin");
	}
    else
	for (; optind < argc; optind++) {
	    if (STREQ(argv[optind], "-")) {
		inname = "stdin";
		in = stdin;
		}
	    else {
		if (( in = fopen(argv[optind], "r")) == NULL) {
		    (void) fprintf(stderr,"%s : can't open file %s\n",
			    progname, argv[optind]);
		    continue;
		}
		inname = argv[optind];
	    }
	    errflg += process(in, inname);
	    if (in != stdin)
		(void) fclose(in);
	}
    exit(errflg);
}

/*
 * process -  check opened files and pass them to translate().
 *	   -  report on disastrous translation failures
 */
int
process(fp, filename)
    FILE *fp;
    char *filename;
{
    struct stat statbuf;
    extern int translate(/* FILE *, char * */);

    if (fstat(fileno(fp), &statbuf) != 0){
	(void) fprintf(stderr,"%s : can't fstat file %s\n", progname, 
								filename);
	return 1;
    }
    if ((statbuf.st_mode & S_IFMT)==S_IFDIR) {
	(void) fprintf(stderr, "%s : %s is a directory\n", progname,
								filename);
	return 1;
    }
    /* translate returns 0 (ok) or -1 (disaster). it isn't worthwhile
     * to try to recover from a disaster.
     */
    if (translate(fp, filename) < 0) {
	(void) fprintf(stderr,
		"%s: error while processing file %s, translation aborted\n",
		progname, filename);
	exit(1); 
    }
    return 0;
}
