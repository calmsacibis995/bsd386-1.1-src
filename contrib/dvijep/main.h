/* -*-C-*- main.h */
/*-->main*/
/**********************************************************************/
/******************************** main ********************************/
/**********************************************************************/

/**********************************************************************/
/***********************  External Definitions  ***********************/
/**********************************************************************/

#if    KCC_20
#include <file.h>
#endif /* KCC_20 */

#if    PCC_20				/* this stuff MUST be first */

#undef tops20				/* to keep definitions alive */
#include <ioctl.h>			/* PCC-20 does not have this in */
					/* the others */
#include <file.h>			/* need for f20open flags and */
					/* JSYS stuff */
#define tops20  1			/* define for tops-20 */
#endif /* PCC_20 */

#include "commands.h"
#include <ctype.h>
#include <math.h>

#if    BBNBITGRAPH
#if    (OS_VAXVMS | IBM_PC_WIZARD | IBM_PC_LATTICE | IBM_PC_MICROSOFT)
	/* not available */
#else /* NOT (OS_VAXVMS | IBM_PC_WIZARD | IBM_PC_LATTICE | IBM_PC_MICROSOFT) */
#include <signal.h>
#endif /* (OS_VAXVMS | IBM_PC_WIZARD | IBM_PC_LATTICE | IBM_PC_MICROSOFT) */
#endif /* BBNBITGRAPH */

#if    OS_UNIX
#if    BSD42

#include <sys/ioctl.h>			/* need for DVISPOOL in dviterm.h */

#ifndef _NFILE
/* VAX VMS, NMTCC, PCC-20, and HPUX have _NFILE in stdio.h.  V7 called
it NFILE, and Posix calls it OPEN_MAX.  KCC-20 calls it SYS_OPEN.  VAX
4.3BSD and Gould UTX/32 don't define _NFILE in stdio.h; they use
NOFILE from sys/param.h.  Sigh.... */
#include <sys/param.h>
#ifdef NOFILE
#define _NFILE NOFILE			/* need for gblvars.h */
#else
#define _NFILE MAXOPEN			/* use our font limit value */
#endif
#endif

#endif /* BSD42 */
#endif /* OS_UNIX */



#include "gendefs.h"

#if    DECLA75
#undef STDMAG

#if    STDRES
#define  STDMAG		720
#else /* NOT STDRES */
#define  STDMAG		720
#endif /* STDRES */

#endif /* DECLA75 */

#if    DECLN03PLUS
#undef STDMAG

#if    STDRES
#define  STDMAG		1500
#else /* NOT STDRES */
#define  STDMAG		750
#endif /* STDRES */

#endif /* DECLN03PLUS */

#if    EPSON
#undef STDMAG

#if    STDRES
#define  STDMAG		1200		/* use 240dpi fonts */
#else /* NOT STDRES */
#define  STDMAG		603		/* 1500 * 1.2**(-5) */
#endif /* STDRES */

#endif /* EPSON */

#if    (HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2)
#undef STDMAG

#if    STDRES
#define  STDMAG		1500		/* 300 dpi Canon LBP-CX print engine */
#else /* NOT STDRES */
#define  STDMAG		1440		/* near value in 1000*1.2**n family */
#endif /* STDRES */

#endif /* (HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2) */

#if    TOSHIBAP1351			/* want to override STDMAG */
#undef STDMAG

#if    STDRES
#define  STDMAG		868		/* 1500 * 1.2**(-3) */
#else /* NOT STDRES */
#define  STDMAG		833		/* 1000 * 1.2**(-1) */
#endif /* STDRES */

#endif /* TOSHIBAP1351 */

#include "gblprocs.h"

#include "gblvars.h"

#if    BBNBITGRAPH
#include "keydef.h"
#endif /* BBNBITGRAPH */

#if    OS_ATARI
long _stksize = 20000L; /* make the stack a bit larger than 2KB */
                        /* number must be even                  */
#endif /* OS_ATARI */


/**********************************************************************/
/*******************************  main  *******************************/
/**********************************************************************/

int
main(argc, argv)
int argc;
char *argv[];
{
    register int k;		/* loop index */
    register int file_args;	/* count of file arguments */


    (void)strcpy(g_progname, argv[0]); /* save program name */

    (void)initglob();		/* do this before argc check! */

#if    OS_UNIX
    /* On Unix, we allow filtering of stdin to stdout */
#else /* NOT OS_UNIX */
    if (argc < 2)
    {
	(void)usage(stderr);
	(void)EXIT(1);
    }
#endif /* OS_UNIX */

    for (k = 1; k < argc; ++k)
    {
	if (*argv[k] == '-')	/* -switch */
	    (void)option(argv[k]);
    }

    if (!quiet)
    {
	(void)fprintf(stderr,"[TeX82 DVI Translator Version %s]",VERSION_NO);
	NEWLINE(stderr);
	(void)fprintf(stderr,"[%s]",DEVICE_ID);
	NEWLINE(stderr);
    }

    if (npage == 0)		/* no page ranges given, make a large one */
    {
	page_begin[0] = 1;
	page_end[0] = 32767;	/* arbitrary large integer */
	page_step[0] = 1;
	npage = 1;
    }
    else /* need font defs from postamble if only some pages to be output */
	preload = TRUE;

    file_args = 0;
    for (k = 1; k < argc; ++k)
    {
	if (*argv[k] != '-')	 /* must be file argument */
	{
	    file_args++;
	    (void)dvifile(argc,argv,argv[k]);
	}
    }

#if    OS_UNIX
    if (file_args == 0)		/* use stdin/stdout instead */
        (void)dvifile(argc,argv,"");
#endif

    (void)alldone();		/* this will never return */
    return (0);			/* never executed; avoid compiler warnings */
}
