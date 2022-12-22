#	BSDI $Id: Makefile,v 1.2 1993/12/22 20:29:45 polk Exp $

.include "../Makefile.defs"

DEV=	devps
DISTFILES=text.enc download \
	S ZD ZDR SS AB ABI AI AR BMB BMBI BMI BMR \
	CB CBI CI CR HB HBI HI HR HNB HNBI HNI HNR \
	NB NBI NI NR PB PBI PI PR TB TBI TI TR ZCMI
PSFILES=prologue symbolsl.pfa zapfdr.pfa
DEVGENFILES=generate/Makefile generate/afmname generate/dingbatsmap \
	generate/dingbatsrmap generate/lgreekmap generate/symbol.sed \
	generate/symbolchars generate/symbolsl.afm generate/textmap
CLEANFILES+=DESC ${PSFILES}

tags depend:

all: DESC ${PSFILES}

DESC: ${.CURDIR}/DESC.in
	@echo making ${.TARGET}
	@cat ${.CURDIR}/DESC.in > ${.TARGET}
	@echo broken ${BROKEN_SPOOLER_FLAGS} >>${.TARGET}
	@echo "# For A4 paper use: paperlength 841890" >>${.TARGET}
	@echo "paperlength 792000" >>${.TARGET}
	@echo print lpr >>${.TARGET}

prologue: prologue.ps
zapfdr.pfa: zapfdr.ps
symbolsl.pfa: symbolsl.ps

${PSFILES}:
	sed -f ${.CURDIR}/psstrip.sed ${.CURDIR}/${.OODATE} >${.TARGET}

install:
	for f in ${DISTFILES} ${DEVGENFILES}; do \
		install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
			${.CURDIR}/$$f ${DESTDIR}${FONTDIR}/${DEV}/$$f; \
	done
	for f in DESC ${PSFILES}; do \
		install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
			$$f ${DESTDIR}${FONTDIR}/${DEV}/; \
	done

.include <bsd.prog.mk>
