#	BSDI	$Id: Makefile,v 1.1.1.1 1994/01/13 22:42:32 polk Exp $

PROG=	md5
SRCS=	md5drvr.c md5dual.c md5.c
CFLAGS+=-ansi -fomit-frame-pointer -fcaller-saves
CFLAGS+=-DTLIB=\"/usr/contrib/lib/md5\"

SUBDIR=	tests

.include <bsd.prog.mk>
