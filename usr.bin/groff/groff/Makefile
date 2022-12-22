#	BSDI $Id: Makefile,v 1.3 1994/01/11 22:35:09 polk Exp $

.include "../Makefile.defs"

PROG=	groff
SRCS=	groff.cc pipeline.c
LDADD+=	${LIBGROFF} ${LIBM} ${LIBGCC}
DPADD+=	${LIBGROFF} ${LIBM} ${LIBGCC}
MLINKS+=groff.1 psroff.1 groff.1 xroff.1

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/psroff ${.CURDIR}/xroff ${DESTDIR}${BINDIR}

.include "../Makefile.common"
.include <bsd.prog.mk>
