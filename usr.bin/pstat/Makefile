#	BSDI	$Id: Makefile,v 1.3 1994/02/03 05:31:55 polk Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	pstat
MAN8=	pstat.0
SRCS=	pstat.c getbsize.c
BINGRP=	kmem
BINMODE=2555
DPADD=	${LIBKVM} ${LIBUTIL}
LDADD=	-lkvm -lutil

.include <bsd.prog.mk>
