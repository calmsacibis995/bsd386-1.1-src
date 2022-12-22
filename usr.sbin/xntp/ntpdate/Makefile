#	BSDI	$Id: Makefile,v 1.2 1994/01/30 21:10:34 polk Exp $

PROG=	ntpdate
MAN8=	ntpdate.0
SRCS=	ntpdate.c version.c
CFLAGS+=-DSYS_BSDI -DDEBUG -DREFCLOCK -I${.CURDIR}/../include
LDADD+=	${.CURDIR}/../lib/obj/libntp.a
DPADD+=	${.CURDIR}/../lib/obj/libntp.a
CLEANFILES+=.version version.c

.include "../../Makefile.inc"

.depend: version.c

version.c: ${.CURDIR}/../VERSION
	${.CURDIR}/../mkversion ${PROG} ${.CURDIR}/../VERSION

.include <bsd.prog.mk>
