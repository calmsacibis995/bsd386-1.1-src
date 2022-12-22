#	BSDI	$Id: Makefile,v 1.2 1994/02/03 00:28:36 polk Exp $

PROG=	irc
SRCS=	c_bsd.c c_conf.c c_msg.c c_numeric.c c_version.c edit.c \
	help.c ignore.c irc.c screen.c str.c c_debug.c ctcp.c \
	bsd.c dbuf.c packet.c send.c match.c parse.c support.c
CFLAGS+=-DCLIENT_COMPILE -I${.CURDIR}/../include
LDADD+=	-lcurses -ltermcap
DPADD+=	${LIBCURSES} ${LIBTERMCAP}

.PATH:	${.CURDIR}/../common

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 0644 ${.CURDIR}/ircd.conf \
		${.CURDIR}/ircd.motd ${DESTDIR}/etc/contrib/ircd/

.include "../../Makefile.inc"
.include <bsd.prog.mk>
