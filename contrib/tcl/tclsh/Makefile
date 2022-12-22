#	BSDI	$Id: Makefile,v 1.2 1994/01/05 22:03:56 polk Exp $

PROG=	tclsh
SRCS=	tclAppInit.c
CFLAGS+=-I${.CURDIR}/../libtcl -DHAVE_UNISTD_H

.if exists(${.CURDIR}/../libtcl/obj)
LIBTCL=${.CURDIR}/../libtcl/obj/libtcl.a
.else
LIBTCL=${.CURDIR}/../libtcl/libtcl.a
.endif

LDADD+=	${LIBTCL} -lm
DPADD+=	${LIBTCL} ${LIBM}

.include "../../Makefile.inc"
.include <bsd.prog.mk>
