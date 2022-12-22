#	BSDI	$Id: Makefile,v 1.1.1.1 1994/01/03 23:14:26 polk Exp $

all:
obj:
depend:
clean:
cleandir:
tags:

TOOLS=	play_au play_aiff play_wav

.include "../../Makefile.inc"

install:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} ${TOOLS} \
		    ${DESTDIR}${BINDIR}

.include <bsd.prog.mk>
