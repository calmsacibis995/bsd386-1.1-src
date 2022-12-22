# BSDI $Id: Makefile,v 1.3 1992/11/30 16:48:40 polk Exp $

PROG=	taintperl
SRCS=	array.c cmd.c cons.c consarg.c doarg.c doio.c dolist.c dump.c eval.c \
	form.c hash.c perl.c regcomp.c regexec.c stab.c str.c toke.c \
	usersub.c util.c perly.c unexec.c
CFLAGS+= -I. -I${.CURDIR} -DTAINT -DUNEXEC
LDADD=	-lm
DPADD=	${LIBM}
.PATH:	${.CURDIR}/../perl
NOMAN=	noman

CLEANFILES+= perly.c perly.h y.tab.h y.tab.c

perly.c: perly.y perly.fixer
	${YACC} -d ${.IMPSRC}
	sh ${.CURDIR}/../perl/perly.fixer y.tab.c perly.c
	mv y.tab.h perly.h
	echo 'extern YYSTYPE yylval;' >>perly.h
	
.include "../../Makefile.inc"

beforeinstall:
	-ln -s ../contrib/bin/taintperl ${DESTDIR}/usr/bin/taintperl

.include <bsd.prog.mk>
