#	BSDI	$Id: Makefile,v 1.2 1994/01/30 21:11:15 polk Exp $

PROG=	ntptrace
MAN8=	ntptrace.0
SRCS=	ntptrace.c version.c
CFLAGS+=-DSYS_BSDI -DDEBUG -DREFCLOCK -I${.CURDIR}/../include
LDADD+=	${.CURDIR}/../lib/obj/libntp.a
DPADD+=	${.CURDIR}/../lib/obj/libntp.a
CLEANFILES+=.version version.c

.include "../../Makefile.inc"

.depend: version.c

version.c: ${.CURDIR}/../VERSION
	${.CURDIR}/../mkversion ${PROG} ${.CURDIR}/../VERSION

.include <bsd.prog.mk>
