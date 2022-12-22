#	BSDI	$Id: Makefile,v 1.1.1.1 1994/01/03 20:50:54 polk Exp $

PROG=	mkastods
SRCS=	mkastods.c asc_ebc.c ebc_disp.c
NOMAN=	noman
CFLAGS+=-I${.CURDIR}/..

.PATH:	${.CURDIR}/../../api

install:

.depend: ${PROG}

.include <bsd.prog.mk>
