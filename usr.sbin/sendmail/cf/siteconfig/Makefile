#       @(#)BSDI $Id: Makefile,v 1.2 1993/12/23 17:27:28 polk Exp $

FILES=	uucp.cogsci.m4 \
	uucp.old.arpa.m4 \
	uucp.ucbarpa.m4 \
	uucp.ucbvax.m4

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
		${DESTDIR}${CFDIR}/siteconfig

obj objdir:

.include <bsd.prog.mk>
