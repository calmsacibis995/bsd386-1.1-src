#	BSDI	$Id: Makefile,v 1.2 1994/01/14 22:56:51 polk Exp $

PROG=	areacode
SRCS=
STRIP=

tags depend:

${PROG}: ${PROG}.sh
	rm -f ${PROG}
	cp ${.CURDIR}/${PROG}.sh ${PROG}
	chmod ${BINMODE} ${PROG}

.include <bsd.prog.mk>
