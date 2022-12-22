#	BSDI $Id: Makefile,v 1.5 1994/01/30 20:41:41 polk Exp $

SCRIPTS=faxaddmodem
MAN8=	faxaddmodem.0
FILES=	config.abaton config.att-dataport config.class1 config.class2 \
	config.dsi-scout+ config.everex-2496d config.intel-400e \
	config.mt-1432 config.nuvo-voyager config.ppi-pm14400fxmt \
	config.ppi-pm14400fxsa config.rc32acl config.rockwell \
	config.rockwell-class1 config.skel config.telebit-wb \
	config.usr-courier config.usr-sportster config.zyxel-1496e \
	dialrules dialrules.sf-ba
CLEANFILES+=hosts

all clean depend:

install: maninstall
	(cd ${.CURDIR}; for i in ${SCRIPTS}; do \
	    install -c -o ${BINOWN} -g ${BINGRP} -m 555 \
		${.CURDIR}/$$i.sh ${DESTDIR}${SPOOLBINDIR}/$$i; done)
	[ -f ${DESTDIR}${SPOOLETC}/hosts ] || \
	    (echo "127.0.0.1" > hosts; \
	     install -o uucp -g dialer hosts ${DESTDIR}${SPOOLETC})
	[ -f ${DESTDIR}${SPOOLETC}/xferlog ] || \
	    install -c -o uucp -g dialer /dev/null \
		${DESTDIR}${SPOOLETC}/xferlog
	(cd ${.CURDIR}; install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
	    ${FILES} ${DESTDIR}${SPOOLETC})

faxaddmodem.8: ${.CURDIR}/../man/faxaddmodem.1m
	sed ${MANSED} < ${.ALLSRC} > ${.TARGET}
CLEANFILES+=faxaddmodem.8

.include <bsd.prog.mk>
