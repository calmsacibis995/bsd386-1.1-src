#	BSDI $Id: Makefile,v 1.2 1994/01/12 11:34:26 donn Exp $
#	@(#)Makefile	5.4 (Berkeley) 6/24/90
#
# By default, flex will be configured to generate 8-bit scanners only if the
# -8 flag is given.  If you want it to always generate 8-bit scanners, add
# "-DDEFAULT_CSIZE=256" to CFLAGS.  Note that doing so will double the size
# of all uncompressed scanners.
# 
# If on your system you have trouble building flex due to 8-bit character
# problems, remove the -8 from FLEX_FLAGS and the "#define FLEX_8_BIT_CHARS"
# from the beginning of flexdef.h.
#
# To bootstrap lex, cp grot/initscan.c to scan.c and run make.

PROG=	lex
CFLAGS+=-I. -I${.CURDIR} -DHAVE_STRING_H
LFLAGS=	-is8
SRCS=   ccl.c dfa.c ecs.c gen.c main.c misc.c nfa.c parse.c scan.c \
	skel.c sym.c tblcmp.c yylex.c
SUBDIR=	lib
CLEANFILES+=parse.c parse.h scan.c y.tab.h
MAN1=	lex.0 flexdoc.0

parse.h parse.c: parse.y
	${YACC} -d ${.CURDIR}/parse.y
	mv y.tab.c parse.c
	mv y.tab.h parse.h

scan.o: parse.h
yylex.o: parse.h
.depend: parse.h

.include <bsd.prog.mk>
