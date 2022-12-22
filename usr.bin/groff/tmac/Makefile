#	BSDI $Id: Makefile,v 1.2 1994/01/07 21:03:48 donn Exp $

.include "../Makefile.defs"

MAN7=	ms.0
FILES=	eqnrc tmac.X tmac.Xps tmac.dvi tmac.latin1 tmac.pic tmac.ps \
	tmac.psatk tmac.psnew tmac.psold tmac.pspic tmac.s tmac.tty \
	tmac.tty-char troffrc

tags depend:

install: maninstall beforeinstall

beforeinstall:
	for f in ${FILES}; do \
		install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
			${.CURDIR}/$$f ${DESTDIR}${TMACDIR}/; \
	done

.include "../Makefile.common"
.include <bsd.prog.mk>
