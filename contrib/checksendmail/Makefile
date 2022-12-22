#	BSDI	$Id: Makefile,v 1.1.1.1 1994/01/13 21:15:30 polk Exp $

PROG=	checksendmail
MAN8=	checksendmail.0
SRCS=
STRIP=

tags depend:

${PROG}: checksendmail.perl
	cp ${.CURDIR}/checksendmail.perl checksendmail

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 0444  \
		${.CURDIR}/address.resolve ${DESTDIR}/usr/contrib/lib/
	
.include <bsd.prog.mk>
