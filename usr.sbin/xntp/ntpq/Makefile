#	BSDI	$Id: Makefile,v 1.2 1994/01/30 21:10:48 polk Exp $

PROG=	ntpq
MAN8=	ntpq.0
SRCS=	ntpq.c ntpq_ops.c version.c
CFLAGS+=-DSYS_BSDI -DDEBUG -DREFCLOCK -I${.CURDIR}/../include
LDADD+=	${.CURDIR}/../lib/obj/libntp.a
DPADD+=	${.CURDIR}/../lib/obj/libntp.a
CLEANFILES+=.version version.c

.include "../../Makefile.inc"

.depend: version.c

version.c: ${.CURDIR}/../VERSION
	${.CURDIR}/../mkversion ${PROG} ${.CURDIR}/../VERSION

.include <bsd.prog.mk>
