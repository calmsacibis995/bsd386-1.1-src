#	BSDI	$Id: Makefile,v 1.2 1993/12/23 17:38:07 polk Exp $

PROG=	ftpd
SRCS=	ftpd.c ftpcmd.c glob.c logwtmp.c popen.c vers.c access.c extensions.c \
	realpath.c acl.c private.c authenticate.c conversions.c hostacc.c
CFLAGS+=-I${.CURDIR}/../support 
LDADD+= ${.CURDIR}/../support/obj/libsupport.a -lutil
DPADD+=	${.CURDIR}/../support/obj/libsupport.a ${LIBUTIL}
MAN8=	ftpd.0
MAN5=	ftpaccess.0 ftpconversions.0 ftphosts.0 xferlog.0

.include "../../Makefile.inc"

all: ${FILEMANS}

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/xferstats ${DESTDIR}/usr/sbin
	install -c -o ${BINOWN} -g ${BINGRP} -m 0444 \
		${.CURDIR}/examples/[a-z]* ${DESTDIR}/etc/ftpd_samples

.depend: vers.c

vers.c: ftpd.c ftpcmd.y
	sh ${.CURDIR}/newvers.sh

.include <bsd.prog.mk>
