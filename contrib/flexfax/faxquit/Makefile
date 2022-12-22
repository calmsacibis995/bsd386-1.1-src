#	BSDI $Id: Makefile,v 1.2 1994/01/30 20:42:45 polk Exp $

PROG=	faxquit
MAN8=	faxquit.0
BINDIR=	${SBINDIR}
CFLAGS+=${FAXINCS}

${PROG}.8: ${.CURDIR}/../man/${PROG}.1m
	sed ${MANSED} < ${.ALLSRC} > ${.TARGET}
CLEANFILES+=${PROG}.8

.include <bsd.prog.mk>
