#	@(#)Makefile	6.13 (Berkeley) 3/20/91

# tmac.doc.old is currently missing
FILES=	tmac.an.old tmac.r tmac.andoc man.local
MSRCS=  doc doc-ditroff doc-common doc-nroff doc-syms
LINKS=	${BINDIR}/tmac/tmac.andoc ${BINDIR}/tmac/tmac.an
NOOBJ=	noobj

# old macros not currently installed...
OFILES=	tmac.an.v6compat tmac.an6n tmac.an6t tmac.a tmac.cp

all clean cleandir depend lint tags:

beforeinstall:
	for i in ${MSRCS}; do \
		sed -f strip.sed < $$i > ${DESTDIR}${BINDIR}/tmac/tmac.$$i; \
		chown ${BINOWN}.${BINGRP} ${DESTDIR}${BINDIR}/tmac/tmac.$$i; \
		chmod 444 ${DESTDIR}${BINDIR}/tmac/tmac.$$i; \
	done
	install -c -o ${BINOWN} -g ${BINOWN} -m 444 ${FILES} \
		${DESTDIR}${BINDIR}/tmac

afterinstall:
	-rm -f ${DESTDIR}${BINDIR}/tmac/tmac.an
	ln ${DESTDIR}${BINDIR}/tmac/tmac.andoc ${DESTDIR}${BINDIR}/tmac/tmac.an


.include <bsd.prog.mk>
