#	@(#)Makefile	5.6 (Berkeley) 4/1/91

PROG=	disklabel
SRCS=	disklabel.c dkcksum.c
MAN8=	disklabel.0
LDADD+=	-lutil
DPADD+=	${LIBUTIL}
BINGRP=operator
BINMODE=2550
CLEANFILES=disklabel.5.0

all: disklabel.5.0

beforeinstall:
	install -c -o ${MANOWN} -g ${MANGRP} -m ${MANMODE} disklabel.5.0 \
	    ${DESTDIR}${MANDIR}5/disklabel.0

.include <bsd.prog.mk>
