SRCS= hash.c mkmodules.c myndbm.c subr.c version.c
PROG= mkmodules
CFLAGS+= -DDIRENT -DSTDC_HEADERS -DPOSIX -DFTIME_MISSING -DHAVE_TIMEZONE
CFLAGS+= -DUTIME_NULL_MISSING
CFLAGS+= -I. -I${.CURDIR} -I${.CURDIR}/../src -I${.CURDIR}/../lib 
LDFLAGS+= -L${.CURDIR}/../lib/obj
LDADD=	-lcvs
.PATH: ${.CURDIR}/../src

.include "../../Makefile.inc"
.include <bsd.prog.mk>
