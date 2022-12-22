#	@(#)bsd.man.mk	5.2 (Berkeley) 5/11/90

.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif

MANGRP?=	bin
MANOWN?=	bin
MANMODE?=	444

MANDIR?=	/usr/share/man/cat

MINSTALL=	install -c -o ${MANOWN} -g ${MANGRP} -m ${MANMODE}

maninstall:
.if defined(MAN1) && !empty(MAN1)
	${MINSTALL} ${MAN1} ${DESTDIR}${MANDIR}1${MANSUBDIR}
.endif
.if defined(MAN2) && !empty(MAN2)
	${MINSTALL} ${MAN2} ${DESTDIR}${MANDIR}2${MANSUBDIR}
.endif
.if defined(MAN3) && !empty(MAN3)
	${MINSTALL} ${MAN3} ${DESTDIR}${MANDIR}3${MANSUBDIR}
.endif
.if defined(MAN3F) && !empty(MAN3F)
	${MINSTALL} ${MAN3F} ${DESTDIR}${MANDIR}3f${MANSUBDIR}
.endif
.if defined(MAN4) && !empty(MAN4)
	${MINSTALL} ${MAN4} ${DESTDIR}${MANDIR}4${MANSUBDIR}
.endif
.if defined(MAN5) && !empty(MAN5)
	${MINSTALL} ${MAN5} ${DESTDIR}${MANDIR}5${MANSUBDIR}
.endif
.if defined(MAN6) && !empty(MAN6)
	${MINSTALL} ${MAN6} ${DESTDIR}${MANDIR}6${MANSUBDIR}
.endif
.if defined(MAN7) && !empty(MAN7)
	${MINSTALL} ${MAN7} ${DESTDIR}${MANDIR}7${MANSUBDIR}
.endif
.if defined(MAN8) && !empty(MAN8)
	${MINSTALL} ${MAN8} ${DESTDIR}${MANDIR}8${MANSUBDIR}
.endif
.if defined(MLINKS) && !empty(MLINKS)
	@set ${MLINKS}; \
	while test $$# -ge 2; do \
		name=$$1; \
		shift; \
		dir=${DESTDIR}${MANDIR}`expr $$name : '[^\.]*\.\(.*\)'`; \
		l=$${dir}${MANSUBDIR}/`expr $$name : '\([^\.]*\)'`.0; \
		name=$$1; \
		shift; \
		dir=${DESTDIR}${MANDIR}`expr $$name : '[^\.]*\.\(.*\)'`; \
		t=$${dir}${MANSUBDIR}/`expr $$name : '\([^\.]*\)'`.0; \
		echo $$t -\> $$l; \
		rm -f $$t; \
		ln $$l $$t; \
	done; true
.endif

#
# Target to install man source files into a man source directory.
#
# XXX
# This isn't clean, and is done this way because the Makefile's list
# the .0's for the man pages, not the source files.  This should be
# redone with the source for the man pages listed in the Makefile instead,
# everything would be a lot cleaner.

MANSRCDIR?=	/usr/share/man/man
MAN1SRC?=	${MAN1:S/.0$/.1/g}
MAN2SRC?=	${MAN2:S/.0$/.2/g}
MAN3SRC?=	${MAN3:S/.0$/.3/g}
MAN4SRC?=	${MAN4:S/.0$/.4/g}
MAN5SRC?=	${MAN5:S/.0$/.5/g}
MAN6SRC?=	${MAN6:S/.0$/.6/g}
MAN7SRC?=	${MAN7:S/.0$/.7/g}
MAN8SRC?=	${MAN8:S/.0$/.8/g}
MANALLSRC=	${MAN1SRC} ${MAN2SRC} ${MAN3SRC} ${MAN4SRC} ${MAN5SRC} \
		${MAN6SRC} ${MAN7SRC} ${MAN8SRC}

_MANSRCSUBDIR: .USE
.if defined(SUBDIR) && !empty(SUBDIR)
	@for entry in ${SUBDIR}; do \
		(echo "===> $$entry"; \
		if test -d ${.CURDIR}/$${entry}.${MACHINE}; then \
			cd ${.CURDIR}/$${entry}.${MACHINE}; \
		else \
			cd ${.CURDIR}/$${entry}; \
		fi; \
		${MAKE} ${.TARGET}); \
	done
.endif

mansourceinstall: ${MANALLSRC} _MANSRCSUBDIR
.if !empty(MANALLSRC:S/[ 	]*//)
	@for name in ${.ALLSRC}; do \
		sec=`expr $$name : '.*\.\(.\)$$'`; \
		echo ${MINSTALL} $$name ${DESTDIR}${MANSRCDIR}$$sec${MANSUBDIR}/;\
		${MINSTALL} $$name ${DESTDIR}${MANSRCDIR}$$sec${MANSUBDIR}/; \
	done; true
.endif
