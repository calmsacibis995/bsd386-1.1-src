#	BSDI $Id: Makefile,v 1.1 1994/01/15 00:03:22 torek Exp $

PROG=	dialtest
SRCS=	dialtest.c++
MAN8=	dialtest.0
CFLAGS+=${FAXINCS}
C++FLAGS+=${FAXINCS}
LDCC=	g++
DPADD=	${FAXLIBS}
LDADD=	${FAXLIBS}
MKDEP=	mkdep -2

BINDIR=	${SBINDIR}

${PROG}.8: ${.CURDIR}/../man/${PROG}.1m
	sed ${MANSED} < ${.ALLSRC} > ${.TARGET}
CLEANFILES+=${PROG}.8

.include <bsd.prog.mk>
