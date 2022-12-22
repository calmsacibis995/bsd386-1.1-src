#  BSDI $Id: Makefile,v 1.1.1.1 1992/07/27 23:25:05 polk Exp $ 

all s2p:	

clean depend lint tags cleandir:

install:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/s2p.pl ${DESTDIR}/${BINDIR}/s2p

.include "../../Makefile.inc"
.include <bsd.prog.mk>
