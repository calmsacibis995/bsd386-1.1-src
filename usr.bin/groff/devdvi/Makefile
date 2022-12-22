#	BSDI $Id: Makefile,v 1.3 1994/01/11 23:49:03 polk Exp $

.include "../Makefile.defs"

DEV=	devdvi
DEVFILES=R I B BI CB CO CR CW MI S EX H HI HB SA SB \
  	generate/CompileFonts generate/Makefile generate/msam.map \
	generate/msbm.map generate/texb.map generate/texex.map \
	generate/texi.map generate/texmi.map generate/texr.map \
	generate/texsy.map generate/textt.map
CLEANFILES+=DESC

tags depend: 

all: DESC

DESC: DESC.in
	@echo making ${.TARGET}
	@cat ${.CURDIR}/DESC.in >${.TARGET}
	@echo print 'lpr -d' >>${.TARGET}

install:
	for f in ${DEVFILES}; do \
		install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
			${.CURDIR}/$$f ${DESTDIR}${FONTDIR}/${DEV}/$$f; \
	done
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 DESC \
		${DESTDIR}${FONTDIR}/${DEV}/

.include <bsd.prog.mk>
