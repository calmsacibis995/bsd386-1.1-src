PROG=ispell

CFLAGS += -DRETSIGTYPE=void -DHAVE_TERMIOS_H=1 -DHAVE_STRING_H=1 \
	-DDICT_LIB=\"/usr/libdata\"

SRCS= access.c case.c charset.c ctbl.c downcase.c exp.c gen.c getopt.c \
	getopt1.c good.c hash.c ispell.c lexfsm.c nextprime.c priv.c \
	screen.c suf.c term.c tex.c util.c version.c

LDADD =  -ltermlib

LINKS += ${BINDIR}/ispell ${BINDIR}/spell
MLINKS += ispell.1 spell.1

.include "../../Makefile.inc"
.include <bsd.prog.mk>
