#	BSDI $Id: Makefile,v 1.1.1.1 1993/12/21 04:16:45 polk Exp $

PROG=	diff3
SRCS=	diff3.c getopt.c getopt1.c version.c
CFLAGS+=-DDIFF_PROGRAM=\"/usr/bin/diff\" 
CFLAGS+=-DHAVE_CONFIG_H -I${.CURDIR}/../../usr.bin/diff/diff

.PATH:	${.CURDIR}/../../usr.bin/diff/diff

.include <bsd.prog.mk>
