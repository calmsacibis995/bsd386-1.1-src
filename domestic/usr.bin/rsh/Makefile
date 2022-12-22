#	@(#)Makefile	5.6 (Berkeley) 9/27/90

PROG=	rsh
CFLAGS+=-DKERBEROS -DCRYPT
SRCS=	rsh.c krcmd.c kcmd.c des_rw.c
DPADD=	${LIBKRB} ${LIBDES}
LDADD=	-lkrb -ldes
BINOWN=	root
BINMODE=4555
.PATH:	${.CURDIR}/../../../usr.bin/rsh ${.CURDIR}/../../../usr.bin/rlogin

.include <bsd.prog.mk>
