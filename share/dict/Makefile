#	@(#)Makefile	5.1.1.1 (Berkeley) 5/7/91

# Missing: connectives, words

FILES=	README propernames web2 web2a words
NOOBJ=	noobj

all clean cleandir depend lint tags:

install:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
	    ${DESTDIR}${BINDIR}/dict

.include <bsd.prog.mk>
