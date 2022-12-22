#	BSDI $Id: Makefile,v 1.3 1993/12/30 01:53:28 polk Exp $

.include "../Makefile.defs"

PROG=	indxbib
SRCS=	dirnamemax.c indxbib.cc signal.c
LDADD+=	${LIBBIB} ${LIBGROFF} ${LIBGCC} -lm
DPADD+=	${LIBBIB} ${LIBGROFF} ${LIBGCC} ${LIBM}

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/eign \
		${DESTDIR}${FONTDIR}/

.include "../Makefile.common"
.include <bsd.prog.mk>
