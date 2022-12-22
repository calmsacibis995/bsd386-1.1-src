#	BSDI	$Id: Makefile,v 1.1.1.1 1994/01/13 22:42:28 polk Exp $

PROG=	pico
SRCS=	main.c attach.c ansi.c basic.c bind.c browse.c buffer.c \
	composer.c display.c file.c fileio.c line.c osdep.c \
	pico.c random.c region.c search.c spell.c tcap.c window.c word.c
CFLAGS+=-Dbsd -DJOB_CONTROL
LDADD+=	-ltermcap
DPADD+=	${LIBTERMCAP}
NOMAN=	noman

.include <bsd.prog.mk>
