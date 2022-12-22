#	BSDI $Id: Makefile,v 1.3 1994/02/03 05:51:48 polk Exp $

PROG=	mrinfo
MAN8=	mrinfo.0
SRCS=	igmp.c inet.c kern.c mrinfo.c
CFLAGS+=-I${.CURDIR}/../common
.PATH:	${.CURDIR}/../common

.include <bsd.prog.mk>
