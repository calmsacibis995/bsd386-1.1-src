
FILES=	blackdvi.sty blackdvi.tex colordvi.sty colordvi.tex dvipsmac.tex \
	epsf.sty epsf.tex rotate.sty rotate.tex rotsample.tex test.tex \
	texinfo.tex

beforeinstall:
	-mkdir ${DESTDIR}${LIBDIR}
	for file in ${FILES}; do \
		install -c -o ${BINOWN} -g ${BINGRP} -m 0644 ${.CURDIR}/$$file ${DESTDIR}${LIBDIR}; \
	done

.include <bsd.prog.mk>
