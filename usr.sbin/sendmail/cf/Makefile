#       @(#)BSDI $Id: Makefile,v 1.1 1993/12/19 19:50:58 sanders Exp $

.include "Makefile.inc"

SUBDIR=	cf domain feature hack m4 mailer ostype sh siteconfig
FILES=	README

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 644 ${FILES} \
		${DESTDIR}${CFDIR}

.include <bsd.subdir.mk>
