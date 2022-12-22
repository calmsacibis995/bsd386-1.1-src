#	BSDI $Id: Makefile,v 1.4 1993/03/11 16:13:00 polk Exp $

depend tags:

MAN1=teachjove.0

all: teachjove.0

install: maninstall
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/teachjove.sh ${DESTDIR}/${BINDIR}/teachjove
	install -c -o ${BINOWN} -g ${BINGRP} -m 644 ${.CURDIR}/teach-jove \
		${DESTDIR}/usr/contrib/lib/jove

.include "../../Makefile.inc"

.include <bsd.prog.mk>
