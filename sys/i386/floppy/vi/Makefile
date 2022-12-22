#	BSDI $Id: Makefile,v 1.1.1.1 1992/12/02 18:06:27 polk Exp $

PROG=	vi
SRCS=	blk.c cmd1.c cmd2.c ctype.c curses.c cut.c ex.c input.c main.c \
	misc.c modify.c move1.c move2.c move3.c move4.c move5.c opts.c \
	recycle.c redraw.c regexp.c regsub.c system.c tio.c tmp.c \
	unix.c vars.c vcmd.c vi.c
DPADD=	${LIBTERMCAP}
LDADD=	-ltermcap
CFLAGS+=-Dbsd
LDFLAGS+=-N

.PATH:	/usr/src/usr.bin/vi/vi

.include <bsd.prog.mk>
