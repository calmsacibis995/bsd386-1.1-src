#	BSDI	$Id: Makefile,v 1.1 1994/01/15 00:55:24 polk Exp $

PROG=	chkconfig
SRCS=
NOMAN=noman
STRIP=

tags depend:

${PROG}: ${PROG}.sh
	rm -f ${PROG}
	cp ${.CURDIR}/${PROG}.sh ${PROG}
	chmod ${BINMODE} ${PROG}

.include <bsd.prog.mk>
