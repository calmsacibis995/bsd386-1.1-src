#	BSDI $Id: Makefile,v 1.1.1.1 1994/01/04 17:12:05 polk Exp $

#CFLAGS=-g
CFLAGS+=-I${.CURDIR} -I${.CURDIR}/.. -I${.CURDIR}/../bash
CFLAGS+=-DVI_MODE
CFLAGS+=-DHAVE_SETLINEBUF
CFLAGS+=-DHAVE_UNISTD_H
CFLAGS+=-DHAVE_GETGROUPS
CFLAGS+=-DHAVE_RESOURCE
CFLAGS+=-DHAVE_WAIT_H
CFLAGS+=-DHAVE_GETWD
CFLAGS+=-DHAVE_DUP2
CFLAGS+=-DSHELL
CFLAGS+=-DHAVE_ALLOCA
CFLAGS+=-DVOID_SIGHANDLER
CFLAGS+=-D"$(MACHINE)"
CFLAGS+=-DHOSTTYPE=\"$(MACHINE)\"
LIB=	readline
SRCS=	bind.c complete.c display.c funmap.c history.c isearch.c \
	keymaps.c parens.c readline.c rltty.c search.c signals.c vi_mode.c
SRCS+=	tilde.c
NOMAN=	noman
NOPROFILE=noprofile

install:

.include <bsd.lib.mk>
