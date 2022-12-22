#	BSDI	$Id: Makefile,v 1.1.1.1 1994/01/03 23:16:19 polk Exp $

PROG=	top
SRCS=	top.c commands.c display.c screen.c username.c utils.c \
	version.c getopt.c machine.c
CFLAGS+=-I.
LDADD+=	-ltermcap -lm -lkvm 
DPADD+=	${LIBTERMCAP} ${LIBM} ${LIBKVM}
BINGRP=	kmem
BINMODE=2555
CLEANFILES+=sigdesc.h

.depend: sigdesc.h

sigdesc.h: /usr/include/signal.h
	awk -f ${.CURDIR}/sigconv.awk /usr/include/signal.h > $@

.include <bsd.prog.mk>
