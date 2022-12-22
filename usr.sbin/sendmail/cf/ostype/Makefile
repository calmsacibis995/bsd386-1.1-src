#       @(#)BSDI $Id: Makefile,v 1.1 1993/12/19 19:51:54 sanders Exp $

FILES=	aix3.m4 \
	aux.m4 \
	bsd4.3.m4 \
	bsd4.4.m4 \
	hpux.m4 \
	irix.m4 \
	linux.m4 \
	nextstep.m4 \
	osf1.m4 \
	riscos4.5.m4 \
	solaris2.m4 \
	sunos3.5.m4 \
	sunos4.1.m4 \
	svr4.m4 \
	ultrix4.1.m4

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
		${DESTDIR}${CFDIR}/ostype

obj objdir:

.include <bsd.prog.mk>
