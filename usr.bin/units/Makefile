#	BSDI $Id: Makefile,v 1.1.1.1 1993/03/08 06:46:48 polk Exp $

MAN1=	units.0

all units: ${MAN1}

clean depend lint tags:

cleandir:
	rm -f ${MAN1}

install: maninstall
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/units.pl ${DESTDIR}/${BINDIR}/units

.include <bsd.prog.mk>
.include <bsd.man.mk>
