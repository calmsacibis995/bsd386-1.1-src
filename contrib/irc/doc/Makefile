#	BSDI	$Id: Makefile,v 1.1.1.1 1994/01/13 21:15:41 polk Exp $

FILES=	Advertisement Authors Etiquette Makefile Manual Networking \
	Operators example.conf

obj objdir all tags depend clean cleandir:

install:
	install -c -o ${BINOWN} -g ${BINGRP} -m 0444 ${FILES} \
		${DESTDIR}/usr/contrib/lib/irc/

.include <bsd.prog.mk>
