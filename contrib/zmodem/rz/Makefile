PROG=	rz
SRCS=	rz.c
CFLAGS += -I${.CURDIR}/../common -DMD=2 -DV7
.PATH:	${.CURDIR}/../common
LINKS+= ${BINDIR}/rz ${BINDIR}/rb 
MLINKS+=rz.1 rb.1
MLINKS+=rz.1 zdown.1

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/zdown ${DESTDIR}${BINDIR}

.include "../../Makefile.inc"
.include <bsd.prog.mk>
