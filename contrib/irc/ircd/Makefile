#	BSDI	$Id: Makefile,v 1.3 1994/02/03 04:37:07 polk Exp $

PROG=	ircd
MAN8=	ircd.0
SRCS=	bsd.c dbuf.c packet.c send.c match.c parse.c support.c channel.c \
	class.c hash.c ircd.c list.c res.c s_auth.c s_bsd.c s_conf.c \
	s_debug.c s_err.c s_misc.c s_numeric.c s_serv.c s_user.c whowas.c \
	note.c version.c
CFLAGS+=-I${.CURDIR}/../include
LDADD+=	-lcurses -ltermcap
DPADD+=	${LIBCURSES} ${LIBTERMCAP}

.PATH:	${.CURDIR}/../common

.include "../../Makefile.inc"
.include <bsd.prog.mk>
