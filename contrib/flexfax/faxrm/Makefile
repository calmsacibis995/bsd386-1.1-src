#	BSDI $Id: Makefile,v 1.2 1994/01/15 00:04:10 torek Exp $

PROG=	faxrm
SRCS=	faxrm.c++
CFLAGS+=${FAXINCS}
C++FLAGS+=${FAXINCS}
LDCC=	g++
DPADD=	${FAXLIBS}
LDADD=	${FAXLIBS}
MKDEP=	mkdep -2

${PROG}.1: ${.CURDIR}/../man/${PROG}.1
	sed ${MANSED} -e "s;\$${USRETC};/etc;" < ${.ALLSRC} > ${.TARGET}
CLEANFILES+=${PROG}.1

.include <bsd.prog.mk>
