#	BSDI	$Id: Makefile,v 1.1.1.1 1994/01/03 20:50:55 polk Exp $

PROG=	mkhits
SRCS=	mkhits.c dohits.c asc_ebc.c ebc_disp.c
CFLAGS+=-I${.CURDIR}/..
NOMAN=	noman

.PATH:	${.CURDIR}/../../api

install:

.depend: ${PROG}

.include <bsd.prog.mk>
