#	BSDI	$Id: Makefile,v 1.1.1.1 1994/01/03 20:50:54 polk Exp $

PROG=	mkastosc
SRCS=	mkastosc.c dohits.c asc_ebc.c ebc_disp.c
NOMAN=	noman
CFLAGS+=-I${.CURDIR}/.. -I${.CURDIR}/../mkhits

.PATH:	${.CURDIR}/../../api ${.CURDIR}/../mkhits

install:

.depend: ${PROG}

.include <bsd.prog.mk>
