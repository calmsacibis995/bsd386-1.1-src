MAN1+=uuto.0
MLINKS+=uuto.1 uupick.1

EXECDIR?=	/usr/libexec
BINDIR?=	/usr/bin

FINSTALL?=	install -c -o ${BINOWN} -g ${BINGRP}
FADMIN?=	L-devices L-dialcodes L.aliases L.cmds USERFILE remote.unknown
SHADMIN?=	uuxqt_hook.sh daily.sh weekly.sh

all:

install:: maninstall
	test -d ${DESTDIR}${LOGDIR} || mkdir -p ${DESTDIR}${LOGDIR}
	test -d ${DESTDIR}${PARAMSDIR} || mkdir -p ${DESTDIR}${PARAMSDIR}
	test -d ${DESTDIR}${PUBDIR} || mkdir -p ${DESTDIR}${PUBDIR}
	chown ${BINOWN}.${BINGRP} ${DESTDIR}${LOGDIR} ${DESTDIR}${PARAMSDIR} ${DESTDIR}${PUBDIR}
	chmod 2755 ${DESTDIR}${LOGDIR} ${DESTDIR}${PARAMSDIR}
	chmod 1777 ${DESTDIR}${PUBDIR}
	( set -x; cd ${.CURDIR} ;\
	if [ ! -f ${DESTDIR}${PARAMSFILE} ]; then \
		${FINSTALL} -m 0644 CONFIG ${DESTDIR}${PARAMSFILE} ;\
	else \
		chmod 0644 ${DESTDIR}${PARAMSFILE} ;\
		chown ${BINOWN}.${BINGRP} ${DESTDIR}${PARAMSFILE} ;\
	fi ;\
	if [ ! -f ${DESTDIR}${PARAMSDIR}/L.sys ]; then \
		${FINSTALL} -m 0640 L.sys ${DESTDIR}${PARAMSDIR} ;\
	else \
		chmod 0640 ${DESTDIR}${PARAMSDIR}/L.sys ;\
		chown ${BINOWN}.${BINGRP} ${DESTDIR}${PARAMSDIR}/L.sys ;\
	fi ;\
	for x in ${FADMIN}; do \
		if [ ! -f ${DESTDIR}${PARAMSDIR}/$$x ]; then \
			${FINSTALL} -m 0644 $$x ${DESTDIR}${PARAMSDIR} ;\
		else \
			chmod 0644 ${DESTDIR}${PARAMSDIR}/$$x ;\
			chown ${BINOWN}.${BINGRP} ${DESTDIR}${PARAMSDIR}/$$x ;\
		fi ;\
	done ;\
	for x in ${SHADMIN}; do \
		y=`basename $$x .sh`; \
		if [ ! -f ${DESTDIR}${PARAMSDIR}/$$y ]; then \
			${FINSTALL} -m 0755 $$x ${DESTDIR}${PARAMSDIR}/$$y ;\
		else \
			chmod 0755 ${DESTDIR}${PARAMSDIR}/$$y ;\
			chown ${BINOWN}.${BINGRP} ${DESTDIR}${PARAMSDIR}/$$y ;\
		fi ;\
	done ;\
	${FINSTALL} -m 0755 uuage.sh ${DESTDIR}${EXECDIR}/uuage ;\
	${FINSTALL} -m 0755 uuto.sh ${DESTDIR}${BINDIR}/uuto ;\
	${FINSTALL} -m 0755 uupick.sh ${DESTDIR}${BINDIR}/uupick )

.include <bsd.prog.mk>
.include <bsd.man.mk>
