#	BSDI	$Id: Makefile,v 1.1.1.1 1994/01/03 20:50:49 polk Exp $

PROG=	mset
SRCS=	mset.c map3270.c
CFLAGS+=-I${.CURDIR}/../ascii -g -fwritable-strings

.PATH:	${.CURDIR}/../ascii

.if exists(${.CURDIR}/../api/obj)
LIBAPI=${.CURDIR}/../api/obj/libapi.a
.else
LIBAPI=${.CURDIR}/../api/libapi.a
.endif

LDADD+=	${LIBAPI}
DPADD+=	${LIBAPI}

.include "../../Makefile.inc"
.include <bsd.prog.mk>
