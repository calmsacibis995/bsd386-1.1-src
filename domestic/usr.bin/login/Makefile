#	@(#)Makefile	5.6 (Berkeley) 6/24/90

PROG=	login
SRCS=	klogin.c login.c
DPADD=	${LIBUTIL}
LDADD=	-lutil
CFLAGS+=-DKERBEROS
DPADD+= ${LIBKRB} ${LIBDES}
LDADD+= -lkrb -ldes

BINOWN=	root
BINMODE=4555

.PATH: ${.CURDIR}/../../../usr.bin/login

.include <bsd.prog.mk>
