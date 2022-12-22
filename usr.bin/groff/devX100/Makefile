#	BSDI $Id: Makefile,v 1.1.1.1 1993/12/22 06:28:17 polk Exp $

.include "../Makefile.defs"

DEV=	devX100
DEVFILES=DESC TR TI TB TBI CR CI CB CBI HR HI HB HBI NR NI NB NBI S

obj objdir tags depend all clean cleandir:

install:
	for f in ${DEVFILES}; do \
		install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
			$$f ${DESTDIR}${FONTDIR}/${DEV}/; \
	done

.include <bsd.prog.mk>
