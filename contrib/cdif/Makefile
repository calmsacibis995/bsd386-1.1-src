#	BSDI $Id: Makefile,v 1.1.1.1 1993/03/07 18:58:03 sanders Exp $

MAN1=	cdif.0

all lorder: ${MAN1}

clean depend lint tags:

cleandir:
	rm -f ${MAN1}

install: maninstall
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/cdif.sh ${DESTDIR}/${BINDIR}/cdif

.include <bsd.prog.mk>
.include <bsd.man.mk>
