#	BSDI	$Id: Makefile,v 1.1.1.1 1993/12/21 04:28:12 polk Exp $

PROG=	xntpres
SRCS=	xntpres.c version.c
CFLAGS+=-DSYS_BSDI -DDEBUG -DREFCLOCK -I${.CURDIR}/../include
LDADD+=	${.CURDIR}/../lib/obj/libntp.a
DPADD+=	${.CURDIR}/../lib/obj/libntp.a
CLEANFILES+=.version version.c
NOMAN=noman

.include "../../Makefile.inc"

.depend: version.c

version.c: ${.CURDIR}/../VERSION
	${.CURDIR}/../mkversion ${PROG} ${.CURDIR}/../VERSION

.include <bsd.prog.mk>
