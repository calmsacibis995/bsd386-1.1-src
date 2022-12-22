#	BSDI	$Id: Makefile,v 1.2 1994/01/05 20:25:34 polk Exp $

obj objdir tags depend all:

install:
	install -c -o ${BINOWN} -g ${BINGRP} -m 0444 ${.CURDIR}/[a-z]* \
		${DESTDIR}/usr/contrib/lib/tcl/

.include <bsd.prog.mk>
