#	@(#)Makefile	5.6 (Berkeley) 5/22/91

CFLAGS=	-O -DLIBC_SCCS
OBJS=	crt0.o gcrt0.o
CLEANFILES+=	gmon.o moncrt0.o core a.out

all: ${OBJS}

crt0.o: crt0.c
	${CC} -c -DCRT0 ${.ALLSRC}
	${LD} -x -r ${.TARGET}
	mv a.out ${.TARGET}

moncrt0.o: crt0.c
	${CC} -c -DMCRT0 ${.ALLSRC} -o ${.TARGET}
	${LD} -x -r ${.TARGET}
	mv a.out ${.TARGET}

gcrt0.o: moncrt0.o gmon.o
	${LD} -x -r -o ${.TARGET} moncrt0.o gmon.o

gmon.o: gmon.c gmon.h
	${CC} -c ${DEFS} ${.IMPSRC}
	${LD} -x -r ${.TARGET}
	mv a.out ${.TARGET}

install:
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m 444 ${OBJS} \
	    ${DESTDIR}/usr/lib

depend lint tags:

.include <bsd.prog.mk>
