#	BSDI $Id: Makefile,v 1.2 1994/01/15 00:03:48 torek Exp $

PROG=	faxcover
SRCS=	faxcover.c++
CFLAGS+=${FAXINCS}
C++FLAGS+=${FAXINCS}
LDCC=	g++
DPADD=	${FAXLIBS}
LDADD=	${FAXLIBS}
MKDEP=	mkdep -2

${PROG}.1: ${.CURDIR}/../man/${PROG}.1
	sed ${MANSED} < ${.ALLSRC} > ${.TARGET}
CLEANFILES+=${PROG}.1

afterinstall:
	install -c -m 444 ${.CURDIR}/faxcover.ps ${DESTDIR}${LIBDATADIR}

.include <bsd.prog.mk>
