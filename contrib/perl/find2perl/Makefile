#  BSDI $Id: Makefile,v 1.1.1.1 1992/07/27 23:24:01 polk Exp $ 

all find2perl:	

clean depend lint tags cleandir:

install:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/find2perl.pl ${DESTDIR}/${BINDIR}/find2perl

.include "../../Makefile.inc"
.include <bsd.prog.mk>
