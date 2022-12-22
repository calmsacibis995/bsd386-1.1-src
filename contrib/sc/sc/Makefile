PROG=	sc
LIBDIR=	${BINDIR}/../lib

CFLAGS+=	-I. -I${.CURDIR} -DBSD43
CFLAGS+=	-DDOBACKUPS -DINTERNATIONAL -DSIGVOID -DREGCOMP
CFLAGS+=	-DDFLT_PAGER=\"more\" -DSAVENAME=\"SC.SAVE\" -DTROFF

LDADD+=		-lm -lcurses -ltermcap
DPADD+=		${LIBMATH} ${LIBCURSES} ${LIBTERMCAP}

CLEANFILES+=	y.tab.h gram.c experres.h statres.h sc.1

SRCS=cmds.c crypt.c format.c gram.c help.c interp.c lex.c range.c sc.c \
	screen.c version.c vi.c vmtbl.c xmalloc.c

y.tab.h: gram.y

experres.h: gram.y
	-rm -f experres.h
	sed < ${.CURDIR}/gram.y > experres.h -f ${.CURDIR}/eres.sed

statres.h: gram.y
	-rm -f statres.h
	sed < ${.CURDIR}/gram.y > statres.h -f ${.CURDIR}/sres.sed

sc.1:	sc.doc
	name=sc NAME=SC VERSION=${.CURDIR}/version.c LIBDIR=/usr/contrib/lib \
		sh ${.CURDIR}/torev ${.CURDIR}/sc.doc > sc.1

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 0444 \
		${.CURDIR}/tutorial.sc ${DESTDIR}/${LIBDIR}

.depend: y.tab.h experres.h statres.h

.include "../../Makefile.inc"
.include <bsd.prog.mk>
