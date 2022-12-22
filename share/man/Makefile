#	@(#)Makefile	5.9 (Berkeley) 7/1/91

SUBDIR=	man1 man3 man4 man5 man7 man8

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 755 makewhatis \
	    ${DESTDIR}/usr/bin/makewhatis
	install -c -o ${BINOWN} -g ${BINGRP} -m 755 makewhatis.perl \
	    ${DESTDIR}/usr/share/man/makewhatis.perl
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 man0/COPYRIGHT \
	    ${DESTDIR}/usr/share/man/COPYRIGHT

makedb:
	find ${DESTDIR}/usr/share/man -type f -name '*.0' -print | \
		${DESTDIR}/usr/share/man/makewhatis.perl | \
		sort -u > whatis.db
	install -o ${BINOWN} -g ${BINGRP} -m 444 whatis.db \
	    ${DESTDIR}/usr/share/man

.include <bsd.subdir.mk>
