#	BSDI $Id: Makefile,v 1.4 1994/02/03 05:53:20 polk Exp $

PROG=	mrouted
MAN8=	mrouted.0
SRCS=	config.c igmp.c inet.c kern.c main.c route.c vif.c
CFLAGS+=-I${.CURDIR}/../common
.PATH:	${.CURDIR}/../common

.include <bsd.prog.mk>
