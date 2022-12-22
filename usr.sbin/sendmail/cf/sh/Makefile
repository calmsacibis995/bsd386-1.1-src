#       @(#)BSDI $Id: Makefile,v 1.1 1993/12/19 19:51:58 sanders Exp $

FILES=	makeinfo.sh

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
		${DESTDIR}${CFDIR}/sh

obj objdir:

.include <bsd.prog.mk>
