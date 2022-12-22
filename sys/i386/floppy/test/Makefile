#        @(#)Makefile	5.1 (Berkeley) 6/8/92

PROG=	test
SRCS=   test.c operators.c
CFLAGS+=-I.
LDFLAGS+=-N
CLEANFILES+= operators.c operators.h unary_op binary_op
SRCDIR=/usr/src/bin/test
NOMAN=noman

.PATH:	/usr/src/bin/test

operators.c operators.h: unary_op binary_op mkops
	cp ${SRCDIR}/unary_op ${SRCDIR}/binary_op .
	sh ${SRCDIR}/mkops

.include <bsd.prog.mk>
