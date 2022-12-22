PROG=		psc
CFLAGS+=	-DBSD43 -DPSC -I${.CURDIR}/../sc
SRCS=		psc.c vmtbl.c xmalloc.c
CLEANFILES+=	psc.1

.PATH: ${.CURDIR}/../sc

psc.1:	psc.doc
	name=sc NAME=SC LIBDIR=/usr/contrib/lib \
		VERSION=${.CURDIR}/../sc/version.c \
		sh ${.CURDIR}/../sc/torev ${.CURDIR}/psc.doc > psc.1

.include "../../Makefile.inc"
.include <bsd.prog.mk>
