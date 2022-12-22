#	BSDI $Id: Makefile,v 1.2 1993/03/02 18:19:45 polk Exp $

PROG=	scsicmd
SRCS=	main.c support.c table.c bomb.c
NOMAN=	noman

CFLAGS+=	-I/sys
LDFLAGS+=	-N
LDADD+=		-lutil

.PATH:	/usr/src/sbin/scsicmd

.include <bsd.prog.mk>
