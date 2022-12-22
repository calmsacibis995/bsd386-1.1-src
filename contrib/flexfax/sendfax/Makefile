#	BSDI $Id: Makefile,v 1.2 1994/01/15 00:04:29 torek Exp $

PROG=	sendfax
SRCS=	sendfax.c++
CFLAGS+=${FAXINCS}
C++FLAGS+=${FAXINCS}
LDCC=	g++
DPADD=	${FAXLIBS}
LDADD=	${FAXLIBS}
MKDEP=	mkdep -2

${PROG}.1: ${.CURDIR}/../man/${PROG}.1
	sed ${MANSED} < ${.ALLSRC} > ${.TARGET}
CLEANFILES+=${PROG}.1

.include <bsd.prog.mk>
