#	BSDI $Id: Makefile,v 1.2 1994/01/15 00:04:07 torek Exp $

PROG=	faxmail
SRCS=	faxmail.c++
CFLAGS+=${FAXINCS}
C++FLAGS+=${FAXINCS} -DFONTDIR=\"${FONTDIR}\"
LDCC=	g++
DPADD=	${FAXLIBS} ${LIBM}
LDADD=	${FAXLIBS} -lm
MKDEP=	mkdep -2

${PROG}.1: ${.CURDIR}/../man/${PROG}.1
	sed ${MANSED} < ${.ALLSRC} > ${.TARGET}
CLEANFILES+=${PROG}.1

.include <bsd.prog.mk>
