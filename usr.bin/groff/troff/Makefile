#	BSDI $Id: Makefile,v 1.1.1.1 1993/12/22 06:27:23 polk Exp $

.include "../Makefile.defs"

PROG=	troff
SRCS=   column.cc dictionary.cc div.cc env.cc input.cc majorminor.cc \
	node.cc number.cc reg.cc symbol.cc
LDADD+=	${LIBGROFF} ${LIBM} ${LIBGCC}
DPADD+=	${LIBGROFF} ${LIBM} ${LIBGCC}
CLEANFILES+=majorminor.cc

majorminor.cc: ${.CURDIR}/../VERSION
	@echo Making ${.TARGET}
	@echo const char \*major_version = \
	\"`sed -e 's/^\([^.]*\)\..*$$/\1/' ${.CURDIR}/../VERSION`\"\; >${.TARGET}
	@echo const char \*minor_version = \
	\"`sed -e 's/^[^.]*\.\([0-9]*\).*$$/\1/' ${.CURDIR}/../VERSION`\"\; >>${.TARGET}

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/hyphen.us \
		${DESTDIR}/${TMACDIR}/

.include "../Makefile.common"
.include <bsd.prog.mk>
