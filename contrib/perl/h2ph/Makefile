# BSDI $Id: Makefile,v 1.2 1994/01/06 06:53:45 polk Exp $

PROG=	h2ph
MAN1=	h2ph.0
SRCS=
STRIP=

.include "../../Makefile.inc" 

depend:

h2ph: h2ph.pl
	cp ${.CURDIR}/h2ph.pl ./h2ph

afterinstall:
	(cd ${DESTDIR}/usr/include; ${DESTDIR}${BINDIR}/h2ph -destdir ${DESTDIR}${BINDIR}/../lib/perl * sys/* machine/*)
	
.include <bsd.prog.mk>
