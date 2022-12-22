#	BSDI $Id: Makefile,v 1.1 1994/01/15 00:04:31 torek Exp $

PROG=	textfmt
SRCS=	textfmt.c++
CFLAGS+=${FAXINCS}
C++FLAGS+=${FAXINCS} -DFONTDIR=\"${FONTDIR}\"
LDCC=	g++
DPADD=	${FAXLIBS}
LDADD=	${FAXLIBS}
MKDEP=	mkdep -2

BINDIR=	${LIBEXECDIR}

${PROG}.1: ${.CURDIR}/../man/${PROG}.1
	sed ${MANSED} < ${.ALLSRC} > ${.TARGET}
CLEANFILES+=${PROG}.1

.include <bsd.prog.mk>
