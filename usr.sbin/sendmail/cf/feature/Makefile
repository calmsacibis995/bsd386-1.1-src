#       @(#)BSDI $Id: Makefile,v 1.1 1993/12/19 19:51:22 sanders Exp $

FILES=	allmasquerade.m4 \
	always_add_domain.m4 \
	bitdomain.m4 \
	domaintable.m4 \
	mailertable.m4 \
	nocanonify.m4 \
	nodns.m4 \
	notsticky.m4 \
	nouucp.m4 \
	nullclient.m4 \
	redirect.m4 \
	use_cw_file.m4 \
	uucpdomain.m4

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
		${DESTDIR}${CFDIR}/feature

obj objdir:

.include <bsd.prog.mk>
