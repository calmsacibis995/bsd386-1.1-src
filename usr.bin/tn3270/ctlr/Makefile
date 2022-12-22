#	BSDI	$Id: Makefile,v 1.1.1.1 1994/01/03 20:50:48 polk Exp $

LIB=	ctlr
SRCS=	api.c function.c inbound.c oia.c options.c outbound.c
NOPROFILE=noprofile
CFLAGS+=-I.
CLEANFILES+=kbd.out

# Which keyboard are we emulating.
#KBD	= ${.CURDIR}/3180.kbd
#KBD	= ${.CURDIR}/3270pc.kbd
KBD	= ${.CURDIR}/unix.kbd

.if exists(${.CURDIR}/../tools/mkhits/obj)
MKHITS=${.CURDIR}/../tools/mkhits/obj/mkhits
.else
MKHITS=${.CURDIR}/../tools/mkhits/mkhits
.endif

install:

.depend: kbd.out

kbd.out: ${KBD} hostctlr.h
	-rm -f $@ TMPfunc.out
	${CC} ${CFLAGS} -E ${.CURDIR}/function.c > TMPfunc.out
	${MKHITS} ${.CURDIR}/hostctlr.h TMPfunc.out < ${KBD} > $@
	-rm -f TMPfunc.out

.include <bsd.lib.mk>
