#	BSDI	$Id: Makefile,v 1.2 1994/01/04 04:05:35 polk Exp $
#	@(#)Makefile	5.13 (Berkeley) 3/16/92

PROG=	restore
LINKS=	${BINDIR}/restore ${BINDIR}/rrestore
CFLAGS+=-DRRESTORE -I${.CURDIR} -I${.CURDIR}/../dump
SRCS=	main.c interactive.c restore.c dirs.c symtab.c tape.c utilities.c \
	dumprmt.c
BINOWN=	root
BINGRP=	tty
BINMODE=6555
MAN8=	restore.0
MLINKS+=restore.8 rrestore.8
.PATH:	${.CURDIR}/../dump

all: ${PROG} ${MAN8}

.include <bsd.prog.mk>
