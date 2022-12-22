#	BSDI $Id: Makefile,v 1.1 1994/01/15 00:03:44 torek Exp $

PROG=	faxanswer
MAN8=	faxanswer.0
BINDIR=	${SBINDIR}
CFLAGS+=${FAXINCS}

${PROG}.8: ${.CURDIR}/../man/${PROG}.1m
	sed ${MANSED} < ${.ALLSRC} > ${.TARGET}
CLEANFILES+=${PROG}.8

.include <bsd.prog.mk>
