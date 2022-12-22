#	BSDI $Id: Makefile,v 1.1.1.1 1993/03/07 18:57:22 sanders Exp $

MAN1=	sdif.0

all lorder: ${MAN1}

clean depend lint tags:

cleandir:
	rm -f ${MAN1}

install: maninstall
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/sdif.sh ${DESTDIR}/${BINDIR}/sdif

.include <bsd.prog.mk>
.include <bsd.man.mk>
