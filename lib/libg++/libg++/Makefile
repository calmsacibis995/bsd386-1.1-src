#	BSDI $Id: Makefile,v 1.4 1994/01/15 17:19:01 polk Exp $

# libg++ Makefile -- built by hand from numerous Makefile.in files
# in the 2.5.2 distribution.

CC=	gcc2
INC=	-I${.CURDIR}
CFLAGS=	-O2 ${INC}
C++FLAGS+=-nostdinc++ ${INC}

MKDEP=	mkdep -2 -nostdinc++

LIB=	g++2

# libio sources.
.PATH: ${.CURDIR}/../D/libio
INC+=	-I${.CURDIR}/../D/libio

SRCS+=	cleanup.c filedoalloc.c fileops.c floatconv.c genops.c iofclose.c \
	iofdopen.c iofflush.c iofgetpos.c iofgets.c iofopen.c iofprintf.c \
	iofread.c iofscanf.c iofsetpos.c ioftell.c iofwrite.c iogetline.c \
	iogets.c ioignore.c iopadn.c ioperror.c iopopen.c ioprims.c \
	ioprintf.c ioputs.c ioputs.c ioscanf.c ioseekoff.c ioseekpos.c \
	iosetbuffer.c iosetvbuf.c iosprintf.c iosscanf.c iostrerror.c \
	ioungetc.c iovfprintf.c iovfscanf.c iovsprintf.c iovsscanf.c \
	outfloat.c stdfiles.c strops.c
SRCS+=	PlotFile.cc SFile.cc builtinbuf.cc editbuf.cc filebuf.cc fstream.cc \
	indstream.cc ioextend.cc iomanip.cc iostream.cc isgetline.cc \
	isgetsb.cc isscan.cc osform.cc parsestream.cc pfstream.cc \
	procbuf.cc sbform.cc sbgetline.cc sbscan.cc stdiostream.cc \
	stdstreams.cc stream.cc streambuf.cc strstream.cc

# libg++ sources.
.PATH: ${.CURDIR}/../D/libg++/src
INC+=	-I${.CURDIR}/../D/libg++/src

SRCS+=	ACG.cc AllocRing.cc Binomial.cc BitSet.cc BitString.cc Complex.cc \
	CursesW.cc DLList.cc DiscUnif.cc Erlang.cc Fix.cc Fix16.cc Fix24.cc \
	Geom.cc GetOpt.cc HypGeom.cc Intdouble.cc Integer.cc LogNorm.cc \
	MLCG.cc NegExp.cc Normal.cc Obstack.cc Poisson.cc RNG.cc Random.cc \
	Rational.cc Regex.cc RndInt.cc SLList.cc SmplHist.cc SmplStat.cc \
	String.cc Uniform.cc Weibull.cc builtin.cc chr.cc compare.cc \
	error.cc fmtq.cc gcd.cc hash.cc ioob.cc lg.cc new.cc pow.cc \
	regex.cc sqrt.cc str.cc timer.c
#SRCS+=	math.cc

NOMAN=		noman

afterinstall:
	rm -f ${DESTDIR}${LIBDIR}/libg++.a
	ln ${DESTDIR}${LIBDIR}/libg++2.a ${DESTDIR}${LIBDIR}/libg++.a
	rm -f ${DESTDIR}${LIBDIR}/libg++_p.a
	ln ${DESTDIR}${LIBDIR}/libg++2_p.a ${DESTDIR}${LIBDIR}/libg++_p.a

.include <bsd.lib.mk>
