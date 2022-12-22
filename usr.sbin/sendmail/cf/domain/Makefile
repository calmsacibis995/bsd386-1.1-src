#       @(#)BSDI $Id: Makefile,v 1.1 1993/12/19 19:51:17 sanders Exp $

FILES=	Berkeley.m4 \
	cs.exposed.m4 \
	cs.hidden.m4 \
	eecs.hidden.m4 \
	s2k.m4

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
		${DESTDIR}${CFDIR}/domain

obj objdir:

.include <bsd.prog.mk>
