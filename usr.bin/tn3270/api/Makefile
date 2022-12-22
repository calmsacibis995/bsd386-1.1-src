#	BSDI	$Id: Makefile,v 1.1.1.1 1994/01/03 20:50:45 polk Exp $

LIB=	api
SRCS=	apilib.c api_bsd.c api_exch.c asc_ebc.c astosc.c dctype.c \
	disp_asc.c ebc_disp.c
CFLAGS+=-I.
NOPROFILE=noprofile
CLEANFILES+=astosc.OUT asc_disp.out disp_asc.OUT 

#KBD	= ${.CURDIR}/../ctlr/3270pc.kbd
KBD	= ${.CURDIR}/../ctlr/unix.kbd

.if exists(${.CURDIR}/../tools/mkastosc/obj)
MKASTOSC=${.CURDIR}/../tools/mkastosc/obj/mkastosc
.else
MKASTOSC=${.CURDIR}/../tools/mkastosc/mkastosc
.endif

.if exists(${.CURDIR}/../tools/mkastods/obj)
MKASTODS=${.CURDIR}/../tools/mkastods/obj/mkastods
.else
MKASTODS=${.CURDIR}/../tools/mkastods/mkastods
.endif

.if exists(${.CURDIR}/../tools/mkdstoas/obj)
MKDSTOAS=${.CURDIR}/../tools/mkdstoas/obj/mkdstoas
.else
MKDSTOAS=${.CURDIR}/../tools/mkdstoas/mkdstoas
.endif

install:

astosc.OUT: ${.CURDIR}/../ctlr/hostctlr.h ${.CURDIR}/../ctlr/function.h ${KBD}
	-rm -f $@ 
	${MKASTOSC} ${.CURDIR}/../ctlr/hostctlr.h \
		${.CURDIR}/../ctlr/function.h < ${KBD} > $@

asc_disp.out: 
	-rm -f $@
	${MKASTODS} > $@

disp_asc.OUT: 
	-rm -f $@
	${MKDSTOAS} > $@

# Some special dependencies...
.depend: astosc.OUT asc_disp.out disp_asc.OUT

.include <bsd.lib.mk>
