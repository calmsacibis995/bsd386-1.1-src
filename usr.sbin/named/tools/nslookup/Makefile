#	BSDI $Id: Makefile,v 1.2 1993/12/19 08:34:45 donn Exp $
#	@(#)Makefile	5.23 (Berkeley) 6/24/90

PROG=	nslookup
SRCS=	main.c getinfo.c debug.c send.c skip.c list.c subr.c
OBJS+=	commands.o
MAN8=	nslookup.0
LFLAGS=	-I
CFLAGS+=-I${.CURDIR} -I${.CURDIR}/../..
DPADD=	${LIBL}
LDADD=	-ll
CLEANFILES+=commands.c lex.yy.c lex.yy.o

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/nslookup.help \
	    ${DESTDIR}/usr/share/misc

.include "../../../Makefile.inc"
.include <bsd.prog.mk>
