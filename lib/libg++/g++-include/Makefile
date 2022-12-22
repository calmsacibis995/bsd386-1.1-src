#	BSDI $Id: Makefile,v 1.2 1994/01/13 07:11:22 torek Exp $

# install G++ headers -- built by hand from numerous Makefile.in files
# in the 2.5.2 distribution.

INCDIR=	/usr/include/g++

# headers changed from the distribution
LIBBSD=	${.CURDIR}/../libg++
LIBBSD_H=_G_config.h curses.h

# libio
LIBIO=	${.CURDIR}/../D/libio
LIBIO_H=PlotFile.h SFile.h builtinbuf.h editbuf.h floatio.h \
	stream.h indstream.h iolibio.h iomanip.h iostdio.h iostream.h \
	iostreamP.h istream.h libio.h libioP.h ostream.h parsestream.h \
	pfstream.h procbuf.h stdiostream.h streambuf.h strfile.h \
	strstream.h

# libg++
LIBGXX=	${.CURDIR}/../D/libg++/src
LIBGXX_H=ACG.h AllocRing.h Binomial.h BitSet.h BitString.h Complex.h \
	CursesW.h DLList.h DiscUnif.h Erlang.h Fix.h Fix16.h Fix24.h Geom.h \
	GetOpt.h HypGeom.h Incremental.h Integer.h LogNorm.h MLCG.h \
	NegExp.h Normal.h Obstack.h Pix.h Poisson.h RNG.h Random.h \
	Rational.h Regex.h RndInt.h SLList.h SmplHist.h SmplStat.h \
	String.h Uniform.h Weibull.h bool.h builtin.h compare.h complex.h \
	generic.h getpagesize.h libc.h minmax.h new.h osfcn.h \
	regex.h std.h strclass.h swap.h typemacros.h

# gen *P files
GEN=	${.CURDIR}/../D/libg++/src/gen

# gen

all clean cleandir depend lint obj tags:

install:
	-[ -d ${DESTDIR}${INCDIR} ] || mkdir ${DESTDIR}${INCDIR}
	@echo installing ${LIBBSD_H}
	@-cd ${LIBBSD}; for i in ${LIBBSD_H}; do \
	    cmp -s $$i ${DESTDIR}${INCDIR}/$$i || \
	    install -c -o ${BINOWN} -g ${BINGRP} -m 444 $$i \
		${DESTDIR}${INCDIR}/$$i; \
	done
	@echo installing ${LIBIO_H}
	@-cd ${LIBIO}; for i in ${LIBIO_H}; do \
	    cmp -s $$i ${DESTDIR}${INCDIR}/$$i || \
	    install -c -o ${BINOWN} -g ${BINGRP} -m 444 $$i \
		${DESTDIR}${INCDIR}/$$i; \
	done
	@echo installing ${LIBGXX_H}
	@-cd ${LIBGXX}; for i in ${LIBGXX_H}; do \
	    cmp -s $$i ${DESTDIR}${INCDIR}/$$i || \
	    install -c -o ${BINOWN} -g ${BINGRP} -m 444 $$i \
		${DESTDIR}${INCDIR}/$$i; \
	done
	@echo installing gen
	@-[ -d ${DESTDIR}${INCDIR}/gen ] || mkdir ${DESTDIR}${INCDIR}/gen
	@-cd ${GEN}; for j in *.*P; do \
	    cmp -s $$j ${DESTDIR}${INCDIR}/gen/$$j || \
	    install -c -o ${BINOWN} -g ${BINGRP} -m 444 $$j \
		${DESTDIR}${INCDIR}/gen/$$j; \
	done

.include <bsd.prog.mk>
