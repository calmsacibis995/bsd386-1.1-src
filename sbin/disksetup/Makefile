#	BSDI	$Id: Makefile,v 1.3 1994/01/11 17:19:59 polk Exp $
#	@(#)Makefile	5.6 (Berkeley) 4/1/91

PROG=	disksetup
SRCS=	disksetup.c dkcksum.c checkget.c z.c interactive.c
MAN8=	disksetup.0
LDFLAGS+=-N
LDADD=	-lcurses -ltermcap -lutil
DPADD=	${LIBCURSES} ${LIBTERMCAP} ${LIBUTIL}
BINGRP=operator
BINMODE=2550

.include <bsd.prog.mk>
