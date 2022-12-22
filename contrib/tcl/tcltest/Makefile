#	BSDI	$Id: Makefile,v 1.1.1.1 1994/01/03 23:14:48 polk Exp $

PROG=	tcltest
SRCS=	tclTest.c
NOMAN=	noman
CFLAGS+=-I${.CURDIR}/../libtcl -DHAVE_UNISTD_H

.if exists(${.CURDIR}/../libtcl/obj)
LIBTCL=${.CURDIR}/../libtcl/obj/libtcl.a
.else
LIBTCL=${.CURDIR}/../libtcl/libtcl.a
.endif

LDADD+=	${LIBTCL} -lm
DPADD+=	${LIBTCL} ${LIBM}

install:

.include <bsd.prog.mk>
