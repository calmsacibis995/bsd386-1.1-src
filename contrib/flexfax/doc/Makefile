#	BSDI $Id: Makefile,v 1.2 1994/01/15 00:03:28 torek Exp $

FILES=	class1.txt class2.txt hayes.txt modems-faq suprainfo scorecard \
	zyxel
NOOBJ=	noobj

all clean cleandir depend:

install:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
	    ${DESTDIR}${DOCDIR}

.include <bsd.prog.mk>
