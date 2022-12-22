#	@(#)Makefile	5.9 (Berkeley) 9/27/90

PROG=	rlogind
SRCS=	rlogind.c des_rw.c
MAN8=	rlogind.0
DPADD=	${LIBUTIL} 
LDADD=	-lutil

# Kerberized version now gets built in domestic
#CFLAGS+=-DKERBEROS -DCRYPT
#DPADD+= ${LIBKRB} ${LIBDES}
#LDADD+= -lkrb -ldes

.PATH: ${.CURDIR}/../../usr.bin/rlogin

.include <bsd.prog.mk>
