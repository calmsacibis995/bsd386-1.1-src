PROG=less
CFLAGS+=-I${.CURDIR}/obj -g
SRCS=	brac.c ch.c charset.c cmdbuf.c command.c decode.c edit.c \
	filename.c forwback.c help.c ifile.c input.c jump.c line.c \
	linenum.c lsystem.c main.c mark.c optfunc.c option.c opttbl.c \
	os.c output.c position.c prompt.c screen.c search.c signal.c \
	tags.c ttyin.c version.c
LDADD+=	-ltermcap
DPADD+=	${LIBTERMCAP}
CLEANFILES+= funcs.h

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 0444 ${.CURDIR}/less.hlp \
		${DESTDIR}${BINDIR}/../lib

funcs.h ../obj/funcs.h:
	(cd ${.CURDIR}; awk -f mkfuncs.awk $(SRCS) > obj/funcs.h)

.depend: funcs.h

.include "../../Makefile.inc"
.include <bsd.prog.mk>
