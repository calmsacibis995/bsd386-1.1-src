#	BSDI	$Id: Makefile,v 1.1.1.1 1993/12/21 04:17:52 polk Exp $

PROG=	ftpcount
SRCS=	ftpcount.c vers.c
CFLAGS+=-I${.CURDIR}/../ftpd
BINDIR=	/usr/sbin

.PATH:	${.CURDIR}/../ftpd/obj

.include <bsd.prog.mk>
