#       @(#)BSDI $Id: Makefile,v 1.1 1993/12/19 19:51:45 sanders Exp $

FILES=	fax.m4 \
	local.m4 \
	smtp.m4 \
	usenet.m4 \
	uucp.m4

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
		${DESTDIR}${CFDIR}/mailer

obj objdir:

.include <bsd.prog.mk>
