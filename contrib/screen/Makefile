PROG=	screen
SRCS=	acl.c ansi.c attacher.c comm.c display.c fileio.c \
	help.c input.c loadav.c mark.c misc.c process.c pty.c \
	putenv.c resize.c screen.c search.c socket.c term.c termcap.c \
	tty.c utmp.c window.c
CFLAGS+= -DBSDI
DPADD+=	${LIBCURSES} ${LIBTERMCAP} ${LIBUTIL} ${LIBKVM}
LDADD+=	-lcurses -ltermcap -lutil -lkvm
MAN1=screen.0

.include "../Makefile.inc"

CFLAGS+= -DETCSCREENRC=\"/usr/contrib/lib/screenrc\"

install: beforeinstall maninstall
	install ${COPY} ${STRIP} -o root -g ${BINGRP} -m 4555 screen \
		${DESTDIR}${BINDIR}

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/etcscreenrc \
		${DESTDIR}${BINDIR}/../lib/screenrc

.include <bsd.prog.mk>
