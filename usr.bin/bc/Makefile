# BSDI $Id: Makefile,v 1.3 1993/12/21 03:57:07 polk Exp $

PROG=	bc
CFLAGS+=-I. -I${.CURDIR}		# -D_POSIX_SOURCE 
SRCS=	execute.c global.c global.h load.c main.c number.c \
	storage.c util.c scan.c bc.c
CLEANFILES+=y.tab.h bc.c scan.c

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/libmath.b \
	    ${DESTDIR}/usr/share/misc
	    
.include <bsd.prog.mk>
