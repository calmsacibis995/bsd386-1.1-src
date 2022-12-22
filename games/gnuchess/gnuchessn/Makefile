#	BSDI $Id: Makefile,v 1.1.1.1 1994/01/26 19:30:22 donn Exp $

PROG=	gnuchessn
SRCS=	ataks.c book.c dspcom.c eval.c genmoves.c init.c main.c \
	nuxdsp.c search.c util.c
CC=	gcc2
CFLAGS+=-DBINBOOK=\"${LIBDIR}/gnuchess.data\" -DCACHE -DGDX \
	-DHASGETTIMEOFDAY -DHASHFILE=\"/var/games/gnuchess.hash\" \
	-DKILLT -DLANGFILE=\"${LIBDIR}/gnuchess.lang\" -DNULLMOVE \
	-DOLDTIME -DQUIETBACKGROUND -DSEMIQUIETBOOKGEN \
	-O2 -funroll-loops -I. -I${.CURDIR}/../gnuchess
.PATH:	${.CURDIR}/../gnuchess
DPADD+=	${LIBTERM} ${LIBM}
LDADD+=	-lnewcurses -ltermcap -lm
NOMAN=	noman
HIDEGAME=hidegame
CLEANFILES+=curses.h

.depend: curses.h

curses.h:
	ln -s /usr/include/newcurses.h curses.h

.include "../../Makefile.inc"
.include <bsd.prog.mk>
