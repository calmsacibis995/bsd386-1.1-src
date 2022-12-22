#	BSDI	$Id: Makefile,v 1.3 1994/02/03 05:06:29 polk Exp $

PROG=	ftpshut
MAN8=	ftpshut.0
SRCS=	ftpshut.c vers.c
CFLAGS+=-I${.CURDIR}/../ftpd
BINDIR=	/usr/sbin

.PATH:	${.CURDIR}/../ftpd/obj

.include <bsd.prog.mk>
