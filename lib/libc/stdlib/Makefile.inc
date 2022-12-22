#	BSDI $Id: Makefile.inc,v 1.4 1993/11/12 11:07:15 donn Exp $
#	from @(#)Makefile.inc	5.6 (Berkeley) 6/4/91

# stdlib sources
.PATH: ${.CURDIR}/${MACHINE}/stdlib ${.CURDIR}/stdlib

SRCS+=	abort.c atexit.c atoi.c atol.c bigint.c bsearch.c calloc.c \
	div.c dtoa.c exit.c getenv.c getopt.c getsubopt.c heapsort.c \
	labs.c ldiv.c malloc.c putenv.c qsort.c \
	radixsort.c rand.c random.c setenv.c strtod.c strtol.c \
	strtoq.c strtoul.c strtouq.c system.c

.if   (${MACHINE} == "hp300")
SRCS+=	abs.s atof.c
.elif (${MACHINE} == "i386")
SRCS+=	abs.s atof.c
.elif (${MACHINE} == "tahoe")
SRCS+=	abs.s atof.s
.elif (${MACHINE} == "vax")
SRCS+=	abs.s atof.s
.endif

MAN3+=	abort.0 abs.0 alloca.0 atexit.0 atof.0 atoi.0 atol.0 bsearch.0 \
	calloc.0 div.0 exit.0 free.0 getenv.0 getopt.0 getsubopt.0 \
	labs.0 ldiv.0 malloc.0 memory.0 qsort.0 \
	radixsort.0 rand.0 random.0 realloc.0 strtod.0 strtol.0 \
	strtoul.0 system.0

MLINKS+=getenv.3 setenv.3 getenv.3 unsetenv.3 getenv.3 putenv.3
MLINKS+=getopt.3 optarg.3 getopt.3 opterr.3 getopt.3 optind.3 getopt.3 optopt.3
MLINKS+=getsubopt.3 suboptarg.3
MLINKS+=qsort.3 heapsort.3
MLINKS+=rand.3 srand.3
MLINKS+=random.3 initstate.3 random.3 setstate.3 random.3 srandom.3
