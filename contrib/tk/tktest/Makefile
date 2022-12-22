#	BSDI	$Id: Makefile,v 1.1.1.1 1994/01/03 23:15:46 polk Exp $

PROG=	tktest
SRCS=	tkTest.c
CFLAGS+=-I${.CURDIR}/../libtk -I${.CURDIR}/../../tcl/libtcl 
CFLAGS+=-I/usr/X11/include -DHAVE_UNISTD_H
LDFLAGS+=-L/usr/X11/lib
NOMAN=	noman

.if exists(${.CURDIR}/../libtk/obj)
LIBTK=${.CURDIR}/../libtk/obj/libtk.a
.else
LIBTK=${.CURDIR}/../libtk/libtk.a
.endif

.if exists(${.CURDIR}/../../tcl/libtcl/obj)
LIBTCL=${.CURDIR}/../../tcl/libtcl/obj/libtcl.a
.else
LIBTCL=${.CURDIR}/../../tcl/libtcl/libtcl.a
.endif

LDADD+=	${LIBTK} ${LIBTCL} -lX11 -lm
DPADD+=	${LIBTK} ${LIBTCL} /usr/X11/lib/libX11.a ${LIBM}

install:

.include "../../Makefile.inc"
.include <bsd.prog.mk>
