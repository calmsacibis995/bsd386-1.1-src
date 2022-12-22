#	BSDI $Id: Makefile,v 1.5 1993/03/09 15:01:32 polk Exp $
#	@(#)Makefile	5.11 (Berkeley) 6/25/90

PROG=	more
CFLAGS+= -DHELPFILE=\"/usr/share/misc/omore.help\"
DPADD=	${LIBTERM}
LDADD=	-ltermcap
MLINKS=	more.1 page.1
LINKS=	${BINDIR}/more ${BINDIR}/page

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/more.help \
	    ${DESTDIR}/usr/share/misc/omore.help

.include <bsd.prog.mk>
