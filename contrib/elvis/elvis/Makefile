#	BSDI $Id: Makefile,v 1.3 1994/01/15 07:12:56 polk Exp $

.include "../../Makefile.inc"

PROG=	elvis
SRCS=	blk.c cmd1.c cmd2.c ctype.c curses.c cut.c ex.c input.c main.c \
	misc.c modify.c move1.c move2.c move3.c move4.c move5.c opts.c \
	recycle.c redraw.c regexp.c regsub.c system.c tio.c tmp.c \
	unix.c vars.c vcmd.c vi.c
DPADD=	${LIBTERMCAP}
LDADD=	-ltermcap
CFLAGS+=-Dbsd

# Nvi has replaced this as vi, so just call it elvis
# LINKS=	${BINDIR}/vi ${BINDIR}/ex ${BINDIR}/vi ${BINDIR}/view
# MLINKS=	vi.1 ex.1 vi.1 view.1

.include <bsd.prog.mk>
