#       @(#)BSDI $Id: Makefile,v 1.1 1993/12/19 19:51:26 sanders Exp $

FILES=	cssubdomain.m4

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
		${DESTDIR}${CFDIR}/hack

obj objdir:

.include <bsd.prog.mk>
