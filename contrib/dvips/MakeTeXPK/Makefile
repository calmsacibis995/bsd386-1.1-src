
NOMAN=
TMPPKDIR=	/usr/contrib/lib/tex/fonts/tmp

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/MakeTeXPK.sh ${DESTDIR}${BINDIR}/MakeTeXPK
	-mkdir -p ${DESTDIR}${TMPPKDIR}
	chmod 777 ${DESTDIR}${TMPPKDIR}

.include <bsd.prog.mk>
