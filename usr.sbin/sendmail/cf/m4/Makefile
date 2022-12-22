#       @(#)BSDI $Id: Makefile,v 1.1 1993/12/19 19:51:36 sanders Exp $

FILES=	cf.m4 \
	nullrelay.m4 \
	proto.m4 \
	version.m4

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
		${DESTDIR}${CFDIR}/m4

obj objdir:

.include <bsd.prog.mk>
