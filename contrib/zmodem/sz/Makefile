PROG=	sz
SRCS=	sz.c
CFLAGS += -I${.CURDIR}/../common -DV7 -DTXBSIZE=32768 -DNFGVMIN
.PATH:	${.CURDIR}/../common
LINKS+= ${BINDIR}/sz ${BINDIR}/sb
LINKS+= ${BINDIR}/sz ${BINDIR}/sx
MLINKS+=sz.1 sb.1
MLINKS+=sz.1 sx.1
MLINKS+=sz.1 zup.1


afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/zup ${DESTDIR}${BINDIR}

.include "../../Makefile.inc"
.include <bsd.prog.mk>
