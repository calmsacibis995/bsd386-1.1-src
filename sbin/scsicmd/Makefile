#	BSDI $Id: Makefile,v 1.3 1993/02/16 00:04:11 polk Exp $

PROG=	scsicmd
SRCS=	main.c support.c table.c bomb.c
MAN8=	scsicmd.0

CFLAGS+=	-I/sys
LDFLAGS+=	-n
LDADD+=		-lutil
DPADD+=		${LIBUTIL}

.include <bsd.prog.mk>
