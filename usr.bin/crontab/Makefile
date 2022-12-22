#	BSDI $Id: Makefile,v 1.4 1992/09/25 19:43:30 sanders Exp $

PROG=	crontab
SRCS=	crontab.c entry.c env.c misc.c
CRONDIR=${.CURDIR}/../../usr.sbin/cron
CFLAGS+=-I${CRONDIR}
MAN1=	crontab.0
CLEANFILES=crontab.5.0
.PATH:	${CRONDIR}

BINMODE=4711
BINOWN=root

crontab.0: crontab.5.0

afterinstall:
	install -c -o ${MANOWN} -g ${MANGRP} -m ${MANMODE} crontab.5.0 \
		${DESTDIR}${MANDIR}5/crontab.0

.include <bsd.prog.mk>
