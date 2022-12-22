#	BSDI $Id: Makefile,v 1.4 1994/01/30 21:11:53 polk Exp $

PROG=	faxd.recv
SRCS=	alter.c auth.c jobs.c main.c regex.c++ remove.c status.c submit.c
MAN8=	faxd.recv.0
CFLAGS+=${FAXINCS}
C++FLAGS+=${FAXINCS}
LDCC=	g++
DPADD=	${FAXLIBS}
LDADD=	${FAXLIBS}
MKDEP=	mkdep -2

BINDIR=	${SBINDIR}

${PROG}.8: ${.CURDIR}/../man/${PROG}.1m
	sed -e "s;\$${USRETC};${SBINDIR};" ${MANSED} < ${.ALLSRC} > ${.TARGET}
CLEANFILES+=${PROG}.8

.include <bsd.prog.mk>
