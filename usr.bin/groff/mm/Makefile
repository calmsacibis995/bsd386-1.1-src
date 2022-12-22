#	BSDI $Id: Makefile,v 1.1.1.1 1993/12/22 06:28:28 polk Exp $

.include "../Makefile.defs"

MAN7=	groff_mm.0 groff_mmse.0
MLINKS=	groff_mm.7 mm.7
FILES=	0.MT 4.MT 5.MT ms.cov se_ms.cov

tags depend:

install: maninstall beforeinstall

beforeinstall:
	for f in ${FILES}; do \
		install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
			${.CURDIR}/mm/$$f ${DESTDIR}${TMACDIR}/mm/; \
	done
	touch ${DESTDIR}${TMACDIR}/mm/locale
	touch ${DESTDIR}${TMACDIR}/mm/se_locale
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/tmac.mse \
		${DESTDIR}${TMACDIR}/
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/tmac.m \
		${DESTDIR}${TMACDIR}/

.include "../Makefile.common"
.include <bsd.prog.mk>
