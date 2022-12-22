#	BSDI $Id: Makefile,v 1.3 1993/03/23 20:02:30 polk Exp $

PROG=   vgafont
FONTDIR=${DESTDIR}/usr/share/vgafont
CFLAGS+=-I/sys/i386/isa
BINOWN=root
BINMODE=4555

afterinstall:
	-[ -d ${FONTDIR} ] || mkdir ${FONTDIR}
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
		${.CURDIR}/fonts/[a-z]* ${FONTDIR}

.include <bsd.prog.mk>
