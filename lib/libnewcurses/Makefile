#	BSDI $Id: Makefile,v 1.4 1993/12/20 01:24:15 donn Exp $
#	@(#)Makefile	8.2 (Berkeley) 8/21/93

CFLAGS+=#-DTFILE=\"/dev/ttyp0\"
CFLAGS+=-D_CURSES_PRIVATE -I${.CURDIR}
LIB=	newcurses
SRCS=	addbytes.c addch.c addnstr.c box.c clear.c clrtobot.c clrtoeol.c \
	cr_put.c ctrace.c cur_hash.c curses.c delch.c deleteln.c delwin.c \
	erase.c fullname.c getch.c getstr.c id_subwins.c idlok.c initscr.c \
	insch.c insertln.c longname.c move.c mvwin.c newwin.c overlay.c \
	overwrite.c printw.c putchar.c refresh.c scanw.c scroll.c setterm.c \
	standout.c toucholap.c touchwin.c tscroll.c tstp.c tty.c unctrl.c
MAN3=	newcurses.0

beforeinstall:
	-cd ${.CURDIR}; cmp -s curses.h ${DESTDIR}/usr/include/newcurses.h > \
	    /dev/null 2>&1 || \
	    install -c -o ${BINOWN} -g ${BINGRP} -m 444 curses.h \
	    ${DESTDIR}/usr/include/newcurses.h

.include <bsd.lib.mk>
