#	@(#)Makefile	5.7 (Berkeley) 1/9/92

PROG=	fstat
SRCS=	fstat.c
CFLAGS+=-I/sys
DPADD=	${LIBKVM} ${LIBUTIL}
LDADD=	-lkvm -lutil
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>
