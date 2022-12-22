#	BSDI	$Id: Makefile,v 1.1.1.1 1994/01/03 23:13:56 polk Exp $

PROG=	calc
SRCS=	addop.c alloc.c assocfunc.c calc.c codegen.c comfunc.c commath.c \
	config.c const.c file.c func.c hist.c input.c label.c listfunc.c \
	matfunc.c obj.c opcodes.c qfunc.c qio.c qmath.c qmod.c qtrans.c \
	string.c symbol.c token.c value.c version.c zfunc.c zio.c zmath.c \
	zmod.c zmul.c
CFLAGS+=-I${.CURDIR}/gen_h

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 0444 ${.CURDIR}/lib/README \
		${.CURDIR}/lib/[a-z]* ${DESTDIR}/usr/contrib/lib/calc/
	install -c -o ${BINOWN} -g ${BINGRP} -m 0444 ${.CURDIR}/help/[a-z]* \
		${DESTDIR}/usr/contrib/lib/calc/

.include <bsd.prog.mk>
