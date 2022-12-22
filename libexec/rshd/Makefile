#	@(#)Makefile	5.6 (Berkeley) 9/27/90

PROG=	rshd
SRCS=	rshd.c des_rw.c
MAN8=	rshd.0

DPADD=	${LIBUTIL} 
LDADD=	-lutil

# Kerberized version now gets build in domestic
#CFLAGS+=-DKERBEROS -DCRYPT
#DPADD+= ${LIBKRB} ${LIBDES}
#LDADD+= -lkrb -ldes

.PATH: ${.CURDIR}/../../usr.bin/rlogin

.include <bsd.prog.mk>
