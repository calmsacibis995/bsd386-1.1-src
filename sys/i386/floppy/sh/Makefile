#	%W% (Berkeley) %G%

PROG=	sh
SRCS=	builtins.c cd.c dirent.c echo.c error.c eval.c exec.c expand.c \
	input.c jobs.c mail.c main.c memalloc.c miscbltin.c \
	mystring.c nodes.c options.c parser.c redir.c show.c signames.c \
	syntax.c trap.c output.c var.c
OBJS+=	init.o
CFLAGS+=-DSHELL -I. -I/usr/src/bin/sh
# LDFLAGS+=-N
.PATH:	/usr/src/bin/sh /usr/src/bin/sh/bltin
NOMAN=noman
CLEANFILES+=\
	builtins.c builtins.h init.c mkinit mknodes mksignames mksyntax \
	nodes.c nodes.h signames.c signames.h syntax.c syntax.h token.def

.depend parser.o: token.def
token.def: mktokens
	sh /usr/src/bin/sh/mktokens

builtins.h builtins.c: /usr/src/bin/sh/mkbuiltins /usr/src/bin/sh/builtins
	sh /usr/src/bin/sh/mkbuiltins /usr/src/bin/sh

init.c: mkinit ${SRCS}
	./mkinit '${CC} -c ${CFLAGS} init.c' ${.ALLSRC}

mkinit: /usr/src/bin/sh/mkinit.c
	${CC} ${CFLAGS} /usr/src/bin/sh/mkinit.c -o $@

nodes.c nodes.h: mknodes /usr/src/bin/sh/nodetypes /usr/src/bin/sh/nodes.c.pat
	./mknodes /usr/src/bin/sh/nodetypes /usr/src/bin/sh/nodes.c.pat

mknodes: /usr/src/bin/sh/mknodes.c
	${CC} ${CFLAGS} /usr/src/bin/sh/mknodes.c -o $@

signames.c signames.h: mksignames
	./mksignames

mksignames: /usr/src/bin/sh/mksignames.c
	${CC} ${CFLAGS} /usr/src/bin/sh/mksignames.c -o $@

syntax.c syntax.h: mksyntax
	./mksyntax

mksyntax: /usr/src/bin/sh/mksyntax.c /usr/src/bin/sh/parser.h
	${CC} ${CFLAGS} /usr/src/bin/sh/mksyntax.c -o $@

.include <bsd.prog.mk>
