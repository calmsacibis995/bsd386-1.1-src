## How to make .pro files from .lpro files:
.SUFFIXES: .pro .lpro

PROLOGUES=	tex.pro texps.pro texc.pro special.pro finclude.pro \
		color.pro crop.pro
CLEANFILES+=	${PROLOGUES}

all: ${PROLOGUES}

clean:
	rm -f ${PROLOGUES} texc.lpro
.lpro.pro:
	${.CURDIR}/../obj/squeeze < $< > $@

texc.lpro: ${.CURDIR}/../texc.script tex.lpro
	${.CURDIR}/../texc.script tex.lpro texc.lpro

beforeinstall:
	-mkdir ${DESTDIR}${LIBDIR}
	for file in ${PROLOGUES}; do \
		install -c -o ${BINOWN} -g ${BINGRP} -m 0644 $$file ${DESTDIR}${LIBDIR}; \
	done

.include <bsd.prog.mk>
.include "../../Makefile.inc"
