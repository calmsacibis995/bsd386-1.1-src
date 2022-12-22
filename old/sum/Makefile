# BSDI $Id: Makefile,v 1.1.1.1 1992/02/16 21:43:51 trent Exp $

MAN1=	sum.0

all sum: ${MAN1}

clean depend lint tags:

cleandir:
	rm -f ${MAN1}

install:	maninstall
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/sum.sh ${DESTDIR}/${BINDIR}/sum

.include <bsd.prog.mk>
.include <bsd.man.mk>
