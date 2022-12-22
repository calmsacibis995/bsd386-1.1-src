#	@(#)Makefile	5.6 (Berkeley) 9/27/90

PROG=	rsh
SRCS=	rsh.c krcmd.c kcmd.c des_rw.c
# Kerberized version now gets built in domestic
#CFLAGS+=-DKERBEROS -DCRYPT
#DPADD=	${LIBKRB} ${LIBDES}
#LDADD=	-lkrb -ldes
BINOWN=	root
BINMODE=4555
.PATH:	${.CURDIR}/../rlogin

.include <bsd.prog.mk>
