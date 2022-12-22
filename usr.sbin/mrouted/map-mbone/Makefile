#	BSDI $Id: Makefile,v 1.1 1994/01/05 02:03:20 torek Exp $

PROG=	map-mbone
SRCS=	igmp.c inet.c kern.c mapper.c
CFLAGS+=-I${.CURDIR}/../common
.PATH:	${.CURDIR}/../common
NOMAN=	noman

.include <bsd.prog.mk>
