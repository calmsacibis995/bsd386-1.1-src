#	BSDI $Id: Makefile,v 1.1 1993/12/16 00:57:56 torek Exp $

# pared-down GCC2 library.
.include "../Makefile.symlinks"

# The functions we want.  These are extracted from libgcc2.c individually,
# and the code is left in a separate file.  It would be nice to have all
# of libgcc2.c split into separate files already, but...

NEED=	__gcc_bcmp __main _bb _ctor_list _dtor_list _new_handler \
	_op_new _op_delete

.if	${MACHINE} == "hp300"
NEED+=	_clear_cache
.elif	${MACHINE} == "i386"
NEED+=	_clear_cache
.elif	${MACHINE} == "sparc"
NEED+=	_clear_cache
.elif	${MACHINE} == "tahoe"
NEED+=	_clear_cache _shtab
.endif

LIB=	gcc
SRCS=	${NEED:S/$/.c/g}
CFLAGS+=-I. -I${.CURDIR} -I${CFG} -I${.CURDIR}/../common

${SRCS}: libgcc2.c tconfig.h
	${CC} ${CFLAGS} -E -DL${.TARGET:R} ${.ALLSRC:M*.c} > ${.TARGET}

all .depend: tm.h ${SRCS}

CLEANFILES+=${SRCS} tconfig.h tm.h

.include <bsd.lib.mk>
