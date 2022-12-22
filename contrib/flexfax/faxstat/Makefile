#	BSDI $Id: Makefile,v 1.2 1994/01/15 00:04:12 torek Exp $

PROG=	faxstat
SRCS=	FaxStatClient.c++ RecvStatus.c++ SendStatus.c++ ServerStatus.c++ \
	faxstat.c++
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
