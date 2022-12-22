#	BSDI $Id: Makefile,v 1.2 1994/01/14 21:41:28 donn Exp $

.SUFFIXES:
.SUFFIXES: .elc .el

NOMAN=	noman
CLEANFILES+=ispell.elc

.if exists(/usr/contrib/bin/emacs)
.el.elc:
	if [ ! -r ${.IMPSRC:T} ]; then ln -s ${.IMPSRC} ${.IMPSRC:T}; fi
	PWD=`pwd` \
	/usr/contrib/bin/emacs -batch -l ${.IMPSRC:T} -f batch-make-ispell
.else
.el.elc:
	touch -c ${.TARGET}
.endif

all: ispell.elc

.if exists(/usr/contrib/bin/emacs)
install:
	-@mkdir -p ${DESTDIR}/usr/contrib/lib/emacs/lisp
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
		ispell.el ${DESTDIR}/usr/contrib/lib/emacs/lisp
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
		ispell.elc ${DESTDIR}/usr/contrib/lib/emacs/lisp
.else
install:
.endif

depend:

.include <bsd.prog.mk>
