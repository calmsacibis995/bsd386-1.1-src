#	BSDI $Id: Makefile,v 1.3 1993/02/23 17:56:13 polk Exp $

PROG=	jove
SRCS=	argcount.c ask.c buf.c ctype.c delete.c \
	disp.c insert.c io.c jove.c marks.c misc.c re.c \
	screen.c util.c vars.c list.c keymaps.c \
	abbrev.c rec.c paragraph.c fmt.c c.c wind.c fp.c move.c \
	extend.c macros.c iproc.c proc.c scandir.c term.c case.c \
	funcdefs.c re1.c version.c tune.c
CFLAGS+=-I. -I${.CURDIR} -I${.CURDIR}/../include
LDADD+=	-ltermcap -lcompat
DPADD+= ${LIBTERMCAP} ${LIBCOMPAT}
OBJS+=keys.o
CLEANFILES+= setmaps keys.c


.include "../../Makefile.inc"
.include <bsd.prog.mk>

.depend: setmaps keys.c

beforeinstall: 
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/cmds.doc \
		${DESTDIR}/usr/contrib/lib/jove
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/system.rc \
		${DESTDIR}/usr/contrib/lib/jove

setmaps: ${.CURDIR}/setmaps.c
	cc ${CFLAGS} -o setmaps ${.CURDIR}/setmaps.c

keys.c:	setmaps keys.txt
	./setmaps < ${.CURDIR}/keys.txt > keys.c
