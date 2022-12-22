#	BSDI $Id: Makefile,v 1.1.1.1 1993/12/21 04:23:11 polk Exp $

.include "../../Makefile.inc"

MAN1=	rcsfreeze.0

all rcsfreeze: ${MAN1}

clean depend lint tags:

cleandir:
	rm -f ${MAN1}

install: maninstall
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/rcsfreeze.sh ${DESTDIR}/${BINDIR}/rcsfreeze

.include <bsd.prog.mk>
.include <bsd.man.mk>
