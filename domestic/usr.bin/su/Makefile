#	@(#)Makefile	5.5 (Berkeley) 5/11/90

PROG=	su
SRCS=	su.c
CFLAGS+=-DKERBEROS
DPADD=	${LIBKRB} ${LIBDES}
LDADD=	-lkrb -ldes
BINOWN=	root
BINMODE=4555

.PATH:	${.CURDIR}/../../../usr.bin/su

.include <bsd.prog.mk>
