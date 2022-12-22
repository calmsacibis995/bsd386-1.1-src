#	BSDI	$Id: Makefile,v 1.2 1994/01/05 20:00:16 polk Exp $

PROG=	psgraph
SRCS=	main.c grid.c input.c output.c
CFLAGS+=-DPROLOG=\"/usr/contrib/lib/psgraph/psgraph.pro\"
LDADD+=	-lm
DPADD+=	${LIBM}

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 0444 ${.CURDIR}/psgraph.pro \
		${DESTDIR}/usr/contrib/lib/psgraph/

.include "../../Makefile.inc"
.include <bsd.prog.mk>
