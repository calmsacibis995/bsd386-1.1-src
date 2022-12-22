#	@(#)Makefile	5.15 (Berkeley) 5/27/92

PROG=	netstat
SRCS=	host.c inet.c if.c main.c mbuf.c route.c unix.c ns.c iso.c \
	tp_astring.c mroute.c
BINGRP=	kmem
BINMODE=2555
LDADD=	-lkvm
DPADD=	${LIBKVM}

.PATH: ${.CURDIR}/../../sys/netiso

.include <bsd.prog.mk>
