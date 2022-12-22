#	BSDI $Id: Makefile,v 1.3 1994/02/01 17:53:02 donn Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/7/93

PROG=	mklocale
SRCS=	yacc.c lex.c
CFLAGS+=-I.
CLEANFILES+=y.tab.h yacc.c lex.c

LOCALE=Japanese-EUC POSIX-UTF2 ISO8859-1

afterinstall:
	-umask 022; \
	for f in ${LOCALE}; do \
		mkdir -p ${DESTDIR}/usr/share/locale/$$f; \
		./mklocale < ${.CURDIR}/$$f > \
			${DESTDIR}/usr/share/locale/$$f/LC_CTYPE; \
		chmod 0444 ${DESTDIR}/usr/share/locale/$$f/LC_CTYPE; \
		chown ${BINOWN}.${BINGRP} \
			${DESTDIR}/usr/share/locale/$$f/LC_CTYPE; \
	done

.include <bsd.prog.mk>
