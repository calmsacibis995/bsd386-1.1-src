#	BSDI	$Id: Makefile,v 1.1.1.1 1994/01/13 22:42:34 polk Exp $

PROG=	shs
SRCS=	shsdrvr.c shsdual.c shs.c
CFLAGS+=-ansi -fomit-frame-pointer -fcaller-saves
CFLAGS+=-DTLIB=\"/usr/contrib/lib/shs\"

SUBDIR=	tests

.include <bsd.prog.mk>
