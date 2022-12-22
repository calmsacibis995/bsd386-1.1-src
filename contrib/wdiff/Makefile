#	BSDI	$Id: Makefile,v 1.1.1.1 1994/01/13 21:15:46 polk Exp $

PROG=	wdiff
CFLAGS+=-DSTDC_HEADERS -DHAVE_UNISTD_H -DHAVE_STRING_H -DHAVE_TPUTS \
	-DDIFF_PROGRAM=\"diff\" -DPAGER_PROGRAM=\"more\"
SRCS=	wdiff.c readpipe.c writepipe.c error.c getopt.c getopt1.c
LDADD+=	-ltermcap
DPADD+=	${LIBTERMCAP}

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/cbars.sh ${DESTDIR}${BINDIR}/cbars

.include <bsd.prog.mk>
