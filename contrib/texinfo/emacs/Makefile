#	BSDI $Id: Makefile,v 1.1.1.1 1994/01/04 09:03:25 donn Exp $

LISPDIR=/usr/contrib/lib/emacs/lisp
ELFILES=detexinfo.el info.el informat.el makeinfo.el texinfmt.el \
	texinfo.el texnfo-tex.el texnfo-upd.el

all clean cleandir obj objdir:

install:
	-mkdir -p ${DESTDIR}${LISPDIR}
	for i in ${ELFILES}; \
	do \
		cmp -s ${.CURDIR}/$$i ${DESTDIR}${LISPDIR}/$$i || \
			install -c -m 0444 -o ${BINOWN} -g ${BINGRP} \
			    ${.CURDIR}/$$i ${DESTDIR}${LISPDIR}/$$i; \
	done

.include "../../Makefile.inc"
.include <bsd.prog.mk>
