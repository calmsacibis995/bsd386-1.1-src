#	BSDI $Id: Makefile,v 1.2 1994/01/12 00:47:33 torek Exp $

PROG=	bash
# N.B.: bashgetopt.c logically should be here, but is in the builtins
# subdirectory in the bash distribution.
SRCS=	alias.c bashgetopt.c bashline.c bracecomp.c braces.c copy_cmd.c \
	dispose_cmd.c error.c execute_cmd.c expr.c flags.c general.c \
	getcwd.c glob.c hash.c input.c jobs.c mailcheck.c make_cmd.c \
	parse.y print_cmd.c shell.c siglist.c subst.c test.c trap.c \
	unwind_prot.c variables.c version.c
.PATH: ${.CURDIR}/builtins
CFLAGS+=-I. -I${.CURDIR} -I${.CURDIR}/..  -I${.CURDIR}/builtins/obj \
	-I${.CURDIR}/../readline
CFLAGS+=-DHAVE_ALLOCA -DHAVE_DUP2 -DHAVE_GETGROUPS -DHAVE_GETWD \
	-DHAVE_RESOURCE -DHAVE_SETLINEBUF -DHAVE_STRCHR -DHAVE_SYS_SIGLIST \
	-DHAVE_UNISTD_H -DHAVE_WAIT_H
CFLAGS+=-DBsd
CFLAGS+=-DSHELL
CFLAGS+=-DVOID_SIGHANDLER
CFLAGS+=-D"$(MACHINE)"
CFLAGS+=-DHOSTTYPE=\"$(MACHINE)\"
BASHLIB=${.CURDIR}/builtins/obj/libbuiltins.a \
	${.CURDIR}/../readline/obj/libreadline.a
DPADD+= ${BASHLIB} ${LIBTERMCAP}
LDADD+= ${BASHLIB} -ltermcap
CLEANFILES+=parse.c y.tab.h signames.h signames.o

.depend: parse.c signames.c signames.h

parse.c: parse.y
	${YACC} ${YFLAGS} ${.ALLSRC}
	mv y.tab.c ${.TARGET}
	sh ${.CURDIR}/:parsefix ${.TARGET}

signames: signames.o
	${CC} ${CFLAGS} ${.ALLSRC} -o ${.TARGET}

signames.h: signames
	./signames > ${.TARGET}

afterinstall:
	-rm -f ${DESTDIR}/bin/${PROG}
	ln -s ../usr/contrib/bin/${PROG} ${DESTDIR}/bin/${PROG}

.include "../../Makefile.inc"
.include <bsd.prog.mk>
