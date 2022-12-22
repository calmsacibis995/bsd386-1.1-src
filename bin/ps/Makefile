#	BSDI $Id: Makefile,v 1.3 1993/01/07 18:36:00 sanders Exp $
#	@(#)Makefile	5.9 (Berkeley) 4/8/92

PROG=	ps
SRCS=	fmt.c keyword.c nlist.c print.c ps.c
CFLAGS+=-I/sys
DPADD=	${LIBMATH} ${LIBKVM} ${LIBUTIL}
LDADD=	-lm -lkvm -lutil
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>
