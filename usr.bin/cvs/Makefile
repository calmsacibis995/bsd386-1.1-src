SUBDIR=lib src mkmodules

.include "../Makefile.inc"
.include <bsd.subdir.mk>

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} cvsinit \
			${DESTDIR}/${BINDIR}/cvsinit
