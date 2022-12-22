#	$Id: Makefile,v 1.3 1994/01/13 23:02:31 polk Exp $

PROG=	zsh
SRCS=	builtin.c cond.c exec.c glob.c hist.c init.c jobs.c lex.c \
	loop.c math.c mem.c params.c parse.c subst.c table.c text.c \
	utils.c watch.c zle_bindings.c zle_hist.c zle_main.c \
	zle_misc.c zle_move.c zle_refresh.c zle_tricky.c zle_utils.c \
	zle_vi.c zle_word.c
CFLAGS+=-I.
PROTO=	${SRCS:R:S/$/.pro/g}
CLEANFILES+=${PROTO}

LDADD=	-ltermcap

afterinstall:
	-rm -f ${DESTDIR}/bin/${PROG}
	ln -s ../usr/contrib/bin/${PROG} ${DESTDIR}/bin/${PROG}

.include "../../Makefile.inc"
.include <bsd.prog.mk>

.SUFFIXES: .c .pro

.c.pro:
	sed -n '/\/\*\*\/$$/{N;s/^\([^(]*\).*\/\*\*\/.\(.*\)/\1 DCLPROTO((\2))/p;}' $< | sed -e 's/;/,/g' -e 's/,))$$/));/' -e 's/(({))$$/((void));/' >$@

.depend: ${PROTO}
