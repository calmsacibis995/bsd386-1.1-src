PROG=	texi2roff
SRCS=	texi2roff.c table.c translate.c items.c

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
			${.CURDIR}/texi2index ${DESTDIR}${BINDIR}

.include <bsd.prog.mk>
