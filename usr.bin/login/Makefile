#	@(#)Makefile	5.6 (Berkeley) 6/24/90

PROG=	login
SRCS=	klogin.c login.c
DPADD=	${LIBUTIL}
LDADD=	-lutil

# Kerberized version now gets built in domestic
#CFLAGS+=-DKERBEROS
#DPADD+= ${LIBKRB} ${LIBDES}
#LDADD+= -lkrb -ldes

BINOWN=	root
BINMODE=4555

.include <bsd.prog.mk>
