#	BSDI Makefile,v 1.1.1.1 1992/02/14 05:29:45 trent Exp

PROG=	ksh

CFLAGS+= -D_POSIX_TERM -D_BSDI -DHAVE_SYS_STDTYPES
SRCS=	alloc.c c_ksh.c c_sh.c c_test.c do_ulimit.c edit.c emacs.c \
	eval.c exec.c expr.c getopts.c history.c io.c jobs.c lex.c \
	mail.c main.c misc.c sigact.c syn.c table.c trace.c trap.c \
	tree.c var.c version.c vi.c

afterinstall:
	rm -f ${DESTDIR}/bin/ksh
	ln -s ../usr/contrib/bin/ksh ${DESTDIR}/bin/ksh

.include <bsd.prog.mk>
